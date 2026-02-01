#include "imu.h"
#include "config.h"
#include "display.h"
#include "SensorQMI8658.hpp"
#include <Wire.h>
#include <math.h>

static SensorQMI8658 qmi;

// Gravity estimate (LPF-tracked)
static float gX = 0, gY = 0, gZ = 0;
static bool isCalibrated = false;

// Velocity state (vertical-axis velocity in m/s)
static float currentVelocity = 0;

// Latest sensor readings (cached for external queries)
static float lastAx = 0, lastAy = 0, lastAz = 0;
static float lastGx = 0, lastGy = 0, lastGz = 0;

// Stationary detection threshold (how close to 1g)
static const float STATIONARY_THRESHOLD = 0.08f;

bool imuInit() {
  bool ret = qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, I2C_SDA, I2C_SCL);
  if (!ret) { Serial.println("Failed to find QMI8658!"); return false; }

  if (!qmi.selfTestAccel()) { Serial.println("Accelerometer self-test failed!"); return false; }
  if (!qmi.selfTestGyro())  { Serial.println("Gyroscope self-test failed!"); return false; }

  qmi.configAccelerometer(
    SensorQMI8658::ACC_RANGE_4G,
    SensorQMI8658::ACC_ODR_500Hz,
    SensorQMI8658::LPF_MODE_0);

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
  float sumAx = 0, sumAy = 0, sumAz = 0;

  displayShowCalibrating(true);

  for (int i = 0; i < samples; i++) {
    if (qmi.getDataReady()) {
      float ax, ay, az;
      if (qmi.getAccelerometer(ax, ay, az)) {
        sumAx += ax;
        sumAy += ay;
        sumAz += az;
      }
    }
    delay(10);
  }

  // Initialize gravity estimate (in g units)
  gX = sumAx / samples;
  gY = sumAy / samples;
  gZ = sumAz / samples;

  isCalibrated = true;
  currentVelocity = 0;

  displayShowCalibrating(false);
  displayDrawSwipeIndicator();

  Serial.printf("Calibrated - Gravity init: X=%.3f Y=%.3f Z=%.3f (g)\n", gX, gY, gZ);
}

bool imuIsCalibrated() { return isCalibrated; }

void imuZeroVelocity() {
  currentVelocity = 0;
}

static inline float invSqrt(float x) {
  return 1.0f / sqrtf(x);
}

bool imuProcess(float &velocity, float dt) {
  if (!isCalibrated || !qmi.getDataReady()) return false;

  float ax, ay, az;
  if (!qmi.getAccelerometer(ax, ay, az)) return false;

  // Cache accel readings
  lastAx = ax;
  lastAy = ay;
  lastAz = az;

  // Also read gyro (cache for external use)
  qmi.getGyroscope(lastGx, lastGy, lastGz);

  // Sanity check on dt
  if (dt <= 0 || dt > 0.1f) return false;

  // 1) Only update gravity estimate when stationary
  //    Stationary = raw accel magnitude ≈ 1g (no linear acceleration)
  float accelMag = sqrtf(ax*ax + ay*ay + az*az);
  bool likelyStationary = fabsf(accelMag - 1.0f) < STATIONARY_THRESHOLD;

  if (likelyStationary) {
    const float a = GRAVITY_LPF_ALPHA;
    gX = (1.0f - a) * gX + a * ax;
    gY = (1.0f - a) * gY + a * ay;
    gZ = (1.0f - a) * gZ + a * az;
  }

  // 2) Linear acceleration (g units)
  float linX = ax - gX;
  float linY = ay - gY;
  float linZ = az - gZ;

  // 3) Define "vertical axis" as gravity direction (unit vector)
  float gNorm2 = gX*gX + gY*gY + gZ*gZ;
  if (gNorm2 < 0.5f) return false; // safety

  float invG = invSqrt(gNorm2);
  float gx = gX * invG;
  float gy = gY * invG;
  float gz = gZ * invG;

  // 4) Project linear acceleration onto gravity axis (signed)
  float linAccG = linX*gx + linY*gy + linZ*gz;

  // Convert to m/s^2
  float linAcc = linAccG * ACCEL_SCALE;

  // 5) Integrate to vertical velocity (m/s) + mild decay
  float decay = expf(-dt / 0.5f);  // 0.5s time constant
  currentVelocity = currentVelocity * decay + linAcc * dt;

  // Clamp tiny velocities to zero
  if (fabsf(currentVelocity) < VELOCITY_NOISE_CLAMP) currentVelocity = 0;

  velocity = currentVelocity;
  return true;
}

bool imuGetGyroMagnitude(float &magnitude) {
  // Return cached gyro magnitude (degrees/sec)
  magnitude = sqrtf(lastGx*lastGx + lastGy*lastGy + lastGz*lastGz);
  return true;
}

bool imuGetAccel(float &ax, float &ay, float &az) {
  ax = lastAx;
  ay = lastAy;
  az = lastAz;
  return isCalibrated;
}

bool imuIsStationary() {
  float accelMag = sqrtf(lastAx*lastAx + lastAy*lastAy + lastAz*lastAz);
  return fabsf(accelMag - 1.0f) < STATIONARY_THRESHOLD;
}

void imuReset() {
  isCalibrated = false;
  currentVelocity = 0;
  gX = gY = gZ = 0;
  lastAx = lastAy = lastAz = 0;
  lastGx = lastGy = lastGz = 0;
}

void imuSleep() {
  qmi.disableAccelerometer();
  qmi.disableGyroscope();
  Serial.println("IMU sleeping");
}

void imuWake() {
  qmi.enableAccelerometer();
  qmi.enableGyroscope();
  isCalibrated = false;
  currentVelocity = 0;
  Serial.println("IMU awake");
}
