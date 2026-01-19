#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#include "i2c_master.h"

#define I2C_MASTER_SDA_IO  GPIO_NUM_15
#define I2C_MASTER_SCL_IO  GPIO_NUM_16
#define I2C_MASTER_NUM     I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 400000
#define HIMAX_ADDON_ADDR     0x24

static const char* TAG_I2C_REC = "i2c_recover";

int i2c_master_init()
{
    i2c_port_t i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {0};
    conf.mode             = I2C_MODE_MASTER;
    conf.sda_io_num       = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en    = GPIO_PULLUP_ENABLE;
    conf.scl_io_num       = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en    = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags        = 0;

    esp_err_t err = i2c_param_config(i2c_master_port, &conf);
    if (err != ESP_OK) return err;

    err = i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
    if (err != ESP_OK) return err;

    return ESP_OK;
}

bool i2c_master_reset_driver(void)
{
    i2c_driver_delete(I2C_MASTER_NUM);
    return (i2c_master_init() == 0);
}

bool i2c_master_recover_bus(void)
{
    const gpio_num_t sda = I2C_MASTER_SDA_IO;
    const gpio_num_t scl = I2C_MASTER_SCL_IO;

    gpio_reset_pin(sda);
    gpio_reset_pin(scl);

    gpio_set_direction(sda, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_direction(scl, GPIO_MODE_INPUT_OUTPUT_OD);

    gpio_set_pull_mode(sda, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(scl, GPIO_PULLUP_ONLY);

    gpio_set_level(sda, 1);
    gpio_set_level(scl, 1);
    esp_rom_delay_us(50);

    int sda_lvl = gpio_get_level(sda);
    int scl_lvl = gpio_get_level(scl);

    ESP_LOGW(TAG_I2C_REC, "Recover start: SDA=%d SCL=%d", sda_lvl, scl_lvl);

    if (scl_lvl == 0) {
        for (int i = 0; i < 2000; i++) {
            esp_rom_delay_us(10);
            scl_lvl = gpio_get_level(scl);
            if (scl_lvl == 1) break;
        }
        if (scl_lvl == 0) {
            ESP_LOGE(TAG_I2C_REC, "SCL still LOW after wait, can't recover (device holds clock?)");
            i2c_set_pin(I2C_MASTER_NUM, sda, scl, GPIO_PULLUP_ENABLE, GPIO_PULLUP_ENABLE, I2C_MODE_MASTER);
            return false;
        }
    }

    if (gpio_get_level(sda) == 0) {
        for (int i = 0; i < 18; i++) {
            gpio_set_level(scl, 0);
            esp_rom_delay_us(5);

            gpio_set_level(scl, 1);

            for (int j = 0; j < 200; j++) { 
                if (gpio_get_level(scl) == 1) break;
                esp_rom_delay_us(5);
            }

            esp_rom_delay_us(5);

            if (gpio_get_level(sda) == 1) break; 
        }
    }

    gpio_set_level(sda, 0);
    esp_rom_delay_us(5);
    gpio_set_level(scl, 1);
    esp_rom_delay_us(5);
    gpio_set_level(sda, 1);
    esp_rom_delay_us(50);

    sda_lvl = gpio_get_level(sda);
    scl_lvl = gpio_get_level(scl);
    ESP_LOGW(TAG_I2C_REC, "Recover end:   SDA=%d SCL=%d", sda_lvl, scl_lvl);

    i2c_set_pin(I2C_MASTER_NUM, sda, scl, GPIO_PULLUP_ENABLE, GPIO_PULLUP_ENABLE, I2C_MODE_MASTER);

    return (sda_lvl == 1 && scl_lvl == 1);
}


int i2c_master_release()
{
    return i2c_driver_delete(I2C_MASTER_NUM);
}


int i2c_master_send(void* data, size_t len, size_t xTicksToWait)
{
    if ((len > 0) && (data == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = ESP_OK;

    ret = i2c_master_start(cmd);
    if (ret != ESP_OK) goto out;

    ret = i2c_master_write_byte(cmd,
                                (HIMAX_ADDON_ADDR << 1) | I2C_MASTER_WRITE,
                                true /* expect ack */);
    if (ret != ESP_OK) goto out;

    if (len > 0) {
        ret = i2c_master_write(cmd, (uint8_t*)data, len, true /* expect ack */);
        if (ret != ESP_OK) goto out;
    }

    ret = i2c_master_stop(cmd);
    if (ret != ESP_OK) goto out;

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, xTicksToWait);

out:
    i2c_cmd_link_delete(cmd);
    return ret;
}

int i2c_master_recv(void* data, size_t len, size_t xTicksToWait)
{
    if ((len > 0) && (data == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (len == 0) {
        return ESP_OK;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = ESP_OK;

    ret = i2c_master_start(cmd);
    if (ret != ESP_OK) goto out;

    ret = i2c_master_write_byte(cmd,
                                (HIMAX_ADDON_ADDR << 1) | I2C_MASTER_READ,
                                true /* expect ack */);
    if (ret != ESP_OK) goto out;

    if (len > 1) {
        ret = i2c_master_read(cmd, (uint8_t*)data, len - 1, I2C_MASTER_ACK);
        if (ret != ESP_OK) goto out;
    }

    ret = i2c_master_read_byte(cmd, ((uint8_t*)data) + (len - 1), I2C_MASTER_NACK);
    if (ret != ESP_OK) goto out;

    ret = i2c_master_stop(cmd);
    if (ret != ESP_OK) goto out;

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, xTicksToWait);

out:
    i2c_cmd_link_delete(cmd);
    return ret;
}


int i2c_master_scan()
{
    int i;
    esp_err_t espRc;
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    printf("00:         ");
    for (i = 3; i < 0x78; i++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MODE_SLAVE, 1 /* expect ack */);
        i2c_master_stop(cmd);

        espRc = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(10));
        if (i % 16 == 0) {
            printf("\n%.2x:", i);
        }
        if (espRc == 0) {
            printf(" %.2x", i);
        } else {
            printf(" --");
        }
        // ESP_LOGI(tag, "i=%d, rc=%d (0x%x)", i, espRc, espRc);
        i2c_cmd_link_delete(cmd);
    }
    printf("\n");
    return 0;
}

int i2c_master_dev_available()
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (HIMAX_ADDON_ADDR << 1) | I2C_MODE_SLAVE, 1 /* expect ack */);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(cmd);
    return err == ESP_OK;
}

#ifdef __cplusplus
}
#endif // __cplusplus
