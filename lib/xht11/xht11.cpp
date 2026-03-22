#include "xht11.h"

xht11::xht11(int pin) : _pin(pin) {}

bool xht11::receive(unsigned char* dat)
{
  unsigned char data[5] = {0, 0, 0, 0, 0};

  // Signal de départ : pull low 18ms puis relâche
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  delay(18);
  digitalWrite(_pin, HIGH);
  delayMicroseconds(30);
  pinMode(_pin, INPUT_PULLUP);

  // Attente réponse capteur
  if (pulseIn(_pin, HIGH, 100000) == 0) return false;

  // Lecture des 40 bits
  for (int i = 0; i < 40; i++)
  {
    unsigned long duration = pulseIn(_pin, HIGH, 100000);
    if (duration == 0) return false;
    data[i / 8] <<= 1;
    if (duration > 50) data[i / 8] |= 1; // >50µs = bit 1
  }

  // Vérification checksum
  if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
    return false;

  dat[0] = data[0]; // humidité (partie entière)
  dat[1] = data[1]; // humidité (décimale)
  dat[2] = data[2]; // température (partie entière)
  dat[3] = data[3]; // température (décimale)
  return true;
}
