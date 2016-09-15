#include <Adafruit_BLE.h>
#include <Adafruit_BluefruitLE_SPI.h>
#include <Adafruit_BluefruitLE_UART.h>
#include <FastLED.h>
#include "displayClass.h"
#include "BluefruitConfig.h"



#define __DEBUG

// Controller button values - not currently used
enum {BUTTON_1 = 1, BUTTON_2, BUTTON_3, BUTTON_4, BUTTON_UP, BUTTON_DOWN, BUTTON_LEFT, BUTTON_RIGHT};


// Data source modes
//enum {DATA_SOURCE_CONTROLLER, DATA_SOURCE_UART};
//int dataMode = DATA_SOURCE_UART;

// Feather setup

// Bluefruit settings
#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"
#define BUFSIZE                     256   // Size of the read buffer for incoming data
#define VERBOSE_MODE                true  // If set to 'true' enables debug output
#define BLE_READPACKET_TIMEOUT      500   // Timeout in ms waiting to read a response


// APA102 LED Strip settings for FastLED
#define DATA_PIN    20
#define CLOCK_PIN   21
#define CHIPSET     APA102
#define BRIGHTNESS  40

// Params for LED matrix width and height
const uint8_t kMatrixWidth = 14;
const uint8_t kMatrixHeight = 8;

#define NUM_LEDS (kMatrixWidth*kMatrixHeight)

// Use an extra matrix value as a safety pixel so we don't overwrite our 
//  array boundaries
CRGB leds_plus_safety_pixel[ NUM_LEDS + 1];
CRGB* const leds( leds_plus_safety_pixel + 1);

// Buffer for for creating smooth led animations or for scrolling
CRGB led_buffer_plus_safety_pixel[NUM_LEDS + 1];
CRGB* const led_buffer(led_buffer_plus_safety_pixel + 1);


// Class instances
ParticleEmitter dEmit(leds, led_buffer, kMatrixWidth, kMatrixHeight);
DrawText        dText(leds, led_buffer, kMatrixWidth, kMatrixHeight);
DisplayRain     dRain(leds, led_buffer, kMatrixWidth, kMatrixHeight);
GameOfLife      dGame(leds, led_buffer, kMatrixWidth, kMatrixHeight);
BouncingPixels  dBounce(leds, led_buffer, kMatrixWidth, kMatrixHeight);
Twinkle         dTwinkle(leds, led_buffer, kMatrixWidth, kMatrixHeight);
//Lines           dLines(leds, led_buffer, kMatrixWidth, kMatrixHeight); -- CURRENTLY NOT WORKING
Worm            dWorm(leds, led_buffer, kMatrixWidth, kMatrixHeight);
SinWave         dSin(leds, led_buffer, kMatrixWidth, kMatrixHeight);

// Display modes
DisplayMatrix *autoDisplays[] = {&dSin, &dRain, &dWorm, &dTwinkle, &dGame, &dBounce};
const int numModes = sizeof(autoDisplays)/sizeof(autoDisplays[0]);
int displayMode = 0;

/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
// Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

// Create Bluefruit object with default pin values
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

// the packet buffer
extern uint8_t packetbuffer[];

void setup() {
  Serial.begin(9600);
#ifdef __DEBUG
  Serial.println(F("Adafruit Bluefruit Command <-> LED Strip Programming"));
  Serial.println(F("--------------waiting 5 seconds------------"));
#endif
  delay(5000);

  /* Initialise the module */
#ifdef __DEBUG
  Serial.print(F("Initialising the Bluefruit LE module: "));
#endif
  if ( !ble.begin(VERBOSE_MODE) )
  {
#ifdef __DEBUG
    Serial.println(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
#endif
  }
#ifdef __DEBUG
  Serial.println( F("OK!") );
#endif
  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
//    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
#ifdef __DEBUG
      Serial.println(F("Couldn't factory reset"));
#endif
    }
  }
  /* Disable command echo from Bluefruit */
  ble.echo(false);

#ifdef __DEBUG
  Serial.println("Requesting Bluefruit info:");
#endif
  /* Print Bluefruit information */
  ble.info();

#ifdef __DEBUG
  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in Controller mode"));
  Serial.println(F("Then Enter directions to send to Bluefruit"));
  Serial.println();
#endif
  ble.verbose(false);  // debug info is a little annoying after this point!


  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
#ifdef __DEBUG
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
#endif
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  // Set module to DATA mode
#ifdef __DEBUG
  Serial.println( F("Switching to DATA mode!") );
