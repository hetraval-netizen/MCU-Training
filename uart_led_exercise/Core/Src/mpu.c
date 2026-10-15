#include "main.h"
#include "mpu.h"
#include <stdio.h>
#include <stdlib.h>

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

void mpu_write_reg (uint8_t reg, uint8_t data) {
    GPIOA->ODR &= ~GPIO_ODR_OD15;

    spi_transfer(reg & 0x7F);
    spi_transfer(data);

    GPIOA->ODR |= GPIO_ODR_OD15;
}

uint8_t mpu_read_reg (uint8_t reg) {
    uint8_t recieved_data = 0;

    GPIOA->ODR &= ~GPIO_ODR_OD15;

    spi_transfer(reg | 0x80);
    recieved_data = spi_transfer(0x00);

    GPIOA->ODR |= GPIO_ODR_OD15;

    return recieved_data;
}

void mpu_configure(void) {
    // 1. Wake up the device and set clock source to Gyroscope Z-axis PLL (highly recommended for stability)
    mpu_write_reg(MPU_REG_PWR_MGMT_1, 0x01); 
    
    // Small delay to let the internal clock source settle
    for (volatile uint32_t i = 0; i < 10000; i++);

    // 2. Configure Accelerometer Full Scale Range to ±2g (0x00)
    mpu_write_reg(MPU_REG_ACCEL_CONFIG, 0x00);
}

void mpu_read_raw_data(int16_t* accel, int16_t* gyro) {
    uint8_t raw_buffer[14];

    // Assert CS Low to open communication
    GPIOA->ODR &= ~GPIO_ODR_OD15;

    // Send the starting address register (0x3B) with the Read bit set (0x80)
    spi_transfer(MPU_REG_ACCEL_XOUT_H | 0x80);

    // Burst read all 14 data bytes consecutively
    for (int i = 0; i < 14; i++) {
        raw_buffer[i] = spi_transfer(0x00);
    }

    // De-assert CS High to close transaction
    GPIOA->ODR |= GPIO_ODR_OD15;

    // Reconstruct 16-bit signed integers from high and low bytes
    accel[0] = (int16_t)((raw_buffer[0] << 8)  | raw_buffer[1]);  // Accel X
    accel[1] = (int16_t)((raw_buffer[2] << 8)  | raw_buffer[3]);  // Accel Y
    accel[2] = (int16_t)((raw_buffer[4] << 8)  | raw_buffer[5]);  // Accel Z
    
    // Bytes 6 and 7 contain raw internal chip temperature data (skipping here)

    gyro[0]  = (int16_t)((raw_buffer[8] << 8)  | raw_buffer[9]);  // Gyro X
    gyro[1]  = (int16_t)((raw_buffer[10] << 8) | raw_buffer[11]); // Gyro Y
    gyro[2]  = (int16_t)((raw_buffer[12] << 8) | raw_buffer[13]); // Gyro Z
}