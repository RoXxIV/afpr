#pragma once
#include <Arduino.h>

class xht11
{
public:
  xht11(int pin);
  bool receive(unsigned char* dat); // dat[0]=humidité, dat[2]=température

private:
  int _pin;
};
