#include "atomic.h"
#include "uglobal.h"
#include "umemory.h"
#include <stdio.h>
#include <string.h>
#include <vector>

#ifdef USE_JEMALLOC
#include <jemalloc.h>
#define rxmalloc je_malloc
#define rxcalloc je_calloc
#define rxrealloc je_realloc
#define rxfree je_free
#define rxmalloc_usable_size je_malloc_usable_size
#define rxmemalign je_memalign
#define rxaligned_alloc je_aligned_alloc
#define rxposix_memalign je_posix_memalign
#else
#define rxmalloc malloc
#define rxcalloc calloc
#define rxrealloc realloc
#define rxfree free
#ifdef __APPLE__
#include <malloc/malloc.h>
#define rxmalloc_useable_size malloc_size
#endif
//#define rxmemalign memalign
//#define rxaligned_alloc alligned_alloc
//#define rxposix_memalign posix_memalign
#endif

#define PREFIX_SIZE sizeof(struct rednet::umemory_hook::mem_cookie)

using namespace std;
namespace rednet
{

class umemory_hook
{
    friend class umemory;

    struct mem_info
    {
        uint32_t context_id;
        ssize_t memory_used;
    };

    struct mem_cookie
    {
        uint32_t context_id;
    };

    static constexpr int MEMORY_BLACK_MAX = 0x10000;

  private:
    R_CONSTRUCTOR(umemory_hook)
  private:
    static void malloc_error(size_t sz);
    static void fill_prefix(void *ptr);
    static void update_meminfo(uint32_t context_id, size_t sz);
    static ssize_t *get_meminfo(uint32_t context_id);

    static void clean_prefix(void *ptr);

  private:
    static size_t __used_memory__;
    static size_t __used_block__;
    static mem_info __memory_blocks[MEMORY_BLACK_MAX];
};

size_t umemory_hook::__used_memory__ = 0;
size_t umemory_hook::__used_block__ = 0;
struct umemory_hook::mem_info umemory_hook::__memory_blocks[MEMORY_BLACK_MAX];

void umemory_hook::malloc_error(size_t sz)
{
    fprintf(stderr, "xmalloc: Out of memory trying to allocate %zu bytes\n",
            sz);
    fflush(stderr);
    abort();
}

void umemory_hook::fill_prefix(void *ptr)
{
    uint32_t context_id = uglobal::instance()->currentContext();
    size_t size = rxmalloc_usable_size(ptr);
    struct mem_cookie *p = (struct mem_cookie *)((char *)ptr + size - sizeof(struct mem_cookie));
    p->context_id = context_id;
    update_meminfo(context_id, size);
}

void umemory_hook::update_meminfo(uint32_t context_id, size_t sz)
{
    ATOM_ADD(&__used_memory__, sz);
    ATOM_INC(&__used_block__);

    ssize_t *used_address = get_meminfo(context_id);
    if (!used_address)
        return;
    ATOM_ADD(used_address, sz);
}

ssize_t *umemory_hook::get_meminfo(uint32_t context_id)
{
    int hash = (int)(context_id & (MEMORY_BLACK_MAX - 1));
    struct mem_info *info = &__memory_blocks[hash];
    uint32_t old_id = info->context_id;
    ssize_t old_used = info->memory_used;
    if (old_id == 0 || old_used <= 0)
    {
        if (!ATOM_CAS(&info->context_id, old_id, context_id))
        {
            return 0;
        }

        if (old_used < 0)
            ATOM_CAS(&info->memory_used, old_used, 0);
    }

    if (info->context_id != context_id)
    {
        return 0;
    }

    return &info->memory_used;
}

void umemory_hook::clean_prefix(void *ptr)
{
    size_t sz = rxmalloc_usable_size(ptr);
    struct mem_cookie *p = (struct mem_cookie *)((char *)ptr + sz - PREFIX_SIZE);
    uint32_t context_id = p->context_id;

    ATOM_SUB(&__used_memory__, sz);
    ATOM_DEC(&__used_block__);

    ssize_t *used_address = get_meminfo(context_id);
    if (!used_address)
        return;
    ATOM_SUB(used_address, sz);
}

void *umemory::malloc(size_t size)
{
    void *ptr = rxmalloc(size + PREFIX_SIZE);
    if (!ptr)
        umemory_hook::malloc_error(size);

    umemory_hook::fill_prefix(ptr);
    return ptr;
}

void umemory::free(void *ptr)
{
    if (!ptr)
        return;
    umemory_hook::clean_prefix(ptr);
    rxfree(ptr);
}

void *umemory::calloc(size_t nmemb, size_t size)
{
    void *ptr = rxcalloc(nmemb + ((PREFIX_SIZE + size - 1) / size), size);
    if (!ptr)
        umemory_hook::malloc_error(size);
    umemory_hook::fill_prefix(ptr);
    return ptr;
}

void *umemory::realloc(void *ptr, size_t size)
{
    if (!ptr)
        return umemory::malloc(size);
    umemory_hook::clean_prefix(ptr);
    void *newptr = rxrealloc(ptr, size + PREFIX_SIZE);
    if (!newptr)
        umemory_hook::malloc_error(size);
    umemory_hook::fill_prefix(newptr);
    return newptr;
}

char *umemory::strdup(const char *str)
{
    size_t strsz = strlen(str);
    char *dstr = (char *)umemory::malloc(strsz + 1);
    memcpy(dstr, str, strsz);
    dstr[strsz] = '\0';
    return dstr;
}

void *umemory::lalloc(void *ptr, size_t osize, size_t nsize)
{
    if (nsize == 0)
    {
        rxfree(ptr);
        return NULL;
    }
    else
    {
        return rxrealloc(ptr, nsize);
    }
}

#ifdef USE_JEMALLOC
void *umemory::memalign(size_t alignment, size_t size)
{
    void *ptr = rxmemalign(alignment, size + PREFIX_SIZE);
    if (!ptr)
        umemory_hook::malloc_error(size);
    umemory_hook::fill_prefix(ptr);
    return ptr;
}

void *umemory::aligned_alloc(size_t alignment, size_t size)
{
    void *ptr = rxaligned_alloc(alignment, size + (size_t)((PREFIX_SIZE + alignment - 1) & ~(alignment - 1)));
    if (!ptr)
        umemory_hook::malloc_error(size);
    umemory_hook::fill_prefix(ptr);
    return ptr;
}

int umemory::posix_memalign(void **memptr, size_t alignment, size_t size)
{
    int err = rxposix_memalign(memptr, alignment, size + PREFIX_SIZE);
    if (err)
        umemory_hook::malloc_error(size);
    umemory_hook::fill_prefix(*memptr);
    return err;
}
#endif

} // namespace rednet