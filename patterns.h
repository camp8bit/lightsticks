/////////////////////////////////////////////////////////////////
// LED STRIP PATTERNS

const byte NUM_PATTERNS = 8;

const byte PATTERN_ON = 0;
const byte PATTERN_WIPE_UP = 1;
const byte PATTERN_WIPE_DOWN = 2;
const byte PATTERN_WIPE_OUT = 3;
const byte PATTERN_WIPE_IN = 4;
const byte PATTERN_STROBE = 5;
const byte PATTERN_PULSE_UP = 6;
const byte PATTERN_PULSE_DOWN = 7;

const byte NUM_BEAT_PATTERNS = 6;
const byte BEAT_PATTERN_SIZE = 8;

// Patterns of beams to turn on.  It's a bitmask:
// Bits 1-4 (least-significant) each reference a beam
// Bits 5-6 reference a pattern number (0,1,2,3) so you can have up to 4 different colour patterns in a sequence

byte beatPatterns[NUM_BEAT_PATTERNS][BEAT_PATTERN_SIZE] = {
  {B000001, B000010, B000100, B001000, B000001, B000010, B000100, B001000},
  {B000001, B011010, B000010, B010101, B000100, B011010, B001000, B010101},
  {B000001, B000011, B000111, B001111, B011000, B011100, B011110, B011111},
  {B000011, B001100, B000011, B001100, B010110, B011001, B010110, B011001},
  {B001111, B001111, B011111, B011111, B101111, B101111, B111111, B111111},
  {B000001, B010010, B100100, B111000, B000001, B010010, B100100, B111000}
};

/**
 * Pick a random "primary" color - one of the 8 key ones.
 */
CRGB randomPrimary() {
  if(currentMode == mode_black){
    return CRGB::Black;
  }else if(currentMode == mode_random){
    return CHSV(random(0,255), 255, 255);
  }else if (currentMode == mode_white){
    return CRGB::White;
  }else{
    return CHSV(currentHue, 255, 255);
  }
}

/**
 * Light up each beam in a colour, one at a time. Optionally strobe
 */
class BeamAtATime {
private:
  byte beatPattern;
  byte currentPattern;
  CRGB col[4];
  byte pattern[4];
  
  byte currentBeam;
  byte displayed;
  
  void fillCurrentBeams(int start, int finish, CRGB col);
  void fillBeam(int beam, int start, int finish, CRGB col);
  
public:  
  BeamAtATime();
  void randomisePattern();
  void onBeat(int beat);
  void onTick(unsigned long tick, int beatPermil);
};

BeamAtATime::BeamAtATime() {
  this->currentBeam = -1;
  this->displayed = true;
  
  this->randomisePattern();
}

void BeamAtATime::onBeat(int beat) {
  if(beat % BEAT_PATTERN_SIZE == 0) this->randomisePattern();

  this->currentBeam = beatPatterns[this->beatPattern][beat % BEAT_PATTERN_SIZE] & B1111;
  this->currentPattern = (beatPatterns[this->beatPattern][beat % BEAT_PATTERN_SIZE] & B110000) >> 4;
  this->onTick(0,0);
}

void BeamAtATime::randomisePattern() {
  this->beatPattern = random(NUM_BEAT_PATTERNS);

  int i;
  for(i=0;i<BEAM_COUNT;i++) {
    this->pattern[i] = random(NUM_PATTERNS);
    this->col[i] = randomPrimary();
  }
}

// For fire simulation
#define COOLING 55
#define SPARKING 120

CRGBPalette16 currentPalette;

byte heat[BEAM_HEIGHT];

void Fire2012()
{
// Array of temperature readings at each simulation cell

  currentPalette = HeatColors_p;
  
  if(globalCurrentPattern == 6){
    currentPalette = RainbowColors_p;
  }
  
  if(globalCurrentPattern == 5){
    currentPalette = OceanColors_p;
  }
  
  if(globalCurrentPattern == 4){
    currentPalette = ForestColors_p;
  }
  
  if(globalCurrentPattern == 3){
    currentPalette = PartyColors_p;
  }
  
  // Step 1.  Cool down every cell a little
    for( int i = 0; i < BEAM_HEIGHT; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / BEAM_HEIGHT) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= BEAM_HEIGHT - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for(int i=0; i < BEAM_COUNT; i++){
      for( int j = 0; j < BEAM_HEIGHT; j++) {
//        leds[j + i * BEAM_HEIGHT] = HeatColor(heat[j]);
        if(i%2==0){
          leds[(BEAM_HEIGHT-j) + i * BEAM_HEIGHT] = ColorFromPalette(currentPalette, heat[j]);
        }else{
          leds[j + i * BEAM_HEIGHT] = ColorFromPalette(currentPalette, heat[j]);
        }
        
//        if(globalCurrentPattern==6){
//        }else if(globalCurrentPattern==7){
//        }
      }
    }
}

