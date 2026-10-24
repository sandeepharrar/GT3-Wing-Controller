#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>

// ====================================================
// BLUETOOTH LE CONFIGURATION
// ====================================================
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_MODE_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_BRIGHTNESS_UUID "beb5483f-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_RUNNING_UUID    "beb54840-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_COLOR_UUID      "beb54841-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_SPEED_UUID      "beb54842-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_OVERRIDE_UUID   "beb54843-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_ANIMATION_UUID  "beb54844-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_SAVE_DEFAULT_UUID "beb54845-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_MANUAL_BRAKE_UUID "beb54846-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_MANUAL_LEFT_UUID  "beb54847-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_MANUAL_RIGHT_UUID "beb54848-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_MANUAL_REVERSE_UUID "beb54849-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_MANUAL_HAZARDS_UUID "beb5484a-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_EFFECT_BRIGHTNESS_UUID "beb5484b-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_POLICE_SEGMENTS_UUID "beb5484c-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_CHASE_WIDTH_UUID "beb5484d-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_SAVED_VALUES_UUID "beb5484e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;
BLECharacteristic* pCharMode = NULL;
BLECharacteristic* pCharBrightness = NULL;
BLECharacteristic* pCharRunning = NULL;
BLECharacteristic* pCharColor = NULL;
BLECharacteristic* pCharSpeed = NULL;
BLECharacteristic* pCharOverride = NULL;
BLECharacteristic* pCharAnimation = NULL;
BLECharacteristic* pCharSaveDefault = NULL;
BLECharacteristic* pCharSavedValues = NULL;
BLECharacteristic* pCharManualBrake = NULL;
BLECharacteristic* pCharManualLeft = NULL;
BLECharacteristic* pCharManualRight = NULL;
BLECharacteristic* pCharManualReverse = NULL;
BLECharacteristic* pCharManualHazards = NULL;
BLECharacteristic* pCharEffectBrightness = NULL;
BLECharacteristic* pCharPoliceSegments = NULL;
BLECharacteristic* pCharChaseWidth = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// Preferences object for EEPROM-like storage
Preferences preferences;

// ====================================================
// PIN DEFINITIONS - Seeed XIAO ESP32-C6
// ====================================================
// Use the silkscreen D-labels; the XIAO_ESP32C6 board package maps them.
// Do NOT use GPIO3 (RF switch power) or GPIO14 (antenna select).
#define DATA_PIN    D8   // GPIO19
#define REVERSE_PIN D9   // GPIO20
#define BRAKE_PIN   D10  // GPIO18
#define LEFT_PIN    D0   // GPIO0
#define RIGHT_PIN   D1   // GPIO1

// Antenna: 0 = onboard ceramic, 1 = external U.FL antenna
#define USE_EXTERNAL_ANTENNA 0
#define RF_SWITCH_EN_PIN  3
#define RF_ANT_SELECT_PIN 14
#define SERIAL_BAUD 115200

// ====================================================
// LED STRIP CONFIGURATION
// ====================================================
#define DIODES_PER_PIXEL 1
#define PHYSICAL_DIODES  90
const uint16_t PIXEL_COUNT = PHYSICAL_DIODES / DIODES_PER_PIXEL;

#define COLOR_ORDER NEO_GRB
#define WS281X_SPEED NEO_KHZ800

Adafruit_NeoPixel strip(PIXEL_COUNT, DATA_PIN, COLOR_ORDER + WS281X_SPEED);

// ====================================================
// SETTINGS (controlled via Bluetooth)
// ====================================================
uint8_t  globalBrightness = 150;
const uint8_t  DOT_BRIGHTNESS = 110;
const uint8_t  MAX_RED        = 150;
uint8_t  runningBrightness    = 30;

// Values stored in flash (what the wing boots with), reported to the app
uint8_t savedBrightness = 150;
uint8_t savedRunning    = 30;
const uint8_t  REVERSE_WHITE  = 150;

uint8_t customR = 255, customG = 0, customB = 0;
uint8_t effectBrightness = 100; // Percentage of global brightness (0-100)
uint8_t policeSegments = 6; // Number of segments for police mode (2-12)
uint8_t chaseWidth = 5; // Number of LEDs in chase effect (1-20)

enum AnimMode : uint8_t {
  MODE_NORMAL = 0,
  MODE_RAINBOW,
  MODE_CHASE,
  MODE_POLICE,
  MODE_STROBE,
  MODE_SOLID
};
AnimMode currentMode = MODE_NORMAL;

