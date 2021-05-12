
#include <stdio.h>
#include "app.h"

#include "ssd1306/ssd1306.h"
#include "ssd1306/ssd1306_tests.h"
#include "touchpad.h"

void displayPS2Error(int8_t err)
{
    char str[60];
    sprintf(str, "PS/2 Error: %d", err);
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(str, Font_6x8, White);
    ssd1306_UpdateScreen();
    printf("PS/2 Error: %d", err);
    HAL_Delay(500);
}

void displayLog(char *msg)
{
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(msg, Font_6x8, White);
    ssd1306_UpdateScreen();
    printf("%s\r\n", msg);
}

void dispMovement()
{
    static int16_t px = 20; // cursor current position
    static int16_t py = 20;
    static const int16_t xmux = 1; // cursor speed calibration
    static const int16_t ymux = 2;
    int16_t vx = 0, vy = 0;  //cursor current speed

    int8_t err = touchapd_readMovement(&vx, &vy, NULL);
    if (err < TOUCHPAD_CORRUPT_DATA_ERROR) // report heavy errors
    {
        displayPS2Error(err);
        return;
    }
    if (err != 0) // ignore a bit more frequent errors
        return;   // for example: FIFO empty (still waiting for data) or data corrupted (due to noise on data lines)

    ssd1306_Fill(Black);
    char str[20];
    sprintf(str, "X: %d", vx);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(str, Font_6x8, White);
    sprintf(str, "Y:  %d", vy);
    ssd1306_SetCursor(64, 0);
    ssd1306_WriteString(str, Font_6x8, White);

    px += vx; // apply movement to cursor position
    py -= vy; // cursor y position mirror
    if (px < 0)
        px = 0;
    if (py < 0)
        py = 0;
    if (py > SSD1306_HEIGHT * ymux)      // stop cursor form leaving the screen
        py = SSD1306_HEIGHT * ymux - 1;
    if (px > SSD1306_WIDTH * xmux)
        px = SSD1306_WIDTH * xmux - 1;
    ssd1306_DrawPixel(px / xmux, py / ymux, White);
    ssd1306_UpdateScreen();
}

void dispAbsolute()
{
    uint16_t px, py;  // touch x and y position
    uint8_t pr;       // touch pressure
    int8_t err = touchapd_readAbsolutePosition(&px, &py, &pr);
    if (err < TOUCHPAD_CORRUPT_DATA_ERROR) // report heavy errors
    {
        displayPS2Error(err);
        return;
    }
    if (err != 0) // ignore a bit more frequent errors
        return;   // for example: FIFO empty (still waiting for data) or data corrupted (due to noise on data lines)

    ssd1306_Fill(Black);
    char str[20];
    sprintf(str, "X: %d", px);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(str, Font_6x8, White);
    sprintf(str, "Y:  %d", py);
    ssd1306_SetCursor(64, 0);
    ssd1306_WriteString(str, Font_6x8, White);
    sprintf(str, "Pressure: %d", pr);
    ssd1306_SetCursor(0, 9);
    ssd1306_WriteString(str, Font_6x8, White);

    px = (px - 1115) / 36;           // apply calibration values
    py = SSD1306_HEIGHT - (py - 792) / 134;      // this depends on your display resolution
    pr = pr / 15;                    // you can experiment with theese values to choose best fit
    ssd1306_DrawCircle(px, py, pr, White);
    ssd1306_UpdateScreen();
}

void main_app()
{
    // ssd1306_TestAll();
    ssd1306_Init();
    displayLog("OLED init OK");

    int8_t err = touchapd_init();
    if (err)
        displayPS2Error(err - 20);
    else
        displayLog("TP init OK");
    HAL_Delay(500);

    /* // Uncomment to test addiional features 

    err = touchpad_setMode(eAbsoluteMode);
    if (err)
        displayPS2Error(err - 30);
    else
        displayLog("TP absolute OK");
    HAL_Delay(500);

    err = touchapd_setSampleRate(eSampleRate10fps);
    if (err)
        displayPS2Error(err - 40);
    else
        displayLog("TP sample rate OK");
    HAL_Delay(500);

    */

    while (1)
    {
        if (touchpad_getCurrentMode() == eMovementMode)
            dispMovement();
        if (touchpad_getCurrentMode() == eAbsoluteMode)
            dispAbsolute();

        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) // check user button
        {
            if (touchpad_getCurrentMode() == eMovementMode) //change mode after pressing button
                err = touchpad_setMode(eAbsoluteMode);
            else
                err = touchpad_setMode(eMovementMode);
            if (err)
                displayPS2Error(err - 30);
            else
                displayLog("TP mode set OK");
            HAL_Delay(300);
        }
    }
}