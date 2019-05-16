# MysExtension<br/>
MySensors custom extension<br/>
<br/><br/>
Available extensions: <br/>
<br/><br/>
-EXT_SLEEP <br/>
provides exactSleep(const uint32_t sleepingMS, const bool smartSleep = false) method.

<br/><br/>
usage in sketch:
<br/><br/>

#define EXT_SLEEP<br/>
#include <MysExtension.h> //include before MySensors.h<br/>
#include <MySensors.h><br/>
<br/>
void setup() <br/>
{<br/>
    //attachInterrupt(digitalPinToInterrupt(DIG_REED_PIN), onGasPulse, FALLING);<br/>
}<br/>
<br/>
void loop()<br/>
{<br/>
  //body<br/>
   exactSleep(60000);<br/>
}<br/>