unsigned long animSpeed = 2000;
bool overrideCarSignals = false;

// Manual control states
bool manualBrake = false;
bool manualLeft = false;
bool manualRight = false;
bool manualReverse = false;
bool manualHazards = false;

// ====================================================
// EEPROM FUNCTIONS
// ====================================================
// Sends [savedBrightness, savedRunning] so the app knows the stored defaults
void publishSavedValues(bool notify) {
  if (!pCharSavedValues) return;
  uint8_t saved[2] = { savedBrightness, savedRunning };
  pCharSavedValues->setValue(saved, 2);
  if (notify) pCharSavedValues->notify();
}

void loadDefaults() {
  preferences.begin("gt3wing", false);
  globalBrightness = preferences.getUChar("brightness", 150);
  runningBrightness = preferences.getUChar("running", 30);
  preferences.end();
  savedBrightness = globalBrightness;
  savedRunning = runningBrightness;
  
  Serial.println("=================================");
  Serial.println("Loaded defaults from storage:");
  Serial.print("  Master Brightness: "); Serial.println(globalBrightness);
  Serial.print("  Running Brightness: "); Serial.println(runningBrightness);
  Serial.println("=================================");
}

void saveDefaults() {
  preferences.begin("gt3wing", false);
  preferences.putUChar("brightness", globalBrightness);
  preferences.putUChar("running", runningBrightness);
  preferences.end();
  savedBrightness = globalBrightness;
  savedRunning = runningBrightness;
  publishSavedValues(true);
  
  Serial.println("=================================");
  Serial.println("Saved new defaults to storage:");
  Serial.print("  Master Brightness: "); Serial.println(globalBrightness);
  Serial.print("  Running Brightness: "); Serial.println(runningBrightness);
  Serial.println("=================================");
}

// ====================================================
// TIMING
// ====================================================
const unsigned long OUT_TOTAL_MS = 3000;
const unsigned long MEET_HOLD_MS = 120;
const unsigned long DIP_DOWN_MS  = 2000;
const unsigned long RAMP_UP_MS   = 2000;
const unsigned long HOLD_MS      = 1000;
const unsigned long RAMP_DOWN_MS = 2000;

const unsigned long IND_SWEEP_MS = 350;
const unsigned long IND_HOLD_MS  = 150;
const unsigned long IND_OFF_MS   = 450;
const uint8_t AMBER_R = 200, AMBER_G = 50, AMBER_B = 0;

// ====================================================
// INPUT LATCHES
// ====================================================
struct LatchIn {
  uint8_t pin;
  bool active = false;
  unsigned long relStart = 0;
  unsigned long relDebounceMs = 120;
  
  void begin() { pinMode(pin, INPUT); }
  
  void update() {
    bool rawHigh = (digitalRead(pin) == HIGH);
    unsigned long now = millis();
    
    if (rawHigh) { 
      active = true; 
      relStart = 0; 
    }
    else if (active) {
      if (relStart == 0) relStart = now;
      else if (now - relStart >= relDebounceMs) active = false;
    } 
    else {
      relStart = 0;
    }
  }
};

LatchIn reverse{REVERSE_PIN};
LatchIn brake{BRAKE_PIN};
LatchIn indL{LEFT_PIN};
LatchIn indR{RIGHT_PIN};

// ====================================================
// EASING & GAMMA
// ====================================================
static inline float easeInOutCubic(float t) {
  return (t < 0.5f) ? 4.f*t*t*t : 1.f - powf(-2.f*t + 2.f, 3.f)/2.f;
}

static inline uint8_t gammaChannel(uint8_t v) {
  float n = v / 255.f;
  n = powf(n, 2.2f);
  int out = (int)(n*255.f + 0.5f);
  if (out < 0) out = 0; 
  if (out > 255) out = 255;
  return (uint8_t)out;
}

// ====================================================
// PIXEL HELPERS
// ====================================================
inline void setPixel(uint16_t i, uint32_t c) { 
  if (i < PIXEL_COUNT) strip.setPixelColor(i, c); 
}

inline void fillPixels(uint32_t c) { 
  for (uint16_t i=0; i<PIXEL_COUNT; i++) strip.setPixelColor(i, c); 
}

inline void clearPixels() { 
  fillPixels(0); 
}

inline void fillRange(int16_t a, int16_t b, uint32_t c) {
  if (a < 0) a = 0;
  if (b >= (int16_t)PIXEL_COUNT) b = (int16_t)PIXEL_COUNT-1;
  if (a > b) return;
  for (int16_t i=a; i<=b; ++i) strip.setPixelColor(i, c);
}

