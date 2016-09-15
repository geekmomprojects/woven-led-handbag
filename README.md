# woven-led-handbag

Source code and some design files for an LED Matrix handbag project.
Project writeup at http://www.geekmomprojects.com/led-matrix-handbag-2-0-how-to

The Arduino source code requires the following libraries to compile:

FastLED Library: https://github.com/FastLED/FastLED/releases

Adafruit Bluefruit Libraries:  https://learn.adafruit.com/adafruit-feather-32u4-bluefruit-le/installing-ble-library


### Bluetooth Connectivity
Connecting to the handbag requires the Adafruit Bluefruit App:  https://learn.adafruit.com/bluefruit-le-connect-for-ios/settings

The Adafruit link contains a tutorial on how the Bluefruit App works.  To use it with the handbag, 
connect to the Feather M0 board and select UART from the menu.  This allows you to send and recieve text.
The code recognizes text starting with an "!" as a command, and responds to three different commands:

-**!next** or !n – moves to the next animation in the series
-**!bxxx** – where xxx is a 3-digit number sets the brightness level from 0-255
-**!pal** – selects a different color palette for the animations.  Palettes are chosen from a pre-set sequence.
"!", it will display as written.

### Getting Data from Twitter using MQTT