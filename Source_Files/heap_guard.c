#include <stdlib.h>
#include <string.h>

void *__real_malloc(size_t size);
void  __real_free(void *ptr);
void *__real_realloc(void *ptr, size_t size);

#define HG_LIVE 0x48475541u
#define HG_DEAD 0x48474445u
#define HG_HDR  16u

static void hg_init(unsigned char *base, unsigned char *user, size_t size)
{
    unsigned int *hdr = (unsigned int *)(user - HG_HDR);
    hdr[0] = HG_LIVE;
    hdr[1] = (unsigned int)size;
    hdr[2] = (unsigned int)base;
    hdr[3] = 0;
}

void *__wrap_malloc(size_t size)
{
    unsigned char *base = (unsigned char *)__real_malloc(size + HG_HDR);
    if (!base) return 0;
    hg_init(base, base + HG_HDR, size);
    return base + HG_HDR;
}

void *__wrap_memalign(size_t align, size_t size)
{
    unsigned char *base;
    unsigned char *user;
    unsigned int a = (unsigned int)align;
    unsigned int addr;

    if (a < 8) a = 8;
    base = (unsigned char *)__real_malloc(size + a + HG_HDR);
    if (!base) return 0;

    addr = (unsigned int)(base + HG_HDR);
    addr = (addr + (a - 1)) & ~(a - 1);
    user = (unsigned char *)addr;

    hg_init(base, user, size);
    return user;
}

void __wrap_free(void *ptr)
{
    unsigned int *hdr;

    if (!ptr) return;

    hdr = (unsigned int *)((unsigned char *)ptr - HG_HDR);

    if (hdr[0] != HG_LIVE) return;

    hdr[0] = HG_DEAD;
    __real_free((void *)hdr[2]);
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

    hdr = (unsigned int *)((unsigned char *)ptr - HG_HDR);
    if (hdr[0] != HG_LIVE) return __wrap_malloc(size);

    old = hdr[1];
    np = __wrap_malloc(size);
    if (!np) return 0;
    memcpy(np, ptr, old < size ? old : size);
    __wrap_free(ptr);
    return np;
}
