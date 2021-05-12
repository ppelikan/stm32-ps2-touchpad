//  PS/2 Driver for the STM32 platform
//
// This driver uses mixed SPI slave Rx only in interrupt mode
// with GPIO signals to implement PS/2 protocol communication.
// This has been tested on the STM32F769 microcontroller
// at SYSCLK = HCLK = 200MHz
//
// Copyright (c) 2019 by ppelikan
// github.com/ppelikan

#include "ps2.h"

static volatile uint8_t RxFIFO[RX_FIFO_SIZE];       // circular buffer for storing unread bytes
static volatile int8_t RxFIFOIn = 0, RxFIFOOut = 0; // circular buffer indexes for put and pop data
static volatile uint16_t RxBuff = 0x0000;           // rx RAW dataframe currently being processed (11 bits)
static volatile bool SPI_BusyFlag = false;

static uint8_t hasEvenParity(uint8_t x);
static void putByte(uint8_t byte);

extern SPI_HandleTypeDef PS2_SPI_HANDLE; // doesn't need to be extern, if you wish


// STM32's HAL SPI callback, called by the HAL_SPI_IRQHandler
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if ((RxBuff & 0x401) == 0x400) // checking start and stop bits
    {
        uint8_t byte = (((uint16_t)RxBuff >> 1) & (uint16_t)0x0FF); // extracting data byte
        if (hasEvenParity(byte) == ((RxBuff & 0x200) == 0x200))     // checking parity bit
            putByte(byte);                                          // put byte into RxFIFO queue
    }
    SPI_BusyFlag = false;
    ps2_scheduleRx(); // get ready for more data
}

// STM32's HAL SPI callback, called by the HAL_SPI_IRQHandler
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    ps2_scheduleRx(); // ignore errors, just get more data
}

static void initSPI()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    PS2_ENABLE_ALL_CLOCKS;
    /*
    SPI2 GPIO Configuration
    PS2_CLK_Pin     ------> SPI2_SCK
    XXXXXXXXXXXX    ------> SPI2_MISO  // not needed!
    PS2_DATA_Pin    ------> SPI2_MOSI  // in reality STM32's MOSI acts here as the real MISO
    */
    GPIO_InitStruct.Pin = PS2_CLK_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = PS2_CLK_AF;
    HAL_GPIO_Init(PS2_CLK_GPIO_Port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = PS2_DATA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = PS2_DATA_AF;
    HAL_GPIO_Init(PS2_DATA_GPIO_Port, &GPIO_InitStruct);

    PS2_SPI_HANDLE.Instance = PS2_SPI;
    PS2_SPI_HANDLE.Init.Mode = SPI_MODE_SLAVE;                   // important (you can damage the CPU or touchpad if you make CLK collision)
    PS2_SPI_HANDLE.Init.Direction = SPI_DIRECTION_2LINES_RXONLY; // important
    PS2_SPI_HANDLE.Init.DataSize = SPI_DATASIZE_11BIT;
    PS2_SPI_HANDLE.Init.CLKPolarity = SPI_POLARITY_HIGH;
    PS2_SPI_HANDLE.Init.CLKPhase = SPI_PHASE_1EDGE;
    PS2_SPI_HANDLE.Init.NSS = SPI_NSS_SOFT;
    PS2_SPI_HANDLE.Init.FirstBit = SPI_FIRSTBIT_LSB;
    PS2_SPI_HANDLE.Init.TIMode = SPI_TIMODE_DISABLE;
    PS2_SPI_HANDLE.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    PS2_SPI_HANDLE.Init.CRCPolynomial = 7;
    PS2_SPI_HANDLE.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    PS2_SPI_HANDLE.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    if (HAL_SPI_Init(&PS2_SPI_HANDLE) != HAL_OK)
        while (1)
            ;

    HAL_SPI_Abort_IT(&PS2_SPI_HANDLE);
    HAL_NVIC_SetPriority(PS2_SPI_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(PS2_SPI_IRQn);
}

// turns off, mutes the SPI and enables the GPIO pins for bitbanging
static void SPI_DeInit()
{
    HAL_SPI_Abort_IT(&PS2_SPI_HANDLE);
    HAL_GPIO_DeInit(PS2_CLK_GPIO_Port, PS2_CLK_Pin);    // needed for below hack (in order not to short the clk to 3V3)
    HAL_GPIO_DeInit(PS2_DATA_GPIO_Port, PS2_DATA_Pin);
    HAL_NVIC_DisableIRQ(PS2_SPI_IRQn);

    PS2_SPI_HANDLE.Init.Mode = SPI_MODE_MASTER;  // hack needed to ignore clocks during data tx to touchpad (STM32 silicon bug?)
    if (HAL_SPI_Init(&PS2_SPI_HANDLE) != HAL_OK) // somehow even disablinkg SPI RCC clock does not prevent it from counting clocks during data Tx
        while (1)
            ;
    PS2_SPI_CLOCK_DISABLE;
}

// ...or use __builtin_parity() if you're using GCC
static uint8_t hasEvenParity(uint8_t x)
{
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return (~x) & 1;
}

// todo remove this ugly contraption
#define WAIT_FOR(X)           \
    resetTimeout();           \
    while (X)                 \
        if (checkTimeout(30)) \
            break;

uint32_t tickstart = 0;
static void resetTimeout()
{
    tickstart = HAL_GetTick();
}

static uint8_t checkTimeout(uint32_t timeout)
{
    return ((HAL_GetTick() - tickstart) >= timeout);
}

static uint8_t isCLKset()
{
    return (HAL_GPIO_ReadPin(PS2_CLK_GPIO_Port, PS2_CLK_Pin) == GPIO_PIN_SET);
}

static uint8_t isDATAset()
{
    return (HAL_GPIO_ReadPin(PS2_DATA_GPIO_Port, PS2_DATA_Pin) == GPIO_PIN_SET);
}

// sets the data pin to 0 or pullup 1
static void setDATA(uint8_t bit)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (bit)
    {
        GPIO_InitStruct.Pin = PS2_DATA_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP; // only the pullup is safe to set this line to 1
        HAL_GPIO_Init(PS2_DATA_GPIO_Port, &GPIO_InitStruct);
    }
    else
    {
        GPIO_InitStruct.Pin = PS2_DATA_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(PS2_DATA_GPIO_Port, &GPIO_InitStruct);
        HAL_GPIO_WritePin(PS2_DATA_GPIO_Port, PS2_DATA_Pin, GPIO_PIN_RESET);
    }
}

