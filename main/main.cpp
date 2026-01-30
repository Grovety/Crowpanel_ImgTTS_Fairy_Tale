#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"      

#include "esp_log.h"
#include "esp_err.h"
#include "esp_spiffs.h"          

#include "esp_display_panel.hpp"
#include "lvgl_v8_port.h"

#include "ui.h"                  
#include "tts_bridge.h"     

#include "HxTTS.h"
#include "uart_manager.h"

#include "uart_json.h"
#include "ui_events.h"
#include "user_name_store.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

static const char* TAG = "main";

HxTTS *g_hx_tts = nullptr;

static QueueHandle_t s_tts_queue   = nullptr;
static TaskHandle_t  s_tts_task    = nullptr;

static void tts_worker_task(void *arg)
{
    (void)arg;
    const char *text = nullptr;

    ESP_LOGI(TAG, "TTS worker task started");

    for (;;) {
        if (xQueueReceive(s_tts_queue, &text, portMAX_DELAY) == pdTRUE) {
            if (!text) {
                continue;
            }

            if (!g_hx_tts) {
                ESP_LOGE(TAG, "TTS worker: g_hx_tts == nullptr");
                continue;
            }

            ESP_LOGI(TAG, "TTS worker: speak text @%p", text);

            ui_bird_talk_anim_start();

            g_hx_tts->sendString(text);
            g_hx_tts->startPlayback();

        }
    }
}


static bool fs_ready_cb(lv_fs_drv_t*) { return true; }

static void* fs_open_cb(lv_fs_drv_t*, const char* path, lv_fs_mode_t mode) {
    
    char real[256];
    snprintf(real, sizeof real, "/spiffs/%s", path);

    const char* m = (mode & LV_FS_MODE_WR) ? ((mode & LV_FS_MODE_RD) ? "rb+" : "wb")
                                           : "rb";
    ESP_LOGI("FS","fopen: %s", real);
    FILE* fp = fopen(real, m);
    if(!fp) ESP_LOGE("FS","fopen failed");
    return fp; 
}

static lv_fs_res_t fs_close_cb(lv_fs_drv_t*, void* f) {
    return f && fclose((FILE*)f) == 0 ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}
static lv_fs_res_t fs_read_cb (lv_fs_drv_t*, void* f, void* buf, uint32_t btr, uint32_t* br){
    if(!f) return LV_FS_RES_FS_ERR;
    size_t n = fread(buf,1,btr,(FILE*)f);
    if(br) *br = (uint32_t)n;
    return ferror((FILE*)f) ? LV_FS_RES_FS_ERR : LV_FS_RES_OK;
}
static lv_fs_res_t fs_seek_cb (lv_fs_drv_t*, void* f, uint32_t pos, lv_fs_whence_t w){
    if(!f) return LV_FS_RES_FS_ERR;
    int wh = (w==LV_FS_SEEK_SET)?SEEK_SET:(w==LV_FS_SEEK_CUR)?SEEK_CUR:SEEK_END;
    return fseek((FILE*)f, (long)pos, wh)==0? LV_FS_RES_OK: LV_FS_RES_FS_ERR;
}
static lv_fs_res_t fs_tell_cb (lv_fs_drv_t*, void* f, uint32_t* pos){
    if(!f) return LV_FS_RES_FS_ERR;
    long p = ftell((FILE*)f);
    if(p < 0) return LV_FS_RES_FS_ERR;
    *pos = (uint32_t)p;
    return LV_FS_RES_OK;
}

void lvgl_register_drive_S(void) {
    lv_fs_drv_t d; lv_fs_drv_init(&d);
    d.letter   = 'S';
    d.ready_cb = fs_ready_cb;
    d.open_cb  = fs_open_cb;
    d.close_cb = fs_close_cb;
    d.read_cb  = fs_read_cb;
    d.seek_cb  = fs_seek_cb;
    d.tell_cb  = fs_tell_cb;
    lv_fs_drv_register(&d);

    char letters[16]={0}; lv_fs_get_letters(letters);
    ESP_LOGI("LVFS","letters: %s", letters);   
}