void BeamAtATime::onTick(unsigned long tick, int beatPermil) {
  FastLED.clear();
  
  // Simple curve for slowed growth
  beatPermil = 1000-beatPermil;
  beatPermil = (long)beatPermil * beatPermil  * beatPermil / 1000000;
  beatPermil = 1000-beatPermil;

  // Quick hack because the simple curve tends to stall on the last item
  int length = (int)((long)(BEAM_HEIGHT+1) * beatPermil / 1000);
  if(length > BEAM_HEIGHT) length = BEAM_HEIGHT;

  if(currentMode == mode_solid){
    FastLED.setBrightness(100);
    for(int z=0;z<BEAM_COUNT;z++){
      this->fillBeam(z, 0, BEAM_HEIGHT, randomPrimary());
    }
    return;
  }

  FastLED.setBrightness(200);

  if(currentMode == mode_fire){
    Fire2012();
    return;
  }
  
  byte pattern = this->pattern[this->currentPattern];
  
  if(globalCurrentPattern > 0){
    pattern = globalCurrentPattern;
  }

  switch(pattern) {
  case PATTERN_ON:
    this->fillCurrentBeams(0, BEAM_HEIGHT, this->col[this->currentPattern]);
    break;
    
  case PATTERN_WIPE_UP:
    this->fillCurrentBeams(BEAM_HEIGHT-length, BEAM_HEIGHT, this->col[this->currentPattern]);
    break;
    
  case PATTERN_WIPE_DOWN:
    this->fillCurrentBeams(0, length, this->col[this->currentPattern]);
    break;
  
  case PATTERN_WIPE_IN:
    length = length/2;
    this->fillCurrentBeams(0, length, this->col[this->currentPattern]);
    this->fillCurrentBeams(BEAM_HEIGHT-length, BEAM_HEIGHT, this->col[this->currentPattern]);
    break;
  
  case PATTERN_WIPE_OUT:
    length = (BEAM_HEIGHT-length)/2;
    this->fillCurrentBeams(length, BEAM_HEIGHT-length, this->col[this->currentPattern]);
    break;

  case PATTERN_PULSE_UP:
    length = length * 3 / 4;
    this->fillCurrentBeams(length, length + BEAM_HEIGHT/4, this->col[this->currentPattern]);
    break;

  case PATTERN_PULSE_DOWN:
    length = (BEAM_HEIGHT-length) * 3 / 4;
    this->fillCurrentBeams(length, length + BEAM_HEIGHT/4, this->col[this->currentPattern]);
    break;

  
  case PATTERN_STROBE:
  
  
    this->displayed--;
    if(this->displayed == 0){
      if(currentMode == mode_white){
         for(int j=0;j<BEAM_COUNT;j++){
           this->fillBeam(j,0,BEAM_HEIGHT, CRGB::White);
         } 
      }else{
        this->fillCurrentBeams(0, BEAM_HEIGHT, this->col[this->currentPattern]);
      }
    }
    if(!this->displayed) this->displayed = 5;
    
    break;
  }
  
  
  // this->strip.show();
}

/**
 * Fill a region of colour on the current beam
 */
void BeamAtATime::fillCurrentBeams(int start, int finish, CRGB col) {
  byte bitMask = 1;
  int beam = 0;
  while(beam < BEAM_COUNT) {
    if(this->currentBeam & bitMask) this->fillBeam(beam, start, finish, col);
    beam++;
    bitMask *= 2;
  }
}
void BeamAtATime::fillBeam(int currentBeam, int start, int finish, CRGB col) {
  int i;
  
  currentBeam = constrain(currentBeam, 0, BEAM_COUNT);
  start = constrain(start, 0, BEAM_HEIGHT);
  finish = constrain(finish, 0, BEAM_HEIGHT);
  
  for(i = start; i < finish; i++) {
    if(currentBeam % 2 == 0){
      leds[i + currentBeam * BEAM_HEIGHT] = (currentMode == mode_white) ? CRGB::White : col;
    }else{
      leds[(BEAM_HEIGHT - i) + currentBeam * BEAM_HEIGHT] = (currentMode == mode_white) ? CRGB::White : col;
    }
  }
}

