#include "rtc.h"
#include "SensorPCF85063.hpp"

SensorPCF85063 rtc;

bool rtcInit() {
  if (!rtc.begin(Wire)) {
    Serial.println("Failed to find PCF8563 RTC IC");
    return false;
  }
  Serial.println("RTC clock started");
  return true;
}

bool rtcIsSet() {
  RTC_DateTime dt = rtc.getDateTime();
  // Consider RTC unset if year is before 2024 or default value
  return dt.getYear() >= 2024;
}

void rtcGetDateTime(DateTime* dt) {
  RTC_DateTime rtcDt = rtc.getDateTime();
  dt->year = rtcDt.getYear();
  dt->month = rtcDt.getMonth();
  dt->day = rtcDt.getDay();
  dt->hour = rtcDt.getHour();
  dt->minute = rtcDt.getMinute();
}

void rtcSetDateTime(const DateTime* dt) {
  rtc.setDateTime(dt->year, dt->month, dt->day, dt->hour, dt->minute, 0);
  Serial.printf("RTC set to: %04d-%02d-%02d %02d:%02d\n",
                dt->year, dt->month, dt->day, dt->hour, dt->minute);
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
