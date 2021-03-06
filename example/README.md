# PS/2 Touchpad Driver for the STM32 platform - Example

This example uses the STM32F769i-disco board with the SSD1306 OLED display connected to I²C.
For OLED driving a following library is used: http://github.com/afiskon/stm32-ssd1306.

## Hardware setup

* Touchpad is connected to the 3v3 board power supply,
* Touchpad's PS/2 Data line -> PB15 (D11 on the board),
* Touchpad's PS/2 Clock line -> PA12 (D13 on the board),
* SSD1306 OLED is connected to the 3v3 board power supply,
* SSD1306 OLED's I2C SCL -> PB8 (D15 on the board),
* SSD1306 OLED's I2C SDA -> PB9 (D14 on the board).

Touchpad used in the example is Synaptics 920-001014-01 RevA.

![](https://github.com/ppelikan/stm32-ps2-touchpad/blob/main/example/img/touchpad_img.jpg)

https://user-images.githubusercontent.com/6893111/118008291-35b77000-b34d-11eb-8477-67572455df17.mp4
