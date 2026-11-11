#ifndef LSM6DSV80_H
#define LSM6DSV80_H

// Pin Definitions-------------
#define LSM_CS_1 9   // IMU 1 on default SPI  (MOSI: 11, MISO: 12, SCK: 13)
#define LSM_CS_2 15  // IMU 2 on SPI1         (MOSI: 26, MISO: 39, SCK: 27)
#define LSM_CS_3 17  // IMU 3 on SPI1

// Includes
#include <Arduino.h>
#include <SPI.h>
#include "spiPlatform.h"
#include "lsm6dsv80x_reg.h" // library files

int32_t platform_writeLSM(void *handle, uint8_t reg, const uint8_t *buf, uint16_t len);
int32_t platform_readLSM(void *handle, uint8_t reg, uint8_t *buf, uint16_t len);
void platform_delayLSM(uint32_t millisec);

void init_imu(stmdev_ctx_t *ctx, SpiDeviceHandle *handle, SPIClass *bus, uint8_t cs_pin, const char *label);
void read_imu(stmdev_ctx_t *ctx, const char *label);
void setupIMU();
void useIMU();

#endif // LSM6DSV80_H