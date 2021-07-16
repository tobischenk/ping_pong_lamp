// ----------------------------------------------------------------
// Import used Libraries
// LED Library
#include <FastLED.h>
// Watch Module Library
#include <DS3232RTC.h>
// Temperature Libraries
#include <OneWire.h>
#include <DallasTemperature.h>
// EEPROM write and read
#include <EEPROM.h>
// Timer to fire at specific points
#include <FireTimer.h>
// IR Remote
#include <IRremote.h>

#include <SerialTransfer.h>


// Self written Libraries
#include "CommandParsing.h"
#include "SimpleTimer.h"
#include "ModeProtocoll.h"
#include "IRCodes.h"
#include "TransferData.h"

// ----------------------------------------------------------------
// Pre Defs

// Basic Usage
#define NUM_LEDS      128     // Number of LEDS
#define LED_TYPE      WS2811  // Type of LED-Strip
#define COLOR_ORDER   GRB     // Order of Colors for RGB
#define SAFETY_DELAY  500     // Delay before Arduino actually starts

// Animations
#define COLOR_SHIFT_VALUE     1     // Hue change in each Animation
#define COLOR_DIFF_VALUE      30    // Color Jump in Forms
#define MILLIS_ANIMATION      90    // Time between two Loop Runs
#define MILLIS_TEMPERATURE    5000  // Time between to Measurements of Temperature

// Brightness
#define BRIGHTNESS_SHIFT      24    // Amount which changes Brightness
#define BRIGHTNESS_MIN        70    // Minimum Brightness Value

// IR Related
#define IR_SIGNAL_DELAY       100   // Delay in which LEDS indicate Signal received
#define CHANNEL_OVERRUN       false

#define BLINK_DELAY           100

// Inputs/Outputs
#define LED_DATA_PIN          52    // LED-Strip Connection
#define THERMOMETER_PIN       9     // Thermometer Connection
#define TEMPERATURE_PRECISION 11    // Precision of Thermometer Measurements
#define AUDIO_INPUT_PIN       A0    // Audio_Input Connection
#define RECV_PIN              7     // Pin of IR Receiver
#define IR_RECV_LED_FEEDBACK  true

// Serial Transfer
#define NUM_BYTES             136   // How many Bytes can be communicated at max  
#define PROCESS_BYTE          0     // This Byte determines the processing of the rest of the Data

// White Colors
#define RGB_WHITE             CRGB(255, 255, 255);  // RGB White Color
#define HSV_WHITE             CHSV(  1,   0, 255);  // HSV White Color

// EEPROM Lookups
#define EEPROM_MODE_LOCATION        0   // Number of Mode is stored in this byte
#define EEPROM_BRIGHTNESS_LOCATION  1   // Brightness is stored in this byte
#define EEPROM_MODE_DEFAULT         0   // Default Mode Value
#define EEPROM_BRIGHTNESS_DEFAULT   255 // Default Brightness Value

#define EEPROM_HUE_LOCATION_START   2   // First Byte which represents a Hue Value
#define EEPROM_HUE_LOCATION_END     130 // Last Byte which represents a Hue Value

#define EEPROM_SATURATION_LOCATION  131 // Saturation Storage Location
#define EEPROM_SATURATION_DEFAULT   255 // Default Saturation Value



#define BUFFER_SIZE 64

// ----------------------------------------------------------------
// Debug Flags

#define DEBUG_SETUP         true
#define DEBUG_LOOP          false
#define DEBUG_WATCH         false
#define DEBUG_DATE          false
#define DEBUG_TEMPERATURE   false
#define DEBUG_TURN_OFF      false
#define DEBUG_TIMER         false
#define DEBUG_ESP_SERIAL    true

// ----------------------------------------------------------------
// Initializations

// LED's
#ifdef NUM_LEDS
  CRGB leds[NUM_LEDS];
  CHSV workLeds[NUM_LEDS];
#endif

// Colors
#ifdef HSV_WHITE
  CHSV whiteScaled = HSV_WHITE;   // Brightness Scaled
#endif

CHSV lampColor(42, 200, 255);

// Watch
DS3232RTC myRTC(false);
tmElements_t t;

// SerialTransfer
SerialTransfer myTransfer;

TransferData recv_data;

// Thermometer
OneWire oneWire(THERMOMETER_PIN);
DallasTemperature sensors(&oneWire);
int currentTemperature;

// Running Variables
int mode;
int prevMode = -1;
int brightness;
int saturation = 255;
unsigned long prevCommand;
unsigned long resetCommand;

