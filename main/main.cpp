#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_spiffs.h"

#include "driver/i2c.h"

#include "esp_display_panel.hpp"
#include "port/esp_io_expander.h"
#include "port/esp_io_expander_tca9554.h"
#include "lvgl_port_v8.h"

#include "ui.h"
#include "tts_bridge.h"

#include "uart_manager.h"
#include "uart_json.h"
#include "ui_events.h"
#include "user_name_store.h"

#include "entities/AudioPlayer.h"
#include "entities/HimaxModule.h"
#include "entities/i2c_comm/i2c_master.h"
#include "entities/i2c_comm/i2c_protocol.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

static const char* TAG = "main";

#define I2C_MASTER_TIMEOUT_MS  1000
#define I2C_PORT_NUM           I2C_NUM_0

#define BACKLIGHT_ADDR_V1_1    0x30
#define BACKLIGHT_ADDR_V1_0    0x18

#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_WARN

static AudioPlayer   s_audio_player;
static HimaxModule   s_himax_module;

SemaphoreHandle_t    i2c_mtx = nullptr;

static QueueHandle_t s_tts_queue = nullptr;
static TaskHandle_t  s_tts_task  = nullptr;

static esp_err_t native_i2c_write_byte(uint8_t dev_addr, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t native_i2c_probe(uint8_t dev_addr)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static void tts_worker_task(void* arg)
{
    (void)arg;
    const char* text = nullptr;

    ESP_LOGI(TAG, "TTS worker task started");

    for (;;) {
        if (xQueueReceive(s_tts_queue, &text, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (!text) {
            continue;
        }

        ESP_LOGD(TAG, "TTS worker: speak text @%p", text);

        if (s_himax_module.sendText(text, pdMS_TO_TICKS(10)) != 0) {
            ESP_LOGE(TAG, "HimaxModule::sendText failed");
            break;
        }
        if (s_himax_module.start() != 0) {
            ESP_LOGE(TAG, "HimaxModule::start failed");
        }
    }
}

static bool fs_ready_cb(lv_fs_drv_t*) { return true; }

static void* fs_open_cb(lv_fs_drv_t*, const char* path, lv_fs_mode_t mode)
{
    
    char real[256];
    snprintf(real, sizeof real, "/spiffs/%s", path);

    const char* m = (mode & LV_FS_MODE_WR) ? ((mode & LV_FS_MODE_RD) ? "rb+" : "wb") : "rb";
    ESP_LOGI("FS","fopen: %s", real);
    FILE* fp = fopen(real, m);
    if (! fp)
        ESP_LOGE("FS", "fopen failed");
    return fp; 
}

static lv_fs_res_t fs_close_cb(lv_fs_drv_t*, void* f)
{
    return f && fclose((FILE*)f) == 0 ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}
static lv_fs_res_t fs_read_cb (lv_fs_drv_t*, void* f, void* buf, uint32_t btr, uint32_t* br)
{
    if (! f)
        return LV_FS_RES_FS_ERR;
    size_t n = fread(buf, 1, btr, (FILE*)f);
    if (br)
        *br = (uint32_t)n;
    return ferror((FILE*)f) ? LV_FS_RES_FS_ERR : LV_FS_RES_OK;
}
static lv_fs_res_t fs_seek_cb (lv_fs_drv_t*, void* f, uint32_t pos, lv_fs_whence_t w)
{
    if (! f)
        return LV_FS_RES_FS_ERR;
    int wh = (w == LV_FS_SEEK_SET) ? SEEK_SET : (w == LV_FS_SEEK_CUR) ? SEEK_CUR : SEEK_END;
    return fseek((FILE*)f, (long)pos, wh) == 0 ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}
static lv_fs_res_t fs_tell_cb (lv_fs_drv_t*, void* f, uint32_t* pos)
{
    if (! f)
        return LV_FS_RES_FS_ERR;
    long p = ftell((FILE*)f);
    if (p < 0)
        return LV_FS_RES_FS_ERR;
    *pos = (uint32_t)p;
    return LV_FS_RES_OK;
}

void lvgl_register_drive_S(void)
{
    lv_fs_drv_t d;
    lv_fs_drv_init(&d);
    d.letter   = 'S';
    d.ready_cb = fs_ready_cb;
    d.open_cb  = fs_open_cb;
    d.close_cb = fs_close_cb;
    d.read_cb  = fs_read_cb;
    d.seek_cb  = fs_seek_cb;
    d.tell_cb  = fs_tell_cb;
    lv_fs_drv_register(&d);

    char letters[16] = {0};
    lv_fs_get_letters(letters);
    ESP_LOGI("LVFS", "letters: %s", letters);   
}

extern "C" void start_tts_playback_impl(const char* text)
{
    if (! text) {
        return;
    }
    if (! s_tts_queue) {
        ESP_LOGE(TAG, "start_tts_playback_impl: TTS queue not initialized");
        return;
    }

    const char* msg = text;

    if (xQueueSend(s_tts_queue, &msg, 0) != pdPASS) {
        ESP_LOGW(TAG, "start_tts_playback_impl: TTS queue full, drop request");
    }
}


extern "C" void stop_tts_playback_impl(void)
{
    ESP_LOGI(TAG, "stop_tts_playback_impl: abort Himax TTS and stop audio");

    s_himax_module.stop();

    ui_bird_talk_anim_stop();
}

extern "C" void pause_tts_playback_impl(void)
{
    ESP_LOGI(TAG, "pause_tts_playback_impl: pause Himax and mute audio");

    s_himax_module.pause();
}

extern "C" void resume_tts_playback_impl(void)
{
    ESP_LOGI(TAG, "resume_tts_playback_impl: resume Himax and unmute audio");

    s_himax_module.resume();
}



bool i2c_bus_lock(int timeout_ms)
{
    if (! i2c_mtx) {
        ESP_LOGE(TAG, "i2c mutex is not initialized");
        return false;
    }
    const TickType_t timeout_ticks =
        (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return (xSemaphoreTake(i2c_mtx, timeout_ticks) == pdTRUE);
}

bool i2c_bus_unlock(void)
{
    if (! i2c_mtx) {
        ESP_LOGE(TAG, "i2c mutex is not initialized");
        return false;
    }
    xSemaphoreGive(i2c_mtx);
    return true;
}

extern "C" void app_main()
{
    esp_log_level_set("*", ESP_LOG_WARN);

    esp_log_level_set("ui_events", ESP_LOG_INFO);
    esp_log_level_set("HimaxModule", ESP_LOG_INFO);
    esp_log_level_set("AudioPlayer", ESP_LOG_INFO);

    esp_log_level_set("uart", ESP_LOG_WARN);
    esp_log_level_set("uart_manager", ESP_LOG_WARN);
    esp_log_level_set("UART_JSON_MODULE", ESP_LOG_WARN);
    esp_log_level_set("UIIMG", ESP_LOG_WARN);
    
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 12,
        .format_if_mount_failed = false
    };
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    i2c_master_init();

    i2c_mtx = xSemaphoreCreateMutex();
    if (!i2c_mtx) {
        ESP_LOGE(TAG, "Create i2c mutex failed");
        return;
    }

    bool is_v1_1 = false;
    int retries = 3;

    esp_io_expander_handle_t io_expander = NULL;

    ESP_LOGW(TAG, "Scanning for Backlight Controller (0x30)...");

    while (retries > 0) {

        esp_err_t probe_err = native_i2c_probe(BACKLIGHT_ADDR_V1_1);

        if (probe_err == ESP_OK) {
            ESP_LOGW(TAG, "Hardware V1.1 detected (0x30 Found via Probe).");
            
            esp_err_t write_err = native_i2c_write_byte(BACKLIGHT_ADDR_V1_1, 0x10);
            
            if (write_err == ESP_OK) {
                ESP_LOGW(TAG, "Backlight V1.1 enabled command sent.");
            } else {
                ESP_LOGW(TAG, "Device 0x30 found, but write command failed.");
            }
            
            is_v1_1 = true;
            break;
        } else {
            ESP_LOGW(TAG, "0x30 not found. Attempts left: %d", retries - 1);
        }

        retries--;

        vTaskDelay(pdMS_TO_TICKS(120));
    }

    if (! is_v1_1) {
        ESP_LOGW(TAG, "Detected V1.0 Hardware. Switching to TCA9534 (0x18)...");

        esp_err_t err = esp_io_expander_new_i2c_tca9554(I2C_PORT_NUM, BACKLIGHT_ADDR_V1_0, &io_expander);
        
        if (err == ESP_OK && io_expander) {
            ESP_LOGW(TAG, "IO Expander initialized successfully. Configuring pins...");
            
            i2c_bus_lock(-1);
            uint32_t output_pins = BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(7);

            esp_io_expander_set_dir(io_expander, output_pins, IO_EXPANDER_OUTPUT);
            esp_io_expander_set_level(io_expander, BIT(1) | BIT(7) | BIT(3), 1);

            gpio_set_direction(GPIO_NUM_1, GPIO_MODE_OUTPUT);
            gpio_set_level(GPIO_NUM_1, 0);

            esp_io_expander_set_level(io_expander, BIT(2), 0);
            vTaskDelay(pdMS_TO_TICKS(20));
            esp_io_expander_set_level(io_expander, BIT(2), 1);

            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_direction(GPIO_NUM_1, GPIO_MODE_INPUT);

            i2c_bus_unlock();

            ESP_LOGW(TAG, "V1.0 Init sequence completed.");
            
        } else {
            ESP_LOGE(TAG, "Failed to init TCA9534: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    Board* board = new Board();
    ESP_UTILS_CHECK_FALSE_EXIT(board->init(), "Board init failed");
    ESP_UTILS_CHECK_FALSE_EXIT(board->begin(), "Board begin failed");

    int himax_reset_pin = is_v1_1 ? GPIO_NUM_8 : GPIO_NUM_0;
    ESP_LOGW(TAG, "Initializing Himax Module with Reset Pin: %d", himax_reset_pin);
    
    s_audio_player.init(io_expander, PAYLOAD_SIZE);
    s_himax_module.init(&s_audio_player, himax_reset_pin);

    ESP_UTILS_CHECK_FALSE_EXIT(lvgl_port_init(board->getLCD(), board->getTouch()), "LVGL init failed");

    lvgl_register_drive_S();
    ui_init();

    s_tts_queue = xQueueCreate(4, sizeof(const char*));
    if (! s_tts_queue) {
        ESP_LOGE(TAG, "Failed to create TTS queue");
        return;
    }

    BaseType_t r = xTaskCreatePinnedToCore(
        tts_worker_task,
        "tts_worker",
        4096,
        nullptr,
        tskIDLE_PRIORITY,   
        &s_tts_task,
        0          
    );
    if (r != pdPASS) {
        ESP_LOGE(TAG, "Failed to create TTS worker task");
        return;
    }

    register_start_tts_cb(start_tts_playback_impl);

    uart_json_init(on_text_update_from_uart);
}
