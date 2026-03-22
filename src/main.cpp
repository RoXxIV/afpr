#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <xht11.h>

#define BTN_GRN 16
#define BTN_YLW 19
#define BTN_RED 18
#define LED_YLW 4
#define LED_GRN 2
#define LED_BLU 13
#define BUZZER  5

#define CYCLE_DURATION 10000UL

LiquidCrystal_I2C lcd(0x27, 16, 2);
xht11 xht(26);
unsigned char dat[4] = {0, 0, 0, 0};

// Chaque phase du test a son propre état
enum State { IDLE, CHARGE, DISCHARGE, CHARGE_FINAL, PAUSED, ALARM, DONE };

State state     = IDLE;
State prevState = IDLE; // état à restaurer après pause/alarme

unsigned long cycleStart = 0;
unsigned long pausedAt   = 0;

bool btnGrnPrev = HIGH;
bool btnYlwPrev = HIGH;
bool btnRedPrev = HIGH;

// --- Utilitaires ---

void setLeds(bool ylw, bool grn, bool blu)
{
  digitalWrite(LED_YLW, ylw);
  digitalWrite(LED_GRN, grn);
  digitalWrite(LED_BLU, blu);
}

void updateDisplay(int soc, const char* label)
{
  xht.receive(dat);

  lcd.setCursor(0, 0);
  lcd.print("SOC:");
  lcd.print(soc);
  lcd.print("%  T:");
  lcd.print(dat[2]);
  lcd.print("C   ");

  lcd.setCursor(0, 1);
  lcd.print(label);
  lcd.print("              ");
}

// --- Fonction de sécurité : écoute pause et alarme à tout moment ---

void safetyWatch(unsigned long now)
{
  bool btnYlw = digitalRead(BTN_YLW);
  bool btnRed = digitalRead(BTN_RED);

  // Bouton jaune : pause / reprise
  if (btnYlwPrev == HIGH && btnYlw == LOW)
  {
    if (state != IDLE && state != PAUSED && state != ALARM && state != DONE)
    {
      prevState = state;
      state     = PAUSED;
      pausedAt  = now;
      setLeds(false, false, false);
    }
    else if (state == PAUSED)
    {
      cycleStart += now - pausedAt;
      state = prevState;
    }
    delay(50);
  }

  // Bouton rouge : alarme / reprise
  if (btnRedPrev == HIGH && btnRed == LOW)
  {
    if (state != IDLE && state != ALARM && state != DONE)
    {
      prevState = state;
      state     = ALARM;
      pausedAt  = now;
      setLeds(false, false, false);
      digitalWrite(BUZZER, HIGH);
    }
    else if (state == ALARM)
    {
      cycleStart += now - pausedAt;
      digitalWrite(BUZZER, LOW);
      state = prevState;
    }
    delay(50);
  }

  btnYlwPrev = btnYlw;
  btnRedPrev = btnRed;
}

// --- Setup ---

void setup()
{
  pinMode(BTN_GRN, INPUT_PULLUP);
  pinMode(BTN_YLW, INPUT_PULLUP);
  pinMode(BTN_RED,  INPUT_PULLUP);
  pinMode(LED_YLW, OUTPUT);
  pinMode(LED_GRN, OUTPUT);
  pinMode(LED_BLU, OUTPUT);
  pinMode(BUZZER,  OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Test batterie");
  lcd.setCursor(0, 1);
  lcd.print("Appui btn vert");
}

// --- Loop : machine d'état centrale ---

void loop()
{
  unsigned long now = millis();
  safetyWatch(now);

  bool btnGrn = digitalRead(BTN_GRN);

  switch (state)
  {
    case IDLE:
      if (btnGrnPrev == HIGH && btnGrn == LOW)
      {
        state      = CHARGE;
        cycleStart = now;
        delay(50);
      }
      break;

    case CHARGE:
      setLeds(true, false, false);
      int socCharge;
      socCharge = (int)((now - cycleStart) * 100 / CYCLE_DURATION);
      if (socCharge > 100) socCharge = 100;
      updateDisplay(socCharge, "Charge");
      if (now - cycleStart >= CYCLE_DURATION)
      {
        state      = DISCHARGE;
        cycleStart = now;
      }
      break;

    case DISCHARGE:
      setLeds(false, true, false);
      int socDischarge;
      socDischarge = 100 - (int)((now - cycleStart) * 100 / CYCLE_DURATION);
      if (socDischarge < 0) socDischarge = 0;
      updateDisplay(socDischarge, "Decharge");
      if (now - cycleStart >= CYCLE_DURATION)
      {
        state      = CHARGE_FINAL;
        cycleStart = now;
      }
      break;

    case CHARGE_FINAL:
      setLeds(false, false, true);
      int socFinal;
      socFinal = (int)((now - cycleStart) * 100 / CYCLE_DURATION);
      if (socFinal > 100) socFinal = 100;
      updateDisplay(socFinal, "Charge finale");
      if (now - cycleStart >= CYCLE_DURATION)
        state = DONE;
      break;

    case PAUSED:
    case ALARM:
      break;

    case DONE:
      setLeds(true, true, true);
      updateDisplay(100, "Test termine!");
      break;
  }

  btnGrnPrev = btnGrn;
}
