#include "TM1650.h"

// Encodage 7 segments pour 0-9
const uint8_t TM1650::DIGITS[] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66,
  0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

// Adresses des 4 digits sur le TM1650
static const uint8_t DIGIT_ADDR[] = { 0x68, 0x6A, 0x6C, 0x6E };

TM1650::TM1650(int clk, int dio) : _clk(clk), _dio(dio) {}

void TM1650::init()
{
  pinMode(_clk, OUTPUT);
  pinMode(_dio, OUTPUT);
  digitalWrite(_clk, HIGH);
  digitalWrite(_dio, HIGH);

  // Allume l'affichage, luminosité maximale
  sendData(0x48, 0x11);
}

void TM1650::start()
{
  digitalWrite(_dio, HIGH);
  digitalWrite(_clk, HIGH);
  delayMicroseconds(2);
  digitalWrite(_dio, LOW);
  delayMicroseconds(2);
  digitalWrite(_clk, LOW);
}

void TM1650::stop()
{
  digitalWrite(_clk, LOW);
  digitalWrite(_dio, LOW);
  delayMicroseconds(2);
  digitalWrite(_clk, HIGH);
  delayMicroseconds(2);
  digitalWrite(_dio, HIGH);
}

void TM1650::writeByte(uint8_t data)
{
  for (int i = 7; i >= 0; i--)
  {
    digitalWrite(_clk, LOW);
    digitalWrite(_dio, (data >> i) & 0x01);
    delayMicroseconds(2);
    digitalWrite(_clk, HIGH);
    delayMicroseconds(2);
  }
  // ACK
  digitalWrite(_clk, LOW);
  pinMode(_dio, INPUT);
  delayMicroseconds(2);
  digitalWrite(_clk, HIGH);
  delayMicroseconds(2);
  digitalWrite(_clk, LOW);
  pinMode(_dio, OUTPUT);
}

void TM1650::sendData(uint8_t addr, uint8_t data)
{
  start();
  writeByte(addr);
  writeByte(data);
  stop();
}

void TM1650::displayNum(int num)
{
  if (num < 0)   num = 0;
  if (num > 9999) num = 9999;

  int d[4] = {
    (num / 1000) % 10,
    (num / 100)  % 10,
    (num / 10)   % 10,
    (num)        % 10
  };

  for (int i = 0; i < 4; i++)
    sendData(DIGIT_ADDR[i], DIGITS[d[i]]);
}
