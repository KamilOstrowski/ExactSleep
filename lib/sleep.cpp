#undef WDT_vect
#include <MySensors.h>

int8_t exactSleep(const uint32_t sleepingMS, const bool smartSleep)
{
	CORE_DEBUG(PSTR("MCO:SLP:MS=%" PRIu32 ",SMS=%" PRIu8 "\n"), sleepingMS, smartSleep);
	// repeater feature: sleeping not possible
#if defined(MY_REPEATER_FEATURE)
	(void)smartSleep;
	

	CORE_DEBUG(PSTR("!MCO:SLP:REP\n"));	// sleeping not possible, repeater feature enabled
	wait(sleepingMS);
	return MY_SLEEP_NOT_POSSIBLE;
#else
	uint32_t sleepingTimeMS = sleepingMS;
#if defined(MY_SENSOR_NETWORK)
	// Do not sleep if transport not ready
	if (!isTransportReady()) {
		CORE_DEBUG(PSTR("!MCO:SLP:TNR\n"));	// sleeping not possible, transport not ready
		const uint32_t sleepEnterMS = hwMillis();
		uint32_t sleepDeltaMS = 0;
		while (!isTransportReady() && (sleepDeltaMS < sleepingTimeMS) &&
		        (sleepDeltaMS < MY_SLEEP_TRANSPORT_RECONNECT_TIMEOUT_MS)) {
			_process();
			sleepDeltaMS = hwMillis() - sleepEnterMS;
		}
		// sleep remainder
		if (sleepDeltaMS < sleepingTimeMS) {
			sleepingTimeMS -= sleepDeltaMS;		// calculate remaining sleeping time
			CORE_DEBUG(PSTR("MCO:SLP:MS=%" PRIu32 "\n"), sleepingTimeMS);
		} else {
			// no sleeping time left
			return MY_SLEEP_NOT_POSSIBLE;
		}
	}
	// OTA FW feature: do not sleep if FW update ongoing
#if defined(MY_OTA_FIRMWARE_FEATURE)
	while (isFirmwareUpdateOngoing() && sleepingTimeMS) {
		CORE_DEBUG(PSTR("!MCO:SLP:FWUPD\n"));	// sleeping not possible, FW update ongoing
		wait(1000ul);
		sleepingTimeMS = sleepingTimeMS >= 1000ul ? sleepingTimeMS - 1000ul : 1000ul;
	}
#endif // MY_OTA_FIRMWARE_FEATURE
	if (smartSleep) {
		// sleeping time left?
		if (sleepingTimeMS > 0 && sleepingTimeMS < ((uint32_t)MY_SMART_SLEEP_WAIT_DURATION_MS)) {
			wait(sleepingMS);
			CORE_DEBUG(PSTR("!MCO:SLP:NTL\n"));	// sleeping not possible, no time left
			return MY_SLEEP_NOT_POSSIBLE;
		}
		// notify controller about going to sleep, payload indicates smartsleep waiting time in MS
		(void)_sendRoute(build(_msgTmp, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL,
		                       I_PRE_SLEEP_NOTIFICATION).set((uint32_t)MY_SMART_SLEEP_WAIT_DURATION_MS));
		wait(MY_SMART_SLEEP_WAIT_DURATION_MS);		// listen for incoming messages
#if defined(MY_OTA_FIRMWARE_FEATURE)
		// check if during smart sleep waiting period a FOTA request was received
		if (isFirmwareUpdateOngoing()) {
			CORE_DEBUG(PSTR("!MCO:SLP:FWUPD\n"));	// sleeping not possible, FW update ongoing
			return MY_SLEEP_NOT_POSSIBLE;
		}
#endif // MY_OTA_FIRMWARE_FEATURE
	}
#else
	(void)smartSleep;
#endif // MY_SENSOR_NETWORK

#if defined(MY_SENSOR_NETWORK)
	transportDisable();
#endif
	setIndication(INDICATION_SLEEP);

#if defined (MY_DEFAULT_TX_LED_PIN) || defined(MY_DEFAULT_RX_LED_PIN) || defined(MY_DEFAULT_ERR_LED_PIN)
	// Wait until leds finish their blinking pattern
	while (ledsBlinking()) {
		doYield();
	}
#endif

	int8_t result = MY_SLEEP_NOT_POSSIBLE;	// default
	
	// no IRQ		
	result = _exactHwSleep(sleepingTimeMS);
	
	setIndication(INDICATION_WAKEUP);
	CORE_DEBUG(PSTR("MCO:SLP:WUP=%" PRIi8 "\n"), result);	// sleep wake-up
#if defined(MY_SENSOR_NETWORK)
	transportReInitialise();
#endif
	if (smartSleep) {
		// notify controller about waking up, payload indicates sleeping time in MS
		(void)_sendRoute(build(_msgTmp, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL,
		                       I_POST_SLEEP_NOTIFICATION).set(sleepingTimeMS));
	}
	return result;
#endif
}

