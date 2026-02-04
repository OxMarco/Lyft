#ifndef RTC_H
#define RTC_H

#include <Arduino.h>

struct DateTime {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
};

bool rtcInit();
bool rtcIsSet();
void rtcGetDateTime(DateTime* dt);
void rtcSetDateTime(const DateTime* dt);
const char* getTimestamp();

#endif // RTC_H
