// PS/2 Touchpad Driver for the STM32 platform
//
// 2019 by ppelikan
// github.com/ppelikan

#include "ps2.h"
#include "touchpad.h"

static volatile touchapd_Mode touchpad_CurrentMode = eUninitialized;

int8_t touchapd_init()
{
    ps2_sendByte(0xFF); // Reset
    if (!ps2_getACK())
        return TOUCHPAD_SET_MODE_FAILED;
    ps2_sendByte(0xF4); // Enable Data Reporting
    if (!ps2_getACK())
        return TOUCHPAD_SET_MODE_FAILED;
    touchpad_CurrentMode = eMovementMode;
    return TOUCHPAD_OK;
}

static int8_t touchpad_turnAbsoluteModeON()
{
    // this only works for Synaptics® devices
    static const uint8_t unlock_sequence[] = {0xE8, 0x02, 0xE8, 0x00, 0xE8, 0x00, 0xE8, 0x00, 0xF3, 0x14};
    size_t cnt = sizeof(unlock_sequence) / sizeof(unlock_sequence[0]);
    for (size_t i = 0; i < cnt; i++)
    {
        ps2_sendByte(unlock_sequence[i]);
        if (!ps2_getACK())
            return TOUCHPAD_SET_MODE_FAILED;
    }
    touchpad_CurrentMode = eAbsoluteMode;
    return TOUCHPAD_OK;
}

int8_t touchpad_setMode(touchapd_Mode mode)
{
    if ((touchpad_CurrentMode == eUninitialized) || (mode == eMovementMode))
        if (touchapd_init())
            return TOUCHPAD_SET_MODE_FAILED;

    if (mode == eAbsoluteMode)
        return touchpad_turnAbsoluteModeON();

    return TOUCHPAD_OK;
}

touchapd_Mode touchpad_getCurrentMode()
{
    return touchpad_CurrentMode;
}

int8_t touchapd_setSampleRate(touchpad_SampleRate value)
{
    HAL_Delay(1);
    ps2_sendByte(0xF3);
    if (!ps2_getACK())
        return TOUCHPAD_SET_MODE_FAILED;
    ps2_sendByte((uint8_t)value);
    if (!ps2_getACK())
        return TOUCHPAD_SET_MODE_FAILED;
    return TOUCHPAD_OK;
}

int8_t touchapd_readMovement(int16_t *px, int16_t *py, bool *button)
{
    if (touchpad_CurrentMode != eMovementMode)
        return TOUCHPAD_WRONG_MODE_ERROR;
    if (!ps2_isDataAvaiable(3))
    {
        ps2_scheduleRx();
        return TOUCHPAD_NO_DATA_TO_READ;
    }

    uint8_t dt, dx, dy;
    uint8_t fx, fy;
    ps2_readByte(&dt);
    if (!(dt & 0x08)) // verify if data is correct
    {
        return TOUCHPAD_CORRUPT_DATA_ERROR;
    }
    ps2_readByte(&dx);
    ps2_readByte(&dy);

    if (dt & 0x10)
        fx = 0xff;
    else
        fx = 0x00;
    if (dt & 0x20)
        fy = 0xff;
    else
        fy = 0x00;
    *py = (int16_t)((uint16_t)(fy << 8) | (uint16_t)dy);
    *px = (int16_t)((uint16_t)(fx << 8) | (uint16_t)dx);
    *button = ((dt & 0x01) == 0x01);

    return TOUCHPAD_OK;
}

int8_t touchapd_readAbsolutePosition(uint16_t *px, uint16_t *py, uint8_t *pz)
{
    // this only works for Synaptics® devices
    if (touchpad_CurrentMode != eAbsoluteMode)
        return TOUCHPAD_WRONG_MODE_ERROR;
    if (!ps2_isDataAvaiable(6))
    {
        ps2_scheduleRx();
        return TOUCHPAD_NO_DATA_TO_READ;
    }

    uint8_t dt1, dt2, dt3, dt4, dx, dy;
    ps2_readByte(&dt1);
    if ((dt1 & 0xC8) != 0x80) // verify if data is correct
        return TOUCHPAD_CORRUPT_DATA_ERROR;
    ps2_readByte(&dt2);
    ps2_readByte(&dt3);
    ps2_readByte(&dt4);
    if ((dt4 & 0xC8) != 0xC0) // verify if data is correct
        return TOUCHPAD_CORRUPT_DATA_ERROR;
    ps2_readByte(&dx);
    ps2_readByte(&dy);

    *px = (uint16_t)dx | (uint16_t)(0x0F & dt2) << 8 | (uint16_t)(dt4 & 0x10) << 8;
    *py = (uint16_t)dy | (uint16_t)(0xF0 & dt2) << 4 | (uint16_t)(dt4 & 0x20) << 7;
    *pz = dt3;
    return TOUCHPAD_OK;
}