// Hue Colors Array
byte hueValues[NUM_LEDS];

// On-Off Variable
boolean turnedOn = true;

// Timers
FireTimer animationTimer;
FireTimer temperatureTimer;

// Timer Functionality
SimpleTimer simpleTimer;


// ----------------------------------------------------------------
// LED-Registers
const int digits [10][10] = {
  {   7,  18,   8,  22,  10,  24,  11,  14},            // 0
  {  18,  22,  16,  24,  14},                           // 1 
  {   7,  18,  22,  16,   9,  10,  11,  14},            // 2
  {   7,  18,  22,   9,  16,  24,  11,  14},            // 3
  {   7,   8,   9,  16,  22,  24,  14},                 // 4
  {  18,   7,   8,   9,  16,  24,  14,  11},            // 5
  {  18,  17,   9,  10,  11,  14,  24,  16},            // 6
  {   7,  18,  22,  16,  15,  11},                      // 7
  {   7,  18,   8,  22,   9,  16,  10,  24,  11,  14},  // 8
  {   7,  18,   8,  22,   9,  16,  15,  11},            // 9 
};

// ----------------------------------------------------------------

void setup() {
  // Safety Delay
  delay(SAFETY_DELAY);
  
  // Start FireTimer Instances
  animationTimer.begin(MILLIS_ANIMATION);
  temperatureTimer.begin(MILLIS_TEMPERATURE);

  // Init Serial Connection
  Serial.begin(9600);
  Serial3.begin(115200);
  myTransfer.begin(Serial3);

  // Init IRReceiver Object
  IrReceiver.begin(RECV_PIN, IR_RECV_LED_FEEDBACK);

  // Read Variables from EEPROM
  readMode();
  readBrightness();
  readSaturation();
  readHueValues();

  // Init FastLED Strip
  FastLED.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);

  // Start Clock Module
  myRTC.begin();

  // Init Thermometer
  sensors.begin();
  sensors.setResolution(TEMPERATURE_PRECISION);

  // Read Thermometer
  sensors.requestTemperatures();
  currentTemperature = floor(sensors.getTempCByIndex(0));
  temperatureTimer.update(MILLIS_TEMPERATURE);

  runInitFunction();

  // DevLogs
  if(DEBUG_SETUP) Serial.println(F("End of Setup"));
}

void loop() {
  if(DEBUG_LOOP) Serial.println("Loop");
  // Scale whiteColor to correct Brightness
  whiteScaled.value = brightness;
  lampColor.value = brightness;

  // Read IR Input
 readIr();

  // Read Serial Input from ESP
  processEspSerial();

  // Turn Off LED's
  if(!turnedOn) {
    for(int i = 0; i < NUM_LEDS; i++) {
      FastLED.clear();
    }

    FastLED.show();
  } 
  // LED's supposed to be on
  else {
    // New mode got selected
    if(mode != prevMode) {
      runInitFunction();
      prevMode = mode; 
    }
    // old mode still on
    else {
      if(animationTimer.fire()) {
        animationTimer.start();
        runLoopFunction();
      }
    }
  }

  if(DEBUG_TIMER) Serial.print(simpleTimer.getSeconds());

  if(DEBUG_LOOP) {
    Serial.print(F("Current Mode: "));
    Serial.println(mode);
  }
}

// ----------------------------------------------------------------put

void processEspSerial() {
  //clearTransferStruct();

  recv_data.clear();
  
  if(myTransfer.available()) {
    Serial.println("\nAvailable Transfer");
    
    myTransfer.rxObj(recv_data);

    String returnText = executeCommand();
    
    if (DEBUG_ESP_SERIAL) {
      Serial.print("\nProcessByte: "); Serial.println(recv_data.processByte);
      Serial.print("Mode: "); Serial.println(recv_data.mode);
      Serial.print("Saturation: "); Serial.println(recv_data.saturation);
      Serial.print("Color: "); Serial.println(recv_data.color);
      Serial.print("Brightness: "); Serial.println(recv_data.brightness);
      Serial.print("Hours: "); Serial.println(recv_data.hours);
      Serial.print("Minutes: "); Serial.println(recv_data.minutes);
      Serial.print("Seconds: "); Serial.println(recv_data.seconds);
    }
  }
}

