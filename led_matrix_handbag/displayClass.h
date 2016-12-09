#ifndef __DISPLAY_CLASS
#define __DISPLAY_CLASS

#define MAX_TEXT_CHARS    256
#define MAX_TEXT_COLUMNS  MAX_TEXT_CHARS*6  // Max 5 cols per char + 1 blank column for intra-char spacing
#include <FastLED.h>


// IF YOU ARE USING A DIFFERENT DIMENSION MATRIX, CHANGE THE VALUES HERE!
#define MATRIX_WIDTH  14
#define MATRIX_HEIGHT 8

// DEPENDING ON HOW YOUR MATRIX IS SET UP, YOU MAY NEED TO INVERT THE HORIZONTAL OR DIRECTION OF TEXT. IF SO, UNCOOMENT THE LINE BELOW
//#define INVERT_TEXT_HORZ

// Palettes from FastLED library
static CRGBPalette16 matrixPaletteList[] = {RainbowColors_p, CloudColors_p, PartyColors_p, OceanColors_p, LavaColors_p};
static const int numPalettes = sizeof(matrixPaletteList)/sizeof(matrixPaletteList[0]);

///////////////////////////////////////////////////////////////////////
//  Main base class for matrix LED functions.  Pure virtual class that 
//  supoorts indexing into the LED matrix array and updating the display
//  after a certain time has passed.
//////////////////////////////////////////////////////////////////////
class DisplayMatrix {

public:

	DisplayMatrix(CRGB *leds, CRGB *buf,  uint8_t w, uint8_t h, uint16_t delayMS = 200, uint8_t palIndex = 0, TBlendType blending = LINEARBLEND) { 
	  _lastUpdateTime = -1; _leds = leds; _buffer = buf;  _width = w; _height = h; _delayMS = delayMS, _paletteIndex = palIndex; _blending = blending;
	}

  // Pure virtual functions must be overriden in child classes
	virtual void init() = 0;
	virtual boolean update() = 0;

  // Helper function
  boolean timeToUpdate();

  // Matrix math funcitons - from fastLED example
  uint16_t XY( uint8_t x, uint8_t y);
  uint16_t XYsafe( uint8_t x, uint8_t y);
  
  // Matrix manipulation functions
  void shiftOneDown(CRGB *leds);
  void shiftOneUp(CRGB *leds);
  void shiftOneRight(CRGB *leds);
  void shiftOneLeft(CRGB *leds);
  void shiftPercentDown(int percent, CRGB* nextRow);
  void shiftPercentLeft(int percent, CRGB* nextCol);
  void copyMatrix(CRGB *from, CRGB *to, uint8_t nLeds);
  void clearDisplay();

  // Palette functions
  CRGBPalette16 getPalette() {return matrixPaletteList[_paletteIndex]; };
  void nextPalette() { _paletteIndex = (_paletteIndex + 1) % numPalettes; };


protected:
  // Data
	long           _lastUpdateTime;
  CRGB          *_leds;
  CRGB          *_buffer;
  CRGB           _color;
  uint8_t        _width;
  uint8_t        _height;
  uint8_t        _paletteIndex;
  uint16_t       _delayMS;
  TBlendType     _blending;

};

/////////////////////////////////////////////////////////////////////////////
//  Helper class that holds a string, the color to display it, and the number
//  of times to display it on the LED Matrix
/////////////////////////////////////////////////////////////////////////////
#define MAX_STRING_LENGTH 256
class StringUnit {
  
public:
  StringUnit() {_str = "", _repeat = 0; _colorIndex = 0; };
  void    setValues(const char* str, uint8_t repeat, uint8_t colorIndex) {if (strlen(str) < MAX_STRING_LENGTH) _str = str; _repeat = repeat; _colorIndex = colorIndex;};  
  uint8_t getRepeat() { return _repeat; };
  void    setRepeat(uint8_t repeat) { _repeat = _repeat; };
  void    setString(char* str) { if ( strlen(str) < MAX_STRING_LENGTH ) _str = str; };
  void    copyString(char* buf) { strcpy( buf, _str.c_str() ); };
  uint8_t getColorIndex() { return _colorIndex; };
  
private:
  String    _str;
  uint8_t   _repeat;   //# of times to repeat displaying
  uint8_t   _colorIndex;
};

