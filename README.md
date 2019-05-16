# MysExtension
MySensors custom extension

Available extensions: 

-EXT_SLEEP 
provides exactSleep(const uint32_t sleepingMS, const bool smartSleep = false) method.


usage in sketch:


#define EXT_SLEEP
#include <MysExtension.h> //include before MySensors.h
#include <MySensors.h>

void setup() 
{
    //attachInterrupt(digitalPinToInterrupt(DIG_REED_PIN), onGasPulse, FALLING);
}

void loop()
{
  //body
   exactSleep(60000);
}


