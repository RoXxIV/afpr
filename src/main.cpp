#include <Arduino.h>

#define LED_RED 5
#define BTN_RED 18
#define LED_YLW 4
#define BTN_YLW 19
#define LED_GRN 2
#define LED_BLU 13
#define POT     34

bool ylwLedState = false;
bool ylwBtnPrev  = HIGH;

bool grnLedState        = false;
unsigned long grnLastToggle = 0;

void setup()
{
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YLW, OUTPUT);
  pinMode(LED_GRN, OUTPUT);
  pinMode(BTN_RED, INPUT_PULLUP);
  pinMode(BTN_YLW, INPUT_PULLUP);

  // PWM 1000Hz, 8 bits (0-255)
  ledcAttach(LED_BLU, 1000, 8);
}

void loop()
{
  // LED rouge : maintien bouton
  digitalWrite(LED_RED, digitalRead(BTN_RED) == LOW ? HIGH : LOW);

  // LED jaune : toggle sur front descendant
  bool ylwBtnCurrent = digitalRead(BTN_YLW);
  if (ylwBtnPrev == HIGH && ylwBtnCurrent == LOW)
  {
    ylwLedState = !ylwLedState;
    digitalWrite(LED_YLW, ylwLedState);
    delay(50); // anti-rebond
  }
  ylwBtnPrev = ylwBtnCurrent;

  // LED verte : clignotement 1s sans blocage
  unsigned long maintenant = millis();
  if (maintenant - grnLastToggle >= 1000)
  {
    grnLastToggle = maintenant;
    grnLedState   = !grnLedState;
    digitalWrite(LED_GRN, grnLedState);
  }

  // LED bleue : intensité via potentiomètre
  ledcWrite(LED_BLU, map(analogRead(POT), 0, 4095, 0, 255));
}
