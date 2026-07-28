#include "i2c_peripheral.h"

void i2c_hardware_init(void) {
    /* 1. Enable clock for GPIOB and I2C1 Bus Engine */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;

    /* 2. Configure Pin 6 and Pin 7 as Alternate Function 4 Open-Drain */
    GPIOB->MODER &= ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE7);
    GPIOB->MODER |= (GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1);

    GPIOB->AFR[0] &= ~(GPIO_AFRL_AFSEL6 | GPIO_AFRL_AFSEL7);
    GPIOB->AFR[0] |= (4UL << GPIO_AFRL_AFSEL6_Pos) | (4UL << GPIO_AFRL_AFSEL7_Pos);
    
    GPIOB->OTYPER |= (GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7);
    GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED6 | GPIO_OSPEEDR_OSPEED7);

    /* 3. Reset I2C1 and configure noise filtering options */
    I2C1->CR1 &= ~I2C_CR1_PE;
    I2C1->CR1 &= ~I2C_CR1_ANFOFF;
    I2C1->CR1 &= ~I2C_CR1_DNF;

    /* 4. Set standard I2C timing configuration matching the working HAL project */
    I2C1->TIMINGR = 0x00F12981;
    I2C1->CR1 |= I2C_CR1_PE;
}

/* Removed control_byte from arguments */
int8_t i2c_master_transmit(uint8_t dev_addr, uint8_t *payload, uint16_t size) {
    while (I2C1->ISR & I2C_ISR_BUSY);
    
    I2C1->CR2 = 0;
    
    /* Use exact size; dev_addr is passed down pre-shifted at 0x78 */
    I2C1->CR2 |= ((uint32_t)dev_addr & 0xFE) | ((uint32_t)size << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | I2C_CR2_START;
    
    /* Stream the payload data (U8g2 already included the control byte in here) */
    for(uint16_t i = 0; i < size; i++) {
        while (!(I2C1->ISR & I2C_ISR_TXIS)) {
            if (I2C1->ISR & I2C_ISR_NACKF) return -1;
        }
        I2C1->TXDR = payload[i];
    }
    
    while (!(I2C1->ISR & I2C_ISR_STOPF));
    I2C1->ICR |= I2C_ICR_STOPCF;
    
    return 0;
}
