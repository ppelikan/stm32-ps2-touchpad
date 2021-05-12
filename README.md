# PS/2 Touchpad Driver for the STM32 platform

This driver allows to connect laptop touchpad or PS/2 mouse to the STM32 microcontroller.
Most of the touchpads in laptops use the PS/2 communication protocol and work fine, when powered at 3.3V.
This allows us to connect the data and clock lines directly to the STM32 microcontroller.

Additional feature is the compatibility with the Synaptics® touchpads. This driver allows to read additional parameters from the Synaptics® touchapad, for example: absolite finger position and touch pressure etc. This is not possible with other touchpads like ALPS for example.

## Implementation

The PS/2 driver uses STM32 HAL CubeMX lib. Please edit the `ps2.h` file in order to adapt the driver to your needs. Communication is performed by utilizing the SPI (IRQ slave Rx only mode) as well as GPIO (polling method).

This driver has been tested on the STM32F769-disco board with the SSD1306 OLED display connected at SYSCLK = HCLK = 200MHz. 
Touchpad used for testing was: Synaptics 920-001014-01 RevA.

## License

MIT License