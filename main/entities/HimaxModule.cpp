#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include "HimaxModule.h"
#include "i2c_comm/crc_table.h"
#include "i2c_comm/i2c_master.h"

#define HIMAX_RESET_PIN GPIO_NUM_0

#define STATUS_HM_BUSY   BIT0
#define STATUS_HM_PAUSED BIT1

extern bool i2c_bus_lock(int timeout_ms);
extern bool i2c_bus_unlock(void);

extern "C" void ui_notify_tts_finished(void);
extern "C" void ui_bird_talk_anim_stop(void);

void HimaxModule::task(void* arg)
{
    HimaxModule* himax_module = static_cast<HimaxModule*>(arg);
    esp_err_t err;

    const TickType_t xDelay = pdMS_TO_TICKS(14);

    while (1) {
        if (xEventGroupWaitBits(himax_module->status_,
                                STATUS_HM_BUSY,
                                pdFALSE,
                                pdFALSE,
                                portMAX_DELAY) & STATUS_HM_BUSY) {

            size_t packet_counter            = 0;
            size_t corrupted_packets_counter = 0;
            bool stop                        = false;
            bool abort_due_to_i2c            = false;
            uint8_t status                   = 0xff;

            ESP_LOGI(TAG, "Start audio recv");

            TickType_t xLastWakeTime = xTaskGetTickCount();

            while (!stop) {
                // pause support
                if (xEventGroupGetBits(himax_module->status_) & STATUS_HM_PAUSED) {
                    break;
                }

                size_t bursted_packets = 0;

                // --- status poll ---
                err = himax_module->dev_get_status(status);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Unable to get dev status");

                    // IMPORTANT: avoid hammering I2C when the bus is stuck
                    vTaskDelay(pdMS_TO_TICKS(20));
                    continue;
                }

                switch (status) {
                case ST_OUTPUT_RDY: {
                    xLastWakeTime = xTaskGetTickCount();

                    while (1) {
                        data_packet_t data;
                        memset(&data, 0, sizeof(data_packet_t));

                        err = himax_module->dev_get_data_packet(data);
                        if (err != ESP_OK) {
                            // dev_get_data_packet() already tried recovery+retry (if you implemented it there)
                            abort_due_to_i2c = true;
                            break;
                        }

                        if (data.error != ERR_OK) {
                            // device reported an error / no more data / etc.
                            break;
                        }

                        uint16_t crc = crc16_compute(crc16_lut,
                                                     &crc16_ccitt_false_config,
                                                     (const uint8_t*)data.data,
                                                     data.data_length);

                        ESP_LOGD(TAG,
                                 "rx packet: err=%u, idx=%u, total=%u, length=%u, crc=%u(%u)",
                                 data.error, data.index, data.total, data.data_length, data.crc, crc);

                        if (data.crc != crc) {
                            corrupted_packets_counter++;
                            continue;
                        }

                        bursted_packets++;
                        packet_counter++;

                        // If player is off and its buffer is full, will block indefinitely
                        himax_module->player_->write(data.data, data.data_length, portMAX_DELAY);

                        ESP_LOG_BUFFER_HEXDUMP(TAG, &data, sizeof(data_packet_t), ESP_LOG_VERBOSE);

                        vTaskDelayUntil(&xLastWakeTime, xDelay);
                    }
                } break;

                case ST_IDLE:
                    // make sure that dev have started
                    if (packet_counter != 0) {
                        stop = true;
                    }
                    break;

                default:
                    break;
                }

                // If packet read failed hard (bus stuck even after recovery), stop this TTS
                if (abort_due_to_i2c) {
                    stop = true;
                }

                if (bursted_packets > 0) {
                    ESP_LOGI(TAG, "Bursted %u packets", bursted_packets);
                }

                if (packet_counter == 0) {
                    vTaskDelay(pdMS_TO_TICKS(500)); // lower frequency while dev is busy / not outputting yet
                } else {
                    vTaskDelayUntil(&xLastWakeTime, xDelay);
                }
            }

            if (packet_counter > 0) {
                ESP_LOGI(TAG, "Recieved %u packets", packet_counter);
            }
            if (corrupted_packets_counter > 0) {
                ESP_LOGW(TAG, "Recieved %u corrupted packets", corrupted_packets_counter);
            }

            const EventBits_t bits_now = xEventGroupGetBits(himax_module->status_);
            const bool paused_now      = (bits_now & STATUS_HM_PAUSED) != 0;

            const bool finished_now = (!paused_now) && (!abort_due_to_i2c) && (packet_counter != 0) && stop;

            if (finished_now) {
                ui_bird_talk_anim_stop();
                ui_notify_tts_finished();
            }

            xEventGroupClearBits(himax_module->status_, STATUS_HM_BUSY);
        }
    }
}

int HimaxModule::waitReady(size_t timeout)
{
    static constexpr size_t polling_period = pdMS_TO_TICKS(500);
    uint8_t status                         = 0xff;
    size_t elapsed                         = 0;
    do {
        esp_err_t ret = dev_get_status(status);
        if (ret != ESP_OK) {
            return ret;
        }
        vTaskDelay(polling_period);
        elapsed += polling_period;
        if (elapsed > timeout) {
            return ESP_ERR_TIMEOUT;
        }
    } while (status != ST_IDLE);
    return 0;
}

