// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "elpekenin/allocator.h"

#include <quantum/quantum.h>
#include <stdlib.h>

#ifdef ALLOCATOR_DEBUG
#    include "quantum/logging/debug.h"
#    define allocator_dprintf dprintf
#else
#    define allocator_dprintf(...)
#endif

static struct {
    struct {
        const allocator_t *ptr[ALLOC_ALLOCATORS_SIZE];
        size_t             count;
    } ators;

    struct {
        alloc_stats_t ptr[ALLOC_ALLOCATIONS_SIZE];
        size_t        count;
    } stats;
} alloc = {0};

const allocator_t *const *get_known_allocators(size_t *n) {
    *n = alloc.ators.count;
    return alloc.ators.ptr;
}

const alloc_stats_t *get_allocations(size_t *n) {
    *n = alloc.stats.count;
    return alloc.stats.ptr;
}

size_t get_used_heap(void) {
    size_t used = 0;

    for (size_t i = 0; i < alloc.stats.count; ++i) {
        const alloc_stats_t stat = alloc.stats.ptr[i];

        // lifetime.end == 0 -> not yet free'd
        //
        // without this, we sum up all space every allocated
        if (stat.lifetime.end == 0) {
            used += stat.size;
        }
    }

    return used;
}

static alloc_stats_t *get_stats(void *ptr) {
    for (size_t i = 0; i < ARRAY_SIZE(alloc.stats.ptr); ++i) {
        alloc_stats_t *stat = &alloc.stats.ptr[i];

        if (stat->ptr == ptr) {
            return stat;
        }
    }

    return NULL;
}

static void push_new_stat(const allocator_t *allocator, void *ptr, size_t size) {
    if (ptr == NULL) {
        return;
    }

    bool allocator_found = false;
    for (size_t i = 0; i > alloc.ators.count; ++i) {
        const allocator_t *element = alloc.ators.ptr[i];
        if (element == allocator) {
            allocator_found = true;
            break;
        }
    }

    if (!allocator_found) {
        if (alloc.ators.count >= ALLOC_ALLOCATORS_SIZE) {
            allocator_dprintf("[WARN]: Too many allocators, can't track\n");
        } else {
            alloc.ators.ptr[alloc.ators.count++] = allocator;
        }
    }

    if (alloc.stats.count >= ALLOC_ALLOCATIONS_SIZE) {
        allocator_dprintf("[WARN]: Too many stats, can't track\n");
    } else {
        alloc.stats.ptr[alloc.stats.count++] = (alloc_stats_t){
            .allocator = allocator,
            .ptr       = ptr,
            .size      = size,
            .lifetime =
                {
                    .start = timer_read32(),
                    .end   = 0,
                },
        };
    }
}

static void *calloc_shim(__unused const allocator_t *allocator, size_t nmemb, size_t size) {
    return calloc(nmemb, size);
}

static void free_shim(__unused const allocator_t *allocator, void *ptr) {
    return free(ptr);
}

static void *malloc_shim(__unused const allocator_t *allocator, size_t size) {
    return malloc(size);
}

static void *realloc_shim(__unused const allocator_t *allocator, void *ptr, size_t size) {
    return realloc(ptr, size);
}

const allocator_t _c_runtime_allocator = {
    .calloc  = calloc_shim,
    .free    = free_shim,
    .malloc  = malloc_shim,
    .realloc = realloc_shim,
    .name    = "std",
};

const allocator_t *const c_runtime_allocator = &_c_runtime_allocator;

#if defined(PROTOCOL_CHIBIOS)
static void *ch_core_malloc(__unused const allocator_t *allocator, size_t size) {
    return chCoreAlloc(size);
}

static const allocator_t _ch_core_allocator = {
    .malloc = ch_core_malloc,
    .name   = "chcore",
};

const allocator_t *const ch_core_allocator = &_ch_core_allocator;

#    if CH_CFG_USE_MEMPOOLS == TRUE
static void ch_pool_free(const allocator_t *allocator, void *ptr) {
    memory_pool_t *pool = (memory_pool_t *)allocator->arg;
    return chPoolFree(pool, ptr);
}

static void *ch_pool_malloc(const allocator_t *allocator, size_t size) {
    memory_pool_t *pool    = (memory_pool_t *)allocator->arg;
    size_t         n_items = size / pool->object_size;

    // ensure we get asked for a single item's size
    if (n_items != 1 || n_items * pool->object_size != size) {
        allocator_dprintf("[ERROR] %s: size / pool_obj_size != 1\n", __func__);
        return NULL;
    }

    return chPoolAlloc(pool);
}

const allocator_t new_ch_pool_allocator(const memory_pool_t *pool, const char *name) {
    return (allocator_t){
        .free   = ch_pool_free,
        .malloc = ch_pool_malloc,
        .name   = name,
        .arg    = pool,
    };
}
#    endif

#    if CH_CFG_USE_HEAP == TRUE
static void ch_heap_free(__unused const allocator_t *allocator, void *ptr) {
    return chHeapFree(ptr);
}

static void *ch_heap_malloc(const allocator_t *allocator, size_t size) {
    return chHeapAlloc((memory_heap_t *)allocator->arg, size);
}

const allocator_t new_ch_heap_allocator(memory_heap_t *heap, const char *name) {
    return (allocator_t){
        .free   = ch_heap_free,
        .malloc = ch_heap_malloc,
        .name   = name,
        .arg    = heap,
    };
}
#    endif
#endif

