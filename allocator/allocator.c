// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

// TODO: Find issue causing spam of log_success error msgs
//       Maybe a bug here, maybe a bug on scrolling_text API

#include "elpekenin/allocator.h"

#include <quantum/quantum.h>
#include <stdlib.h>

#include "elpekenin/shortcuts.h"

// *** Track heap usage ***

#ifdef ALLOCATOR_DEBUG
#    define allocator_dprintf dprintf
#else
#    define allocator_dprintf(...)
#endif

static size_t n_known = 0;

#define N_ALLOCATORS 10
static const allocator_t *known_allocators[N_ALLOCATORS];

// TODO: per-allocator stats
typedef struct PACKED {
    allocator_t *allocator;
    void        *ptr;
    size_t       size;
} alloc_info_t;

// array to prevent the tracker to be dynamic (use malloc) itself
static alloc_info_t  alloc_info_buff[ALLOC_BUFF_SIZE] = {0};
static memory_pool_t alloc_info_pool;

#if ENABLE_MALLOC_TRACKING == 1
void keyboard_pre_init_allocator(void) {
    chPoolObjectInit(&alloc_info_pool, sizeof(alloc_info_t), NULL);
    chPoolLoadArray(&alloc_info_pool, alloc_info_buff, ALLOC_BUFF_SIZE);
    allocator_dprintf("Pool initialized\n");
}
#endif

const allocator_t **get_known_allocators(int8_t *n) {
    *n = n_known;
    return known_allocators;
}

size_t get_used_heap(void) {
    size_t used = 0;

    int8_t              n;
    const allocator_t **allocators = get_known_allocators(&n);

    for (uint8_t i = 0; i < n; ++i) {
        const allocator_t *allocator = allocators[i];
        used += allocator->used;
    }

    return used;
}

static alloc_info_t *find_info(void *ptr) {
    for (uint8_t i = 0; i < ALLOC_BUFF_SIZE; ++i) {
        alloc_info_t *info = &alloc_info_buff[i];
        if (info->ptr == ptr) {
            return info;
        }
    }

    return NULL;
}

static void memory_allocated(allocator_t *allocator, void *ptr, size_t size) {
    if (ptr == NULL) {
        return;
    }

    bool insert = true;
    for (size_t i = 0; i < n_known; ++i) {
        if (known_allocators[i] == allocator) {
            insert = false;
            break;
        }
    }
    if (insert) {
        if (n_known >= N_ALLOCATORS) {
            allocator_dprintf("[ERROR] %s: Too many allocators, can't track\n", __func__);
        } else {
            known_allocators[n_known++] = allocator;
        }
    }

    alloc_info_t *info = chPoolAlloc(&alloc_info_pool);

    bool pushed = info != NULL;
    if (pushed) {
        allocator->used += size;

        *info = (alloc_info_t){
            .allocator = allocator,
            .ptr       = ptr,
            .size      = size,
        };
    } else {
        allocator_dprintf("[ERROR] %s: No space to track memory allocation\n", __func__);
    }
}

static void memory_freed(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    alloc_info_t *info = find_info(ptr);

    bool popped = info != NULL;
    if (popped) {
        info->allocator->used -= info->size;
        chPoolFree(&alloc_info_pool, ptr);
    } else {
        allocator_dprintf("[ERROR] %s: Could not find pointer in tracked allocations\n", __func__);
    }
}

static inline void *calloc_shim(allocator_t *allocator, size_t nmemb, size_t size) {
    return calloc(nmemb, size);
}

static inline void free_shim(allocator_t *allocator, void *ptr) {
    return free(ptr);
}

static inline void *malloc_shim(allocator_t *allocator, size_t size) {
    return malloc(size);
}

static inline void *realloc_shim(allocator_t *allocator, void *ptr, size_t size) {
    return realloc(ptr, size);
}

allocator_t c_runtime_allocator = {
    .calloc  = calloc_shim,
    .free    = free_shim,
    .malloc  = malloc_shim,
    .realloc = realloc_shim,
    .name    = "C runtime",
};

#if defined(PROTOCOL_CHIBIOS)
static void *ch_core_malloc(allocator_t *allocator, size_t size) {
    return chCoreAlloc(size);
}

allocator_t ch_core_allocator = {
    .malloc = ch_core_malloc,
    .name   = "ChibiOS core",
};