static void checkStatus(HxTTS& tts)
{
    hm_status_t status;
    if (tts.getStatus(status) != HxTTS::Error::OK) {
        ESP_LOGE(TAG, "unable to read status register");
        return;
    }
    ESP_LOGI(TAG, "status=%s", hm_status_to_str(status));
}

static void checkError(HxTTS& tts)
{
    hm_err_t error;
    if (tts.getError(error) != HxTTS::Error::OK) {
        ESP_LOGE(TAG, "unable to read error register");
        return;
    }
    ESP_LOGI(TAG, "error=%s", hm_err_to_str(error));
}

extern "C" void start_tts_playback_impl(const char *text)
{
    if (!text) {
        return;
    }
    if (!g_hx_tts) {
        ESP_LOGE(TAG, "start_tts_playback_impl: g_hx_tts == nullptr");
        return;
    }
    if (!s_tts_queue) {
        ESP_LOGE(TAG, "start_tts_playback_impl: TTS queue not initialized");
        return;
    }

    const char *msg = text;

    if (xQueueSend(s_tts_queue, &msg, 0) != pdPASS) {
        ESP_LOGW(TAG, "start_tts_playback_impl: TTS queue full, drop request");
    }
}

extern "C" void stop_tts_playback_impl(void)
{
    if (!g_hx_tts) {
        ESP_LOGW(TAG, "stop_tts_playback_impl: g_hx_tts == nullptr");
        return;
    }

    g_hx_tts->stopPlayback();
}

extern "C" void pause_tts_playback_impl(void)
{
    if (!g_hx_tts) {
        ESP_LOGW(TAG, "pause_tts_playback_impl: g_hx_tts == nullptr");
        return;
    }

    g_hx_tts->pausePlayback();
}

extern "C" void resume_tts_playback_impl(void)
{
    if (!g_hx_tts) {
        ESP_LOGW(TAG, "resume_tts_playback_impl: g_hx_tts == nullptr");
        return;
    }

    g_hx_tts->resumePlayback();
}

extern "C" void app_main()
{
    esp_log_level_set("HxTTS",     ESP_LOG_WARN);  
    esp_log_level_set("HM_COMM",   ESP_LOG_WARN);  
    esp_log_level_set("GIF_LOADING", ESP_LOG_WARN); 
    esp_log_level_set("ui_events", ESP_LOG_INFO);  
    esp_log_level_set("UIIMG",       ESP_LOG_WARN);


    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 12,
        .format_if_mount_failed = false
    };
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    Board* board = new Board();
    ESP_UTILS_CHECK_FALSE_EXIT(board->init(),  "Board init failed");
    ESP_UTILS_CHECK_FALSE_EXIT(board->begin(), "Board begin failed");
    ESP_UTILS_CHECK_FALSE_EXIT(lvgl_port_init(board->getLCD(), board->getTouch()),
                               "LVGL init failed");

    lvgl_register_drive_S();

    ui_init();

    ESP_UTILS_CHECK_FALSE_EXIT(lvgl_port_start(), "LVGL start failed");

    g_hx_tts = new HxTTS(HxTTS::BusType::UART);
    if (!g_hx_tts) {
        ESP_LOGE(TAG, "Failed to create HxTTS instance");
        return;
    }

    s_tts_queue = xQueueCreate(4, sizeof(const char *));
    if (!s_tts_queue) {
        ESP_LOGE(TAG, "Failed to create TTS queue");
        return;
    }

    BaseType_t r = xTaskCreatePinnedToCore(
        tts_worker_task,
        "tts_worker",
        4096,
        nullptr,
        tskIDLE_PRIORITY+1,   
        &s_tts_task,
        1
    );
    if (r != pdPASS) {
        ESP_LOGE(TAG, "Failed to create TTS worker task");
        return;
    }

    register_start_tts_cb(start_tts_playback_impl);

    uart_json_init(on_text_update_from_uart);
    
    checkStatus(*g_hx_tts);
    checkError(*g_hx_tts);
}