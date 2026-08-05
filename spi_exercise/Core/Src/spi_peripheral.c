#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "spi_peripheral.h"

void spi_init(void) {
    // 1. Enable clocks for GPIO A, GPIO B, and the SPI1 peripheral engine
    RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN);
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
  
    // 2. Set GPIOA-15 as standard Output Mode (01) for Software Chip Select
    GPIOA->MODER &= ~GPIO_MODER_MODE15;
    GPIOA->MODER |= GPIO_MODER_MODE15_0;
  
    // 3. Set PA15 High immediately (Idle state for MPU-9250 SPI)
    GPIOA->ODR |= GPIO_ODR_OD15; 
  
    // 4. Safely clear Port B pins 3, 4, and 5 completely
    GPIOB->MODER &= ~(GPIO_MODER_MODE3 | GPIO_MODER_MODE4 | GPIO_MODER_MODE5);
    // Configure PB3, PB4, PB5 into Alternate Function Mode (10)
    GPIOB->MODER |= (GPIO_MODER_MODE3_1 | GPIO_MODER_MODE4_1 | GPIO_MODER_MODE5_1);
  
    // 5. Map PB3, PB4, PB5 to AF5 (SPI1) - Explicitly mask and overwrite
    GPIOB->AFR[0] &= ~(GPIO_AFRL_AFSEL3 | GPIO_AFRL_AFSEL4 | GPIO_AFRL_AFSEL5);
    GPIOB->AFR[0] |= (5UL << GPIO_AFRL_AFSEL3_Pos) |
                     (5UL << GPIO_AFRL_AFSEL4_Pos) |
                     (5UL << GPIO_AFRL_AFSEL5_Pos);
  
    // 6. Configure the SPI1 Hardware Control Register 1 (CR1)
    SPI1->CR1 = 0; // Clear register to wipe out default uninitialized states
    SPI1->CR1 |= SPI_CR1_MSTR;                  // Set STM32 as SPI Bus Master
    SPI1->CR1 |= (SPI_CR1_BR_0 | SPI_CR1_BR_1); // Baud Rate Prescaler: /16 (5MHz clock)
    SPI1->CR1 |= SPI_CR1_CPOL;                  // CPOL = 1 (Clock High when idle)
    SPI1->CR1 |= SPI_CR1_CPHA;                  // CPHA = 1 (Capture data on 2nd clock edge)
    SPI1->CR1 |= SPI_CR1_SSM;                   // Enable Software Slave Management
    SPI1->CR1 |= SPI_CR1_SSI;                   // Force internal master token High
  
    // 7. Configure SPI1 Control Register 2 (CR2)
    SPI1->CR2 = 0;
    SPI1->CR2 |= (7UL << SPI_CR2_DS_Pos);       // Set 8-bit data size frame
    SPI1->CR2 |= SPI_CR2_FRXTH;                 // Force RX FIFO Threshold to 8-bit mode
  
    // 8. Fire up the SPI1 hardware peripheral
    SPI1->CR1 |= SPI_CR1_SPE;
}

uint8_t spi_transfer (uint8_t data) {
    // 1. Clear out any unread residual garbage bytes sitting in the RX FIFO buffer
    while (SPI1->SR & SPI_SR_RXNE) {
        volatile uint8_t flush = *(__IO uint8_t *)&SPI1->DR;
        (void)flush; // Prevents compiler unused-variable warnings
    }

    // 2. Wait until the Transmit buffer is fully clear
    while (!(SPI1->SR & SPI_SR_TXE));

    // 3. Drop the data byte using an explicit 8-bit pointer write
    *(__IO uint8_t *)&SPI1->DR = data;

    // 4. Wait until the MPU-9250 shifts back a live response byte
    while (!(SPI1->SR & SPI_SR_RXNE));
    
    // 5. Collect and return the fresh byte
    return *(__IO uint8_t *)&SPI1->DR; 
}