int HimaxModule::sendText(const char* str, size_t xTicksToWait)
{
    if (xEventGroupGetBits(status_) & STATUS_HM_BUSY) {
        return -1;
    }

    if (dev_probe() < 0) {
        return -1;
    }

    if (xEventGroupGetBits(status_) & STATUS_HM_PAUSED) {
        stop();
        if (waitReady(xTicksToWait) < 0) {
            return -1;
        }
    }

    esp_err_t err = ESP_OK;
    size_t n = strlen(str);
    ESP_LOGI(TAG, "send text: len=%u", (unsigned)n);


    transaction_t tr = {.cmd = CMD_RECV_MSG, .data_length = static_cast<uint16_t>(n)};
    i2c_bus_lock(-1);
    err = i2c_master_send((void*)&tr, sizeof(transaction_t), xTicksToWait);
    if (err != ESP_OK) {
        i2c_bus_unlock();
        return -1;
    }
    err = i2c_master_send((void*)str, n, pdMS_TO_TICKS(1000));
    if (err != ESP_OK) {
        i2c_bus_unlock();
        return -1;
    }
    i2c_bus_unlock();

    return 0;
}

int HimaxModule::start()
{
    if (xEventGroupGetBits(status_) & STATUS_HM_BUSY) {
        return -1;
    }

    esp_err_t err    = ESP_OK;
    transaction_t tr = {.cmd = CMD_START, .data_length = 0};
    i2c_bus_lock(-1);
    err = i2c_master_send((void*)&tr, sizeof(transaction_t), pdMS_TO_TICKS(50));
    if (err != ESP_OK) {
        i2c_bus_unlock();
        return -1;
    }
    i2c_bus_unlock();

    xEventGroupSetBits(status_, STATUS_HM_BUSY);
    return 0;
}

int HimaxModule::stop()
{
    if (dev_probe() < 0) {
        return -1;
    }

    esp_err_t err    = ESP_OK;
    transaction_t tr = {.cmd = CMD_STOP, .data_length = 0};
    i2c_bus_lock(-1);
    err = i2c_master_send((void*)&tr, sizeof(transaction_t), pdMS_TO_TICKS(50));
    if (err != ESP_OK) {
        i2c_bus_unlock();
        return -1;
    }
    i2c_bus_unlock();

    xEventGroupClearBits(status_, STATUS_HM_BUSY | STATUS_HM_PAUSED);
    player_->stop();
    return 0;
}

int HimaxModule::pause()
{
    const auto xBits = xEventGroupGetBits(status_);
    if ((xBits & STATUS_HM_PAUSED) || ! (xBits & STATUS_HM_BUSY)) {
        return -1;
    }

    if (dev_probe() < 0) {
        return -1;
    }

    esp_err_t err    = ESP_OK;
    transaction_t tr = {.cmd = CMD_PAUSE, .data_length = 0};
    i2c_bus_lock(-1);
    err = i2c_master_send((void*)&tr, sizeof(transaction_t), pdMS_TO_TICKS(50));
    if (err != ESP_OK) {
        i2c_bus_unlock();
        return -1;
    }
    i2c_bus_unlock();

    xEventGroupSetBits(status_, STATUS_HM_PAUSED);
    player_->pause();
    return 0;
}

int HimaxModule::resume()
{
    if (! (xEventGroupGetBits(status_) & STATUS_HM_PAUSED)) {
        return -1;
    }

    if (dev_probe() < 0) {
        return -1;
    }

    esp_err_t err    = ESP_OK;
    transaction_t tr = {.cmd = CMD_RESUME, .data_length = 0};
    i2c_bus_lock(-1);
    err = i2c_master_send((void*)&tr, sizeof(transaction_t), pdMS_TO_TICKS(50));
    if (err != ESP_OK) {
        i2c_bus_unlock();
        return -1;
    }
    i2c_bus_unlock();

    xEventGroupClearBits(status_, STATUS_HM_PAUSED);
    xEventGroupSetBits(status_, STATUS_HM_BUSY);
    player_->resume();
    return 0;
}

bool HimaxModule::init(AudioPlayer* player)
{
    dev_reset_counter_ = 0;
    player_            = player;

    status_ = xEventGroupCreate();
    if (! status_) {
        ESP_LOGE(TAG, "Unable to crate status bits");
        return false;
    }

    if (xTaskCreate(task, "himax_i2c", 4096, this, 1, nullptr) != pdTRUE)
        return false;

    gpio_reset_pin(HIMAX_RESET_PIN);
    gpio_set_direction(HIMAX_RESET_PIN, GPIO_MODE_OUTPUT);
    dev_reset();
    vTaskDelay(pdMS_TO_TICKS(300));

    if (dev_probe() < 0) {
        return false;
    }

    uint8_t status;
    esp_err_t err = dev_get_status(status);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Unable to get dev status");
        return false;
    }

    return true;
}

