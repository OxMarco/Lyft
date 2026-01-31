#include "storage.h"
#include "config.h"
#include "rtc.h"
#include <LittleFS.h>

bool storageInit() {
    if (!LittleFS.begin(true)) {  // true = format if mount fails
        Serial.println("LittleFS mount failed!");
        return false;
    }
    Serial.println("LittleFS ready");
    return true;
}

bool storageSaveSession(const WorkoutData &data) {
    // Skip invalid sessions (less than 5 seconds or no reps)
    if (data.elapsedTime < 5 || data.repCount == 0) {
        Serial.println("Session too short, not saving");
        return false;
    }

    File f = LittleFS.open(LOGFILE, FILE_APPEND);
    if (!f) {
        Serial.println("Failed to open file for writing");
        return false;
    }

    // Write CSV format: timestamp, reps, peak_velocity, duration
    f.printf("%lu,%d,%.2f,%lu\n",
             getTimestamp(),
             data.repCount,
             data.peakVelocity,
             data.elapsedTime);

    f.close();
    Serial.println("Session saved to log file");
    return true;
}

const char* storageGetLogPath() {
    return LOGFILE;
}

String storageReadLog() {
    File f = LittleFS.open(LOGFILE, FILE_READ);
    if (!f) {
        return "No data";
    }

    String content = f.readString();
    f.close();
    return content;
}

bool storageClearLog() {
    if (LittleFS.exists(LOGFILE)) {
        return LittleFS.remove(LOGFILE);
    }
    return true;
}
