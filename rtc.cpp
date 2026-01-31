#include "rtc.h"
#include "SensorPCF85063.hpp"

SensorPCF85063 rtc;

bool rtcInit() {
  if (!rtc.begin(Wire)) {
    Serial.println("Failed to find PCF8563 RTC IC");
    return false;
  }
  RTC_DateTime datetime = rtc.getDateTime();

  Serial.println("RTC clock started");
  return true;
}

const char* getTimestamp() {
    static char buf[20];  // "YYYY-MM-DD,HH:MM:SS" + '\0'

    RTC_DateTime dt = rtc.getDateTime();
    snprintf(buf, sizeof(buf),
             "%04d-%02d-%02d,%02d:%02d:%02d",
             dt.getYear(),
             dt.getMonth(),
             dt.getDay(),
             dt.getHour(),
             dt.getMinute(),
             dt.getSecond());

    return buf;
}
