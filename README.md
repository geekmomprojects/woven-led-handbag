# woven-led-handbag

Source code and some design files for an LED Matrix handbag project.

Project writeup at http://www.geekmomprojects.com/led-matrix-handbag-2-0-how-to

The code requires the following libraries to compile:

- FastLED Library: https://github.com/FastLED/FastLED/releases
- Adafruit Bluefruit Libraries:  https://learn.adafruit.com/adafruit-feather-32u4-bluefruit-le/installing-ble-library


### Bluetooth Connectivity
Connecting to the handbag requires the Adafruit Bluefruit App:  https://learn.adafruit.com/bluefruit-le-connect-for-ios/settings

The Adafruit link contains a tutorial on how the Bluefruit App works.  To use it with the handbag, 
connect to the Feather M0 board and select UART from the menu.  This allows you to send and recieve text.
The code recognizes text starting with an "!" as a command, and responds to three different commands:

- **!next** or **!n** – moves to the next animation in the series
- **!bxxx** – where xxx is a 3-digit number sets the brightness level from 0-255
- **!pal** – selects a different color palette for the animations.  Palettes are chosen from a pre-set sequence.

You can also simply type in text which will scroll across the matrix.  As long as the text is not preceded by an exclamation point, it will display as written, scrolling across the matrix three times.

### Getting Data from Twitter using MQTT
The Bluefruit App can subscribe to an MQTT feed, and will automatically send received text data to the Feather MO BLE board. Here are two ways to get Twitter data to an MQTT feed:

#### Adafruit.io and IFTTT
If you have never used MQTTT before, this will be the easiest way to start.  Adafruit.io is basically a simplified MQTT server.  You can use the free service IFTTT (If This Then That, http::www.ifttt.com) to make "recipes" that send data from one application to another.  The workflow is:

**Twitter**--(IFTTT recipe)-->**IFTTT**--(IFTTT recipe)-->**Adafruit.io**--(MQTT)-->**Bluefruit App**--(BLE)-->**Feather M0 BLE**

It is pretty easy to figure out how to set up a recipe in IFTTT to retrieve data from twitter and send it to Adafruit.io.  If you need help setting up the resto of the workflow, here are some useful articles:
- Connecting IFTTT with Adafruit.io:  https://learn.adafruit.com/using-ifttt-with-adafruit-io/ifttt-to-adafruit-io-setup
- Connecting Bluefruit App to Adafruit.io via MQTT: https://learn.adafruit.com/datalogging-hat-with-flora-ble/bluefruit-le-connect-settings

The downside of using Adafruit.io and IFTTT is you have fairly little control over the workflow, and sometimes the response times between tweeting and receiving the data are long (10s of seconds).

#### Node-RED and Mosquitto