int8_t _exactHwSleep(uint32_t ms)
{
	// Return what woke the mcu.
	// Default: no interrupt triggered, timer wake up
	
	if (ms > 0u) {
		// sleep for defined time
		_exacthwInternalSleep(ms);
	} else {
		// sleep until ext interrupt triggered
		_exacthwPowerDown(WDTO_SLEEP_FOREVER);
	}
	
	return MY_WAKE_UP_BY_TIMER;
}

void _exacthwPowerDown(const uint8_t wdto)
{

	// disable ADC for power saving
	ADCSRA &= ~(1 << ADEN);
	// save WDT settings
	const uint8_t WDTsave = WDTCSR;
	
	if (wdto != WDTO_SLEEP_FOREVER) {
		wdt_enable(wdto);
		// enable WDT interrupt before system reset
		WDTCSR |= (1 << WDCE) | (1 << WDIE);
	} else {
		// if sleeping forever, disable WDT
		wdt_disable();
	}
	
	
	do {
		
	if (wdto != WDTO_SLEEP_FOREVER) {
		WDTCSR |= (1 << WDCE) | (1 << WDIE); //reenable wd interrupt
	}
		
	// Let serial prints finish (debug, log etc)
#ifndef MY_DISABLED_SERIAL
	MY_SERIALDEVICE.flush();
#endif
	
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	cli();  // clear interrupts flag
	sleep_enable();
#if defined(__AVR_ATmega328P__)
	sleep_bod_disable();
#endif

_wokeUpByWatchDog = false; //reset flag before sleep;

	// Enable interrupts & sleep until WDT or ext. interrupt
	sei(); // set interrupts flag
	// Directly sleep CPU, to prevent race conditions!
	// Ref: chapter 7.7 of ATMega328P datasheet
	
if (_wokeUpByWatchDog)	
{
	cli(); // clear interrupts flag
	break;
}

	sleep_cpu();
	sleep_disable();
	// restore previous WDT settings	

	cli(); // clear interrupts flag

	 
	
	} while (!_wokeUpByWatchDog);
	
	wdt_reset();
	
	// enable WDT changes
	WDTCSR |= (1 << WDCE) | (1 << WDE);
	// restore saved WDT settings
	WDTCSR = WDTsave;
	
	
	sei(); // set interrupts flag
	// enable ADC
	ADCSRA |= (1 << ADEN);
}

void _exacthwInternalSleep(uint32_t ms)
{
	// Sleeping with watchdog only supports multiples of 16ms.
	// Round up to next multiple of 16ms, to assure we sleep at least the
	// requested amount of time. Sleep of 0ms will not sleep at all!
	ms += 15u;
	while (/*!interruptWakeUp() &&*/ ms >= 16) {
		for (uint8_t period = 9u; ; --period) {
			const uint16_t comparatorMS = 1 << (period + 4);
			if ( ms >= comparatorMS) {
				_exacthwPowerDown(period); // 8192ms => 9, 16ms => 0
				
				ms -= comparatorMS;
				break;
			}
		}
	}
	
}