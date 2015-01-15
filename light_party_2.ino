// How many lights on each stick
const int BEAM_HEIGHT = 32;
// How many light sticks
const int BEAM_COUNT = 4;
// Frame length in ms
const int FRAME_LENGTH = 20;

// Pin identifiers
const int BUTTON_A = 6;
const int BUTTON_B = 5;

const int STATUS_R = 2;
const int STATUS_G = 3;
const int STATUS_B = 4;

// Colours
const int BLACK = 0;
const int RED = 1;
const int GREEN = 2;
const int GOLD = 3;
const int BLUE = 4;
const int PINK = 5;
const int CYAN = 6;
const int WHITE = 7;

// Operating modes
const byte MODE_LIGHT = 0;
const byte MODE_HEAVY = 1;

byte currentMode = 0;

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

#include "LPD8806.h"
#include "SPI.h"
#include "patterns.h"

LPD8806 strip = LPD8806(BEAM_HEIGHT * BEAM_COUNT);

BeamAtATime rule = BeamAtATime(strip);

void setup() {
  // Set up controller UI elements
  pinMode(BUTTON_A, INPUT);
  pinMode(BUTTON_B, INPUT);
  
  pinMode(STATUS_R, OUTPUT);
  pinMode(STATUS_G, OUTPUT);
  pinMode(STATUS_B, OUTPUT);
  
  digitalWrite(STATUS_R, HIGH);
  digitalWrite(STATUS_G, HIGH);
  digitalWrite(STATUS_B, HIGH);

  // Set up light strip
  strip.begin();
  strip.show();

  // For debugging
  Serial.begin(9600);
}

/**
 * Main loop - doesn't do much except detect ticks
 */
void loop() {
  unsigned long tick, lastTick = millis();
  
  while(true) {
    tick = millis();
    checkButtons(tick);

    // 20 frames / sec
    if((tick - lastTick) >= FRAME_LENGTH) {
      handleTick(tick);
      
      lastTick = tick;
    }
  }
}

/**
 * What happens on a single tick - dispatch to all subsystems
  */
void handleTick(unsigned long tick) {
  // Status lights
  handleStatusTick();

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

void checkButtons(unsigned long tick) {
  int newButtonA = digitalRead(BUTTON_A);
  int newButtonB = digitalRead(BUTTON_B);

  // Click button A  
  if(buttonA != newButtonA) {
    // Avoid "flickering" due to button hardware imperfections
    if((tick - timeButtonALastChange) > 50) {    
      if(newButtonA) {
//        Serial.println('A');
        markTimer(tick);
      }
    }
    timeButtonALastChange = tick;
  }

  // Click button B
  if(buttonB != newButtonB) {
    // Avoid "flickering" due to button hardware imperfections
    if((tick - timeButtonBLastChange) > 50) {    
      if(newButtonB) {
//        Serial.println('B');
          toggleMode();
      }
    }
    timeButtonBLastChange = tick;
  }

  buttonA = newButtonA;
  buttonB = newButtonB;
}

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

    //Serial.print(beatNumber);
    //Serial.print(',');
    //Serial.print(beatGap10X);
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
    
  // Start of bar
  } else if(beatNumber % 4 == 0) {
    setStatusColour(GOLD, 50);

  // Regular beat
  } else {
    setStatusColour(currentMode == MODE_HEAVY ? RED : GREEN, 50);
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

void toggleMode() {
  currentMode = 1 - currentMode;
}
  

/////////////////////////////////////////////////////////////////
// LED STRIP SUPPORT

void clearStrip() {
  int i;
  for(i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, 0); // Erase pixel, but don't refresh!
  }
}

void allOn(byte r, byte g, byte b) {
  int i;
  int col = strip.Color(r, g, b);
  for(i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, col); // Erase pixel, but don't refresh!
  }
}
