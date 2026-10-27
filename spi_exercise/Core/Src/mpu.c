#include "main.h"
#include "mpu.h"
#include "spi_peripheral.h"
#include <stdio.h>
#include <stdlib.h>

#define MPU_COMBINE_BYTES(msb, lsb) \
    ((int16_t)(((uint16_t)(msb) << 8) | (uint16_t)(lsb)))

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
    
    // The MPU needs time to switch clock sources and allow the PLL to stabilize before
    // subsequent configuration or sensor readings. Approx timing: ~0.6 ms
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
    accel[0] = MPU_COMBINE_BYTES(raw_buffer[0], raw_buffer[1]);
    accel[1] = MPU_COMBINE_BYTES(raw_buffer[2], raw_buffer[3]);
    accel[2] = MPU_COMBINE_BYTES(raw_buffer[4], raw_buffer[5]);

    gyro[0] = MPU_COMBINE_BYTES(raw_buffer[8], raw_buffer[9]);
    gyro[1] = MPU_COMBINE_BYTES(raw_buffer[10], raw_buffer[11]);
    gyro[2] = MPU_COMBINE_BYTES(raw_buffer[12], raw_buffer[13]);
}