#endif
  ble.setMode(BLUEFRUIT_MODE_DATA);



  // Set up LED stripts
  FastLED.addLeds<CHIPSET, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness( BRIGHTNESS );
  FastLED.clear();

  pinMode(A0, OUTPUT);  // This is a hack to fix a problem with Ground
  digitalWrite(A0, LOW);

  // Initialize random functions
  randomSeed(analogRead(0));

  // Initialize class instances
  for (int i = 0; i < numModes; i++) {
    autoDisplays[i]->init();
  }
  
  //dText.addStringToBuffer("Hi", 3, 64);
}

/*
boolean getControllerData() {
  boolean modeChanged = false;
  
    // Echo received data
  while ( ble.available() )
  {
    uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);
    if (len > 0) {
      if (packetbuffer[1] == 'C') {   // Color picker
        uint8_t red = packetbuffer[2];
        uint8_t green = packetbuffer[3];
        uint8_t blue = packetbuffer[4];
#ifdef __DEBUG
        Serial.print ("RGB = ");
        Serial.print(red);
        Serial.print(", ");
        Serial.print(green);
        Serial.print(", ");
        Serial.println(blue);
#endif
        dText.setColor(CRGB(red,green,blue));

      } else if (packetbuffer[1] == 'B') {  // Buttons
        uint8_t buttnum = packetbuffer[2] - '0';
        boolean pressed = packetbuffer[3] - '0';
#ifdef __DEBUG        
        Serial.print ("Button "); Serial.print(buttnum);
        if (pressed) Serial.println(" pressed");
        else Serial.println(" released");
#endif
        if ((buttnum == BUTTON_RIGHT) && pressed) {
          displayMode = (displayMode + 1) % numModes;
          modeChanged = true;
        } else if ((buttnum == BUTTON_4) && pressed) {
          modeChanged = true;
          dataMode = DATA_SOURCE_UART;
          
          //dText.init();
          //dText.setDisplayText("UART Mode");
          
          Serial.println("button 4 pressed - changing to uart mode");
        } else if ((buttnum == BUTTON_UP) && pressed) {
          autoDisplays[displayMode]->nextPalette();
        }

      }
    } 
  }
  return modeChanged;
}
*/

boolean getUartData() {
  boolean gotData = false;
  boolean modeChanged = false;
  String str = "";
  while (ble.available()) {
    gotData = true;
    int c = ble.read();
    str += (char)c;
#ifdef DEBUG   
    Serial.print((char) c);
#endif
    delay(10); // Give the rest of the data a chance to come in
  }
  if (gotData) {
    if (str[0] == '!') {
      str.toLowerCase();
      if (str == "!next" || str == "!n") {       // choose next display mode
        displayMode = (displayMode + 1) % numModes;
        modeChanged = true;
      } else if (str == "!pal") { // Select next palette
        dText.nextPalette();
        autoDisplays[displayMode]->nextPalette();
      }  else if (str[1] == 'b' || str[1] == 'B') {  // Set the brightness of the display (0-255)
        uint8_t bright = constrain((str.substring(2)).toInt(), 1, 255);
        FastLED.setBrightness(bright);    
      }
    } else {
      //dText.init();
      // Be sure we don't have multiple twitter usernames conflated here - separate any usernames
      int strStart = 0;
      int findChar = str.indexOf('@', 1);
      while (findChar != -1) {
        String sub = str.substring(strStart, findChar);
        dText.addStringToBuffer(sub.c_str(), 3, random(255));
        strStart = findChar;
        findChar = str.indexOf('@', strStart+1);
      }
      String lastStr = str.substring(strStart);
      dText.addStringToBuffer(lastStr.c_str(), 3, random(255));
    }
#ifdef DEBUG
    Serial.println("");
#endif
  }
  return modeChanged;
}

void loop() {

  // Check for user input
  boolean modeChanged = false;

/*
  // Check for incoming data
  switch(dataMode) {
    case DATA_SOURCE_CONTROLLER:
      modeChanged = getControllerData();
      break;
    case DATA_SOURCE_UART:
      modeChanged = getUartData();
      break;
  }
*/
  // Check for data from the BLE
  modeChanged = getUartData();  
  // Update display
  // if (dataMode == DATA_SOURCE_UART && dText.displayingText()) {
  if (dText.displayingText()) {
    dText.update();
  } else {
    if (modeChanged) {
      autoDisplays[displayMode]->clearDisplay();
    }
    autoDisplays[displayMode]->update();
  }
}