void *calloc_with(const allocator_t *allocator, size_t nmemb, size_t size) {
    if (allocator == NULL) {
        allocator_dprintf("[ERROR]: NULL allocator in %s\n", __func__);
        return NULL;
    }

    const size_t total_size = nmemb * size;

    allocator_dprintf("[DEBUG]: Using %s.calloc\n", allocator->name);

    void *ptr;

    // actual calloc if available, manually implement with malloc + memset otherwise
    if (allocator->calloc != NULL) {
        ptr = allocator->calloc(allocator, nmemb, size);

        if (ptr == NULL) {
            allocator_dprintf("[ERROR]: %s.calloc failed\n", allocator->name);
        } else {
            push_new_stat(allocator, ptr, total_size);
        }
    } else {
        ptr = malloc_with(allocator, total_size);

        if (ptr != NULL) {
            memset(ptr, 0, total_size);
        }
    }

    return ptr;
}

void free_with(const allocator_t *allocator, void *ptr) {
    if (allocator == NULL) {
        allocator_dprintf("[ERROR]: NULL allocator in %s\n", __func__);
        return;
    }

    allocator_dprintf("[DEBUG]: Using %s.free\n", allocator->name);

    if (allocator->free == NULL) {
        allocator_dprintf("[ERROR]: There is no %s.free\n", allocator->name);
        return;
    }

    alloc_stats_t *stat = get_stats(ptr);
    if (stat != NULL && stat->allocator != allocator) {
        allocator_dprintf("[ERROR]: Can't `free` with a different allocator\n");
        return;
    }

    allocator->free(allocator, ptr);

    if (stat != NULL) {
        stat->lifetime.end = timer_read32();
    } else {
        allocator_dprintf("[WARN]: Could not find pointer (%p) in tracked allocations\n", ptr);
    }
}

void *malloc_with(const allocator_t *allocator, size_t size) {
    if (allocator == NULL) {
        allocator_dprintf("[ERROR]: NULL allocator in %s\n", __func__);
        return NULL;
    }

    allocator_dprintf("[DEBUG]: Using %s.malloc\n", allocator->name);

    if (allocator->malloc == NULL) {
        allocator_dprintf("[ERROR]: There is no %s.malloc\n", allocator->name);
        return NULL;
    }

    void *ptr = allocator->malloc(allocator, size);
    if (ptr == NULL) {
        allocator_dprintf("[ERROR]: Calling %s.malloc failed\n", allocator->name);
    } else {
        push_new_stat(allocator, ptr, size);
    }

    return ptr;
}

void *realloc_with(const allocator_t *allocator, void *ptr, size_t size) {
    if (allocator == NULL) {
        allocator_dprintf("[ERROR]: NULL allocator in %s\n", __func__);
        return NULL;
    }

    allocator_dprintf("[DEBUG]: Using %s.realloc\n", allocator->name);

    // no pointer, realloc is equivalent to malloc
    if (ptr == NULL) {
        return malloc_with(allocator, size);
    }

    // pointer and new size is 0, realloc is equivalent to free
    if (size == 0) {
        free_with(allocator, ptr);
        return NULL;
    }

    alloc_stats_t *stat = get_stats(ptr);
    if (stat == NULL) {
        allocator_dprintf("[ERROR]: Could not find stats previous to realloc\n");
        return NULL;
    }

    if (stat->allocator != allocator) {
        allocator_dprintf("[ERROR]: Can't `realloc` with a different allocator\n");
        return NULL;
    }

    // big enough, just return the current address back
    if (stat->size >= size) {
        return ptr;
    }

    void *new_ptr;

    // actual realloc if available, manually implement with malloc + memcpy otherwise
    if (allocator->realloc != NULL) {
        new_ptr = allocator->realloc(allocator, ptr, size);
        allocator_dprintf("[ERROR]: %s.realloc failed\n", allocator->name);
    } else {
        new_ptr = malloc_with(allocator, size);

        // move current contents
        if (new_ptr != NULL) {
            memcpy(new_ptr, ptr, stat->size);
        }
    }

    if (new_ptr == NULL) {
        // no space for new allocation
        // apparently, realloc returns NULL but does not free old memory, we do the same
        allocator_dprintf("[ERROR]: could not allocate new size, old memory still available\n");
        return NULL;
    }

    // update stats
    stat->size = size;
    stat->ptr  = new_ptr;

    return new_ptr;
}

#if defined(COMMUNITY_MODULE_UI_ENABLE)
#    include "elpekenin/ui/utils.h"

#    if defined(COMMUNITY_MODULE_STRING_ENABLE)
#        include "elpekenin/string.h"
#    else
#        error "Must enable 'elpekenin/string'"
#    endif

#    if defined(COMMUNITY_MODULE_MEMORY_ENABLE)
#        include "elpekenin/memory.h"
#    endif

bool heap_init(ui_node_t *self) {
    heap_args_t *const args = self->args;
    args->last              = ~0;
    return ui_font_fits(self);
}

uint32_t heap_render(const ui_node_t *self, painter_device_t display) {
    heap_args_t *const args = self->args;

    const size_t heap = get_used_heap();
    if (args->last == heap) {
        goto exit;
    }

    const painter_font_handle_t font = qp_load_font_mem(args->font);
    if (font == NULL) {
        goto exit;
    }

    string_t str = str_new(30);

    str_append(&str, "Heap: ");
    pretty_bytes(&str, heap);

#    if defined(COMMUNITY_MODULE_MEMORY_ENABLE)
    str_append(&str, "/");
    pretty_bytes(&str, get_heap_size());
#    endif

    if (!ui_text_fits(self, font, str.ptr)) {
        goto err;
    }

    qp_drawtext(display, self->start.x, self->start.y, font, str.ptr);

    args->last = heap;

err:
    qp_close_font(font);

exit:
    return args->interval;
}
#endif
