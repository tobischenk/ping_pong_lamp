#include "Arduino.h"
#include "TransferData.h"


void TransferData::clear(){
  processByte = 0;
  mode = 0;
  brightness = 0;
  hours = 0;
  minutes = 0;
  seconds = 0;
  color = 0;
  saturation = 0;
}