// ====================================================
// FSM
// ====================================================
enum LedState { 
  IDLE_RUNNING,
  ANIMATING,
  BRAKE_ON,
  REVERSE_ON
};
LedState state = ANIMATING;

// ====================================================
// STARTUP ANIMATION
// ====================================================
struct StartupAnim {
  enum Phase : uint8_t {
    OUT_NO_TRAIL, IN_FILL, MEET_HOLD, FADE_TO_BLACK, FADE_UP, HOLD, FADE_DOWN, DONE
  } phase = OUT_NO_TRAIL;

  unsigned long t0 = 0, lastStep = 0, outStepMs = 20;
  int16_t radius = -1;
  uint16_t cL = 0, cR = 0;

  void cancel() { phase = DONE; }

  void begin() {
    int16_t maxR = (PIXEL_COUNT - 1) / 2;
    uint16_t steps = maxR + 1;
    outStepMs = OUT_TOTAL_MS / (steps ? steps : 1);
    if (outStepMs < 8) outStepMs = 8;
    if (PIXEL_COUNT & 1) { cL = cR = PIXEL_COUNT / 2; }
    else { cR = PIXEL_COUNT / 2; cL = cR - 1; }
    phase = OUT_NO_TRAIL; 
    radius = -1;
    t0 = millis(); 
    lastStep = t0;
  }

  void drawOutFrame() {
    clearPixels();
    lightRadius(radius, DOT_BRIGHTNESS);
  }

  void drawInFillFrame() {
    clearPixels();
    int16_t Lpos, Rpos;
    if (PIXEL_COUNT & 1) { 
      Lpos = (int16_t)(PIXEL_COUNT/2) - radius; 
      Rpos = (int16_t)(PIXEL_COUNT/2) + radius; 
    }
    else { 
      Lpos = (int16_t)cL - radius; 
      Rpos = (int16_t)cR + radius; 
    }
    fillRange(0, Lpos, strip.Color(DOT_BRIGHTNESS,0,0));
    fillRange(Rpos, PIXEL_COUNT-1, strip.Color(DOT_BRIGHTNESS,0,0));
    lightRadius(radius, DOT_BRIGHTNESS);
  }

  void lightRadius(int16_t r, uint8_t redVal) {
    if (PIXEL_COUNT & 1) {
      int16_t center = PIXEL_COUNT / 2;
      int16_t L = center - r, R = center + r;
      if (L >= 0) setPixel((uint16_t)L, strip.Color(redVal,0,0));
      if (R < PIXEL_COUNT) setPixel((uint16_t)R, strip.Color(redVal,0,0));
    } else {
      if (r == 0) { 
        setPixel(cL, strip.Color(redVal,0,0)); 
        setPixel(cR, strip.Color(redVal,0,0)); 
      }
      else {
        int16_t L = (int16_t)cL - r, R = (int16_t)cR + r;
        if (L >= 0) setPixel((uint16_t)L, strip.Color(redVal,0,0));
        if (R < PIXEL_COUNT) setPixel((uint16_t)R, strip.Color(redVal,0,0));
      }
    }
  }