void HimaxModule::dev_reset()
{
    ESP_LOGI(TAG, "Reset device: %u", dev_reset_counter_++);
    gpio_set_level(HIMAX_RESET_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(HIMAX_RESET_PIN, 1);
}

int HimaxModule::dev_probe()
{
    i2c_bus_lock(-1);
    if (! i2c_master_dev_available()) {
        i2c_bus_unlock();
        ESP_LOGW(TAG, "Himax module is not available on I2C bus");
        return -1;
    }
    i2c_bus_unlock();
    return 0;
}

int HimaxModule::dev_get_status(uint8_t& status)
{
    esp_err_t err    = ESP_OK;
    transaction_t tr = {.cmd = CMD_GET_STATUS, .data_length = 0};
    i2c_bus_lock(-1);
    err = i2c_master_send((void*)&tr, sizeof(transaction_t), pdMS_TO_TICKS(50));
    if (err != ESP_OK) {
        if (err == ESP_ERR_TIMEOUT) {
        int sda = gpio_get_level(GPIO_NUM_15);
        int scl = gpio_get_level(GPIO_NUM_16);
        ESP_LOGE(TAG, "I2C TIMEOUT (Himax TX status). Lines: SDA=%d SCL=%d", sda, scl);
        }
        i2c_bus_unlock();
        ESP_LOGE(TAG, "I2C err=%s", esp_err_to_name(err));
        return err;
    }
    memset(&tr, 0, sizeof(transaction_t));
    err = i2c_master_recv((void*)&tr, sizeof(transaction_t), pdMS_TO_TICKS(50));
    if (err != ESP_OK) {
        if (err == ESP_ERR_TIMEOUT) {
        int sda = gpio_get_level(GPIO_NUM_15);
        int scl = gpio_get_level(GPIO_NUM_16);
        ESP_LOGE(TAG, "I2C TIMEOUT (Himax RX status). Lines: SDA=%d SCL=%d", sda, scl);
    }
        i2c_bus_unlock();
        ESP_LOGE(TAG, "I2C err=%s", esp_err_to_name(err));
        return err;
    }
    i2c_bus_unlock();
    status = tr.status;
    ESP_LOGD(TAG, "status=%s", status_to_str(status));
    return ESP_OK;
}

int HimaxModule::dev_get_data_packet(data_packet_t& data)
{
    esp_err_t err = ESP_OK;
    transaction_t tr = {.cmd = CMD_SEND_DATA, .data_length = 0};

    i2c_bus_lock(-1);

    // Helper lambda: handle TIMEOUT based on line levels
    auto handle_timeout = [&](const char* what) {
        int sda1 = gpio_get_level(GPIO_NUM_15);
        int scl1 = gpio_get_level(GPIO_NUM_16);

        esp_rom_delay_us(3000); // 3ms

        int sda2 = gpio_get_level(GPIO_NUM_15);
        int scl2 = gpio_get_level(GPIO_NUM_16);

        ESP_LOGE(TAG,
                 "I2C TIMEOUT (%s). Lines: before SDA=%d SCL=%d; after3ms SDA=%d SCL=%d",
                 what, sda1, scl1, sda2, scl2);

        // Case A: stuck-bus (any line held low) -> do GPIO bus recovery (9 pulses + STOP)
        if ((sda2 == 0) || (scl2 == 0)) {
            bool ok = i2c_master_recover_bus();
            ESP_LOGW(TAG, "I2C recover (bus-clear) result: %s", ok ? "OK" : "FAIL");
            return;
        }

        // Case B: lines look idle (SDA=1, SCL=1) but driver timed out -> re-init I2C peripheral/driver
        // This does NOT change I2C clock, it just resets the controller state.
        ESP_LOGW(TAG, "I2C timeout with idle lines -> resetting I2C driver");
        bool ok = i2c_master_reset_driver();
        ESP_LOGW(TAG, "I2C reset driver result: %s", ok ? "OK" : "FAIL");

    };

    for (int attempt = 0; attempt < 2; attempt++) {
        // TX command
        err = i2c_master_send((void*)&tr, sizeof(transaction_t), pdMS_TO_TICKS(50));
        if (err != ESP_OK) {
            if (err == ESP_ERR_TIMEOUT) {
                handle_timeout("Himax TX pkt");
                continue; // retry
            }

            i2c_bus_unlock();
            ESP_LOGE(TAG, "I2C (w) err=%s", esp_err_to_name(err));
            return err;
        }

        // RX packet
        memset(&data, 0, sizeof(data_packet_t));
        err = i2c_master_recv((void*)&data, sizeof(data_packet_t), pdMS_TO_TICKS(50));
        if (err != ESP_OK) {
            if (err == ESP_ERR_TIMEOUT) {
                handle_timeout("Himax RX pkt");
                continue; // retry
            }

            i2c_bus_unlock();
            ESP_LOGE(TAG, "I2C (r) err=%s", esp_err_to_name(err));
            return err;
        }

        // success
        i2c_bus_unlock();
        return ESP_OK;
    }

    i2c_bus_unlock();
    return ESP_ERR_TIMEOUT;
}


