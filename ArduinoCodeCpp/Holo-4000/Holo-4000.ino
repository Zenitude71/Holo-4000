#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Paramètres
// -----------------------------------------------------------------------------
#define LED_PIN        6        // Data pin pour NeoPixel
#define NUM_LEDS       30
#define BRIGHTNESS     255
#define DELAY_MS       50
#define RADIUS         8        // rayon en « pixels image »
#define CENTER_INDEX   (NUM_LEDS/2)
#define REFRESH_RADIUS 3        // pas angulaire en degrés
#define TOUR_RADIUS    360
#define NUM_STEPS      (TOUR_RADIUS/REFRESH_RADIUS)

static const int16_t K_FP = (RADIUS * 256) / CENTER_INDEX; // Q8.8

// -----------------------------------------------------------------------------
// Tables pré-calculées
// -----------------------------------------------------------------------------

// 1) r_table[i] = ((i−CENTER_INDEX)/CENTER_INDEX)*RADIUS * 256  (Q8.8)
static int16_t r_table[NUM_LEDS];

// 2) cos/sin lookup pour 0…90° en Q1.7
// cos(θ) en Q1.7 pour θ = 0…90°
// c[i] = round( cos(i * PI/180) * 127 )
static const int8_t DEGREE_FP[91] = {
  127,127,127,127,127,127,126,126,126,125,  // 0..9
  125,125,124,124,123,123,122,121,121,120,  // 10..19
  119,119,118,117,116,115,114,113,112,111,  // 20..29
  110,109,108,107,106,104,103,102,100, 99,  // 30..39
   97, 96, 94, 93, 91, 90, 88, 87, 85, 83,  // 40..49
   82, 79, 78, 76, 75, 73, 71, 69, 67, 65,  // 50..59
   64, 62, 60, 57, 56, 54, 52, 50, 48, 46,  // 60..69
   44, 42, 39, 37, 35, 33, 31, 29, 26, 24,  // 70..79
   22, 20, 18, 15, 13, 11,  9,  7,  4,  2,  // 80..89
    0                                     // 90
};

// ᕦ(ò_óˇ)ᕤ



