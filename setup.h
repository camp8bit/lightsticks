#include <FastLED.h>

// How many lights on each stick
const int BEAM_HEIGHT = 47;
// How many light sticks
const int BEAM_COUNT = 2;

// How many leds in your strip?
#define NUM_LEDS BEAM_HEIGHT * BEAM_COUNT

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
#define DATA_PIN 13

// Define the array of leds
CRGB leds[NUM_LEDS];

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
