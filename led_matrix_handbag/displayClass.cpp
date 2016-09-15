/////////////////////////////////////////////////////
//  Functions for DisplayMatrix and its child classes
/////////////////////////////////////////////////////

#include "DisplayClass.h"
#include "terminal_font_6_by_8.h"

//#include "WProgram.h" // Allows calls to Serial.print
#include "Arduino.h"

/////////////////////////////////////////////////////////
// Converts 2 dimensional position to LED index.  Copied
// from FastLED matrix example.  Only works for matrix
// with serpentine (back and forth) layout
////////////////////////////////////////////////////////
uint16_t DisplayMatrix::XY( uint8_t x, uint8_t y)
{
  uint16_t i;
  /* 
  if( y & 0x01) {                             // Upside down
    // Odd rows run backwards
    uint8_t reverseX = ( _width - 1) - x;
    i = (y *  _width) + reverseX;
  } else {
    // Even rows run forwards
    i = (y *  _width) + x;
  }
  */
  y = _height - 1 - y;
  if( y & 0x01) {                           // Right side up
   // Odd rows run forwards
    i = (y *  _width) + x;
  } else {
     // Even rows run backwards
    uint8_t reverseX = ( _width - 1) - x;
    i = (y *  _width) + reverseX;
    
  }
  
  return i;                 // If right side up; // If upside down
}

uint16_t DisplayMatrix::XYsafe( uint8_t x, uint8_t y)
{
  if( x >=  _width) return -1;
  if( y >=  _height) return -1;
  return XY(x,y);
}

////////////////////////////////////
// Shifts all rows down by one
////////////////////////////////////
void DisplayMatrix::shiftOneDown(CRGB *leds) {
  for (byte y = _height-1; y > 0; y--) {
    for (byte x = 0; x < _width; x++) {
      leds[ XY(x, y)] = leds[ XY(x, y-1)];
    }
  }
}
////////////////////////////////////
// Shifts all rows up by one
////////////////////////////////////
void DisplayMatrix::shiftOneUp(CRGB *leds) {
  for (byte y = 0; y < _height; y++) {
    for (byte x = 0; x < _width; x++) {
      leds[ XY(x,y)] = leds[ XY(x, y+1)];
    }
  }
}

/////////////////////////////////////
// Shifts all columns right by one
/////////////////////////////////////
void DisplayMatrix::shiftOneRight(CRGB *leds) {
  for (byte x = _width-1; x > 0; x--) {
    for (byte y = 0; y < _height; y++) {
      leds[ XY(x,y)] = leds[ XY(x-1,y)];  
    }
  } 
}

/////////////////////////////////////
// Shifts all columns left by one
/////////////////////////////////////
void DisplayMatrix::shiftOneLeft(CRGB *leds) {
  for (byte x = 0; x < _width-1; x++){
    for (byte y = 0; y < _height; y++) {
      leds[ XY(x,y)] = leds[ XY(x+1, y)];
    }
  }
}

