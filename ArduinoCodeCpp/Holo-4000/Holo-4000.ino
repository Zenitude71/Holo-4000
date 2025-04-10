#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <math.h>

#define LED_PIN     6      // Pin connectée au Data Out (DO)
#define NUM_LEDS    72     // Nombre total de LED
#define BRIGHTNESS  25     // Luminosité (0 à 255)
#define DELAY_MS    0      // Délai entre chaque affichage (vitesse de rotation)
#define RADIUS      8.0    // Rayon de l’hélice (en "pixels image")
#define LOOKUP_SCALE 0.000015625f  // 1/64000
#define CENTER_INDEX (NUM_LEDS / 2)

// Déclaration de la bande de LED
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Angle global de rotation (en degrés)
int angle = 0;
int count = 100;
bool haveMakeALoop = false;
unsigned long startFast = 0;
unsigned long durationFast = 0;

// Table de lookup pour 0° à 90° (91 valeurs)
// Chaque valeur est pré-calculée et normalisée (1.0 = 64000)
const float DEGREE_LOOKUP_TABLE[91] = {
  64000 * LOOKUP_SCALE,
  63990 * LOOKUP_SCALE,
  63961 * LOOKUP_SCALE,
  63912 * LOOKUP_SCALE,
  63844 * LOOKUP_SCALE,
  63756 * LOOKUP_SCALE,
  63649 * LOOKUP_SCALE,
  63523 * LOOKUP_SCALE,
  63377 * LOOKUP_SCALE,
  63212 * LOOKUP_SCALE,
  63028 * LOOKUP_SCALE,
  62824 * LOOKUP_SCALE,
  62601 * LOOKUP_SCALE,
  62360 * LOOKUP_SCALE,
  62099 * LOOKUP_SCALE,
  61819 * LOOKUP_SCALE,
  61521 * LOOKUP_SCALE,
  61204 * LOOKUP_SCALE,
  60868 * LOOKUP_SCALE,
  60513 * LOOKUP_SCALE,
  60140 * LOOKUP_SCALE,
  59749 * LOOKUP_SCALE,
  59340 * LOOKUP_SCALE,
  58912 * LOOKUP_SCALE,
  58467 * LOOKUP_SCALE,
  58004 * LOOKUP_SCALE,
  57523 * LOOKUP_SCALE,
  57024 * LOOKUP_SCALE,
  56509 * LOOKUP_SCALE,
  55976 * LOOKUP_SCALE,
  55426 * LOOKUP_SCALE,
  54859 * LOOKUP_SCALE,
  54275 * LOOKUP_SCALE,
  53675 * LOOKUP_SCALE,
  53058 * LOOKUP_SCALE,
  52426 * LOOKUP_SCALE,
  51777 * LOOKUP_SCALE,
  51113 * LOOKUP_SCALE,
  50433 * LOOKUP_SCALE,
  49737 * LOOKUP_SCALE,
  49027 * LOOKUP_SCALE,
  48301 * LOOKUP_SCALE,
  47561 * LOOKUP_SCALE,
  46807 * LOOKUP_SCALE,
  46038 * LOOKUP_SCALE,
  45255 * LOOKUP_SCALE,
  44458 * LOOKUP_SCALE,
  43648 * LOOKUP_SCALE,
  42824 * LOOKUP_SCALE,
  41988 * LOOKUP_SCALE,
  41138 * LOOKUP_SCALE,
  40277 * LOOKUP_SCALE,
  39402 * LOOKUP_SCALE,
  38516 * LOOKUP_SCALE,
  37618 * LOOKUP_SCALE,
  36709 * LOOKUP_SCALE,
  35788 * LOOKUP_SCALE,
  34857 * LOOKUP_SCALE,
  33915 * LOOKUP_SCALE,
  32962 * LOOKUP_SCALE,
  32000 * LOOKUP_SCALE,
  31028 * LOOKUP_SCALE,
  30046 * LOOKUP_SCALE,
  29055 * LOOKUP_SCALE,
  28056 * LOOKUP_SCALE,
  27048 * LOOKUP_SCALE,
  26031 * LOOKUP_SCALE,
  25007 * LOOKUP_SCALE,
  23975 * LOOKUP_SCALE,
  22936 * LOOKUP_SCALE,
  21889 * LOOKUP_SCALE,
  20836 * LOOKUP_SCALE,
  19777 * LOOKUP_SCALE,
  18712 * LOOKUP_SCALE,
  17641 * LOOKUP_SCALE,
  16564 * LOOKUP_SCALE,
  15483 * LOOKUP_SCALE,
  14397 * LOOKUP_SCALE,
  13306 * LOOKUP_SCALE,
  12212 * LOOKUP_SCALE,
  11113 * LOOKUP_SCALE,
  10012 * LOOKUP_SCALE,
  8907 * LOOKUP_SCALE,
  7800 * LOOKUP_SCALE,
  6690 * LOOKUP_SCALE,
  5578 * LOOKUP_SCALE,
  4464 * LOOKUP_SCALE,
  3350 * LOOKUP_SCALE,
  2234 * LOOKUP_SCALE,
  1117 * LOOKUP_SCALE,
  0 * LOOKUP_SCALE
};

