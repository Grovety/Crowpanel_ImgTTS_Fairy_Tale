#pragma once

#include "esp_display_panel.hpp"
#include "freertos/event_groups.h"

#include "fifo_ringbuf.h"

class AudioPlayer
{
public:
    bool init(esp_expander::Base* io_expander, size_t frame_size);
    bool write(const uint8_t* data, size_t bytes, size_t ticks_to_wait);
    bool isOn();
    bool enable();
    bool disable();
    bool pause();
    bool resume();
    bool stop();

private:
    static void control_task(void*);
    static void play_task(void*);

    static constexpr size_t kRingBufferSize    = 50;
    static constexpr size_t kPlaybackStartThre = 20;
    static constexpr char TAG[]                = "AudioPlayer";
    fifo_ringbuf_t* audio_buffer_;
    size_t frame_size_;
    esp_expander::Base* io_expander_;
    EventGroupHandle_t status_;
};
