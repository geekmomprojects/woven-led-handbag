# woven-led-handbag

Source code and some design files for an LED Matrix handbag project.

Project writeup at http://www.geekmomprojects.com/led-matrix-handbag-2-0-how-to

The code requires the following libraries to compile:

- FastLED Library: https://github.com/FastLED/FastLED/releases
- Adafruit Bluefruit Libraries:  https://learn.adafruit.com/adafruit-feather-32u4-bluefruit-le/installing-ble-library


## Bluetooth Connectivity
Connecting to the handbag requires the Adafruit Bluefruit App:  https://learn.adafruit.com/bluefruit-le-connect-for-ios/settings

The Adafruit link contains a tutorial on how the Bluefruit App works.  To use it with the handbag, 
connect to the Feather M0 board and select UART from the menu.  This allows you to send and recieve text.
The code recognizes text starting with an "!" as a command, and responds to three different commands:

- **!next** or **!n** – moves to the next animation in the series
- **!bxxx** – where xxx is a 3-digit number sets the brightness level from 0-255
- **!pal** – selects a different color palette for the animations.  Palettes are chosen from a pre-set sequence.

You can also simply type in text which will scroll across the matrix.  As long as the text is not preceded by an exclamation point, it will display as written, scrolling across the matrix three times.

## Getting Data from Twitter using MQTT
MQTT is a lightweight messaging protocol, well suited for IoT communications.  The Bluefruit App can subscribe to an MQTT feed, and will automatically send received text data to the Feather M0 BLE board. Here are two possible ways to send Twitter data to an MQTT feed:

### (1) Adafruit.io and IFTTT
If you have never used MQTTT before, this will be the easiest way to start.  Adafruit.io is basically a user-friendly MQTT server.  You can use the free service IFTTT (If This Then That, www.ifttt.com) to make "recipes" that send data from one application to another.  The workflow is:

**Twitter**--(IFTTT recipe)-->**IFTTT**--(IFTTT recipe)-->**Adafruit.io**--(MQTT)-->**Bluefruit App**--(BLE)-->**Feather M0 BLE**

It is straightforward to figure out how to set up a recipe in IFTTT to retrieve data from Twitter and send it to Adafruit.io.  If you need help setting up the rest of of the workflow, these links are useful:
- Connecting IFTTT with Adafruit.io:  https://learn.adafruit.com/using-ifttt-with-adafruit-io/ifttt-to-adafruit-io-setup
- Connecting Bluefruit App to Adafruit.io via MQTT: https://learn.adafruit.com/datalogging-hat-with-flora-ble/bluefruit-le-connect-settings

The downside of using Adafruit.io and IFTTT is you have fairly little control over the timing of the data flow, and sometimes the response times between tweeting and receiving the data are long (10s of seconds).

### (2) Node-RED and Mosquitto
This method is more complicated to set up, but gives you much greater control over the data flow and has a faster response time.  I run both Node-Red and Mosquitto on a Raspberry Pi.  The workflow is:

**Twitter**--(Node-RED)-->**MQTT message**--(Node-RED)-->Mosquitto--(MQTT)-->**Bluefruit App**--(BLE)-->**Feather M0 BLE**

To set up this workflow, install both Mosquitto and Node-RED on a designated computer (again, a Raspberry Pi works very well for this).  

Configure Mosquitto with whatever security parameters you'd like.  Username/password restrictions are a good idea, but not configured automatically.  If you want to have your LED handbag retrieve messages from outside your home, then set up port forwarding to make the Mosquitto broker avaialable when you are out.  

In Node-RED, set up a flow with only two nodes as shown below.  The Twitter node conducts a Twitter search for a specific hashtag,  and the MQTT node sends the message out on a specified MQTT topic.  Set up the Bluefruit app to receive MQTT data from your Mosquitto server on that same topic, the messages will automatically appear as scrolling text on the LED Handbag

![Node-RED Flow](node-red-flow.png?raw=true "Node RED Flow")


