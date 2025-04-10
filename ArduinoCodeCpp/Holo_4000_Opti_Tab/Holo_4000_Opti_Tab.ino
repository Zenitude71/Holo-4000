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
#define REFRESH_RADIUS 9  // Degré de rotation par actualisation
#define TOUR_RADIUS 360   // Degré  d'un tour
#define NUM_STEPS      (TOUR_RADIUS / REFRESH_RADIUS)  // 360/9 = 40 étapes

// Déclaration de la bande de LED
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Angle global de rotation (en degrés)
int angle = 0;
int count = 100;
bool haveMakeALoop = false;
unsigned long startFast = 0;
unsigned long durationFast = 0;
// int coords[NUM_LEDS][2];
float cosTheta, sinTheta;
int currentStep = 0;   // Indice de l'étape courante (0 à NUM_STEPS-1)
byte coords[NUM_STEPS][NUM_LEDS][2];

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


  // Calcul du quadrant (0 à 3) et de l'angle local dans [0, 90)
  int quadrant = angleDegrees / 90;
  int localAngle = (angleDegrees % 90);

  // Interpolation linéaire dans la table
  int deg1 = (int)localAngle;
  int deg2 = 90 - deg1;  // Pour obtenir sin(localAngle) = cos(90-localAngle)

  float vectorX = DEGREE_LOOKUP_TABLE[deg1] ;
  float vectorZ = DEGREE_LOOKUP_TABLE[deg2] ;


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

static inline void calcImageCoordinates(int i,  float cosTheta, float sinTheta, byte &ix, byte &iy) {
    // Calculer la distance normalisée (r)
    float r = (float)(i - CENTER_INDEX) / CENTER_INDEX * RADIUS;
    // Calculer les coordonnées dans l'image, centrées en (8,8)
    ix = round(r * cosTheta + 8);
    iy = round(r * sinTheta + 8);
}

//
// Pré-calcule le tableau de coordonnées pour chaque étape de rotation.
// Le tableau coords a pour dimensions [NUM_STEPS][NUM_LEDS][2].
// Pour chaque étape (angle = step * REFRESH_RADIUS) et pour chaque LED,
// la fonction calcImageCoordinates() calcule les coordonnées (ix,iy) qui sont stockées.
void precomputeCoords() {
  float cosTheta, sinTheta;
  for (int step = 0; step < NUM_STEPS; step++) {
    int a = step * REFRESH_RADIUS;  // Angle en degrés pour cette étape
    fastSinCosLookup(a, cosTheta, sinTheta);
    for (int i = 0; i < NUM_LEDS; i++) {
      calcImageCoordinates(i, cosTheta, sinTheta, coords[step][i][0], coords[step][i][1]);
    }
  }
}

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

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();

   // Pré-calcule le tableau des coordonnées
  precomputeCoords();

  // Affiche un extrait du tableau pour vérification
  Serial.println("Coordonnées pré-calculées (format: x / y) pour la 1ère LED de chaque étape) :");
  for (int step = 0; step < NUM_STEPS; step++) {
    Serial.print("Step ");
    Serial.print(step * REFRESH_RADIUS);
    Serial.print("°: ");
    // Attention : dans l'image, le premier indice est la ligne (y) et le second est la colonne (x)
    Serial.println(String(coords[step][0][0]) + " / " + String(coords[step][0][1]));
  }
}

void loop() {
  // Utilise la table pré-calculée pour afficher la bande de LED
  for (int i = 0; i < NUM_LEDS; i++) {
    // Notez que les coordonnées sont stockées comme {ix, iy} et que l'image est indexée image[y][x]
    int x = coords[currentStep][i][0];
    int y = coords[currentStep][i][1];
    if (x >= 0 && x < 16 && y >= 0 && y < 16) {
      strip.setPixelColor(i, strip.Color(image[y][x][0], image[y][x][1], image[y][x][2]));
    } else {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
  }
  strip.show();
  delay(DELAY_MS);

  currentStep++;
  if (currentStep >= NUM_STEPS) {
    currentStep = 0;
  }
  // if(!haveMakeALoop){
  //   haveMakeALoop = true;
  //   startFast = micros();
  // }
  // fastSinCosLookup((float)angle, cosTheta, sinTheta);
  // // Pour chaque LED de la bande, on calcule la position à afficher
  // for (int i = 0; i < NUM_LEDS; i++) {
  //   calcImageCoordinates(i, cosTheta, sinTheta, coords[i][0], coords[i][1]);
  // }
  
  // for(int i = 0; i<NUM_LEDS; i++){
  //   // Correspond à : 
  //     // uint8_t rcol = image[iy][ix][0];
  //     // uint8_t gcol = image[iy][ix][1];
  //     // uint8_t bcol = image[iy][ix][2];
  //     // strip.setPixelColor(i, strip.Color(image[iy][ix][0], image[iy][ix][1], image[iy][ix][2]));
  //     strip.setPixelColor(i, strip.Color(image[coords[i][0]][coords[i][1]][0], image[coords[i][0]][coords[i][1]][1], image[coords[i][0]][coords[i][1]][2]));
  // }
  // strip.show();
  
  // // Mise à jour de l'angle de rotation
  // angle += REFRESH_RADIUS;
  // if (angle >= TOUR_RADIUS){
  //   angle -= TOUR_RADIUS;
  //   haveMakeALoop = false;
  //   durationFast = micros() - startFast;
  //   Serial.println(durationFast);   
  // }
  
  // // delay(DELAY_MS);
}