  bool update() {
    unsigned long now = millis();
    int16_t maxR = (PIXEL_COUNT - 1) / 2;

    switch (phase) {
      case OUT_NO_TRAIL: {
        drawOutFrame();
        if (now - lastStep >= outStepMs) {
          lastStep = now; 
          radius++;
          if (radius >= maxR) { 
            phase = IN_FILL; 
            lastStep = now; 
            radius = maxR; 
          }
        }
        return false;
      }

      case IN_FILL: {
        drawInFillFrame();
        if (now - lastStep >= outStepMs) {
          lastStep = now; 
          radius--;
          if (radius < 0) {
            fillPixels(strip.Color(DOT_BRIGHTNESS,0,0));
            phase = MEET_HOLD; 
            t0 = now;
          }
        }
        return false;
      }

      case MEET_HOLD:
        if (now - t0 >= MEET_HOLD_MS) { 
          phase = FADE_TO_BLACK; 
          t0 = now; 
        }
        return false;

      case FADE_TO_BLACK: {
        unsigned long e = now - t0;
        if (e >= DIP_DOWN_MS) { 
          clearPixels(); 
          phase = FADE_UP; 
          t0 = now; 
          return false; 
        }
        float t = 1.f - ((float)e / (float)DIP_DOWN_MS);
        uint8_t v = (uint8_t)(DOT_BRIGHTNESS * t + 0.5f);
        fillPixels(strip.Color(v,0,0));
        return false;
      }

      case FADE_UP: {
        unsigned long e = now - t0;
        if (e >= RAMP_UP_MS) { 
          fillPixels(strip.Color(MAX_RED,0,0)); 
          phase = HOLD; 
          t0 = now; 
          return false; 
        }
        float t = (float)e / (float)RAMP_UP_MS;
        float eased = easeInOutCubic(t);
        uint8_t v = gammaChannel((uint8_t)(MAX_RED * eased));
        fillPixels(strip.Color(v,0,0));
        return false;
      }

      case HOLD:
        fillPixels(strip.Color(MAX_RED,0,0));
        if (now - t0 >= HOLD_MS) { 
          phase = FADE_DOWN; 
          t0 = now; 
        }
        return false;

      case FADE_DOWN: {
        unsigned long e = now - t0;
        if (e >= RAMP_DOWN_MS) { 
          fillPixels(strip.Color(runningBrightness,0,0)); 
          phase = DONE; 
          return true;
        }
        float t = 1.f - ((float)e / (float)RAMP_DOWN_MS);
        float eased = easeInOutCubic(t);
        uint8_t range = MAX_RED - runningBrightness;
        uint8_t v = runningBrightness + gammaChannel((uint8_t)(range * eased));
        fillPixels(strip.Color(v,0,0));
        return false;
      }

      case DONE:
      default: 
        return true;
    }
  }
} anim;

// ====================================================
// INDICATORS
// ====================================================
struct Indicator {
  int16_t segStart, segEnd;
  bool outwardLeft;
  unsigned long t0 = 0;
  enum Phase : uint8_t { OFF, SWEEP, HOLD, GAP } phase = OFF;

  void configure(bool isLeft, uint16_t segLen) {
    if (isLeft) { 
      segStart = 0; 
      segEnd = (int16_t)segLen - 1; 
      outwardLeft = true; 
    }
    else {
      segStart = (int16_t)PIXEL_COUNT - (int16_t)segLen; 
      segEnd = (int16_t)PIXEL_COUNT - 1; 
      outwardLeft = false; 
    }
  }

  void updateAndRender(bool requestActive) {
    unsigned long now = millis();

    switch (phase) {
      case OFF:
        if (requestActive) {
          fillRange(segStart, segEnd, strip.Color(0,0,0));
          phase = SWEEP;
          t0 = now;
        }
        break;

      case SWEEP: {
        fillRange(segStart, segEnd, strip.Color(0,0,0));
        unsigned long e = now - t0;
        float t = (e >= IND_SWEEP_MS) ? 1.f : (float)e / (float)IND_SWEEP_MS;
        uint16_t segLen = (uint16_t)(segEnd - segStart + 1);
        uint16_t n = (uint16_t)(segLen * t + 0.5f);
        if (n == 0) n = 1;
        if (n > segLen) n = segLen;

        if (outwardLeft) {
          int16_t from = segEnd - (int16_t)n + 1;
          int16_t to   = segEnd;
          fillRange(from, to, strip.Color(AMBER_R, AMBER_G, AMBER_B));
        } else {
          int16_t from = segStart;
          int16_t to   = segStart + (int16_t)n - 1;
          fillRange(from, to, strip.Color(AMBER_R, AMBER_G, AMBER_B));
        }

        if (e >= IND_SWEEP_MS) { 
          phase = HOLD; 
          t0 = now; 
        }
      } break;

      case HOLD:
        fillRange(segStart, segEnd, strip.Color(AMBER_R, AMBER_G, AMBER_B));
        if (now - t0 >= IND_HOLD_MS) { 
          phase = GAP; 
          t0 = now; 
        }
        break;

      case GAP:
        fillRange(segStart, segEnd, strip.Color(0,0,0));
        if (now - t0 >= IND_OFF_MS) {
          if (requestActive) { 
            phase = SWEEP; 
            t0 = now; 
          }
          else { 
            phase = OFF;
          }
        }
        break;
    }
  }
};

Indicator indLeft, indRight;

// ====================================================
// ANIMATIONS
// ====================================================
unsigned long lastAnimUpdate = 0;
uint16_t animCounter = 0;

