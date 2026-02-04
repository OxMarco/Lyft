#ifndef SOUND_H
#define SOUND_H

#include <Arduino.h>

uint8_t getVolume();
void setVolume(uint8_t _volume);
bool audioInit();
void playPowerOnSound();
void playPowerOffSound();
void playStartWorkoutSound();
void playStopWorkoutSound();

#endif // SOUND_H
