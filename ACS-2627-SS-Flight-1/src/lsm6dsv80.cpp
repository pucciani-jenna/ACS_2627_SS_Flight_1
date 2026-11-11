// This is code for the LSM6DSV80X IMU on the Teensy 4.1
#include "lsm6dsv80.h"

static SPISettings spiSettings(4000000, MSBFIRST, SPI_MODE3);

int32_t platform_writeLSM(void *handle, uint8_t reg, const uint8_t *buf, uint16_t len) {
  SpiDeviceHandle *dev = static_cast<SpiDeviceHandle *>(handle);

  dev->bus->beginTransaction(spiSettings);
  digitalWrite(dev->cs_pin, LOW);

  dev->bus->transfer(reg & 0x7F);
  for (uint16_t i = 0; i < len; i++) {
    dev->bus->transfe
    r(buf[i]);
  }

  digitalWrite(dev->cs_pin, HIGH);
  dev->bus->endTransaction();
  return 0;
}


int32_t platform_readLSM(void *handle, uint8_t reg, uint8_t *buf, uint16_t len) {
  SpiDeviceHandle *dev = (SpiDeviceHandle *)handle;

  dev->bus->beginTransaction(spiSettings);
  digitalWrite(dev->cs_pin, LOW);

  dev->bus->transfer(reg | 0x80);
  for (uint16_t i = 0; i < len; i++) {
    buf[i] = dev->bus->transfer(0x00);
  }

  digitalWrite(dev->cs_pin, HIGH);
  dev->bus->endTransaction();
  return 0;
}

void platform_delayLSM(uint32_t millisec) {
  delay(millisec);
}

// Sensor Contexts------------------
static stmdev_ctx_t dev_ctx1; static SpiDeviceHandle sensor1_handle;
static stmdev_ctx_t dev_ctx2; static SpiDeviceHandle sensor2_handle;
static stmdev_ctx_t dev_ctx3; static SpiDeviceHandle sensor3_handle;

void init_imu(stmdev_ctx_t *ctx, SpiDeviceHandle *handle, SPIClass *bus, uint8_t cs_pin, const char *label) {
  pinMode(cs_pin, OUTPUT);
  
  digitalWrite(cs_pin, HIGH);

  handle->bus = bus;
  handle->cs_pin = cs_pin;

  ctx->write_reg = platform_writeLSM;
  ctx->read_reg  = platform_readLSM;
  ctx->mdelay    = platform_delayLSM;
  ctx->handle    = handle;

  uint8_t whoamI = 0;
  lsm6dsv80x_device_id_get(ctx, &whoamI);
  Serial.printf("%s Device ID: 0x%02X\n", label, whoamI);

  if (whoamI != LSM6DSV80X_ID) {
    Serial.printf("%s mismatch! Check wiring/bus.\n", label);
    return;
  }

  lsm6dsv80x_sw_reset(ctx);
  delay(15);

  lsm6dsv80x_block_data_update_set(ctx, PROPERTY_ENABLE);
  lsm6dsv80x_xl_setup(ctx, LSM6DSV80X_ODR_AT_120Hz, LSM6DSV80X_XL_HIGH_PERFORMANCE_MD);
  lsm6dsv80x_xl_full_scale_set(ctx, LSM6DSV80X_2g);
  lsm6dsv80x_gy_setup(ctx, LSM6DSV80X_ODR_AT_120Hz, LSM6DSV80X_GY_HIGH_PERFORMANCE_MD);
  lsm6dsv80x_gy_full_scale_set(ctx, LSM6DSV80X_250dps);
}

void setupIMU() {
  // Define CS Pins and set them high (inactive)
  pinMode(LSM_CS_1, OUTPUT); digitalWrite(LSM_CS_1, HIGH);
  pinMode(LSM_CS_2, OUTPUT); digitalWrite(LSM_CS_2, HIGH);
  pinMode(LSM_CS_3, OUTPUT); digitalWrite(LSM_CS_3, HIGH);
  

  // Initialize sensors
  init_imu(&dev_ctx1, &sensor1_handle, &SPI,  LSM_CS_1, "IMU 1");
  init_imu(&dev_ctx2, &sensor2_handle, &SPI1, LSM_CS_2, "IMU 2");
  init_imu(&dev_ctx3, &sensor3_handle, &SPI1, LSM_CS_3, "IMU 3");

  Serial.println("LSM6DSV80X setup finished.");
}

// I might want to fix the order of this later!
void read_imu(stmdev_ctx_t *ctx, const char *label) {
  int16_t accel[3] = {0};
  int16_t gyro[3] = {0};
  int16_t temp_raw = 0;

  // Read registers
  lsm6dsv80x_acceleration_raw_get(ctx, accel);
  lsm6dsv80x_angular_rate_raw_get(ctx, gyro);
  lsm6dsv80x_temperature_raw_get(ctx, &temp_raw);

  float ax = (lsm6dsv80x_from_fs2_to_mg(accel[0]) / 1000.0f) * 9.80665f;
  float ay = (lsm6dsv80x_from_fs2_to_mg(accel[1]) / 1000.0f) * 9.80665f;
  float az = (lsm6dsv80x_from_fs2_to_mg(accel[2]) / 1000.0f) * 9.80665f;

  float gx = (lsm6dsv80x_from_fs250_to_mdps(gyro[0]) / 1000.0f) * 0.0174533f;
  float gy = (lsm6dsv80x_from_fs250_to_mdps(gyro[1]) / 1000.0f) * 0.0174533f;
  float gz = (lsm6dsv80x_from_fs250_to_mdps(gyro[2]) / 1000.0f) * 0.0174533f;

  float temp_c = lsm6dsv80x_from_lsb_to_celsius(temp_raw);

  Serial.printf("[%s] Temp: %5.1f C | Accel m/s^2: [%6.2f, %6.2f, %6.2f] | Gyro rad/s: [%6.3f, %6.3f, %6.3f]\n",
                label, temp_c, ax, ay, az, gx, gy, gz);
}

void useIMU() {
  read_imu(&dev_ctx1, "IMU1");
  read_imu(&dev_ctx2, "IMU2");
  read_imu(&dev_ctx3, "IMU3");
  Serial.println();

  delay(200); // Another big massive delay
}