#include "i2s_tx.h"

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_err.h"

#define AMP_BLK_PIN      GPIO_NUM_5
#define AMP_WS_PIN       GPIO_NUM_6
#define AMP_DATA_OUT_PIN GPIO_NUM_4

#define I2S_TX_SAMPLE_RATE  16000
#define I2S_TX_FRAME_LEN_MS 16
#define I2S_TX_FRAME_LEN    (I2S_TX_SAMPLE_RATE / 1000 * I2S_TX_FRAME_LEN_MS)
#define I2S_TX_FRAME_SZ     (I2S_TX_FRAME_LEN * 2)

#define I2S_TX_PORT_NUMBER I2S_NUM_1

static i2s_chan_handle_t s_tx_handle = NULL;

void i2s_tx_init()
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_TX_PORT_NUMBER, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &s_tx_handle, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(I2S_TX_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg =
            {
                .mclk = GPIO_NUM_NC,
                .bclk = AMP_BLK_PIN,
                .ws   = AMP_WS_PIN,
                .dout = AMP_DATA_OUT_PIN,
                .din  = GPIO_NUM_NC,
                .invert_flags =
                    {
                        .mclk_inv = false,
                        .bclk_inv = false,
                        .ws_inv   = false,
                    },
            },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(s_tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(s_tx_handle));
}

void i2s_tx_release()
{
    ESP_ERROR_CHECK(i2s_channel_disable(s_tx_handle));
    ESP_ERROR_CHECK(i2s_del_channel(s_tx_handle));
}

size_t i2s_tx_write(void* buffer, size_t bytes, size_t xTicksToWait)
{
    size_t wrote_bytes = 0;
    ESP_ERROR_CHECK(i2s_channel_write(s_tx_handle, buffer, bytes, &wrote_bytes, xTicksToWait));
    return wrote_bytes;
}
