volatile bool _wokeUpByWatchDog = false; 
ISR (WDT_vect)
{
	_wokeUpByWatchDog = true;
}
int8_t exactSleep(const uint32_t sleepingMS, const bool smartSleep = false);
int8_t _exactHwSleep(uint32_t ms);
void _exacthwPowerDown(const uint8_t wdto);
void _exacthwInternalSleep(uint32_t ms);