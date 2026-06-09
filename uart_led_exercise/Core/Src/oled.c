#include "main.h"
#include "oled.h"
#include "console.h"


void oled_write_packet(uint8_t control_type, uint8_t payload) {
    // Wait until the I2C bus is free
    while (I2C1->ISR & I2C_ISR_BUSY);
  
    // Configure for 2 bytes, Auto-End, and generate START
    I2C1->CR2 = 0;
    I2C1->CR2 |= (0x3C << 1) | (2 << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | I2C_CR2_START;
  
    // Send Byte 1: The Control Byte
    while (!(I2C1->ISR & I2C_ISR_TXIS)) {
      if (I2C1->ISR & I2C_ISR_NACKF) {
        GPIOA->ODR |= (1 << 5);      // Turn ON the Green LED to flag an error!
        I2C1->ICR |= I2C_ICR_NACKCF; 
        return;                      
      }
    }
    I2C1->TXDR = control_type;
  
    // Send Byte 2: The payload
    while (!(I2C1->ISR & I2C_ISR_TXIS)) {
      if (I2C1->ISR & I2C_ISR_NACKF) {
        GPIOA->ODR |= (1 << 5);      // Turn ON the Green LED to flag an error!
        I2C1->ICR |= I2C_ICR_NACKCF; 
        return;                      
      }
    }
    I2C1->TXDR = payload;
    
    // Wait for AUTOEND to finish and clear the flag
    while (!(I2C1->ISR & I2C_ISR_STOPF));
    I2C1->ICR |= I2C_ICR_STOPCF;
  }
  
  void oled_send_cmd(uint8_t cmd) { 
      oled_write_packet(0x00, cmd); 
  }
  
  void oled_send_data(uint8_t data) { 
    oled_write_packet(0x40, data); // 0x40 tells the OLED "this is a pixel, not a command!"
  }
  
  void oled_clear(void) {
    for (uint8_t page = 0; page < 8; page++) {
        oled_send_cmd(0xB0 + page); // Set Page (0-7)
        oled_send_cmd(0x00);        // Set Lower Column
        oled_send_cmd(0x10);        // Set Higher Column
        
        for (uint8_t col = 0; col < 128; col++) {
            oled_send_data(0x00);   // Write 0s to clear all 128 columns
        }
    }
  }