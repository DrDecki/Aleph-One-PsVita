#include <stdlib.h>
#include <string.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/clib.h>
#include <psp2/io/fcntl.h>

void *__real_malloc(size_t size);
void  __real_free(void *ptr);
void *__real_realloc(void *ptr, size_t size);

#define HG_POOL_SIZE (160u * 1024u * 1024u)
#define HG_LIVE 0x48475541u
#define HG_DEAD 0x48474445u
#define HG_PAD  16u
#define HG_HDR  32u

static SceClibMspace hg_mspace = 0;
static unsigned int  hg_lo = 0;
static unsigned int  hg_hi = 0;

static const char hg_digits[] = "0123456789abcdef";

static void hg_hex(unsigned int v, char *out)
{
    int i;
    for (i = 0; i < 8; i++)
        out[i] = hg_digits[(v >> ((7 - i) * 4)) & 0xf];
}

static void hg_log(const char *tag, unsigned int a, unsigned int b, unsigned int c, unsigned int d)
{
    char buf[120];
    int n = 0;
    SceUID fd;

    while (*tag && n < 40) buf[n++] = *tag++;
    buf[n++] = ' ';
    hg_hex(a, buf + n); n += 8; buf[n++] = ' ';
    hg_hex(b, buf + n); n += 8; buf[n++] = ' ';
    hg_hex(c, buf + n); n += 8; buf[n++] = ' ';
    hg_hex(d, buf + n); n += 8;
    buf[n++] = '\n';

    fd = sceIoOpen("ux0:/hg_bounds.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
    if (fd >= 0) {
        sceIoWrite(fd, buf, n);
        sceIoClose(fd);
    }
}

static void hg_setup(void)
{
    SceUID blk;
    void *base = 0;

    if (hg_mspace) return;

    blk = sceKernelAllocMemBlock("alephone_heap",
                                 SCE_KERNEL_MEMBLOCK_TYPE_USER_RW,
                                 HG_POOL_SIZE, 0);
    if (blk < 0) return;
    if (sceKernelGetMemBlockBase(blk, &base) < 0) return;

    hg_mspace = sceClibMspaceCreate(base, HG_POOL_SIZE);
    if (!hg_mspace) return;

    hg_lo = (unsigned int)base;
    hg_hi = hg_lo + HG_POOL_SIZE;
}

__attribute__((constructor(101)))
static void hg_ctor(void)
{
    hg_setup();
}

static void hg_setup_block(unsigned char *base, unsigned char *user, size_t size, unsigned int caller)
{
    unsigned int *hdr = (unsigned int *)(user - HG_PAD - HG_HDR);
    hdr[0] = HG_LIVE;
    hdr[1] = (unsigned int)size;
    hdr[2] = caller;
    hdr[3] = (unsigned int)base;
    hdr[4] = 0; hdr[5] = 0; hdr[6] = 0; hdr[7] = 0;
    memset(user - HG_PAD, 0xA5, HG_PAD);
    memset(user + size, 0x5A, HG_PAD);
}

void *__wrap_malloc(size_t size)
{
    unsigned char *base;
    unsigned char *user;
    if (!hg_mspace) hg_setup();
    if (!hg_mspace) return __real_malloc(size);

    base = (unsigned char *)sceClibMspaceMalloc(hg_mspace, size + HG_HDR + HG_PAD * 2);
    if (!base) return 0;
    user = base + HG_HDR + HG_PAD;
    hg_setup_block(base, user, size, (unsigned int)__builtin_return_address(0));
    return user;
}

void *__wrap_calloc(size_t n, size_t size)
{
    size_t total = n * size;
    void *p = __wrap_malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void *__wrap_memalign(size_t align, size_t size)
{
    unsigned char *base;
    unsigned char *user;
    unsigned int a = (unsigned int)align;
    unsigned int addr;

    if (!hg_mspace) hg_setup();
    if (!hg_mspace) return __real_malloc(size);
    if (a < 8) a = 8;

    base = (unsigned char *)sceClibMspaceMalloc(hg_mspace, size + a + HG_HDR + HG_PAD * 2);
    if (!base) return 0;

    addr = (unsigned int)(base + HG_HDR + HG_PAD);
    addr = (addr + (a - 1)) & ~(a - 1);
    user = (unsigned char *)addr;

    hg_setup_block(base, user, size, (unsigned int)__builtin_return_address(0));
    return user;
}

static int hg_owns_raw(const void *p)
{
    unsigned int a = (unsigned int)p;
    return hg_mspace && a >= hg_lo && a < hg_hi;
}

void __wrap_free(void *ptr)
{
    unsigned char *user;
    unsigned int *hdr;
    unsigned int size, caller, i;
    unsigned int free_caller = (unsigned int)__builtin_return_address(0);

    if (!ptr) return;
    if (!hg_owns_raw(ptr)) { __real_free(ptr); return; }

    user = (unsigned char *)ptr;
    hdr = (unsigned int *)(user - HG_PAD - HG_HDR);

    if (hdr[0] == HG_DEAD) {
        hg_log("DOUBLEFREE", (unsigned int)user, hdr[1], hdr[2], free_caller);
        return;
    }
    if (hdr[0] != HG_LIVE) {
        return;
    }

    size = hdr[1];
    caller = hdr[2];

    for (i = 0; i < HG_PAD; i++) {
        if (*(user - HG_PAD + i) != 0xA5) {
            hg_log("UNDERFLOW", (unsigned int)user, size, caller, free_caller);
            break;
        }
    }
    for (i = 0; i < HG_PAD; i++) {
        if (user[size + i] != 0x5A) {
            hg_log("OVERFLOW", (unsigned int)user, size, caller, free_caller);
            break;
        }
    }

    hdr[0] = HG_DEAD;
    hdr[4] = free_caller;
    sceClibMspaceFree(hg_mspace, (void *)hdr[3]);
}

void *__wrap_realloc(void *ptr, size_t size)
{
    unsigned int *hdr;
    unsigned int old;
    void *np;

    if (!ptr) return __wrap_malloc(size);
    if (!hg_owns_raw(ptr)) return __real_realloc(ptr, size);

    hdr = (unsigned int *)((unsigned char *)ptr - HG_PAD - HG_HDR);
    if (hdr[0] != HG_LIVE) return __real_realloc(ptr, size);

    old = hdr[1];
    np = __wrap_malloc(size);
    if (!np) return 0;
    memcpy(np, ptr, old < size ? old : size);
    __wrap_free(ptr);
    return np;
}
