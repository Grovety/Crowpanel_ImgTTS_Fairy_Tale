#include "lv_mem_psram.h"

#ifndef LV_HEAP_CAPS
#define LV_HEAP_CAPS  (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA)
#endif

void * lv_mem_custom_alloc(size_t size) {
    return heap_caps_malloc(size, LV_HEAP_CAPS);
}

void * lv_mem_custom_realloc(void * p, size_t new_size) {
    return heap_caps_realloc(p, new_size, LV_HEAP_CAPS);
}

void lv_mem_custom_free(void * p) {
    heap_caps_free(p);
}