void renderRainbow() {
  unsigned long now = millis();
  if (now - lastAnimUpdate < (animSpeed / 256)) return;
  lastAnimUpdate = now;
  
  animCounter++;
  for (uint16_t i = 0; i < PIXEL_COUNT; i++) {
    uint8_t hue = (i * 256 / PIXEL_COUNT + animCounter) & 0xFF;
    uint8_t r, g, b;
    if (hue < 85) {
      r = hue * 3;
      g = 255 - hue * 3;
      b = 0;
    } else if (hue < 170) {
      hue -= 85;
      r = 255 - hue * 3;
      g = 0;
      b = hue * 3;
    } else {
      hue -= 170;
      r = 0;
      g = hue * 3;
      b = 255 - hue * 3;
    }
    
    // Apply effect brightness
    r = (r * effectBrightness) / 200; // Divide by 200 instead of 100 to keep it dimmer
    g = (g * effectBrightness) / 200;
    b = (b * effectBrightness) / 200;
    
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
}

void renderChase() {
  unsigned long now = millis();
  if (now - lastAnimUpdate < (animSpeed / 50)) return;
  lastAnimUpdate = now;
  
  clearPixels();
  animCounter++;
  uint16_t pos = animCounter % PIXEL_COUNT;
  
  // Apply effect brightness as a percentage of the custom color
  uint8_t r = (customR * effectBrightness) / 100;
  uint8_t g = (customG * effectBrightness) / 100;
  uint8_t b = (customB * effectBrightness) / 100;
  
  // Use variable width (ensure it's at least 1)
  uint8_t width = chaseWidth;
  if (width < 1) width = 1;
  if (width > PIXEL_COUNT) width = PIXEL_COUNT;
  
  for (uint8_t i = 0; i < width; i++) {
    uint16_t p = (pos + i) % PIXEL_COUNT;
    strip.setPixelColor(p, strip.Color(r, g, b));
  }
}

void renderPolice() {
  unsigned long now = millis();
  if (now - lastAnimUpdate < (animSpeed / 10)) return;
  lastAnimUpdate = now;
  
  animCounter++;
  
  // Use variable number of segments (ensure it's at least 2 and even)
  uint16_t numSegments = policeSegments;
  if (numSegments < 2) numSegments = 2;
  if (numSegments % 2 != 0) numSegments++; // Make even
  
  uint16_t segmentSize = PIXEL_COUNT / numSegments;
  bool showRed = (animCounter % 2) == 0;
  
  clearPixels();
  
  // Apply effect brightness to red and blue
  uint8_t redVal = (255 * effectBrightness) / 100;
  uint8_t blueVal = (255 * effectBrightness) / 100;
  
  // Alternate segments between red and blue
  for (uint16_t seg = 0; seg < numSegments; seg++) {
    uint16_t startPixel = seg * segmentSize;
    uint16_t endPixel = (seg == numSegments - 1) ? PIXEL_COUNT - 1 : (startPixel + segmentSize - 1);
    
    // Alternate which color shows based on segment and counter
    bool isRedSegment = ((seg % 2) == 0) ? showRed : !showRed;
    
    uint32_t color = isRedSegment ? strip.Color(redVal, 0, 0) : strip.Color(0, 0, blueVal);
    
    for (uint16_t i = startPixel; i <= endPixel && i < PIXEL_COUNT; i++) {
      strip.setPixelColor(i, color);
    }
  }
}

void renderStrobe() {
  unsigned long now = millis();
  if (now - lastAnimUpdate < (animSpeed / 10)) return;
  lastAnimUpdate = now;
  
  animCounter++;
  
  // Apply effect brightness as a percentage of the custom color
  uint8_t r = (customR * effectBrightness) / 100;
  uint8_t g = (customG * effectBrightness) / 100;
  uint8_t b = (customB * effectBrightness) / 100;
  
  if (animCounter % 2 == 0) {
    fillPixels(strip.Color(r, g, b));
  } else {
    clearPixels();
  }
}

// ====================================================
// BLUETOOTH CALLBACKS
// ====================================================
class ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE: Client connected");
    
    // Send current values to the app
    if (pCharBrightness) {
      pCharBrightness->setValue(&globalBrightness, 1);
      pCharBrightness->notify();
    }
    if (pCharRunning) {
      pCharRunning->setValue(&runningBrightness, 1);
      pCharRunning->notify();
    }
    publishSavedValues(true);
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BLE: Client disconnected");
    
    // Reset to Normal mode and disable override
    currentMode = MODE_NORMAL;
    overrideCarSignals = false;
    animCounter = 0;
    
    // Reset manual controls
    manualBrake = false;
    manualLeft = false;
    manualRight = false;
    manualReverse = false;
    manualHazards = false;
    
    Serial.println("BLE: Reset to Normal mode");
  }
};

class CharCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String uuid = pCharacteristic->getUUID().toString();
    uint8_t* pData = pCharacteristic->getData();
    size_t len = pCharacteristic->getValue().length();
    
    if (len > 0) {
      // Mode
      if (uuid == CHAR_MODE_UUID) {
        currentMode = (AnimMode)pData[0];
        animCounter = 0;
        Serial.print("BLE: Mode = ");
        Serial.println(currentMode);
      }
      // Brightness
      else if (uuid == CHAR_BRIGHTNESS_UUID) {
        globalBrightness = pData[0];
        strip.setBrightness(globalBrightness);
        Serial.print("BLE: Brightness = ");
        Serial.println(globalBrightness);
      }
      // Running
      else if (uuid == CHAR_RUNNING_UUID) {
        runningBrightness = pData[0];
        Serial.print("BLE: Running = ");
        Serial.println(runningBrightness);
      }
      // Color (3 bytes: R, G, B)
      else if (uuid == CHAR_COLOR_UUID && len >= 3) {
        customR = pData[0];
        customG = pData[1];
        customB = pData[2];
        Serial.printf("BLE: Color = R:%d G:%d B:%d\n", customR, customG, customB);
      }
      // Speed (2 bytes for uint16)
      else if (uuid == CHAR_SPEED_UUID && len >= 2) {
        animSpeed = (pData[1] << 8) | pData[0];
        Serial.print("BLE: Speed changed to: ");
        Serial.print(animSpeed);
        Serial.println(" ms");
      }
      // Override
      else if (uuid == CHAR_OVERRIDE_UUID) {
        overrideCarSignals = pData[0];
        Serial.print("BLE: Override = ");
        Serial.println(overrideCarSignals);
      }
      // Animation trigger
      else if (uuid == CHAR_ANIMATION_UUID) {
        Serial.println("BLE: Animation triggered");
        if (pData[0] == 1) {
          currentMode = MODE_NORMAL;
          animCounter = 0;
          anim.begin();
          state = ANIMATING;
          Serial.println("BLE: Animation started!");
        }
      }
      // Save defaults
      else if (uuid == CHAR_SAVE_DEFAULT_UUID) {
        Serial.println("BLE: Save defaults triggered");
        if (pData[0] == 1) {
          saveDefaults();
        }
      }
      // Manual brake
      else if (uuid == CHAR_MANUAL_BRAKE_UUID) {
        manualBrake = pData[0];
        Serial.print("BLE: Manual Brake = ");
        Serial.println(manualBrake);
      }
      // Manual left indicator
      else if (uuid == CHAR_MANUAL_LEFT_UUID) {
        manualLeft = pData[0];
        Serial.print("BLE: Manual Left = ");
        Serial.println(manualLeft);
      }
      // Manual right indicator
      else if (uuid == CHAR_MANUAL_RIGHT_UUID) {
        manualRight = pData[0];
        Serial.print("BLE: Manual Right = ");
        Serial.println(manualRight);
      }
      // Manual reverse
      else if (uuid == CHAR_MANUAL_REVERSE_UUID) {
        manualReverse = pData[0];
        Serial.print("BLE: Manual Reverse = ");
        Serial.println(manualReverse);
      }
      // Manual hazards
      else if (uuid == CHAR_MANUAL_HAZARDS_UUID) {
        manualHazards = pData[0];
        Serial.print("BLE: Manual Hazards = ");
        Serial.println(manualHazards);
      }
      // Effect brightness
      else if (uuid == CHAR_EFFECT_BRIGHTNESS_UUID) {
        effectBrightness = pData[0];
        Serial.print("BLE: Effect Brightness = ");
        Serial.print(effectBrightness);
        Serial.println("%");
      }
      // Police segments
      else if (uuid == CHAR_POLICE_SEGMENTS_UUID) {
        policeSegments = pData[0];
        Serial.print("BLE: Police Segments = ");
        Serial.println(policeSegments);
      }
      // Chase width
      else if (uuid == CHAR_CHASE_WIDTH_UUID) {
        chaseWidth = pData[0];
        Serial.print("BLE: Chase Width = ");
        Serial.println(chaseWidth);
      }
    }
  }
  
  void onRead(BLECharacteristic *pCharacteristic) {
    String uuid = pCharacteristic->getUUID().toString();
    
    // Return current brightness value
    if (uuid == CHAR_BRIGHTNESS_UUID) {
      pCharacteristic->setValue(&globalBrightness, 1);
      Serial.print("BLE: Read Brightness = ");
      Serial.println(globalBrightness);
    }
    // Return current running brightness value
    else if (uuid == CHAR_RUNNING_UUID) {
      pCharacteristic->setValue(&runningBrightness, 1);
      Serial.print("BLE: Read Running = ");
      Serial.println(runningBrightness);
    }
    // Return saved defaults
    else if (uuid == CHAR_SAVED_VALUES_UUID) {
      publishSavedValues(false);
      Serial.print("BLE: Read Saved = ");
      Serial.print(savedBrightness);
      Serial.print(", ");
      Serial.println(savedRunning);
    }
  }
};