static void *manual_realloc(allocator_t *allocator, void *ptr, size_t new_size) {
    // no pointer, realloc is equivalent to malloc
    if (ptr == NULL) {
        return malloc_with(allocator, new_size);
    }

    // pointer and new size is 0, realloc is equivalent to free
    if (new_size == 0) {
        free_with(allocator, ptr);
        return NULL;
    }

    // find current size
    alloc_info_t *info = find_info(ptr);
    if (info == NULL) {
        allocator_dprintf("[ERROR] %s: Could not find info for realloc\n", __func__);
        return NULL;
    }

    // big enough, just return the current address back
    size_t curr_size = info->size;
    if (curr_size >= new_size) {
        return ptr;
    }

    // actual realloc
    void *new_ptr = malloc_with(allocator, new_size);
    if (new_ptr == NULL) {
        // no space for new allocation
        // return NULL and **do not** free old memory
        allocator_dprintf("[ERROR] %s:New size could not be allocated\n", __func__);
        return NULL;
    }

    memcpy(new_ptr, ptr, curr_size);
    free_with(allocator, ptr);
    return new_ptr;
}

#    if CH_CFG_USE_MEMPOOLS == TRUE
static void ch_pool_free(allocator_t *allocator, void *ptr) {
    memory_pool_t *pool = (memory_pool_t *)allocator->arg;
    return chPoolFree(pool, ptr);
}

static void *ch_pool_malloc(allocator_t *allocator, size_t size) {
    memory_pool_t *pool    = (memory_pool_t *)allocator->arg;
    size_t         n_items = size / pool->object_size;

    // ensure we get asked for a single item's size
    if (n_items != 1 || n_items * pool->object_size != size) {
        allocator_dprintf("[ERROR] %s: size / pool_obj_size != 1\n", __func__);
        return NULL;
    }

    return chPoolAlloc(pool);
}

allocator_t new_ch_pool_allocator(memory_pool_t *pool, const char *name) {
    return (allocator_t){
        .free   = ch_pool_free,
        .malloc = ch_pool_malloc,
        .name   = name,
        .arg    = pool,
    };
}
#    endif

#    if CH_CFG_USE_HEAP == TRUE
static void ch_heap_free(allocator_t *allocator, void *ptr) {
    return chHeapFree(ptr);
}

static void *ch_heap_malloc(allocator_t *allocator, size_t size) {
    return chHeapAlloc((memory_heap_t *)allocator->arg, size);
}

allocator_t new_ch_heap_allocator(memory_heap_t *heap, const char *name) {
    return (allocator_t){
        .free    = ch_heap_free,
        .malloc  = ch_heap_malloc,
        .realloc = manual_realloc,
        .name    = name,
        .arg     = heap,
    };
}
#    endif
#endif

allocator_t *get_default_allocator(void) {
    return &c_runtime_allocator;
}

// ^ allocators
// -----
// v convenience wrappers

static inline void __entry(const char *fn, allocator_t *allocator) {
    allocator_dprintf("[DEBUG]: Using %s.%s\n", allocator->name, fn);
}

static inline void *__bad_allocator(const char *fn, allocator_t *allocator) {
    allocator_dprintf("[ERROR]: There is no %s.%s\n", allocator->name, fn);
    return NULL;
}

static inline void __error(const char *fn) {
    allocator_dprintf("[ERROR]: Calling %s failed\n", fn);
}

void *calloc_with(allocator_t *allocator, size_t nmemb, size_t size) {
    const char fn[] = "calloc";

    __entry(fn, allocator);

    if (allocator->calloc == NULL) {
        return __bad_allocator(fn, allocator);
    }

    void *ptr = allocator->calloc(allocator, nmemb, size);
    if (ptr == NULL) {
        __error(fn);
    } else {
        memory_allocated(allocator, ptr, nmemb * size);
    }

    return ptr;
}

void free_with(allocator_t *allocator, void *ptr) {
    const char fn[] = "free";

    __entry(fn, allocator);

    if (allocator->free == NULL) {
        __bad_allocator(fn, allocator);
        return;
    }

    allocator->free(allocator, ptr);
    memory_freed(ptr);
}

void *malloc_with(allocator_t *allocator, size_t size) {
    const char fn[] = "malloc";

    __entry(fn, allocator);

    if (allocator->malloc == NULL) {
        return __bad_allocator(fn, allocator);
    }

    void *ptr = allocator->malloc(allocator, size);
    if (ptr == NULL) {
        __error(fn);
    } else {
        memory_allocated(allocator, ptr, size);
    }

    return ptr;
}

void *realloc_with(allocator_t *allocator, void *ptr, size_t size) {
    const char fn[] = "realloc";

    __entry(fn, allocator);

    if (allocator->realloc == NULL) {
        return __bad_allocator(fn, allocator);
    }

    void *new_ptr = allocator->realloc(allocator, ptr, size);
    if (new_ptr == NULL) {
        __error(fn);
    } else {
        // maybe it already went thru free and got cleaned?
        memory_freed(ptr);
        memory_allocated(allocator, new_ptr, size);
    }

    return new_ptr;
}