String executeCommand() {
  String returnText = "";
  switch(recv_data.processByte) {
    case CP_ON_OFF:
      if(turnedOn == true)  {
          returnText = "Switched to Off";
          turnedOn = false;
        } else {
          returnText = "Switched to On";
          turnedOn = true;
        }
      break;
        
    case CP_MODE:
      mode = int(recv_data.mode);
      returnText = "Switching Mode to ";
      writeMode();
      break;

    case CP_BRIGHTNESS:
      brightness = int(recv_data.brightness);
      writeBrightness();
      break;

    case CP_SATURATION:
      saturation = int(recv_data.saturation);
      writeSaturation();
      break;

    case CP_COLOR:
      int hue = int(recv_data.color);
      initializeColor(CHSV(hue, saturation, brightness));
      break;
      
    case CP_TIMER:
      returnText = "Setting Timer";
      mode = MODE_TIMER;
      unsigned long seconds = 0;
      seconds += recv_data.seconds;
      seconds += recv_data.minutes * 60;
      seconds += recv_data.hours * 3600;
      simpleTimer.startSingle(t, seconds);
      writeMode();
      break;
  }

  if(DEBUG_ESP_SERIAL) Serial.println(returnText);

  return returnText;
  
}

void readIr() {
  // Block if Signal is incomplete
  while(!IrReceiver.isIdle());

  // Serial.print("read IR");
  if(IrReceiver.decode()) {
    decodeResults(IrReceiver.decodedIRData.decodedRawData);
    IrReceiver.resume();
  }
}

void decodeResults(unsigned long value) {
  boolean blink = true;

  switch(value) {
    case IR_PLAY_PAUSE:
      if(turnedOn == true)  {
        if(DEBUG_TURN_OFF) Serial.println(F("Switched to Off"));
        turnedOn = false;
      } else {
        if(DEBUG_TURN_OFF) Serial.println(F("Switched to On"));
        turnedOn = true;
      }
      break;
    
    case IR_VOL_DOWN:  //  VOL-
      changeBrightness(-BRIGHTNESS_SHIFT);
      break;
      
    case IR_VOL_UP:  // VOL+
      changeBrightness(+BRIGHTNESS_SHIFT);
      break; 

    case IR_CHANNEL_DOWN:  // CH-
      if(CHANNEL_OVERRUN) {
        mode--;
        if(mode < 0) mode = NUM_MODES - 1;
      } else {
        if(mode > 0) {
          mode--;
        }
      }
      break;
    
    case IR_CHANNEL_UP:  // CH+
      if(CHANNEL_OVERRUN) {
        mode++;
        if(mode >= NUM_MODES) mode = 0;
      } else {
        if(mode < NUM_MODES - 1){
          mode++;
        }
      }
      break;
      

      
    case IR_ONE:    // 1
      mode = 0;
      break;
    case IR_TWO:    // 2
      mode = 1;
      break;
    case IR_THREE:    // 3
      mode = 2;
      break;
    case IR_FOUR:    // 4
      mode = 3;
      break;

    case IR_REPEAT:  // Repeating Signal
      blink = false;
      break;

    default:  // Blink Red since Command not accepted
      blink=false;
      blinkRed();
      FastLED.show();
      delay(BLINK_DELAY);
  }

  if(blink) {
    blinkWhite();
    FastLED.show();
    delay(BLINK_DELAY);
  }
  
  writeMode();
  writeBrightness();
}

// ----------------------------------------------------------------

// Change Brightness of all LED's by value
void changeBrightness(int value) {
  brightness += value;

  // Boundaries of brightness
  brightness = max(BRIGHTNESS_MIN, brightness);
  brightness = min(254, brightness);
}


// ----------------------------------------------------------------
// Init Section
void runInitFunction() {
  // Read which mode should be displayed and act accordingly
  // Check if Screen is already fading so there is no break in the Fade Effect
  switch(mode) {
    case MODE_WATCH:
      if(isFading()) adaptScreen();
      else initializeCleanFade();
      break;
    case MODE_DATE:
      if(isFading()) adaptScreen();
      else initializeCleanFade();
      break;
    case MODE_TEMP:
      if(isFading()) {
        adaptScreen();
        initializeTemperature();
      } else {
        initializeTemperatureFull();
      }
      break;
    case MODE_TIMER:
      if(isFading()) adaptScreen();
      else initializeCleanFade();
      break;
    default:
      initializeColor(whiteScaled);
  }
}

// Check if modes both Fade
bool isFading() {
  if(prevMode == MODE_WATCH) return true;
  if(prevMode == MODE_DATE) return true;
  if(prevMode == MODE_TEMP) return true;
  if(prevMode == MODE_TIMER) return true;

  return false;
}

