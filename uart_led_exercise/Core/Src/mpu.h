#ifndef SRC_MPU_H_
#define SRC_MPU_H_

#include <stdint.h>

// --- Register Map Definitions ---
#define MPU_REG_XG_OFFS_TC_H 0x04
#define MPU_REG_ACCEL_CONFIG 0x1C
#define MPU_REG_INT_ENABLE   0x38
#define MPU_REG_ACCEL_XOUT_H 0x3B // Start of 14-byte sensor data block
#define MPU_REG_PWR_MGMT_1   0x6B
#define MPU_REG_WHO_AM_I     0x75

// --- Public Drivers ---
void mpu_configure(void);
void mpu_read_raw_data(int16_t* accel, int16_t* gyro);
void mpu_write_reg(uint8_t reg, uint8_t data);
uint8_t mpu_read_reg(uint8_t reg);

#endif // SRC_MPU_H_