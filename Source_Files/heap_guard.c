#include <stdlib.h>
#include <string.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/clib.h>

void *__real_malloc(size_t size);
void  __real_free(void *ptr);
void *__real_realloc(void *ptr, size_t size);

#define HG_POOL_SIZE (160u * 1024u * 1024u)

static SceClibMspace hg_mspace = 0;
static unsigned int  hg_lo = 0;
static unsigned int  hg_hi = 0;

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

static int hg_owns(const void *p)
{
    unsigned int a = (unsigned int)p;
    return hg_mspace && a >= hg_lo && a < hg_hi;
}

void *__wrap_malloc(size_t size)
{
    if (!hg_mspace) hg_setup();
    if (!hg_mspace) return __real_malloc(size);
    return sceClibMspaceMalloc(hg_mspace, size);
}

void *__wrap_calloc(size_t n, size_t size)
{
    if (!hg_mspace) hg_setup();
    if (!hg_mspace) {
        void *p = __real_malloc(n * size);
        if (p) memset(p, 0, n * size);
        return p;
    }
    return sceClibMspaceCalloc(hg_mspace, n, size);
}

void *__wrap_memalign(size_t align, size_t size)
{
    if (!hg_mspace) hg_setup();
    if (!hg_mspace) return __real_malloc(size);
    return sceClibMspaceMemalign(hg_mspace, align, size);
}

void __wrap_free(void *ptr)
{
    if (!ptr) return;
    if (hg_owns(ptr)) {
        sceClibMspaceFree(hg_mspace, ptr);
        return;
    }
    __real_free(ptr);
}

void *__wrap_realloc(void *ptr, size_t size)
{
    if (!ptr) return __wrap_malloc(size);
    if (hg_owns(ptr)) return sceClibMspaceRealloc(hg_mspace, ptr, size);
    return __real_realloc(ptr, size);
}