// changeBrightness and syncWorkLED's to new Mode
void adaptScreen() {
  setBrightness();
  syncWorkLeds();
}

// ----------------------------------------------------------------

// Write a Clean Fade into the LED's
void initializeCleanFade() {
  CHSV color(255, 255, 255);

  // Color Diff between two LED's
  int shiftValue = 256 / NUM_LEDS;

  for(int i = 0; i < NUM_LEDS; i++) {
    workLeds[i] = color;

    // Change Color by Shift Value
    color.hue += shiftValue;
  }

  setBrightness();
  syncWorkLeds();

  FastLED.show();
}

// Write given Color onto all LED's
void initializeColor(CHSV color) {

  for(int i = 0; i < NUM_LEDS; i++) {
    workLeds[i] = color;
  }

  setBrightness();
  syncWorkLeds();

  FastLED.show();
}

// Only set Temperature and write LED's
void initializeTemperature() {
  setBrightness();
  syncWorkLeds();

  whiteScaled.value = brightness;

  displayNumber(0, floor(sensors.getTempCByIndex(0)));
  displayCelsius();

  FastLED.show();
}

// Fully init Temperature Screen
void initializeTemperatureFull() {
  CHSV color(255, 255, 255);

  // Color Diff between two LED's
  int shiftValue = 256 / NUM_LEDS;

  for(int i = 0; i < NUM_LEDS; i++) {
    workLeds[i] = color;

    // Change Color by Shift Value
    color.hue += shiftValue;
  }

  // Run Rest from prev Function
  initializeTemperature();
}


// ----------------------------------------------------------------
// Loop Section
void runLoopFunction() {
  // Read which mode should be displayed and act accordingly
  switch(mode) {
    case MODE_WATCH:
      watchLoop();
      break;
    case MODE_DATE:
      dateLoop();
      break;
    case MODE_TEMP:
      temperatureLoop();
      break;
    case MODE_TIMER:
      timerLoop();
      break;
    default:
      fadeColors();
  }
}

// Simply fade the Color of each LED
void fadeColors() {
  for(int i = 0; i < NUM_LEDS; i++) {
    workLeds[i].hue = (workLeds[i].hue + COLOR_SHIFT_VALUE) % 256;
  }

  setBrightness();
  syncWorkLeds();

  FastLED.show();
}

// Show currentTime and animate dots in between Hour and Minute
void watchLoop() {
  myRTC.read(t);
  
  for(int i = 0; i < NUM_LEDS; i++) {
    workLeds[i].hue = (workLeds[i].hue + COLOR_SHIFT_VALUE) % 256;
  }

  setBrightness();
  syncWorkLeds();

  displayNumber(0, t.Hour);
  displayNumber(70, t.Minute);

  if(t.Second % 2 == 0) {
    leds[64] = whiteScaled;
    leds[66] = whiteScaled;
  }

  if(DEBUG_WATCH) Serial.println("Time: " + String(t.Hour) + ":" + String(t.Minute));

  FastLED.show();
}

// Show currentDate with a dot in between
void dateLoop() {
  myRTC.read(t);

  for(int i = 0; i < NUM_LEDS; i++) {
    workLeds[i].hue = (workLeds[i].hue + COLOR_SHIFT_VALUE) % 256;
  }

  setBrightness();
  syncWorkLeds();

  // Displays Day and Month
  displayNumber(0, t.Day);
  displayNumber(70, t.Month);

  // Point between Day and Month
  leds[66] = whiteScaled;
  leds[56] = whiteScaled;
  leds[67] = whiteScaled;

  if(DEBUG_DATE) Serial.println("Date: " + String(t.Month) + "." + String(t.Day));

  FastLED.show();
}

// Shows the measured Temp
void temperatureLoop() {
  if(temperatureTimer.fire()) {
    temperatureTimer.start();

    if(DEBUG_TEMPERATURE) Serial.println(F("Measuring..."));

    // Triangles to indicate Measuring
    leds[0] = whiteScaled;
    leds[2] = whiteScaled;
    leds[4] = whiteScaled;
    leds[125] = whiteScaled;
    leds[126] = whiteScaled;
    leds[127] = whiteScaled;
    // Show triangles
    FastLED.show();

    sensors.requestTemperatures();
    currentTemperature = floor(sensors.getTempCByIndex(0));

    if(DEBUG_TEMPERATURE) Serial.println(F("Done!"));
  }

  for(int i = 0; i < NUM_LEDS; i++) {
    workLeds[i].hue = (workLeds[i].hue + COLOR_SHIFT_VALUE) % 256;
  }

  setBrightness();
  syncWorkLeds();

  displayNumber(0, currentTemperature);
  displayCelsius();

  FastLED.show();
}

