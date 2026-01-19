#pragma once
#include <stddef.h>
#include "esp_heap_caps.h"

void * lv_mem_custom_alloc(size_t size);
void * lv_mem_custom_realloc(void * p, size_t new_size);
void   lv_mem_custom_free(void * p);
