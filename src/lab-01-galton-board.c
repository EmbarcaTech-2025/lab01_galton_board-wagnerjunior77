#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// Biblioteca para uso do SSD1306, display OLED.
#include "include/ssd1306.h"

// Definições para uso de comunicação I2C com os pinos do display OLED.
#define I2C_PORT i2c0
#define I2C_SDA 14
#define I2C_SCL 15

int main()
{
    stdio_init_all();

    // Inicialização da porta I2C em 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
