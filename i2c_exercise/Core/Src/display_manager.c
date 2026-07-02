#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "main.h"
#include "display_manager.h"
#include "i2c_peripheral.h"

uint8_t u8x8_byte_stm32_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    static uint8_t buffer[512]; // Safe overhead array capacity for 128x64 display pipelines
    static uint16_t buf_index;  // Must be uint16_t to clear values > 255
    static uint8_t control_byte = 0x00;
    
    switch(msg) {
        case U8X8_MSG_BYTE_INIT:
            // Insert your low level clock enabling / GPIO pin configuration here if needed
            break;
            
        case U8X8_MSG_BYTE_SET_DC:
            // U8g2 breaks command sequences down: 0 is control instructions, 1 is graphic streams
            control_byte = (arg_int == 0) ? 0x00 : 0x40;
            break;
            
        case U8X8_MSG_BYTE_START_TRANSFER:
            buf_index = 0; 
            break;
            
        case U8X8_MSG_BYTE_SEND: {
            uint8_t *src = (uint8_t *)arg_ptr;
            for(uint8_t i = 0; i < arg_int; i++) {
                if (buf_index < sizeof(buffer)) {
                    buffer[buf_index++] = src[i];
                }
            }
            break;
        }
        
        case U8X8_MSG_BYTE_END_TRANSFER: {
            // Get standard 7-bit unshifted I2C address (Default: 0x3C)
            // uint8_t target_address = u8x8_GetI2CAddress(u8x8);
            uint8_t target_address = 0x3C; 
            
            // Execute safe custom raw transaction register transfer 
            if(i2c1_master_transmit(target_address, control_byte, buffer, buf_index) != 0) {
                // Optional diagnostic tracker line hook
                // printf("I2C Error Frame Detected\n");
            }
            break;
        }
        default:
            return 0;
    }
    return 1;
}

uint8_t u8x8_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    switch(msg) {
        case U8X8_MSG_DELAY_100NANO: 
            __NOP(); // Direct assembly delay instruction based on core speed clock cycles
            break;
        case U8X8_MSG_DELAY_10MICRO: 
            // Rough 10 microsecond loop calibration calculation for an 80MHz STM32L4 Clock
            for (volatile uint32_t i = 0; i < 80; i++){
                __NOP();
            }
            break;
        case U8X8_MSG_DELAY_MILLI:
            HAL_Delay(arg_int); // Fallback execution using regular SysTick counter loop
            break;
        default:
            return 1; 
    }
    return 1;
}

extern u8g2_t u8g2; // Allocate structural context instance

void display_manager_init(void) {
    // 1. Link setup parameters to library profile mapping layers
    printf ("Start the display manager\r\n");
    // u8x8_SetI2CAddress(&u8g2, 0x3C);
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_stm32_hw_i2c, u8x8_gpio_and_delay_stm32);
    
    printf ("Going for init display\r\n");
    // 2. Transmit display awake and register panel array commands
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0); // Disable power-saving mode (Turn display ON)
    printf ("Display turned on!\r\n");
    // 3. Clear memory frame block states and paint system diagnostic font elements
    // u8g2_ClearBuffer(&u8g2);
    printf ("Setting font\r\n");
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf); // Clean, lightweight core font choice
    printf ("Drawing string\n\r");
    u8g2_DrawStr(&u8g2, 0, 20, "STM32L476RG Active!");
    // u8g2_SendBuffer(&u8g2);
    u8g2_UpdateDisplay(&u8g2);
}