// sets the clk pin to 0 or pullup 1
static void setCLK(uint8_t bit)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (bit)
    {
        GPIO_InitStruct.Pin = PS2_CLK_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP; // setting this line to 1 is safe only using pullup
        HAL_GPIO_Init(PS2_CLK_GPIO_Port, &GPIO_InitStruct);
    }
    else
    {
        GPIO_InitStruct.Pin = PS2_CLK_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(PS2_CLK_GPIO_Port, &GPIO_InitStruct);
        HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_RESET);
    }
}

// waits until the clk line becomes != bit
static int8_t waitForCLK(uint8_t bit)
{
    resetTimeout();
    while (isCLKset() == bit)
        if (checkTimeout(40))
            return -1;
    return 0;
}

// puts received byte into fifo
static void putByte(uint8_t byte)
{
    if (RxFIFOIn == ((RxFIFOOut + RX_FIFO_SIZE - 1) % RX_FIFO_SIZE))
        return; //buffer is full, drop data
    RxFIFO[RxFIFOIn] = byte;
    RxFIFOIn = (RxFIFOIn + 1) % RX_FIFO_SIZE;
}

// pops received byte from fifo and returns
bool ps2_readByte(uint8_t *byte)
{
    if (RxFIFOIn == RxFIFOOut)
        return false; //buffer is empty
    *byte = RxFIFO[RxFIFOOut];
    RxFIFOOut = (RxFIFOOut + 1) % RX_FIFO_SIZE;
    return true;
}

// true if given number of bytes is ready for reading from FIFO
bool ps2_isDataAvaiable(uint8_t n_bytes)
{
    return (((RxFIFOIn + RX_FIFO_SIZE - RxFIFOOut) % RX_FIFO_SIZE) >= n_bytes);
}

// starts the SPI to listen and capture the data 
void ps2_scheduleRx()
{
    if (SPI_BusyFlag)
        return;
    HAL_StatusTypeDef err = HAL_SPI_Receive_IT(&PS2_SPI_HANDLE, (uint8_t *)&RxBuff, 1);
    if (err == HAL_OK)
        SPI_BusyFlag = true;
}

// stops data flow from the device and sends byte to it
void ps2_sendByte(uint8_t byte)
{
    // PS/2 Specification forces us to use bit banging method here
    // also, we rarely use this function, mailnly during the startup init
    SPI_DeInit();
    setCLK(0); // clk low to force the device to switch to rx mode
    HAL_Delay(1);
    setDATA(0); // data low
    // HAL_Delay(1); // optional
    setCLK(1); // release clk

    // TODO fix this problem!
    // waitForCLK(1); //when i connect scope sonde, this two lines are needed in order for this to work correctly
    // waitForCLK(0); //when i disconnect scope, this both lines must be commented for touchpad to work ok

    for (uint8_t i = 0; i < 8; i++)
    {
        waitForCLK(1);               // wait for clk to be low
        setDATA((byte >> i) & 0x01); // set data bit high/low
        waitForCLK(0);
    }
    waitForCLK(1);
    setDATA(hasEvenParity(byte)); // set data according to parity bit
    waitForCLK(0);
    waitForCLK(1);
    setCLK(1);  // release clk
    setDATA(1); // release data
    WAIT_FOR(isDATAset());
    waitForCLK(1);
    WAIT_FOR((!isCLKset()) && (!isDATAset()));
}

bool ps2_getACK()
{
    // flush the FIFO
    RxFIFOIn = RxFIFOOut = 0;
    RxBuff = 0x00;
    // restart the Rx process
    initSPI();
    SPI_BusyFlag = false;
    ps2_scheduleRx();
    // receive the ACK data byte
    WAIT_FOR(!ps2_isDataAvaiable(1));
    uint8_t v = 0;
    if (!ps2_readByte(&v))
        return false; // no response
    if (v != 0xFA) // check ACK
        return false; // wrong response
    return true;
}
