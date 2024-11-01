#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_MASTER_SCL_IO 2
#define I2C_MASTER_SDA_IO 3
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 50000
#define MEMORY_ADDR 0x00

static const char *TAG = "I2C_Reader";
uint8_t DEVICE_ADDR = 0;

void i2c_master_init()
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

uint8_t i2c_scan_device()
{
    for (uint8_t addr = 0x03; addr <= 0x77; addr++)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "Found device at address: 0x%02X", addr);
            return addr;
        }
        else
        {
            ESP_LOGD(TAG, "No response at address 0x%02X", addr);
        }
    }
    ESP_LOGE(TAG, "No I2C device found!");
    return 0;
}

esp_err_t i2c_master_read_memory(uint8_t mem_addr, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DEVICE_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, mem_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DEVICE_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

void app_main()
{
    i2c_master_init();
    ESP_LOGI(TAG, "I2C initialized");

    DEVICE_ADDR = i2c_scan_device();
    if (DEVICE_ADDR == 0)
    {
        ESP_LOGE(TAG, "Device not found, exiting.");
        return;
    }

    uint8_t data[16];
    if (i2c_master_read_memory(MEMORY_ADDR, data, sizeof(data)) == ESP_OK)
    {
        printf("Data from device at address 0x%02X: ", DEVICE_ADDR);
        for (int i = 0; i < sizeof(data); i++)
        {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to read from device");
    }
}