///////////////////////////////////////////////////////////////////////
// Returns weighted interpolation of color that falls percent % 
//  between colors p1 and p2
///////////////////////////////////////////////////////////////////////
CRGB weightPixels(CRGB p1, CRGB p2, uint8_t percent, boolean cubic=false) {
  fract8 f = 255*percent/100;
  if (cubic) f = ease8InOutCubic(f);
  return CRGB(lerp8by8(p2.r, p1.r, f), lerp8by8(p2.g, p1.g, f), lerp8by8(p2.b, p1.b, f));
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// Shifts all rows down by a specified percentage.  Will split the light percentage wise between two 
// pixels.  Assumes the buffer holds the current  matrix and that the display leds get interpolated
// values.
////////////////////////////////////////////////////////////////////////////////////////////////////
void DisplayMatrix::shiftPercentDown(int percent, CRGB *nextRow) {
  
  // All rows but the first
  for (int y = 1; y < _height; y++) {
    for (int x = 0; x < _width; x++) {
      _leds[XY(x,y)] = weightPixels(_buffer[XY(x,y-1)], _buffer[XY(x,y)], percent, true);
    }
  }
  // Special case for first row
  for (int x = 0; x < _width; x++) {
     _leds[XY(x,0)] = weightPixels(nextRow[x], _buffer[XY(x,0)], percent, true);
  }  
}

void DisplayMatrix::shiftPercentLeft(int percent, CRGB *nextCol) {

  // All columns but last
  for (int x = 0; x < _width-1; x++) {
    for (int y = 0; y < _height; y++) {
      _leds[XY(x,y)] = weightPixels(_buffer[XY(x+1,y)], _buffer[XY(x,y)], percent, true);
    }
  }
  // Special case for last column
  for (int y = 0; y < _height; y++) {
    _leds[XY(_width-1, y)] = weightPixels(nextCol[y], _buffer[XY(_width-1,y)], percent, true);
  }
}

///////////////////////////////////////////////////////////////////////
// Helper function to copy led configuration between two arrays
///////////////////////////////////////////////////////////////////////
void DisplayMatrix::copyMatrix(CRGB *from, CRGB *to, uint8_t nleds) {
  for (int i = 0; i < nleds; i++) {
    to[i] = from[i];
  }
}

//////////////////////////////////////////////////
// Sets all pixels to off (black)
//////////////////////////////////////////////////
void DisplayMatrix::clearDisplay() {
  int nLeds = _width*_height;
  for (int i = 0; i < nLeds; i++) {
    _leds[i] = CRGB::Black;
  }
  FastLED.show();
}

/////////////////////////////////////////////////
// Returns true if time to update display.  In-
// crements _lastUpdateTime if so.
/////////////////////////////////////////////////
boolean DisplayMatrix::timeToUpdate() {
  long curTime = millis();
  if ((_lastUpdateTime < 0) || (curTime - _lastUpdateTime > _delayMS)) {
    _lastUpdateTime = curTime;
  } else {
    return false;
  } 
  return true;
}

/////////////////////////////////////////////////
// Initialze variables
/////////////////////////////////////////////////
void DrawText::init() {
  setDisplayText("");
  _lastUpdateTime = -1;
  _colPtr = 0;
}

//////////////////////////////////////////////////////////////////////////
// Update: Scroll text left, and fill in next column from the text buffer.
//////////////////////////////////////////////////////////////////////////
boolean DrawText::update() {

  if (!timeToUpdate()) return false;

  // Shift text one left
  shiftOneLeft(_buffer);

  // Fill in rightmost row from the display buffer
  int mask = 1 << _height;
  if (_colPtr < _colLen) {
    for (int y = 0; y < _height; y++) {
      _buffer[XY(_width-1,y)] = (_displayBuffer[_colPtr] & mask) ? _color : CRGB::Black;
      mask >>=1;
    }
    _colPtr++;
  } else if (_colPtr - _colLen >= _width) {  // Done scrolling - reset [TBD - GET NEXT STRING]
    _textInBuffer = false;
    if (!_stringBuffer.isEmpty()) {
      char      txt[MAX_STRING_LENGTH];
      uint8_t   colorIndex;
      _stringBuffer.popFirst(txt, &colorIndex);
      _color = ColorFromPalette( getPalette(), colorIndex, 64, LINEARBLEND);
      setDisplayText(txt);
      _colPtr = 0;
    } 
  } else {                                   // Text is finished, but keep scrolling till it is off the screen
    for (int y = 0; y < _height; y++) {
      _buffer[XY(_width-1,y)] = CRGB::Black;
    }
    _colPtr++;
  }
  
  copyMatrix(_buffer, _leds, _width*_height);
  FastLED.show();

  return true;
}

/* helper function to reverse bits of num */
uint16_t reverseBits(uint16_t num)
{
    unsigned int  NO_OF_BITS = sizeof(num) * 8;
    unsigned int reverse_num = 0, i, temp;
 
    for (i = 0; i < NO_OF_BITS; i++)
    {
        temp = (num & (1 << i));
        if(temp)
            reverse_num |= (1 << ((NO_OF_BITS - 1) - i));
    }
  
    return reverse_num;
}

//////////////////////////////////////////////////////
//  Writes text pixels to the text buffer
//////////////////////////////////////////////////////
void DrawText::setDisplayText(const char *txt) {
  
  uint8_t len =  strlen(txt);
  if (len < MAX_TEXT_CHARS) {
    // strcpy( _text, txt); 
    // _textLen = len;

    // Clear the display buffer
    for (int i = 0; i < MAX_TEXT_COLUMNS; i++) {
      _displayBuffer[i] = 0x00;
    }
    
    // Determine the number of columns it will take to scroll all text across and off the screen
    _colLen = 0;
    uint16_t ptr = 0;
    for (int i = 0; i < len; i++) {

      uint8_t charIndex = txt[i] - 32;
      uint8_t charLen = Terminal6x8[charIndex][0];
      
      for (int j = 1; j < charLen+1; j++) {
        _displayBuffer[ptr] = reverseBits(Terminal6x8[charIndex][j]) >> 7;  // Shift down to center the text - this may change depending on the height of the font
        ptr++;
      }
      _colLen += charLen;
      if (txt[i] != ' ') {   // leave a blank column after every character except a space
        ptr++;
        _colLen += 1;
      }
    }
    _textInBuffer = true;
  }
  // Reset the column pointer
  _colPtr = 0;
}


/////////////////////////////////////////////////////////////////////////
// Selects next color (in steps of 3) from the palette
////////////////////////////////////////////////////////////////////////
CRGB DisplayRain::nextColorFromPalette() {
  _colorIndex = (_colorIndex + 3) % 256;
  return ColorFromPalette( getPalette(), _colorIndex, _brightness, _blending);
}


//////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////
void DisplayRain::init() {
  _lastUpdateTime = -1;
  _paletteIndex = 0;
  _blending = LINEARBLEND;
  _counter = 0;
}


//////////////////////////////////////////////////////////////
// Update:  Moves raindrops down and creates new ones
//////////////////////////////////////////////////////////////
boolean DisplayRain::update() {
  static CRGB firstRow[16];

  if (!timeToUpdate()) return false;
 
  if (_counter == 0) {  // Load up a new first row and move the buffer pixels down by one

    shiftOneDown(_buffer);
    for (int x = 0; x < _width; x++) {
      _buffer[XY(x, 0)] = firstRow[x];
    }
    
    for (int x = 0; x < _width; x++) {   // Generate a new top row
      firstRow[x] = (random(10) < 1) ? nextColorFromPalette() : CRGB::Black; 
    }   
  }
  
  shiftPercentDown(_counter, firstRow);
  FastLED.show();

  _counter = (_counter + 5) % 100;
  return true;
}


////////////////////////////////////////////
// Init function - reset variables
////////////////////////////////////////////
void GameOfLife::init() {
  _paletteIndex = 0;
  _lastUpdateTime = -1;
}

/////////////////////////////////////////////////////////////////////////////
// Copies the pixels for the next generation from the buffer to the LEDs
////////////////////////////////////////////////////////////////////////////
void GameOfLife::setDisplayPixels(int ptr) {
  int nPixels = _width*_height;
  for (int i = 0; i < nPixels; i++) {
    _leds[i] = _buffer[i][ptr] ? ColorFromPalette( getPalette(), _buffer[i][ptr], _brightness, _blending) : CRGB::Black;
  }
}

//////////////////////////////////////////////////////////////////////////
//  Returns the number of living neighbors for the pixel at (x,y).  Ptr
//   determines whether we use the values in the red or green channel
//   for this particular generation.  Remember pixel (0,0) is top left.
//////////////////////////////////////////////////////////////////////////
int GameOfLife::countNeighbors(int ptr, int x, int y) {
  
  int left  = (x == 0) ? _width - 1 : x - 1;
  int right = (x == (_width - 1)) ? 0 : x + 1;
  int bottom   = (y == (_height - 1)) ? 0 : y + 1;
  int top = (y == 0) ? _height - 1 : y - 1;
  
  int topLeft      = _buffer[XY(left, top)][ptr] ? 1 : 0;
  int topMiddle    = _buffer[XY(x, top)][ptr] ? 1 : 0;
  int topRight     = _buffer[XY(right, top)][ptr] ? 1 : 0;
  int middleLeft   = _buffer[XY(left, y)][ptr] ? 1 : 0;
  int middleRight  = _buffer[XY(right, y)][ptr] ?  1: 0;
  int bottomLeft   = _buffer[XY(left, bottom)][ptr] ? 1 : 0;
  int bottomMiddle = _buffer[XY(x, bottom)][ptr] ? 1 : 0;
  int bottomRight  = _buffer[XY(right, bottom)][ptr] ? 1 : 0;

  return (topLeft + topMiddle + topRight + middleLeft + middleRight + bottomLeft + bottomMiddle + bottomRight);
}

/////////////////////////////////////////////////////////////////////////////////////
// Use the Red and Green channels (0 and 1 respectively) of the buffer to 
// store the current and next generation values of each pixel
/////////////////////////////////////////////////////////////////////////////////////
boolean GameOfLife::update() {

  if (!timeToUpdate()) return false;

  int from = _counter % 2;
  int to = from ? 0 : 1;

  if (_counter == 0) {                            // Re-initialize the matrix with randomly
    int nLeds = _width*_height;                   // placed pixels
    for (int i = 0; i < nLeds; i++) {
      _buffer[i][to] = (random(4) < 1) ? 1 : 0;
    }
  } else {
    boolean allDead = true;
    for (int x = 0; x < _width; x++) {
      for (int y = 0; y < _height; y++) {
        int index = XYsafe(x,y);
        int neighbors = countNeighbors(from,x,y);
        if (_buffer[index][from] == 0) {                // Cell currently dead
          if (neighbors == 3) _buffer[index][to] = 1;   // Comes to life with 3 neighbors
          else _buffer[index][to] = 0;
        } else {
          allDead = false;
          if (neighbors < 2) _buffer[index][to] = 0;    // Too few neighbors, dies of lonliness
          else if (neighbors <= 3) _buffer[index][to] = _buffer[index][from] + 1;
          else _buffer[index][to] = 0;                  // Too many neighbors, dies of overcrowding
        }
      }
    }
    if (allDead) {
      _counter = 0;
      return true;
    }
  }
  setDisplayPixels(to);
  FastLED.show();
  _counter = (_counter + 1)% 254;  // Don't want _counter to get to 255, because we access _counter+1
  return true;
}

///////////////////////////////////////////////////////////////
// Initialization: give each pixel an a position and velocity
///////////////////////////////////////////////////////////////
void BouncingPixels::init() {
  // Assign each pixel anm initial position and a velocity
  for (int i = 0; i < N_BOUNCING_PIXELS; i++) {
     _pos[i][0] = random(100*_width)/100.0;
     _pos[i][1] = random(100*_height)/100.0;
     _vel[i][0] = (random(400) + 200.0)/100.0;
     _vel[i][1] = (random(400) + 200.0)/100.0;
    // _col[i] = floor(i*255/N_BOUNCING_PIXELS);
    _col[i] = constrain(i*10, 0, 255);
  }
  _lastUpdateTime = -1;
}

///////////////////////////////////////////////////////////////
// Move each pixel and redraw display
///////////////////////////////////////////////////////////////
boolean BouncingPixels::update() {

  //Serial.print("in Update");
  if (!timeToUpdate()) return false;
  
  uint16_t dt = _delayMS;
  // Calculate current position for each pixel
  for (int i = 0; i < N_BOUNCING_PIXELS; i++) {
    float xpos = _pos[i][0] + _vel[i][0]*dt/1000;
    if (xpos < 0) {
      xpos = abs(xpos);
      _vel[i][0] = -1*_vel[i][0];
    }
    if (xpos >= _width) {
      xpos = 2*_width - xpos;
      _vel[i][0] = -1*_vel[i][0];
    }
    float ypos = _pos[i][1] + _vel[i][1]*dt/1000;
    if (ypos < 0) {
      ypos = abs(ypos);
      _vel[i][1] = -1*_vel[i][1];
    }
    if (ypos >= _height) {
      ypos = 2*_height - ypos;
      _vel[i][1] = -1*_vel[i][1];
    }
    _pos[i][0] = xpos;
    _pos[i][1] = ypos;    

/*
    Serial.print("xpos = ");
    Serial.print(xpos);
    Serial.print("ypos = ");
    Serial.println(ypos);
*/ 
  }

  // Draw pixels into buffer
  // First clear buffer
  fill_solid(_leds, _width*_height, CRGB::Black);

  // Calculate how far from the four surrounding pixels the "ball" is, and adjust brightness accordingly
  // If pixels intersect, last one overwrites the previous ones (currently) - should try to blend colors, probably
  for (int i = 0; i < N_BOUNCING_PIXELS; i++) {
    int x = floor(_pos[i][0]);
    int y = floor(_pos[i][1]);

    _leds[XY(x,y)] = ColorFromPalette(getPalette(), _col[i], 100, _blending);
    _col[i] = (_col[i] + 1) % 255; // Let colors evolve
    
  }
  FastLED.show();
  return true;
}

///////////////////////////////////////////////////////////////
// Initialization: set display to black
///////////////////////////////////////////////////////////////
void Twinkle::init() {
  // Reset all pixels in init
  _lastUpdateTime = -1;
  fill_solid(_leds, _width*_height, CRGB::Black);
  fill_solid(_buffer, _width*_height, CRGB::Black);
}

///////////////////////////////////////////////////////////////
// H, V values stored in _buffer, TBD - explain algorithm
///////////////////////////////////////////////////////////////
boolean Twinkle::update() {
 if (!timeToUpdate()) return false;
  
// Chance of pixel getting turned on = pct/(pixel lifetime)
  for (int x = 0; x < _width; x++) {
    for (int y = 0; y < _height; y++) {
      uint16_t index = XY(x,y);
      if (isLit(index)) {
        // Increment or decrement light here
        uint8_t brightval = _buffer[index][2];
        if (brightval == 255) {
          _buffer[index] = CRGB::Black;  // Buffer is done
          _leds[index] = CRGB::Black;
        }
        else {
          brightval = sin8(brightval/2);
          _leds[index] = CHSV(_buffer[index][0], brightval, brightval);
          _buffer[index][2]++;
        }
      } else if (random(_oddsFilled) == 1) {  // Create new lit pixel
        _buffer[index][0]= random(255); // Use Hue and Brigthness.  Set saturation = brightness for now.
        _buffer[index][2] = 0;
      }
    }
  }
  // Now copy buff to led matrix and show it
  
  FastLED.show();
  return true;
}

///////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////
void Worm::init() {
  _front = _length; 
  _dir = 1;
  _colorIndex = 0;
  _lastUpdateTime = -1;
}

///////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////
boolean Worm::update() {

  if (!timeToUpdate()) return false;
  
  uint8_t middle = _front - (_length +1)/2;
  for (int i = _front - _length; i < _front; i++) {
    uint8_t bright = (128 - abs(middle - i)*16) % 255;  // Make middle brightest
    _leds[i] = ColorFromPalette(getPalette(), (i*4) % 255, bright, _blending);
  }
  FastLED.show();

  // Now color them all black to be redrawn for next time
  for (int i = _front - _length; i < _front; i++) {
    _leds[i] = CRGB::Black;
  }

  // Move the worm in the correct direction.  Turn around at the ends
  _front += _dir;
  if (_front > _width*_height) {
    _front = _width*_height;
    _dir *= -1;
  } else if (_front < _length) {
    _front = _length;
    _dir *= -1;
  }
  
  return true;
}

///////////////////////////////////////////////////////////////
// Lines class initialization
///////////////////////////////////////////////////////////////
void Lines::init() {
   _rowColorIndex = 1;
   _colColorIndex = 1; 
   _currentRow = 0; 
   _currentCol = 0; 
   _rowIncrement = 1; 
   _colIncrement = 1;
}

///////////////////////////////////////////////////////////////
// Draws moving horz/vert lines
///////////////////////////////////////////////////////////////
boolean Lines::update() {
  if (!timeToUpdate()) return false;
  /*
  Serial.print("Current row = ");
  Serial.print(_currentRow);
  Serial.print(", Current col = ");
  Serial.println(_currentCol);
  */
  // Do row
  uint8_t prevRow = _currentRow;
  if ((_currentRow == _height - 1) || (_currentRow == 0)) {
    _rowIncrement *= -1;
    _rowColorIndex = (_rowColorIndex + 16) % 256;
  }
  _currentRow += _rowIncrement;
  for (int x = 0; x < _width; x++) {
    _leds[XY(x, prevRow)] = CRGB::Black;  // Erase last row
    _leds[XY(x, _currentRow)] = ColorFromPalette(getPalette(), _rowColorIndex, 128, _blending);
  }
  
  // Do col
  uint8_t prevCol = _currentCol;
  if ((_currentCol == _width - 1) || (_currentCol == 0)) {
    _colIncrement *= -1;
    _colColorIndex = (_colColorIndex + 16) % 256;
  }
  _currentCol += _colIncrement;
  for (int y = 0; y < _height; y++) {
    if (y == _currentRow) {
      _leds[XY(_currentCol, y)] = CRGB::Black;  // Pixel at intersection gets different color
    } else if (y != prevRow) {
      _leds[XY(prevCol, y)] = CRGB::Black;
      _leds[XY(_currentCol, y)] = ColorFromPalette(getPalette(), _colColorIndex, 128, _blending);
    } else {
     _leds[XY(_currentCol, y)] = ColorFromPalette(getPalette(), _colColorIndex, 128, _blending); 
    } 
 
  }
  FastLED.show();
  return true;
}

///////////////////////////////////////////////////////////////
// Override of pure virtual init function
///////////////////////////////////////////////////////////////
void ParticleEmitter::init(){
  
}

///////////////////////////////////////////////////////////////
// Update function
///////////////////////////////////////////////////////////////
boolean ParticleEmitter::update() {
  
  if (!timeToUpdate()) return false;
  // Update position - and all particle positions
  updatePos();

  Serial.print("First = ");
  Serial.print(_first);
  Serial.print(" Last = ");
  Serial.println(_last);
  for (int i = _first; i != _last; i = (i + 1) % MAX_PARTICLES) {
    if (!particles[i].updatePos()) _first = (_first + 1) % MAX_PARTICLES;  // Remove any particles whose lifetimes have expired
  }
  
  // Emit new particles
  for (int i = 0; i < 3; i++) {
    if (!particlesFull()) {
      particles[_last].setValues(_px, _py, _vx + (random(30)-15)/10, _vy + (random(30)-15)/10, _hue, 8);
      _last = (_last + 1) % MAX_PARTICLES;
    }
  }
  _hue = (_hue + 1) % 255;
  
  // First clear buffer
  fill_solid(_leds, _width*_height, CRGB::Black);
  // Draw all the particles
  uint8_t x,y;
  for (int i = _first; i < _last; i++) {
    particles[i].getMatrixPos(&x, &y);
    if (x >= 0 && x < _width && y >= 0 && y < _height) {
      Serial.print("Particle at ");
      Serial.print(x);
      Serial.print(" ");
      Serial.println(y);
      _leds[XY(x,y)] = ColorFromPalette(getPalette(), particles[i].getHue(), floor(255*particles[i].getLife()/10), _blending);
    }
  }
    
  FastLED.show();
  return true;
}

///////////////////////////////////////////////////////////////
//  updates particle emitter position
///////////////////////////////////////////////////////////////
void ParticleEmitter::updatePos() {
  _px += _vx;
  if (_px < 0 || _px >= _width-1) {  // Bounce if we hit a wall
    _vx *= -1;
    if (_px < 0) _px *= -1;
    else _px  = _width - 1 - (_px - _width + 1);
  }
  _py += _vy;
  if (_py < 0 || _py >= _height-1) {
    _vy *= -1;
    if (_py < 0) _py *= -1;
    else _py = _height - 1 - (_py - _height + 1);
  }
  /*
  Serial.print("Emitter pos = ");
  Serial.print(_px);
  Serial.print(" ");
  Serial.println(_py);
  */
}

/////////////////////////////////////////////////////////////
//  update function for random walk class
/////////////////////////////////////////////////////////////
boolean SinWave::update() {
  if (!timeToUpdate()) return false;

   // Shift text one right
  shiftOneRight(_leds);

  // Fill in new pixel position
  for (int i = 0; i < _height; i++) {
    _leds[XY(0, i)] = CRGB::Black;
  }
  
  _leds[XY(0, _currentLevel)] = ColorFromPalette(getPalette(), _currentColor, 128, _blending);
  Serial.print("currentLevel = ");
  Serial.println(_currentLevel);

  // Update color, level for next round
  _currentColor = (_currentColor + 1) % 255;
  _currentLevel = (uint8_t) round(sin8(_index)*_height/255);
  _index = (_index + 8) % 255;
  //_currentLevel = constrain(_currentLevel + random(3) - 1, 0, _height-1);
  FastLED.show();
  
  return true;
}

