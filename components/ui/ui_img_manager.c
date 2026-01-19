#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "zlib.h"          
#include "esp_timer.h"

#include "ui_img_manager.h"

static const char *TAG = "UIIMG";

static void map_path(char *dst, size_t dst_sz, const char *src)
{
    if (src && src[0] == 'S' && src[1] == ':') {
        const char *p = src + 2;   
        if (*p == '/') p++;        

        if (strncmp(p, "assets/", 7) == 0) {
            snprintf(dst, dst_sz, "/spiffs/%s", p);
            return;
        }

        if (strncmp(p, "assets_s/", 9) == 0) {
            snprintf(dst, dst_sz, "/spiffs/%s", p);
            return;
        }

        if (strncmp(p, "assets_l/", 9) == 0) {
            snprintf(dst, dst_sz, "/spiffs/%s", p);
            return;
        }

        snprintf(dst, dst_sz, "/spiffs/%s", p);
    } else {
        snprintf(dst, dst_sz, "%s", src ? src : "");
    }
}

uint8_t* _ui_load_binary_direct(const char* fname_S, uint32_t size)
{
    if (!fname_S || size == 0) {
        ESP_LOGE(TAG, "bad args: fname_S=%p size=%u",
                 (void*)fname_S, (unsigned)size);
        return NULL;
    }

    uint64_t t_start_us = esp_timer_get_time();  

    char real[256];
    map_path(real, sizeof(real), fname_S);

    ESP_LOGI(TAG, "load image: %s (uncompressed=%u bytes)",
             real, (unsigned)size);

    FILE *fp = fopen(real, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "fopen failed: %s", real);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        ESP_LOGE(TAG, "fseek(SEEK_END) failed: %s", real);
        fclose(fp);
        return NULL;
    }

    long file_size_long = ftell(fp);
    if (file_size_long <= 0) {
        ESP_LOGE(TAG, "ftell failed or empty file: %s", real);
        fclose(fp);
        return NULL;
    }

    size_t file_size = (size_t)file_size_long;
    rewind(fp);

    if (file_size == size) {
        ESP_LOGD(TAG, "image is RAW (%u bytes), reading directly", (unsigned)file_size);

        uint8_t *buf = (uint8_t*)heap_caps_malloc(size,
                            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!buf) {
            ESP_LOGE(TAG, "heap_caps_malloc(%u) failed", (unsigned)size);
            fclose(fp);
            return NULL;
        }

        size_t rd = fread(buf, 1, size, fp);
        fclose(fp);

        if (rd != size) {
            ESP_LOGE(TAG, "fread short: %u/%u", (unsigned)rd, (unsigned)size);
            heap_caps_free(buf);
            return NULL;
        }

        uint64_t t_end_us = esp_timer_get_time();
        ESP_LOGI(TAG,
                 "load RAW image done: %s (flash=%u, out=%u) in %llu ms",
                 real,
                 (unsigned)file_size,
                 (unsigned)size,
                 (unsigned long long)((t_end_us - t_start_us) / 1000ULL));

        return buf;
    }

    ESP_LOGD(TAG,
             "image assumed COMPRESSED (file=%u bytes, uncompressed=%u)",
             (unsigned)file_size, (unsigned)size);

    uint8_t *comp_buf = (uint8_t*)heap_caps_malloc(file_size, MALLOC_CAP_8BIT);
    if (!comp_buf) {
        ESP_LOGE(TAG, "heap_caps_malloc(comp_buf, %u) failed", (unsigned)file_size);
        fclose(fp);
        return NULL;
    }

    size_t rd = fread(comp_buf, 1, file_size, fp);
    fclose(fp);

    if (rd != file_size) {
        ESP_LOGE(TAG, "fread short (compressed): %u/%u",
                 (unsigned)rd, (unsigned)file_size);
        heap_caps_free(comp_buf);
        return NULL;
    }

    uint8_t *out_buf = (uint8_t*)heap_caps_malloc(size,
                            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!out_buf) {
        ESP_LOGE(TAG, "heap_caps_malloc(out_buf, %u) failed", (unsigned)size);
        heap_caps_free(comp_buf);
        return NULL;
    }

    uLongf dst_len = size;
    int zret = uncompress(out_buf, &dst_len, comp_buf, file_size);

    heap_caps_free(comp_buf);

    if (zret != Z_OK) {
        ESP_LOGE(TAG, "uncompress() failed: zret=%d", zret);
        heap_caps_free(out_buf);
        return NULL;
    }

    if (dst_len != size) {
        ESP_LOGE(TAG, "uncompress() size mismatch: got=%lu expected=%u",
                 (unsigned long)dst_len, (unsigned)size);
        heap_caps_free(out_buf);
        return NULL;
    }

    uint64_t t_end_us = esp_timer_get_time();
    ESP_LOGI(TAG,
             "load COMPRESSED image done: %s (flash=%u, out=%u) in %llu ms",
             real,
             (unsigned)file_size,
             (unsigned)size,
             (unsigned long long)((t_end_us - t_start_us) / 1000ULL));

    ESP_LOGD(TAG, "image decompressed OK: %s", real);
    return out_buf;
}