// Show current Timer time
void timerLoop() {
  for(int i = 0; i < NUM_LEDS; i++) {
    workLeds[i].hue = (workLeds[i].hue + COLOR_SHIFT_VALUE) % 256;
  }
  
  setBrightness();
  syncWorkLeds();

  myRTC.read(t);
  simpleTimer.update(t);

  displayNumber(0, simpleTimer.getMinutes());
  displayNumber(70, simpleTimer.getSeconds());

  leds[64] = whiteScaled;
  leds[66] = whiteScaled;

  // TODO: implement this correctly, so that a Dot gets used for each hour
  if(simpleTimer.getHours() == 1) {
    leds[6] = whiteScaled;
  }

  FastLED.show();
}

// Write Celsius sign into LED's
void displayCelsius() {
  // Write Degree Sign to leds
  leds[63] = whiteScaled;
  leds[74] = whiteScaled;
  leds[64] = whiteScaled;
  leds[78] = whiteScaled;
  leds[65] = whiteScaled;
  leds[72] = whiteScaled;

  // Write C to leds
  leds[105] = whiteScaled;
  leds[102] = whiteScaled;
  leds[ 91] = whiteScaled;
  leds[ 92] = whiteScaled;
  leds[ 86] = whiteScaled;
  leds[ 94] = whiteScaled;
  leds[ 95] = whiteScaled;
  leds[ 98] = whiteScaled;
  leds[109] = whiteScaled;
}


// Displays a Number
void displayNumber(int place, int number) {
  for(int i = 0; i < 10; i++) {
    if(digits[number / 10][i] != 0) {
      leds[(digits[number / 10][i] + place)] = whiteScaled;
    }

    if(digits[number % 10][i] != 0) {
      leds[(digits[number % 10][i] + 28 + place)] = whiteScaled;
    }
  }
}

// ----------------------------------------------------------------
// EEPROM Methods

// Mode
void readMode() { mode = EEPROM.read(EEPROM_MODE_LOCATION);} 
void writeMode() { EEPROM.write(EEPROM_MODE_LOCATION, mode); }

// Brightness
void readBrightness() { brightness = EEPROM.read(EEPROM_BRIGHTNESS_LOCATION); }
void writeBrightness() { EEPROM.write(EEPROM_BRIGHTNESS_LOCATION, brightness); }

void readSaturation() { saturation = EEPROM.read(EEPROM_SATURATION_LOCATION); }
void writeSaturation() { EEPROM.write(EEPROM_SATURATION_LOCATION, saturation); }

// Hue Values
void readHueValues() {
  int storageOffset = EEPROM_HUE_LOCATION_START;

  for(int i = 0; i < NUM_LEDS; i++) {
    hueValues[i] = EEPROM.read(i + storageOffset);
  }
}
void writeHueValues() {
  int storageOffset = EEPROM_HUE_LOCATION_START;

  for(int i = 0; i < NUM_LEDS; i++) {
    EEPROM.write(i + storageOffset, hueValues[i]);
  }
}

// ----------------------------------------------------------------
// Helper Functions

// Write White Values to Edge on LED's
void blinkWhite() {
  leds[0] = whiteScaled;
  leds[2] = whiteScaled;
  leds[4] = whiteScaled;
  leds[125] = whiteScaled;
  leds[126] = whiteScaled;
  leds[127] = whiteScaled;
}

// Write Red Values to Edge on LED's
void blinkRed() {
  leds[0] = CHSV(1, 255, brightness);
  leds[2] = CHSV(1, 255, brightness);
  leds[4] = CHSV(1, 255, brightness);
  leds[125] = CHSV(1, 255, brightness);
  leds[126] = CHSV(1, 255, brightness);
  leds[127] = CHSV(1, 255, brightness);
}

// Calculate seconds of given Time
unsigned long getTime(int hours, int minutes, int seconds) {
  unsigned long value = 0;
  value += (hours * SECS_PER_HOUR);
  value += (minutes * 60);
  value += seconds;

  return value;
}

// Set Brightness on all WorkLED's
void setBrightness() {
  for(int i = 0; i < NUM_LEDS; i++) {
    workLeds[i].value = brightness;
  }
}

// Write Values from WorkLED's into LED's
void syncWorkLeds() {
  hsv2rgb_rainbow(workLeds, leds, NUM_LEDS);
}
