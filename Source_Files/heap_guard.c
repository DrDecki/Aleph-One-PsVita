#include <stdlib.h>
#include <string.h>
#include <psp2/io/fcntl.h>

void *__real_malloc(size_t size);
void  __real_free(void *ptr);
void *__real_realloc(void *ptr, size_t size);

#define HG_MAGIC 0x48475541u
#define HG_PAD   16u
#define HG_HDR   16u

static const char hg_digits[] = "0123456789abcdef";

static void hg_hex(unsigned int v, char *out)
{
    int i;
    for (i = 0; i < 8; i++)
        out[i] = hg_digits[(v >> ((7 - i) * 4)) & 0xf];
}

static void hg_log(const char *tag, unsigned int a, unsigned int b,
                   unsigned int c, const unsigned char *bytes)
{
    char buf[160];
    int n = 0;
    int i;
    SceUID fd;

    while (*tag && n < 40) buf[n++] = *tag++;
    buf[n++] = ' ';
    hg_hex(a, buf + n); n += 8;
    buf[n++] = ' ';
    hg_hex(b, buf + n); n += 8;
    buf[n++] = ' ';
    hg_hex(c, buf + n); n += 8;
    buf[n++] = ' ';
    for (i = 0; i < 16; i++) {
        buf[n++] = hg_digits[(bytes[i] >> 4) & 0xf];
        buf[n++] = hg_digits[bytes[i] & 0xf];
    }
    buf[n++] = '\n';

    fd = sceIoOpen("ux0:/hg_log.txt",
                   SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
    if (fd >= 0) {
        sceIoWrite(fd, buf, n);
        sceIoClose(fd);
    }
}

static void hg_setup(unsigned char *base, unsigned char *user, size_t size)
{
    unsigned int *hdr = (unsigned int *)(user - HG_PAD - HG_HDR);
    hdr[0] = HG_MAGIC;
    hdr[1] = (unsigned int)size;
    hdr[2] = 0;
    hdr[3] = (unsigned int)base;
    memset(user - HG_PAD, 0xA5, HG_PAD);
    memset(user + size, 0x5A, HG_PAD);
}

void *__wrap_malloc(size_t size)
{
    unsigned char *base = (unsigned char *)__real_malloc(size + HG_HDR + HG_PAD * 2);
    unsigned char *user;
    unsigned int *hdr;
    if (!base) return 0;
    user = base + HG_HDR + HG_PAD;
    hg_setup(base, user, size);
    hdr = (unsigned int *)base;
    hdr[2] = (unsigned int)__builtin_return_address(0);
    return user;
}

void *__wrap_memalign(size_t align, size_t size)
{
    unsigned char *base;
    unsigned char *user;
    unsigned int *hdr;
    unsigned int a = (unsigned int)align;
    unsigned int addr;

    if (a < 8) a = 8;
    base = (unsigned char *)__real_malloc(size + a + HG_HDR + HG_PAD * 2);
    if (!base) return 0;

    addr = (unsigned int)(base + HG_HDR + HG_PAD);
    addr = (addr + (a - 1)) & ~(a - 1);
    user = (unsigned char *)addr;

    hg_setup(base, user, size);
    hdr = (unsigned int *)(user - HG_PAD - HG_HDR);
    hdr[2] = (unsigned int)__builtin_return_address(0);
    return user;
}

void __wrap_free(void *ptr)
{
    unsigned char *user;
    unsigned int *hdr;
    unsigned int size;
    unsigned int i;

    if (!ptr) return;

    user = (unsigned char *)ptr;
    hdr = (unsigned int *)(user - HG_PAD - HG_HDR);

    if (hdr[0] != HG_MAGIC) {
        __real_free(ptr);
        return;
    }

    size = hdr[1];

    for (i = 0; i < HG_PAD; i++) {
        if (*(user - HG_PAD + i) != 0xA5) {
            hg_log("UNDERFLOW", (unsigned int)user, size, hdr[2], user - HG_PAD);
            break;
        }
    }
    for (i = 0; i < HG_PAD; i++) {
        if (user[size + i] != 0x5A) {
            hg_log("OVERFLOW", (unsigned int)user, size, hdr[2], user + size);
            break;
        }
    }

    hdr[0] = 0;
    __real_free((void *)hdr[3]);
}

void *__wrap_calloc(size_t n, size_t size)
{
    size_t total = n * size;
    void *p = __wrap_malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void *__wrap_realloc(void *ptr, size_t size)
{
    unsigned int *hdr;
    unsigned int old;
    void *np;

    if (!ptr) return __wrap_malloc(size);

    hdr = (unsigned int *)((unsigned char *)ptr - HG_PAD - HG_HDR);
    if (hdr[0] != HG_MAGIC)
        return __real_realloc(ptr, size);

    old = hdr[1];
    np = __wrap_malloc(size);
    if (!np) return 0;
    memcpy(np, ptr, old < size ? old : size);
    __wrap_free(ptr);
    return np;
}
