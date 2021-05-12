//  PS/2 Touchpad Driver for the STM32 platform
//
// Copyright (c) 2019 by ppelikan
// github.com/ppelikan

#ifndef __TOUCHPAD_H__
#define __TOUCHPAD_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define TOUCHPAD_OK (0)
#define TOUCHPAD_NO_DATA_TO_READ (-1)     // FIFO empty, no new data has been received (happens often)
#define TOUCHPAD_CORRUPT_DATA_ERROR (-2)  // data from touchapd is not correct (happens rarely)
#define TOUCHPAD_WRONG_MODE_ERROR (-7)    // please set correct mode to read data (should never happen)
#define TOUCHPAD_SET_MODE_FAILED (-8)     // touchpad not responding correctly (should never happen)

typedef enum // possible modes of the device
{
    eUninitialized, // touchapd_init() needs to be executed
    eMovementMode,  // standard touchpad or mouse mode (default after init)
    eAbsoluteMode   // Synaptics® absolute position mode
} touchapd_Mode;

typedef enum // possible datarates to select
{
    eSampleRate10fps = 10,
    eSampleRate20fps = 20,
    eSampleRate40fps = 40,
    eSampleRate60fps = 60,
    eSampleRate80fps = 80,
    eSampleRate100fps = 100,
    eSampleRate200fps = 200
} touchpad_SampleRate;

int8_t touchapd_init();
int8_t touchpad_setMode(touchapd_Mode mode);
touchapd_Mode touchpad_getCurrentMode();
int8_t touchapd_setSampleRate(touchpad_SampleRate value);                      // (not all devices support this)
int8_t touchapd_readMovement(int16_t *px, int16_t *py, bool *button);          // needs to be called frequently
int8_t touchapd_readAbsolutePosition(uint16_t *px, uint16_t *py, uint8_t *pz); // this only works for Synaptics® devices

//                              px         py
// Absolute reportable limits  0–6143     0–6143
// Typical bezel limits        1472–5472  1408–4448
// Typical edge margins        1632–5312  1568–4288

// pz = 0    No finger contact
// pz = 10   Finger hovering near the sensor surface
// pz = 30   Very light finger contact
// pz = 80   Normal finger contact
// pz > 110  Very heavy finger contact
// pz = 255  Maximum reportable Z

#endif