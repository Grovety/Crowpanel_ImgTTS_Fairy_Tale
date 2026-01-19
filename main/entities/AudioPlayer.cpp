#include <lvgl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"

#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include "AudioPlayer.h"
#include "i2s/i2s_tx.h"

extern "C" void ui_bird_talk_anim_stop(void);
extern "C" void ui_bird_talk_anim_start(void);

extern "C" void ui_notify_audio_playback_finished(void);

#define SPK_MUTE_PIN GPIO_NUM_3
#define SPK_SHUT_PIN GPIO_NUM_4

#define STATUS_ON      BIT0
#define STATUS_PLAYING BIT1

extern bool i2c_bus_lock(int timeout_ms);
extern bool i2c_bus_unlock(void);

void AudioPlayer::control_task(void* arg)
{
    AudioPlayer* player = static_cast<AudioPlayer*>(arg);
    for (;;) {
        if (! player->isOn()) {
            if (fifo_ringbuf_size(player->audio_buffer_) > kPlaybackStartThre) {
                ESP_LOGD(TAG, "Start playback");
                player->enable();
                xEventGroupSetBits(player->status_, STATUS_PLAYING);

                ui_bird_talk_anim_start();
            }
        }
        else {
            while (! fifo_ringbuf_empty(player->audio_buffer_)) {
                vTaskDelay(pdMS_TO_TICKS(100));
            }

            xEventGroupClearBits(player->status_, STATUS_PLAYING);
            ESP_LOGD(TAG, "Stop playback");
            player->disable();

            ui_bird_talk_anim_stop();

            ui_notify_audio_playback_finished();
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void AudioPlayer::play_task(void* arg)
{
    AudioPlayer* player = static_cast<AudioPlayer*>(arg);
    uint8_t* buffer     = (uint8_t*)malloc(player->frame_size_);
    size_t read         = 0;
    for (;;) {
        if (xEventGroupWaitBits(player->status_, STATUS_PLAYING, pdFALSE, pdFALSE, pdMS_TO_TICKS(100)) &
            STATUS_PLAYING) {
            read = fifo_ringbuf_read(player->audio_buffer_, buffer, player->frame_size_, pdMS_TO_TICKS(100));
            if (read) {
                ESP_LOGV(TAG, "RB, pop, %u", fifo_ringbuf_size(player->audio_buffer_));
                i2s_tx_write(buffer, read, portMAX_DELAY);
            }
        }
    }
    free(buffer);
}

bool AudioPlayer::init(esp_expander::Base* io_expander, size_t frame_size_)
{
    this->io_expander_ = io_expander;

    this->frame_size_ = frame_size_;
    audio_buffer_     = fifo_ringbuf_init(kRingBufferSize, frame_size_);
    if (! audio_buffer_) {
        return false;
    }

    i2s_tx_init();

    if (this->io_expander_) {
        i2c_bus_lock(-1);
        io_expander_->pinMode(SPK_MUTE_PIN, OUTPUT);
        io_expander_->pinMode(SPK_SHUT_PIN, OUTPUT);
        io_expander_->digitalWrite(SPK_MUTE_PIN, HIGH);
        io_expander_->digitalWrite(SPK_SHUT_PIN, LOW);
        i2c_bus_unlock();
    }

    status_ = xEventGroupCreate();
    if (! status_) {
        ESP_LOGE(TAG, "Unable to crate status bits");
        return false;
    }
    xEventGroupClearBits(status_, STATUS_PLAYING);

    if (xTaskCreate(control_task, "PlayerControlTask", 4096, this, 1, nullptr) != pdTRUE)
        return false;

    if (xTaskCreate(play_task, "PlayerPlayTask", 4096, this, 1, nullptr) != pdTRUE)
        return false;

    return true;
}

bool AudioPlayer::write(const uint8_t* data, size_t bytes, size_t ticks_to_wait)
{
    ESP_LOGV(TAG, "RB, push, %u", fifo_ringbuf_size(audio_buffer_));
    return fifo_ringbuf_write(audio_buffer_, data, bytes, ticks_to_wait) == bytes;
}

bool AudioPlayer::enable()
{
    if (xEventGroupGetBits(status_) & STATUS_ON) {
        return false;
    }

    if (io_expander_) {
        i2c_bus_lock(-1);
        io_expander_->digitalWrite(SPK_MUTE_PIN, LOW);
        i2c_bus_unlock();
    }
    
    xEventGroupSetBits(status_, STATUS_ON);
    return true;
}

bool AudioPlayer::disable()
{
    if (! (xEventGroupGetBits(status_) & STATUS_ON)) {
        return false;
    }

    if (io_expander_) {
        i2c_bus_lock(-1);
        io_expander_->digitalWrite(SPK_MUTE_PIN, HIGH);
        i2c_bus_unlock();
    }
    
    xEventGroupClearBits(status_, STATUS_ON);
    return true;
}

bool AudioPlayer::pause()
{
    if (xEventGroupGetBits(status_) & STATUS_PLAYING) {
        return false;
    }
    xEventGroupClearBits(status_, STATUS_PLAYING);
    return true;
}

bool AudioPlayer::resume()
{
    if (! (xEventGroupGetBits(status_) & STATUS_PLAYING)) {
        return false;
    }
    xEventGroupSetBits(status_, STATUS_PLAYING);
    ui_bird_talk_anim_start();
    return true;
}

bool AudioPlayer::stop()
{
    pause();
    fifo_ringbuf_reset(audio_buffer_);
    return true;
}

bool AudioPlayer::isOn()
{
    return xEventGroupGetBits(status_) & STATUS_ON;
}