// Fonction fastSinCosLookup
// Calcule sin et cos pour un angle en degrés (0° à 359°) en utilisant la table de lookup
// et une interpolation linéaire, puis ajuste le résultat en fonction du quadrant.
// Les résultats (cos et sin) sont renvoyés via les références outCos et outSin.
void fastSinCosLookup(int angleDegrees, float &outCos, float &outSin) {
  // Normalisation de l'angle dans [0, 360)
  // int angleInt = (int)angleDegrees;
  // int normalized = angleInt % 360;
  // if (normalized < 0)
  //   normalized += 360;

  // Calcul du quadrant (0 à 3) et de l'angle local dans [0, 90)
  int quadrant = angleDegrees / 90;
  int localAngle = (angleDegrees % 90);
  // if (localAngle >= 90.0f)
  //   localAngle = 89.9999f;

  // Interpolation linéaire dans la table
  int deg1 = (int)localAngle;
  // float module = localAngle - deg1;
  int deg2 = 90 - deg1;  // Pour obtenir sin(localAngle) = cos(90-localAngle)

  //float vX = ;      // approximation de cos(localAngle)
  //float vZ = ;      // approximation de sin(localAngle)
  // float mX = DEGREE_LOOKUP_TABLE[deg1 + 1];
  // float mZ = DEGREE_LOOKUP_TABLE[deg2 - 1];

  // float vectorX = vX + (mX - vX) * module;
  // float vectorZ = vZ + (mZ - vZ) * module;  
  float vectorX = DEGREE_LOOKUP_TABLE[deg1] ;
  float vectorZ = DEGREE_LOOKUP_TABLE[deg2] ;

// Serial.println(String("Module: ") + module);
// Serial.println(String("vectorX: ") + vectorX);
// Serial.println(String("vX + (mX - vX): ") + test);


  // Ajustement en fonction du quadrant
  switch (quadrant) {
    case 0:
      outCos = vectorX;
      outSin = vectorZ;
      break;
    case 1:
      outCos = -vectorZ;
      outSin = vectorX;
      break;
    case 2:
      outCos = -vectorX;
      outSin = -vectorZ;
      break;
    case 3:
      outCos = vectorZ;
      outSin = -vectorX;
      break;
  }
}

static inline void calcImageCoordinates(int i,  float cosTheta, float sinTheta, int &ix, int &iy) {
    // Calculer la distance normalisée (r)
    float r = (float)(i - CENTER_INDEX) / CENTER_INDEX * RADIUS;
    // Calculer les coordonnées cartésiennes
    // float x = ;
    // float y = ;
    // Calculer les coordonnées dans l'image, centrées en (8,8)
    ix = round(r * cosTheta + 8);
    iy = round(r * sinTheta + 8);
}



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

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}

void loop() {
  if(!haveMakeALoop){
    haveMakeALoop = true;
    startFast = micros();
  }
  
  //strip.clear();
  // int centerIndex = NUM_LEDS / 2;
  // Utilisation de la fonction fastSinCosLookup pour obtenir sin et cos
  float cosTheta, sinTheta;
  fastSinCosLookup((float)angle, cosTheta, sinTheta);
  int ix = 0;
  int iy = 0;
  // Pour chaque LED de la bande, on calcule la position à afficher
  for (int i = 0; i < NUM_LEDS; i++) {
    // int offset = i - centerIndex;
    // float ratio = (float)(i - centerIndex) / centerIndex;
    // float ratio = (float)offset / centerIndex;  // Plage de -1 à 1
    // float r = ratio * RADIUS;
    // float r = (float)(i - centerIndex) / centerIndex * RADIUS;
    
    // // Calcul de la position cartésienne
    // float x = r * cosTheta;
    // float y = r * sinTheta;
    
    // // Calcul des coordonnées dans l'image (centrées en 8,8)
    // int ix = round(x + 8);
    // int iy = round(y + 8);
    calcImageCoordinates(i, cosTheta, sinTheta, ix, iy);

    
    // Sélection de la couleur selon l'image (si dans les limites), sinon éteint
    if (ix >= 0 && ix < 16 && iy >= 0 && iy < 16) {
      // uint8_t rcol = image[iy][ix][0];
      // uint8_t gcol = image[iy][ix][1];
      // uint8_t bcol = image[iy][ix][2];
      // strip.setPixelColor(i, strip.Color(rcol, gcol, bcol));
      // uint8_t rcol = image[iy][ix][0];
      // uint8_t gcol = image[iy][ix][1];
      // uint8_t bcol = image[iy][ix][2];
      strip.setPixelColor(i, strip.Color(image[iy][ix][0], image[iy][ix][1], image[iy][ix][2]));
    } else {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
  }
  strip.show();
  
  // Mise à jour de l'angle de rotation
  angle += 9;
  if (angle >= 360){
    angle -= 360;

    count--;
    if(count<=0){
      count = 100;
      haveMakeALoop = false;
      durationFast = micros() - startFast;
      durationFast /= 100;
      Serial.println(durationFast);  
    }
    
  }
  
  
  
  // delay(DELAY_MS);
}