// ====================================================
// FRAME RATE
// ====================================================
unsigned long lastFrame = 0;
const unsigned long frameInterval = 16;
unsigned long lastShow = 0;
const unsigned long minShowInterval = 5;

// ====================================================
// SETUP
// ====================================================
void setup() {
  pinMode(DATA_PIN, OUTPUT);
  digitalWrite(DATA_PIN, LOW);
  delay(300);
  
  Serial.begin(SERIAL_BAUD);
  delay(500);
  
  Serial.println("\n\n=================================");
  Serial.println("GT3 Wing LED Controller - XIAO ESP32-C6");
  Serial.println("Bluetooth LE Mode");
  Serial.println("=================================");

  // Load saved defaults
  loadDefaults();

  digitalWrite(DATA_PIN, LOW);
  
  strip.begin();
  strip.setBrightness(globalBrightness);
  strip.clear(); 
  strip.show(); 
  delayMicroseconds(500);
  strip.clear(); 
  strip.show();
  delayMicroseconds(500);

  reverse.begin();
  brake.begin();
  indL.begin();
  indR.begin();

  uint16_t segLen = (PIXEL_COUNT >= 5) ? (PIXEL_COUNT / 5) : (PIXEL_COUNT > 1 ? PIXEL_COUNT/2 : 1);
  indLeft.configure(true, segLen);
  indRight.configure(false, segLen);

  // Enable the XIAO C6 RF switch and pick the antenna before starting BLE
  pinMode(RF_SWITCH_EN_PIN, OUTPUT);
  digitalWrite(RF_SWITCH_EN_PIN, LOW);
  delay(100);
  pinMode(RF_ANT_SELECT_PIN, OUTPUT);
  digitalWrite(RF_ANT_SELECT_PIN, USE_EXTERNAL_ANTENNA ? HIGH : LOW);

  // Initialize BLE
  BLEDevice::init("GT3_Wing");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  // 17 characteristics need ~40 attribute handles; the default of 15 is too few
  BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID), 64);

  // Create characteristics with READ and WRITE
  pCharMode = pService->createCharacteristic(
    CHAR_MODE_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharMode->setCallbacks(new CharCallbacks());
  pCharMode->addDescriptor(new BLE2902());

  pCharBrightness = pService->createCharacteristic(
    CHAR_BRIGHTNESS_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharBrightness->setCallbacks(new CharCallbacks());
  pCharBrightness->addDescriptor(new BLE2902());

  pCharRunning = pService->createCharacteristic(
    CHAR_RUNNING_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharRunning->setCallbacks(new CharCallbacks());
  pCharRunning->addDescriptor(new BLE2902());

  pCharColor = pService->createCharacteristic(
    CHAR_COLOR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pCharColor->setCallbacks(new CharCallbacks());

  pCharSpeed = pService->createCharacteristic(
    CHAR_SPEED_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pCharSpeed->setCallbacks(new CharCallbacks());

  pCharOverride = pService->createCharacteristic(
    CHAR_OVERRIDE_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pCharOverride->setCallbacks(new CharCallbacks());

  pCharAnimation = pService->createCharacteristic(
    CHAR_ANIMATION_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharAnimation->setCallbacks(new CharCallbacks());

  pCharSaveDefault = pService->createCharacteristic(
    CHAR_SAVE_DEFAULT_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharSaveDefault->setCallbacks(new CharCallbacks());

  pCharManualBrake = pService->createCharacteristic(
    CHAR_MANUAL_BRAKE_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharManualBrake->setCallbacks(new CharCallbacks());

  pCharManualLeft = pService->createCharacteristic(
    CHAR_MANUAL_LEFT_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharManualLeft->setCallbacks(new CharCallbacks());

  pCharManualRight = pService->createCharacteristic(
    CHAR_MANUAL_RIGHT_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharManualRight->setCallbacks(new CharCallbacks());

  pCharManualReverse = pService->createCharacteristic(
    CHAR_MANUAL_REVERSE_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharManualReverse->setCallbacks(new CharCallbacks());

  pCharManualHazards = pService->createCharacteristic(
    CHAR_MANUAL_HAZARDS_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharManualHazards->setCallbacks(new CharCallbacks());

  pCharEffectBrightness = pService->createCharacteristic(
    CHAR_EFFECT_BRIGHTNESS_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharEffectBrightness->setCallbacks(new CharCallbacks());

  pCharPoliceSegments = pService->createCharacteristic(
    CHAR_POLICE_SEGMENTS_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharPoliceSegments->setCallbacks(new CharCallbacks());

  pCharChaseWidth = pService->createCharacteristic(
    CHAR_CHASE_WIDTH_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharChaseWidth->setCallbacks(new CharCallbacks());

  pCharSavedValues = pService->createCharacteristic(
    CHAR_SAVED_VALUES_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharSavedValues->setCallbacks(new CharCallbacks());
  pCharSavedValues->addDescriptor(new BLE2902());
  publishSavedValues(false);

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE advertising started");
  Serial.println("Device name: GT3_Wing");
  Serial.println("Waiting for connection...");

  anim.begin();
  state = ANIMATING;

  Serial.print("PIXEL_COUNT = "); Serial.println(PIXEL_COUNT);
  Serial.print("Indicator segLen = "); Serial.println(segLen);
  Serial.println("=================================\n");
  
  lastShow = millis();
}

// ====================================================
// LOOP
// ====================================================
void loop() {
  // Handle BLE reconnection
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("BLE: Restarting advertising");
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  
  unsigned long now = millis();
  
  if (now - lastFrame < frameInterval) return;
  lastFrame = now;

  // Update physical input latches only if override is off
  if (!overrideCarSignals) {
    reverse.update();
    brake.update();
    indL.update();
    indR.update();
  }

  // Determine active signals (manual or physical)
  bool activeBrake = overrideCarSignals ? manualBrake : brake.active;
  bool activeReverse = overrideCarSignals ? manualReverse : reverse.active;
  bool activeLeft = overrideCarSignals ? (manualLeft || manualHazards) : indL.active;
  bool activeRight = overrideCarSignals ? (manualRight || manualHazards) : indR.active;

  // Render based on mode
  if (currentMode != MODE_NORMAL) {
    switch (currentMode) {
      case MODE_RAINBOW:
        renderRainbow();
        break;
      case MODE_CHASE:
        renderChase();
        break;
      case MODE_POLICE:
        renderPolice();
        break;
      case MODE_STROBE:
        renderStrobe();
        break;
      case MODE_SOLID: {
        // Apply effect brightness to solid color
        uint8_t r = (customR * effectBrightness) / 100;
        uint8_t g = (customG * effectBrightness) / 100;
        uint8_t b = (customB * effectBrightness) / 100;
        fillPixels(strip.Color(r, g, b));
        break;
      }
      default:
        break;
    }
  } else {
    if (activeBrake) {
      if (state != BRAKE_ON) { 
        Serial.println("BRAKE ON"); 
        if (state == ANIMATING) anim.cancel();
        state = BRAKE_ON;
      }
      fillPixels(strip.Color(MAX_RED,0,0));
    }
    else if (activeReverse) {
      if (state != REVERSE_ON) {
        Serial.println("REVERSE ON");
        if (state == ANIMATING) anim.cancel();
        state = REVERSE_ON;
      }
      fillPixels(strip.Color(REVERSE_WHITE, REVERSE_WHITE, REVERSE_WHITE));
    }
    else {
      if (state == BRAKE_ON) { 
        Serial.println("BRAKE RELEASED → RUNNING"); 
        state = IDLE_RUNNING;
      }
      if (state == REVERSE_ON) {
        Serial.println("REVERSE OFF → RUNNING");
        state = IDLE_RUNNING;
      }
      
      if (state == ANIMATING) {
        if (anim.update()) { 
          state = IDLE_RUNNING; 
          Serial.println("ANIMATION END → RUNNING"); 
        }
      } 
      else {
        fillPixels(strip.Color(runningBrightness,0,0));
      }
    }
  }

  // Update indicators (respects override)
  indLeft.updateAndRender(activeLeft);
  indRight.updateAndRender(activeRight);

  if (now - lastShow >= minShowInterval) {
    strip.show();
    lastShow = now;
    delayMicroseconds(300);
  }

  // Clear any pixels beyond our defined range
  for (uint16_t i = PIXEL_COUNT; i < 100; i++) {
    strip.setPixelColor(i, 0);
  }
}
