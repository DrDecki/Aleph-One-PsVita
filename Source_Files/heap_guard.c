#include <stdlib.h>
#include <string.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/threadmgr.h>

void *__real_malloc(size_t size);
void  __real_free(void *ptr);
void *__real_realloc(void *ptr, size_t size);

#define HG_POOL_SIZE (160u * 1024u * 1024u)

static SceClibMspace hg_mspace = 0;
static unsigned int  hg_lo = 0;
static unsigned int  hg_hi = 0;
static SceKernelLwMutexWork hg_lock;
static int hg_lock_ok = 0;

#define HG_LOCK()   do { if (hg_lock_ok) sceKernelLockLwMutex(&hg_lock, 1, NULL); } while (0)
#define HG_UNLOCK() do { if (hg_lock_ok) sceKernelUnlockLwMutex(&hg_lock, 1); } while (0)

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

    if (sceKernelCreateLwMutex(&hg_lock, "hg_lock", 2, 0, NULL) == 0)
        hg_lock_ok = 1;
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
    void *p;
    if (!hg_mspace) hg_setup();
    if (!hg_mspace) return __real_malloc(size);
    HG_LOCK();
    p = sceClibMspaceMalloc(hg_mspace, size);
    HG_UNLOCK();
    return p;
}

void *__wrap_calloc(size_t n, size_t size)
{
    void *p;
    if (!hg_mspace) hg_setup();
    if (!hg_mspace) {
        p = __real_malloc(n * size);
        if (p) memset(p, 0, n * size);
        return p;
    }
    HG_LOCK();
    p = sceClibMspaceCalloc(hg_mspace, n, size);
    HG_UNLOCK();
    return p;
}

void *__wrap_memalign(size_t align, size_t size)
{
    void *p;
    if (!hg_mspace) hg_setup();
    if (!hg_mspace) return __real_malloc(size);
    HG_LOCK();
    p = sceClibMspaceMemalign(hg_mspace, align, size);
    HG_UNLOCK();
    return p;
}

void __wrap_free(void *ptr)
{
    if (!ptr) return;
    if (hg_owns(ptr)) {
        HG_LOCK();
        sceClibMspaceFree(hg_mspace, ptr);
        HG_UNLOCK();
        return;
    }
    __real_free(ptr);
}

void *__wrap_realloc(void *ptr, size_t size)
{
    void *p;
    if (!ptr) return __wrap_malloc(size);
    if (hg_owns(ptr)) {
        HG_LOCK();
        p = sceClibMspaceRealloc(hg_mspace, ptr, size);
        HG_UNLOCK();
        return p;
    }
    return __real_realloc(ptr, size);
}

#include <stdio.h>
void __real_abort(void) __attribute__((noreturn));
void __real_exit(int code) __attribute__((noreturn));
void __real___assert_func(const char *file, int line, const char *func, const char *expr) __attribute__((noreturn));

static void hg_log_exit(const char *what, void *ra, const char *extra)
{
    FILE *f = fopen("ux0:/data/AlephOne/exit_log.txt", "a");
    if (f) { fprintf(f, "%s ra=%p %s\n", what, ra, extra ? extra : ""); fclose(f); }
}

void __wrap_abort(void)
{
    hg_log_exit("abort", __builtin_return_address(0), 0);
    __real_abort();
}

void __wrap_exit(int code)
{
    char b[32];
    snprintf(b, sizeof b, "code=%d", code);
    hg_log_exit("exit", __builtin_return_address(0), b);
    __real_exit(code);
}

void __wrap___assert_func(const char *file, int line, const char *func, const char *expr)
{
    char b[512];
    snprintf(b, sizeof b, "%s:%d %s: %s", file ? file : "", line, func ? func : "", expr ? expr : "");
    hg_log_exit("assert", __builtin_return_address(0), b);
    __real___assert_func(file, line, func, expr);
}
