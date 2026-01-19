#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include <stdbool.h>
#include <stdlib.h>

#include "entities/AudioPlayer.h"
#include "i2c_comm/i2c_protocol.h"

class HimaxModule
{
public:
    bool init(AudioPlayer* player);
    int waitReady(size_t timeout);
    int sendText(const char* str, size_t xTicksToWait);
    int start();
    int stop();
    int pause();
    int resume();

private:
    void dev_reset();
    int dev_probe();
    int dev_get_status(uint8_t& status);
    int dev_get_data_packet(data_packet_t& data);

    static void task(void*);

private:
    static constexpr char TAG[] = "HimaxModule";
    EventGroupHandle_t status_;
    AudioPlayer* player_;
    size_t dev_reset_counter_;
};
