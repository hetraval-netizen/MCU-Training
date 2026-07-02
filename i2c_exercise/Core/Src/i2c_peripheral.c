#include "i2c_peripheral.h"

void i2c_hardware_init(void) {
    /*  Enable clock for GPIOB */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;

    /* Re-map Pin 6 and Pin 7 into Alternate Function 4 (I2C1) */
    GPIOB->MODER &= ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE7);
    GPIOB->MODER |= (GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1);
    GPIOB->AFR[0] &= ~GPIO_AFRL_AFSEL6;
    GPIOB->AFR[0] |= (4UL << GPIO_AFRL_AFSEL6_Pos);
    GPIOB->AFR[0] &= ~GPIO_AFRL_AFSEL7;
    GPIOB->AFR[0] |= (4UL << GPIO_AFRL_AFSEL7_Pos);
    GPIOB->OTYPER |= (GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7);
    
    /* Initialize I2C1 Bus Engine */
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;
    I2C1->CR1 &= ~I2C_CR1_PE;
    I2C1->TIMINGR = 0x90310309; /* Fast-Mode timing for 80MHz operational clock */
    I2C1->CR1 |= I2C_CR1_PE;
}

#define I2C_TIMEOUT_COUNT  100000

int8_t i2c1_master_transmit(uint8_t dev_addr_7bit, uint8_t control_byte, uint8_t *payload, uint16_t size) {
    uint32_t timeout = I2C_TIMEOUT_COUNT;

    // 1. Wait until the I2C bus is free
    while ((I2C1->ISR & I2C_ISR_BUSY) && (--timeout > 0));
    if (timeout == 0) return -1; // Bus stuck/hardware issue

    // 2. Clear previous execution flags (Stop & NACK flag clears)
    I2C1->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF; 
    I2C1->CR2 = 0;

    // 3. Configure CR2. 
    // STM32L4 expects the 7-bit address shifted left by 1 inside SADD[7:1] bits.
    // Total bytes to send = payload size + 1 (for the control byte)
    uint32_t nbytes = (uint32_t)(size + 1);
    I2C1->CR2 |= ((uint32_t)dev_addr_7bit << 1) | (nbytes << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | I2C_CR2_START;
    
    // 4. Send the SSD1306 Control Byte (0x00 for Command / 0x40 for Data)
    timeout = I2C_TIMEOUT_COUNT;
    while (!(I2C1->ISR & I2C_ISR_TXIS)) {
        if (I2C1->ISR & I2C_ISR_NACKF) {
            I2C1->ICR = I2C_ICR_NACKCF;
            return -2; // Device didn't acknowledge address
        }
        if (--timeout == 0) return -3;
    }
    I2C1->TXDR = control_byte;
    
    // 5. Stream out the U8g2 memory buffer data payload
    if (size > 0 && payload != NULL) {
        for(uint16_t i = 0; i < size; i++) {
            timeout = I2C_TIMEOUT_COUNT;
            while (!(I2C1->ISR & I2C_ISR_TXIS)) {
                if (I2C1->ISR & I2C_ISR_NACKF) {
                    I2C1->ICR = I2C_ICR_NACKCF;
                    return -4; // Device dropped mid-transfer
                }
                if (--timeout == 0) return -5;
            }
            I2C1->TXDR = payload[i];
        }
    }
    
    // 6. Wait for AUTOEND hardware sequence to issue and log the STOP flag
    timeout = I2C_TIMEOUT_COUNT;
    while (!(I2C1->ISR & I2C_ISR_STOPF) && (--timeout > 0));
    
    // 7. Clear the stop flag so the interface is entirely clean for your next call
    I2C1->ICR |= I2C_ICR_STOPCF; 
    return 0; // Success
}


int8_t i2c1_master_transmit_1(uint8_t dev_addr_7bit, uint8_t *payload, uint16_t size) {
    uint32_t timeout = I2C_TIMEOUT_COUNT;

    // 1. Wait until the I2C bus is free
    while ((I2C1->ISR & I2C_ISR_BUSY) && (--timeout > 0));
    if (timeout == 0) return -1; // Bus stuck/hardware issue

    // 2. Clear previous execution flags (Stop & NACK flag clears)
    I2C1->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF; 
    I2C1->CR2 = 0;

    // 3. Configure CR2. 
    // STM32L4 expects the 7-bit address shifted left by 1 inside SADD[7:1] bits.
    // Total bytes to send = payload size + 1 (for the control byte)
    uint32_t nbytes = (uint32_t)(size + 1);
    I2C1->CR2 |= ((uint32_t)dev_addr_7bit << 1) | (nbytes << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | I2C_CR2_START;
    
    // 4. Send the SSD1306 Control Byte (0x00 for Command / 0x40 for Data)
    timeout = I2C_TIMEOUT_COUNT;
    while (!(I2C1->ISR & I2C_ISR_TXIS)) {
        if (I2C1->ISR & I2C_ISR_NACKF) {
            I2C1->ICR = I2C_ICR_NACKCF;
            return -2; // Device didn't acknowledge address
        }
        if (--timeout == 0) return -3;
    }
    
    // 5. Stream out the U8g2 memory buffer data payload
    if (size > 0 && payload != NULL) {
        for(uint16_t i = 0; i < size; i++) {
            timeout = I2C_TIMEOUT_COUNT;
            I2C1->TXDR = payload[i];
            while (!(I2C1->ISR & I2C_ISR_TXIS)) {
                if (I2C1->ISR & I2C_ISR_NACKF) {
                    I2C1->ICR = I2C_ICR_NACKCF;
                    return -4; // Device dropped mid-transfer
                }
                if (--timeout == 0) return -5;
            }
        }
    }
    
    // 6. Wait for AUTOEND hardware sequence to issue and log the STOP flag
    timeout = I2C_TIMEOUT_COUNT;
    while (!(I2C1->ISR & I2C_ISR_STOPF) && (--timeout > 0));
    
    // 7. Clear the stop flag so the interface is entirely clean for your next call
    I2C1->ICR |= I2C_ICR_STOPCF; 
    return 0; // Success
}