//////////////////////////////////////////////////////////////////////////
// Helper class that contains a circular buffer of StringUnit objects.  Used
//  inside the DrawText class
//////////////////////////////////////////////////////////////////////////
#define MAX_STRING_BUFFER_SIZE 64  //Must be less than 256
class StringUnitBuffer {

public:
  StringUnitBuffer() { _first = 0; _last = 0; _current = 0; };
  boolean isEmpty() { if ( _last == _first ) return true; else return false; };
  boolean isFull() { if ( ( _last + 1 ) % MAX_STRING_BUFFER_SIZE == _first) return true; else return false; };
  
  // FIFO - add new string to end
  boolean push(const char* str, uint8_t repeat, uint8_t colorIndex) { 
    if (isFull()) return false;  
    _sBuffer[_last].setValues(str, repeat, colorIndex); 
    // Increment last pointer
    _last = ( _last + 1 ) %   MAX_STRING_BUFFER_SIZE;
    return true;
  }
  
  // FIFO pop from beginning - be sure buf is big enough to hold string
  boolean popFirst(char* buf, uint8_t *colorIndex) {
    if (isEmpty()) return false;
    
    // Copy return values
    _sBuffer[_first].copyString(buf);
    *colorIndex = _sBuffer[_first].getColorIndex();
    uint8_t repeatCount = _sBuffer[_first].getRepeat();

    // Increment _first pointer
    _first = (_first + 1) % MAX_STRING_BUFFER_SIZE;
    
    // If repetitions left add to end of buffer
    if (repeatCount > 1) {
      push(buf, repeatCount - 1, *colorIndex);
    }
    return true;
  }
  uint8_t nElements() { if (_last < _first) return  MAX_STRING_BUFFER_SIZE + _last - _first; else return _last - _first; };
  uint8_t getFirstIndex() { while ( _sBuffer[_first].getRepeat() == 0 && _first != _last ) { _first = (_first + 1) % MAX_STRING_BUFFER_SIZE; } return _first; };
  uint8_t getLastIndex() { return _last; }

private:
  StringUnit  _sBuffer[MAX_STRING_BUFFER_SIZE];
  uint8_t     _first;
  uint8_t     _last;
  uint8_t     _current;
  uint16_t    _delayMS;
};

//////////////////////////////////////////////////////////////////////////////////
// Class whose function is to display scrolling text on the LED Matrix
//////////////////////////////////////////////////////////////////////////////////
class DrawText : public DisplayMatrix {

public:
  DrawText(CRGB *leds, CRGB *buff, uint8_t w, uint8_t h, uint16_t delayMS = 120, uint8_t palIndex = 0, CRGB color = CRGB::Red) : DisplayMatrix( leds, buff, w, h, delayMS, palIndex ) { 
    _colPtr = 0; _colLen = 0; _color = color; _textInBuffer = false;
  }
  void    init();
  boolean update();
  boolean displayingText() { if (_textInBuffer || !_stringBuffer.isEmpty()) return true; else return false; };
  void    setDelay(uint16_t ms) { _delayMS = ms; }
  void    setColor(CRGB col) { _color = col; };
  boolean addStringToBuffer(const char* txt, uint8_t repeat = 3, uint8_t colIndex = 0) { return _stringBuffer.push(txt, repeat, colIndex); };

// Functions
private:
  void setDisplayText(const char *txt);

// Data
private:
  uint16_t   _displayBuffer[MAX_TEXT_COLUMNS];
  uint16_t  _colLen;
  uint16_t  _colPtr;
  CRGB      _color;
  boolean   _textInBuffer;
  // Cirucluar buffer of strings to be displayed
  StringUnitBuffer  _stringBuffer;
  
};

//////////////////////////////////////////////////////////////////////////////////
//  Class that displays multicolor falling raindrops on the LED Matrix
//////////////////////////////////////////////////////////////////////////////////
class DisplayRain : public DisplayMatrix {
  
public:
  DisplayRain(CRGB *leds, CRGB *buff, uint8_t w, uint8_t h, uint16_t delayMS = 1, uint8_t palIndex = 0) : DisplayMatrix( leds, buff, w, h, delayMS, palIndex ) {
     _colorIndex = 0; _brightness = 64; _counter = 0;
  }
  void    init();
  boolean update();
  CRGB    nextColorFromPalette();

