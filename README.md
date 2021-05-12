# PS/2 Touchpad Driver for the STM32 platform

This driver allows to connect laptop touchpad or PS/2 mouse to the STM32 microcontroller.
Most of the touchpads in laptops use the PS/2 communication protocol and work fine, when powered at 3.3V.
This allows us to connect the data and clock lines directly to the STM32 microcontroller.

Additional feature, this driver provides, is the possibility to read additional parameters from the Synaptics® touchapad, for example: absolute finger position and touch pressure etc. This is not possible with other touchpads like ALPS for example.

https://user-images.githubusercontent.com/6893111/118008291-35b77000-b34d-11eb-8477-67572455df17.mp4

## Implementation

The PS/2 driver uses STM32 HAL CubeMX lib. Please edit the `ps2.h` file in order to adapt the driver to your needs. Communication is performed by utilizing the SPI (IRQ slave Rx only mode) as well as GPIO (polling method). No external pullups needed.

This driver has been tested on the STM32F769i-disco board at `SYSCLK = HCLK = 200MHz` with the SSD1306 OLED display connected. 
Touchpad used for testing was: Synaptics 920-001014-01 RevA.

## License

MIT License
