#pragma once
#include <Arduino.h>

class TM1650
{
public:
  TM1650(int clk, int dio);
  void init();
  void displayNum(int num); // affiche un entier sur 4 chiffres

private:
  int _clk, _dio;

  void start();
  void stop();
  void writeByte(uint8_t data);
  void sendData(uint8_t addr, uint8_t data);

  static const uint8_t DIGITS[];
};