  // Data

private:
  uint8_t    _colorIndex;
  uint8_t    _brightness;
  uint8_t    _counter;
};

///////////////////////////////////////////////////////////////////////////////
//  Class that displays multiple pixels that bounce around the LED Matrix
///////////////////////////////////////////////////////////////////////////////
class BouncingPixels : public DisplayMatrix {
  
public:
  #define N_BOUNCING_PIXELS 6
  BouncingPixels(CRGB *leds, CRGB *buff, uint8_t w, uint8_t h, uint16_t delayMS = 50, uint8_t palIndex = 0) : DisplayMatrix( leds, buff, w, h, delayMS, palIndex ) {};
  void init();
  boolean update();

// Data
private:
  float       _pos[N_BOUNCING_PIXELS][2], _vel[N_BOUNCING_PIXELS][2];  // Position, velocity (x, y);  uints pixels, and pixels/sec
  uint8_t     _col[N_BOUNCING_PIXELS];
};

/////////////////////////////////////////////////////////////////////////////
//  Class to run Conway's Game of Life on the LED Matrix. Wraps around.
/////////////////////////////////////////////////////////////////////////////
class GameOfLife : public DisplayMatrix {

public:
  GameOfLife(CRGB *leds, CRGB *buff, uint8_t w, uint8_t h, uint16_t delayMS = 50, uint8_t palIndex = 0) : DisplayMatrix( leds, buff, w, h, delayMS, palIndex ) {
    _brightness = 40; _counter = 0;
  }
  void    init();
  boolean update();
  int     countNeighbors(int ptr, int x, int y);
  void    setDisplayPixels(int ptr);
  
// Data
private:
  uint8_t        _brightness;
  uint8_t        _counter;
};
////////////////////////////////////////////////////////////////////////////////////////
//  Class that displays pixels "twinkling" on and off with different colors randomly
////////////////////////////////////////////////////////////////////////////////////////
class Twinkle : public DisplayMatrix {

public:
  Twinkle(CRGB *leds, CRGB *buff, uint8_t w, uint8_t h, uint16_t delayMS = 5, uint8_t palIndex = 0) : DisplayMatrix( leds, buff, w, h, delayMS, palIndex ) {
     _oddsFilled = round(255/.15); // time pixel is lit/15% lit at any time
  }
  void      init();
  boolean   update();
  boolean   isLit(uint8_t i) { if (_buffer[i][0]) return true; else return false; };

// Data
private:
  uint16_t    _oddsFilled;   // 
};

/////////////////////////////////////////////////////////////////////////////////////
//  Class that displays a "worm" of _length pixels travelling back and forth through
//  all pixels of the LED Matrix
/////////////////////////////////////////////////////////////////////////////////////
class Worm : public DisplayMatrix {
  
public:
  Worm(CRGB *leds, CRGB *buff, uint8_t w, uint8_t h, uint16_t delayMS = 50, uint8_t palIndex = 0) : DisplayMatrix( leds, buff, w, h, delayMS, palIndex ) {
    _front = 7, _length = 7; _dir = 1; _colorIndex = 0;
  }
  void    init();
  boolean update();
  
// Data
private:
  uint8_t     _front, _length, _dir;
  uint8_t     _colorIndex;  // Index into the palette
};

//////////////////////////////////////////////////////////////////////////////////
// Displays moving horizontal and vertical lines on the LED Matrix
//////////////////////////////////////////////////////////////////////////////////
class Lines : public DisplayMatrix {
public:
  Lines(CRGB *leds, CRGB *buff, uint8_t w, uint8_t h, uint16_t delayMS = 150, uint8_t palIndex = 0) : DisplayMatrix( leds, buff, w, h, delayMS, palIndex) {
    _rowColorIndex = 1; _colColorIndex = 1; _currentRow = 1; _currentCol = 1; _rowIncrement = 1; _colIncrement = 1;
  }
  void    init();
  boolean update(); 

// Data
private:
  uint8_t   _rowColorIndex, _colColorIndex;
  uint8_t   _currentRow, _currentCol;
  uint8_t   _rowIncrement, _colIncrement;
  
};

