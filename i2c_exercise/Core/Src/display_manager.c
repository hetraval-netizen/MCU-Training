#include "display_manager.h"
#include "i2c_peripheral.h"

extern u8g2_t u8g2;

uint8_t u8g2_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    switch(msg) {
        case U8X8_MSG_DELAY_MILLI:
            HAL_Delay(arg_int); 
            break;
        default:
            return 0;
    }
    return 1;
}

uint8_t u8g2_byte_hw_i2c_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    static uint8_t buffer[32]; /* U8g2 never sends more than 32 bytes per transfer */
    static uint8_t buf_index;
    
    switch(msg) {
        case U8X8_MSG_BYTE_INIT:
            return 1; /* i2c_hardware_init() is already called in main.c */
            
        case U8X8_MSG_BYTE_START_TRANSFER:
            buf_index = 0;
            break;
            
        case U8X8_MSG_BYTE_SEND: {
            uint8_t *src = (uint8_t *)arg_ptr;
            for(uint8_t i = 0; i < arg_int; i++) {
                if (buf_index < 32) {
                    buffer[buf_index++] = src[i];
                }
            }
            break;
        }
        
        case U8X8_MSG_BYTE_END_TRANSFER: {
            /* Pass the exact accumulated buffer to your hardware line */
            uint8_t target_address = 0x78;
            i2c_master_transmit(target_address, buffer, buf_index);
            break;
        }
        default:
            return 0;
    }
    return 1;
}

void display_manager_init(void) {
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8g2_byte_hw_i2c_stm32, u8g2_gpio_and_delay_stm32);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    
    u8g2_ClearDisplay(&u8g2);

    /* Drawing commands restored! */
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
    u8g2_SetFontMode(&u8g2, 1);
    u8g2_DrawStr(&u8g2, 0, 15, "Hello world");
    u8g2_DrawCircle(&u8g2, 60, 30, 10, U8G2_DRAW_ALL);
    u8g2_SendBuffer(&u8g2);
}
