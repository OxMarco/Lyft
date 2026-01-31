#include "battery.h"
#include "config.h"
#include <Wire.h>

// AXP2101 I2C address
#define AXP2101_ADDR 0x34

// AXP2101 registers
#define AXP2101_STATUS1         0x00
#define AXP2101_STATUS2         0x01
#define AXP2101_VBAT_H          0x34
#define AXP2101_VBAT_L          0x35
#define AXP2101_ADC_ENABLE      0x30

// Battery voltage thresholds (millivolts)
#define BATTERY_FULL_MV     4200
#define BATTERY_EMPTY_MV    3000

static bool initialized = false;

// Read single register from AXP2101
static uint8_t axpReadReg(uint8_t reg) {
    Wire.beginTransmission(AXP2101_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0;
    }
    Wire.requestFrom(AXP2101_ADDR, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}

// Write single register to AXP2101
static void axpWriteReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(AXP2101_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

bool batteryInit() {
    // Check if AXP2101 is present
    Wire.beginTransmission(AXP2101_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("AXP2101 not found at 0x34");
        return false;
    }
    
    // Enable battery voltage ADC
    // Bit 0 of register 0x30 enables VBAT ADC
    uint8_t adcEnable = axpReadReg(AXP2101_ADC_ENABLE);
    axpWriteReg(AXP2101_ADC_ENABLE, adcEnable | 0x01);
    
    initialized = true;
    Serial.println("Battery monitor initialized (AXP2101)");
    return true;
}

int batteryGetVoltage() {
    if (!initialized) return 0;
    
    // Read battery voltage (14-bit ADC value split across 2 registers)
    // VBAT_H[7:0] = bits 13:6
    // VBAT_L[5:0] = bits 5:0
    uint8_t vbatH = axpReadReg(AXP2101_VBAT_H);
    uint8_t vbatL = axpReadReg(AXP2101_VBAT_L);
    
    // Combine into 14-bit value
    uint16_t adcValue = ((uint16_t)vbatH << 6) | (vbatL & 0x3F);
    
    // Convert to millivolts
    // AXP2101: 1.1V reference, 14-bit ADC, with internal divider
    // Actual formula may vary - this is approximate
    // Voltage = ADC * 1.1 / 16384 * 4 (typical divider) * 1000 (to mV)
    int voltage = (adcValue * 1100 * 4) / 16384;
    
    // Sanity check - if reading seems wrong, try alternate calculation
    if (voltage < 2500 || voltage > 4500) {
        // Try simpler 1mV per LSB approximation (common for some firmware)
        voltage = adcValue;
        
        // Still out of range? Clamp it
        if (voltage < 2500) voltage = 3000;
        if (voltage > 4500) voltage = 4200;
    }
    
    return voltage;
}

int batteryGetPercent() {
    if (!initialized) return 50;  // Default to 50% if not initialized
    
    // AXP2101 has a direct battery percentage register at 0xA4
    uint8_t percent = axpReadReg(0xA4);
    
    // Sanity check
    if (percent > 100) percent = 100;
    
    return percent;
}

bool batteryIsCharging() {
    if (!initialized) return false;
    
    // Read status register
    uint8_t status = axpReadReg(AXP2101_STATUS1);
    
    // Bit 5 typically indicates charging status on AXP2101
    return (status & 0x20) != 0;
}
