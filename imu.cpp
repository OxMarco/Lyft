#include "imu.h"
#include "config.h"
#include "display.h"
#include "SensorQMI8658.hpp"
#include <Wire.h>

static SensorQMI8658 qmi;

// Calibration data
static float gravityX = 0, gravityY = 0, gravityZ = 0;
static bool isCalibrated = false;

// Velocity state
static float currentVelocity = 0;

bool imuInit() {
  bool ret = qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, I2C_SDA, I2C_SCL);
  if (!ret) {
    Serial.println("Failed to find QMI8658!");
    return false;
  }

  if (!qmi.selfTestAccel()) {
    Serial.println("Accelerometer self-test failed!");
    return false;
  }

  if (!qmi.selfTestGyro()) {
    Serial.println("Gyroscope self-test failed!");
    return false;
  }

  // Configure accelerometer for gym tracking
  // 4G range is good for most barbell exercises
  qmi.configAccelerometer(
    SensorQMI8658::ACC_RANGE_4G,
    SensorQMI8658::ACC_ODR_500Hz,
    SensorQMI8658::LPF_MODE_0);

  // Configure gyroscope
  qmi.configGyroscope(
    SensorQMI8658::GYR_RANGE_256DPS,
    SensorQMI8658::GYR_ODR_448_4Hz,
    SensorQMI8658::LPF_MODE_3);

  qmi.enableAccelerometer();
  qmi.enableGyroscope();

  Serial.println("IMU initialized successfully");
  return true;
}

void imuCalibrate() {
  const int samples = 50;
  float sumX = 0, sumY = 0, sumZ = 0;

  displayShowCalibrating(true);

  for (int i = 0; i < samples; i++) {
    if (qmi.getDataReady()) {
      float ax, ay, az;
      if (qmi.getAccelerometer(ax, ay, az)) {
        sumX += ax;
        sumY += ay;
        sumZ += az;
      }
    }
    delay(10);
  }

  gravityX = sumX / samples;
  gravityY = sumY / samples;
  gravityZ = sumZ / samples;

  isCalibrated = true;
  currentVelocity = 0;

  displayShowCalibrating(false);
  displayDrawSwipeIndicator();

  Serial.printf("Calibrated - Gravity: X=%.2f Y=%.2f Z=%.2f\n",
                gravityX, gravityY, gravityZ);
}

bool imuIsCalibrated() {
  return isCalibrated;
}

bool imuProcess(float &velocity, float dt) {
  if (!isCalibrated || !qmi.getDataReady()) {
    return false;
  }

  float ax, ay, az;
  if (!qmi.getAccelerometer(ax, ay, az)) {
    return false;
  }

  // Sanity check on dt
  if (dt <= 0 || dt > 0.1) {
    return false;
  }

  // Remove gravity to get linear acceleration
  float linearAccX = ax - gravityX;
  float linearAccY = ay - gravityY;
  float linearAccZ = az - gravityZ;

  // Calculate linear acceleration on primary axis (Z when barbell is horizontal)
  float linearAcc = linearAccZ * ACCEL_SCALE;  // Convert to m/s²

  // Simple integration for velocity (with decay to prevent drift)
  currentVelocity = currentVelocity * 0.98f + linearAcc * dt;

  // Clamp small velocities to zero (noise reduction)
  if (fabs(currentVelocity) < 0.02f) {
    currentVelocity = 0;
  }

  velocity = currentVelocity;
  return true;
}

void imuReset() {
  isCalibrated = false;
  currentVelocity = 0;
  gravityX = gravityY = gravityZ = 0;
}

void imuSleep() {
  // Disable accelerometer and gyroscope for power saving
  qmi.disableAccelerometer();
  qmi.disableGyroscope();
  Serial.println("IMU sleeping");
}

void imuWake() {
  // Re-enable sensors
  qmi.enableAccelerometer();
  qmi.enableGyroscope();
  isCalibrated = false;  // Need recalibration after wake
  currentVelocity = 0;
  Serial.println("IMU awake");
}