// Table d'image 16x16 (couleurs RGB) utilisée pour mapper les LED
const uint8_t image[16][16][3] = {
  { {76, 255, 0}, {76, 255, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 0, 0}, {255, 0, 0} },
  { {76, 255, 0}, {76, 255, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 0, 0}, {255, 0, 0} },
  { {76, 255, 0}, {76, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {255, 0, 0} },
  { {76, 255, 0}, {76, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {255, 0, 0} },
  { {76, 255, 0}, {76, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {255, 0, 0} },
  { {76, 255, 0}, {76, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {255, 0, 0} },
  { {76, 255, 0}, {76, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {255, 0, 0} },
  { {76, 255, 0}, {76, 255, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 0, 0}, {255, 0, 0} },
  { {76, 255, 0}, {76, 255, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 216, 0}, {255, 0, 0}, {255, 0, 0} },
  { {76, 255, 0}, {76, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {255, 0, 0} },
  { {76, 255, 0}, {76, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {255, 0, 0} },
  { {76, 255, 0}, {76, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {255, 0, 0} },
  { {76, 255, 0}, {76, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {255, 0, 0} },
  { {0, 255, 0}, {76, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {255, 0, 0} }
};

// -----------------------------------------------------------------------------
// Variables
// -----------------------------------------------------------------------------

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

static int coords[NUM_LEDS][2]; // coords temporaires
static int8_t cos_fp, sin_fp;  // cos/sin en Q1.7

// -----------------------------------------------------------------------------
// calcImageCoordinates en fixed-point
// -----------------------------------------------------------------------------
static inline void calcImageCoordinates(int i, int &ix, int &iy) {
  int16_t r_fp = r_table[i];                 // Q8.8
  int32_t accx = (int32_t)r_fp * cos_fp;     // Q9.15
  int32_t accy = (int32_t)r_fp * sin_fp;
  int16_t x_fp = (int16_t)((accx + (1<<6)) >> 7); // → Q9.8
  int16_t y_fp = (int16_t)((accy + (1<<6)) >> 7);
  ix = x_fp + 8;  // centre image
  iy = y_fp + 8;
}

// -----------------------------------------------------------------------------
// fastSinCosLookup en Q1.7
// -----------------------------------------------------------------------------
static inline void fastSinCosLookup(int angleDeg, int8_t &outCos, int8_t &outSin) {
  int q = angleDeg / 90;
  int la = angleDeg % 90;
  int8_t vx = DEGREE_FP[la];
  int8_t vz = DEGREE_FP[90 - la];
  switch (q) {
    case 0: outCos = vx;  outSin = vz;  break;
    case 1: outCos = -vz; outSin = vx;  break;
    case 2: outCos = -vx; outSin = -vz; break;
    default:outCos = vz;  outSin = -vx; break;
  }
}
// Fonction de debug à ajouter au-dessus de setup()
void debugTrig() {
  Serial.println(F("Angle  |  cos_fp  |  cos_std   |  sin_fp  |  sin_std"));
  for (int angle = 0; angle < 360; angle += REFRESH_RADIUS) {
    int8_t c_fp, s_fp;
    fastSinCosLookup(angle, c_fp, s_fp);

    // Convertit Q1.7 → float
    float cos_fix = (float)c_fp / 127.0f;
    float sin_fix = (float)s_fp / 127.0f;

    // Calcul « standard »
    float rad = angle * (3.14159265f / 180.0f);
    float cos_std = cosf(rad);
    float sin_std = sinf(rad);

    // Affichage formaté
    Serial.print(angle);
    Serial.print(F("°    | "));
    Serial.print(cos_fix, 5);
    Serial.print(F(" | "));
    Serial.print(cos_std, 5);
    Serial.print(F(" | "));
    Serial.print(sin_fix, 5);
    Serial.print(F(" | "));
    Serial.println(sin_std, 5);
  }
}
int compteur = 0;
unsigned long t0, dt;
// -----------------------------------------------------------------------------
// setup()
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  while (!Serial) ;

  // Pré-calcule r_table
  for (int i = 0; i < NUM_LEDS; i++) {
    int delta = i - CENTER_INDEX;
    r_table[i] = (int16_t)((delta * K_FP) >> 8);
  }

  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
  // debugTrig();
  // delay(10000000);
  t0 = micros();
}

// -----------------------------------------------------------------------------
// loop()
// -----------------------------------------------------------------------------
void loop() {
  static int angle = 0;


  // 1) sin/cos fixed-point
  // t0 = micros();
  fastSinCosLookup(angle, cos_fp, sin_fp);

  // 2) calcul des coords
  for (int i = 0; i < NUM_LEDS; i++) {
    calcImageCoordinates(i, coords[i][0], coords[i][1]);
  }
  // dt = micros() - t0;
  // Serial.print("Calcul : ");
  // Serial.print(dt);
  // Serial.println(" µs");

  // 3) affichage NeoPixel
  // t0 = micros();
  for (int i = 0; i < NUM_LEDS; i++) {
    int x = constrain(coords[i][0], 0, 15);
    int y = constrain(coords[i][1], 0, 15);
    uint8_t r = image[y][x][0];
    uint8_t g = image[y][x][1];
    uint8_t b = image[y][x][2];
    strip.setPixelColor(i, r, g, b);
  }
  strip.show();
  // dt = micros() - t0;
  // Serial.print("Affichage : ");
  // Serial.print(dt);
  // Serial.println(" µs");

  // 4) incrémente angle
  angle += REFRESH_RADIUS;
  if (angle >= TOUR_RADIUS) {
    angle -= TOUR_RADIUS;
    compteur++;
    // Serial.println(compteur);
    }
    if(micros()-t0>10000000){
      Serial.println(compteur*6);
      compteur = 0;
      t0=micros();
    }
    // else{
    //   Serial.println("1 pas");
    // }

  //delay(DELAY_MS);
}