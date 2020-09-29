/* I2S Example

    This example code will output 100Hz sine wave and triangle wave to 2-channel of I2S driver
    Every 5 seconds, it will change bits_per_sample [16, 24, 32] for i2s data

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "esp_system.h"
#include <math.h>

//alternative setup 
//64Fs BCK, 256Fs MCK
//bit of a hack, but 32 bit data is supported, we need to make sure we only
//use the top 24 bits
#if 0
#define BITS            (32)
#define MCK             (256*SAMPLE_RATE)
#else
//normal setup
//48Fs BCK, 384Fs MCK
#define BITS            (24)
#define MCK             (384*SAMPLE_RATE)
#endif

#define SAMPLE_RATE     (48000)
#define I2S_NUM         (1)
#define I2S_BCK_IO      (GPIO_NUM_13)
#define I2S_WS_IO       (GPIO_NUM_15)
#define I2S_DO_IO       (GPIO_NUM_21)
#define I2S_DI_IO       (-1)

static void setup(int bits)
{
    //XXX not sure how many bytes there are exactly, but this fills up the
    //whole buffer for a continous output
    const int buffer_size_bytes = 6*60*4*4;
    int *samples_data = malloc(buffer_size_bytes);
    size_t i2s_bytes_write = 0;

    //fill the whole buffer with a pattern of all 1s on the left channel, and
    //all 0s on the right channel
    for(int i = 0; i < buffer_size_bytes/4/2; i++) {
        samples_data[i*2] = ((int) 0xFFFFFFFF);
        samples_data[i*2 + 1] = ((int) 0x00000000);
    }

    i2s_set_clk(I2S_NUM, SAMPLE_RATE, bits, 2);
    i2s_write(I2S_NUM, samples_data, buffer_size_bytes, &i2s_bytes_write, 100);
    printf("wrote %d bytes out of %d\n", i2s_bytes_write, buffer_size_bytes);

    free(samples_data);
}
void app_main(void)
{
    //for 36Khz sample rates, we create 100Hz sine wave, every cycle need 36000/100 = 360 samples (4-bytes or 8-bytes each sample)
    //depend on bits_per_sample
    //using 6 buffers, we need 60-samples per buffer
    //if 2-channels, 16-bit each channel, total buffer is 360*4 = 1440 bytes
    //if 2-channels, 24/32-bit each channel, total buffer is 360*8 = 2880 bytes
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,                                  // Only TX
        .sample_rate = SAMPLE_RATE,
        .fixed_mclk = MCK,
        .bits_per_sample = BITS,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           //2-channels
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = 6,
        .dma_buf_len = 60,
        .use_apll = true,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1                                //Interrupt level 1
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_IO,
        .ws_io_num = I2S_WS_IO,
        .data_out_num = I2S_DO_IO,
        .data_in_num = I2S_DI_IO                                               //Not used
    };
    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
#if I2S_NUM == 0
    WRITE_PERI_REG(PIN_CTRL, READ_PERI_REG(PIN_CTRL) & 0xFFFFFFF0);
#else
    WRITE_PERI_REG(PIN_CTRL, READ_PERI_REG(PIN_CTRL) | 0x0000000F);
#endif
    
    setup(BITS);
    while (1) {
        vTaskDelay(5000/portTICK_RATE_MS);
    }

}
