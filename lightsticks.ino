#include "setup.h"
#include <FastLED.h>

enum lightMode {
  mode_random,
  mode_white,
  mode_primary,
  mode_fire,
  mode_black
};

lightMode currentMode = mode_white;
int globalCurrentPattern = -1;
boolean solidMode = false;

// Time the 'tap the beat' button was last pressed
unsigned long timeLastMarkTimer = 0;
// Time the last 16 bar cycle started
unsigned long timeLastBarStarted = 0;
// Time the last beat ticked, measured in 10x milliseconds, for more accuracy
unsigned long timeLastBeat10X = 0;
// Position of the beat in the 16 bar cycle - 0 to 63;
int beatNumber = 0;
// The position of the beat at the time of last click - reset to this when manually timing
int beatNumberLastClick = 0;
// Gap between beats - measure of tempo. measured in 10x milliseconds, for more accuracy
int beatGap10X = 8000; //-1;

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

boolean x1, x2;

byte currentHue;
byte currentPattern;

// #include "LPD8806.h"
#include "SPI.h"
#include "patterns.h"

// LPD8806 strip = LPD8806(BEAM_HEIGHT * BEAM_COUNT);

BeamAtATime rule = BeamAtATime();

unsigned long tick, lastTick, oldTick;

void setup() {
  // For recieving midi signals
  Serial.begin(57600);
  Serial.println("start");

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(200);

  lastTick = millis();
  timeLastMarkTimer = millis();
}

// Rows 2 to 7 set the Hue
void setHue(int y){
  currentHue = constrain((y - 2) * (255 / 6), 0, 255);
}

// Patterns are columns 0 to 7. Column 0 is set to -1 to indicate use a random pattern.

void setPattern(int x){
  if(x == 0){
    x = -1;
  }
  
  rule.randomisePattern();
  
  globalCurrentPattern = constrain(x, -1, 8);
}

// Row 0 is 'all random', or special functions (fire)
// Row 1 is different patterns on color wihte
// Row 2 to 7 is different patterns on specified hue
void setMode(int x, int y){
  Serial.println("setMode");
  Serial.println(x);
  Serial.println(y);

  if(y==0){
    if(x==0){
      currentMode = mode_black;
      setPattern(0);
    }else if(x>2){
      currentMode = mode_fire;
      setPattern(x);
    }else{
      currentMode = mode_random;
      setPattern(0);
    }
  }else if (y==1){
    currentMode = mode_white;
    setPattern(x);
  }else{
    currentMode = mode_primary;
    setHue(y);
    setPattern(x);
  }
  
  solidMode = false;
  
  if(x==1){
    solidMode = true;
  }
}

void loop() {
  random16_add_entropy( random());

  oldTick = tick;
  tick = millis();

  if (stringComplete) {
    Serial.println(inputString);
    
    if(inputString == "B"){
      markTimer(tick);
    }else{
      char x = inputString[0];
      char y = inputString[1];
      setMode(x - '0', y - '0');
    }
    
    inputString = "";
    stringComplete = false;
  }

  // 20 frames / sec
  if((tick - lastTick) >= FRAME_LENGTH) {
//    for(i=0;i<NUM_LEDS;i++){
//      leds[i] = (beatNumber % 2) ? CRGB::Red : CRGB::Black; // : CRGB::Black;
//    }
  
    handleTick(tick);
    lastTick = tick;
  }
  
  // Every millisecond
  if(oldTick != tick) {
    FastLED.show();
  }
}

/**
 * What happens on a single tick - dispatch to all subsystems
  */

void handleTick(unsigned long tick) {
//  Serial.print('x');
  // Beat tracking
  checkBeat(tick);
  
  rule.onTick(tick, 10000 * (tick - timeLastBeat10X/10) / beatGap10X);
}

/////////////////////////////////////////////////////////////////
// BUTTON MANAGEMENT

// Previous value of each button
int buttonA = 0;
int buttonB =  0;
// Time when the button last changed value - used to detect flickering
unsigned long timeButtonALastChange = 0;
unsigned long timeButtonBLastChange = 0;

/////////////////////////////////////////////////////////////////
// TAP-THE-BEAT MANAGEMENT

/**
 * Handle a tap for the "tap the tempo" code
 */
void markTimer(unsigned long time) {
  //Serial.println(time);
    int timeGap = time - timeLastMarkTimer;
  
  timeLastMarkTimer = time;
  timeLastBeat10X = time * 10;
  
  // Re-start timing
  if(!timeLastMarkTimer || (timeGap > 1000)) {
    //Serial.println('S');

    timeLastBarStarted = time;
    
    beatNumber = -1; // So that doBeat will increment to zero
    
  // Continue timing
  } else {
    // Recalcuate timeGap over entire cycle - more accurate
    beatNumber = beatNumberLastClick;
    beatGap10X = 10*(time-timeLastBarStarted) / (beatNumber+1);
  }

  doBeat();
  beatNumberLastClick = beatNumber;
}

/**
 * Check if it's time to advance the beat
 */
void checkBeat(unsigned long tick) {
  if(beatGap10X != -1 && tick*10 - timeLastBeat10X >= beatGap10X) {
    timeLastBeat10X += beatGap10X;
    doBeat();
  }
}

/**
 * Trigger a beat, incrementing the beat counter and showing a light
  */
void doBeat() {
  beatNumber = (beatNumber+1) % 64;
  
  // Start of 16 bar cycle
  if(beatNumber == 0) {
    setStatusColour(BLUE, 50);
    timeLastBarStarted = millis();

    // currentHue = CRGB::Blue;
    
  // Start of bar
  } else if(beatNumber % 4 == 0) {
    setStatusColour(GOLD, 50);

    // currentHue = CRGB::Yellow;

  // Regular beat
  } else {
    // currentHue = MODE_HEAVY ? CRGB::White : CRGB::Green;
  }
  
  // Trigger event to currently loaded rule
  rule.onBeat(beatNumber);
}
 
/////////////////////////////////////////////////////////////////
// STATUS LIGHT MANAGEMENT

// Lifetime in ms of current status colour

int statusLife = 0;

/**
 * Tick method for the status LED
 */
void handleStatusTick() {
  if(statusLife > 0) {
    statusLife--;
    if(statusLife == 0) setStatusColour(0);
  }
}

/**
 * Set the status colour for a specific number of ticks
 */
void setStatusColour(int colour, int time) {
  setStatusColour(colour);
  statusLife = time / FRAME_LENGTH;
}

/**
 * Change the colour of the Status LED permanently
 */
void setStatusColour(int colour) {
  digitalWrite(STATUS_R, (colour & 1) ? LOW : HIGH);
  digitalWrite(STATUS_G, (colour & 2) ? LOW : HIGH);
  digitalWrite(STATUS_B, (colour & 4) ? LOW : HIGH);
}

  

/////////////////////////////////////////////////////////////////
// LED STRIP SUPPORT

void clearStrip() {
  int i;
   for(i=0; i<NUM_LEDS; i++) {
     leds[i] = CRGB::Black;
   }
}

void allOn(byte r, byte g, byte b) {
  int i;
  // int col = strip.Color(r, g, b);
   for(i=0; i<NUM_LEDS; i++) {
     leds[i] = CRGB::White;
   }
}

/* Serial input */

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    // add it to the inputString:
    if(inChar != '\n'){
      inputString += inChar;
    }
    
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    } 
  }
}