//////////////////////////////////////////////////////////////////////////////////
// Displays a moving particle emitter
//////////////////////////////////////////////////////////////////////////////////
#define MAX_PARTICLES 60
class Particle {
public:  
  Particle() {
    _px = 0; _py = 0; _vx = 0; _vy = 0; _lt = 0; _hue = 0; 
  }
  void setValues(float x, float y, float vx, float vy, uint8_t hue, uint8_t life = 10) {
    _px = x; _py = y; _vx = vx; _vy = vy; _lt = life; _hue = hue;
  }
  uint8_t getHue() {return _hue;}
  uint8_t getLife() {return _lt;}
  void getMatrixPos(uint8_t *x, uint8_t *y) { *x = ((uint8_t) round(_px)); *y = ((uint8_t) round(_py)); }  // Caller must check to see if we have wandered off the matrix
  boolean updatePos() { if (_lt <= 0) return false; else {_px += _vx; _py += _vy; _lt--; return true;} Serial.print("lt = "); Serial.println(_lt);} // Should be called every frame
// Data
private:
  float _px, _py;
  float _vx, _vy;  // In units of pixels per frame
  uint8_t _hue;
  uint8_t _lt;
};

class ParticleEmitter : public DisplayMatrix {
public:
  ParticleEmitter(CRGB *leds, CRGB *buff, uint8_t w, uint8_t h, uint16_t delayMS = 150, uint8_t palIndex = 0) : DisplayMatrix( leds, buff, w, h, delayMS, palIndex) {
    _px = random(w); _py = random(h); _vx = (random(30)-15)/10; _vy = (random(30)-15)/10; _first = 0; _last = 0; _hue = 0;
  }
  void init();
  boolean update();
  uint8_t nParticles() { if (_last < _first) return  MAX_STRING_BUFFER_SIZE + _last - _first; else return _last - _first; };
  boolean particlesFull() { if ( ( _last + 1 ) % MAX_STRING_BUFFER_SIZE == _first) return true; else return false; };
  void updatePos();
  void emit();

// Data
private:
  Particle  particles[MAX_PARTICLES];
  float     _px, _py, _vx, _vy;
  uint8_t   _first, _last;
  uint8_t   _hue;
};

////////////////////////////////////////////////////////////////////////////////////////////
//  Displays a moving SinWave
////////////////////////////////////////////////////////////////////////////////////////////
class SinWave : public DisplayMatrix {
public:
    SinWave(CRGB *leds, CRGB *buff, uint8_t w, uint8_t h, uint16_t delayMS = 30, uint8_t palIndex = 0): DisplayMatrix( leds, buff, w, h, delayMS, palIndex) {
      _currentLevel = 0;
      _currentColor = 0;
      _index = 0;
    }
    void init() {};
    boolean update();
    
// Data
private:
  uint8_t  _currentLevel, _currentColor, _index;
  
};

////////////////////////////////////////////////////////////////////////////////////////////
//  A constantly color-changing blob flies around the screen in different patterns
////////////////////////////////////////////////////////////////////////////////////////////
class TestPattern : public DisplayMatrix {
public:
    TestPattern(CRGB *leds, CRGB *buff, uint8_t w, uint8_t h, uint16_t delayMS = 30, uint8_t palIndex = 0): DisplayMatrix( leds, buff, w, h, delayMS, palIndex) {
      _isOn = true;
      _index = 0;
      _increment = 0;
    }
    void init() {};
    boolean update();
    
// Data
private:
  boolean _isOn;
  uint8_t _index;
  uint32_t _increment;
  
};

////////////////////////////////////////////////////////////////////////////////////////////
//  Star-like twinkles vary in intensity in random positions on the screen
////////////////////////////////////////////////////////////////////////////////////////////
class SoftTwinkle : public DisplayMatrix {
public:
    SoftTwinkle(CRGB *leds, CRGB *buff, uint8_t w, uint8_t h, uint16_t delayMS = 30, uint8_t palIndex = 0): DisplayMatrix( leds, buff, w, h, delayMS, palIndex) {
    }
    void init() {};
    boolean update();
    
// Data
private:

};

#endif
