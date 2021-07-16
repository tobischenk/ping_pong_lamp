

#ifndef TransferData_h
#define TransferData_h

#include "Arduino.h"

class TransferData {
  public:
    void clear();

    byte processByte;
    byte mode;
    byte brightness;
    byte hours;
    byte minutes;
    byte seconds;
    byte color;
    byte saturation;
};

#endif
