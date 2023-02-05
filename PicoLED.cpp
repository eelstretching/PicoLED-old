#define FASTLED_INTERNAL
#include "PicoLED.h"

#include <pico/stdlib.h>
#include <hardware/timer.h>

/// @file PicoLED.cpp
/// Central source file for FastLED, implements the CFastLED class/object



/// Pointer to the matrix object when using the Smart Matrix Library
/// @see https://github.com/pixelmatix/SmartMatrix
void *pSmartMatrix = NULL;

CLEDController *CLEDController::m_pHead = NULL;
CLEDController *CLEDController::m_pTail = NULL;
static uint32_t lastshow = 0;

/// Global frame counter, used for debugging ESP implementations
/// @todo Include in FASTLED_DEBUG_COUNT_FRAME_RETRIES block?
uint32_t _frame_cnt=0;

/// Global frame retry counter, used for debugging ESP implementations
/// @todo Include in FASTLED_DEBUG_COUNT_FRAME_RETRIES block?
uint32_t _retry_cnt=0;

// uint32_t CRGB::Squant = ((uint32_t)((__TIME__[4]-'0') * 28))<<16 | ((__TIME__[6]-'0')*50)<<8 | ((__TIME__[7]-'0')*28);

PicoLED::PicoLED() {
	// clear out the array of led controllers
	// m_nControllers = 0;
	scale = 255;
	nFPS = 0;
	pPowerFunc = NULL;
	nPowerData = 0xFFFFFFFF;
}

CLEDController &PicoLED::addLeds(CLEDController *pLed,
								  struct CRGB *data,
								  int nLedsOrOffset, int nLedsIfOffset) {
	int nOffset = (nLedsIfOffset > 0) ? nLedsOrOffset : 0;
	int nLeds = (nLedsIfOffset > 0) ? nLedsIfOffset : nLedsOrOffset;

	pLed->init();
	pLed->setLeds(data + nOffset, nLeds);
	GPicoLED.setMaxRefreshRate(pLed->getMaxRefreshRate(),true);
	return *pLed;
}

void PicoLED::show(uint8_t scale) {
	// guard against showing too rapidly
	while(nMinMicros && ((micros()-lastshow) < nMinMicros));
	lastshow = micros();

	// If we have a function for computing power, use it!
	if(pPowerFunc) {
		scale = (*pPowerFunc)(scale, nPowerData);
	}

	CLEDController *pCur = CLEDController::head();
	while(pCur) {
		uint8_t d = pCur->getDither();
		if(nFPS < 100) { pCur->setDither(0); }
		pCur->showLeds(scale);
		pCur->setDither(d);
		pCur = pCur->next();
	}
	countFPS();
}

int PicoLED::count() {
    int x = 0;
	CLEDController *pCur = CLEDController::head();
	while( pCur) {
        ++x;
		pCur = pCur->next();
	}
    return x;
}

CLEDController & PicoLED::operator[](int x) {
	CLEDController *pCur = CLEDController::head();
	while(x-- && pCur) {
		pCur = pCur->next();
	}
	if(pCur == NULL) {
		return *(CLEDController::head());
	} else {
		return *pCur;
	}
}

void PicoLED::showColor(const struct CRGB & color, uint8_t scale) {
	while(nMinMicros && ((micros()-lastshow) < nMinMicros));
	lastshow = micros();

	// If we have a function for computing power, use it!
	if(pPowerFunc) {
		scale = (*pPowerFunc)(scale, nPowerData);
	}

	CLEDController *pCur = CLEDController::head();
	while(pCur) {
		uint8_t d = pCur->getDither();
		if(nFPS < 100) { pCur->setDither(0); }
		pCur->showColor(color, scale);
		pCur->setDither(d);
		pCur = pCur->next();
	}
	countFPS();
}

void PicoLED::clear(bool writeData) {
	if(writeData) {
		showColor(CRGB(0,0,0), 0);
	}
    clearData();
}

void PicoLED::clearData() {
	CLEDController *pCur = CLEDController::head();
	while(pCur) {
		pCur->clearLedData();
		pCur = pCur->next();
	}
}

void PicoLED::delay(uint32_t delay_ms) {
    if (delay_ms <= 0x7fffffffu / 1000) {
        busy_wait_us_32(delay_ms * 1000);
    } else {
        busy_wait_us(delay_ms * 1000ull);
    }
}

void PicoLED::setTemperature(const struct CRGB & temp) {
	CLEDController *pCur = CLEDController::head();
	while(pCur) {
		pCur->setTemperature(temp);
		pCur = pCur->next();
	}
}

void PicoLED::setCorrection(const struct CRGB & correction) {
	CLEDController *pCur = CLEDController::head();
	while(pCur) {
		pCur->setCorrection(correction);
		pCur = pCur->next();
	}
}

void PicoLED::setDither(uint8_t ditherMode)  {
	CLEDController *pCur = CLEDController::head();
	while(pCur) {
		pCur->setDither(ditherMode);
		pCur = pCur->next();
	}
}

//
// template<int m, int n> void transpose8(unsigned char A[8], unsigned char B[8]) {
// 	uint32_t x, y, t;
//
// 	// Load the array and pack it into x and y.
//   	y = *(unsigned int*)(A);
// 	x = *(unsigned int*)(A+4);
//
// 	// x = (A[0]<<24)   | (A[m]<<16)   | (A[2*m]<<8) | A[3*m];
// 	// y = (A[4*m]<<24) | (A[5*m]<<16) | (A[6*m]<<8) | A[7*m];
//
        // // pre-transform x
        // t = (x ^ (x >> 7)) & 0x00AA00AA;  x = x ^ t ^ (t << 7);
        // t = (x ^ (x >>14)) & 0x0000CCCC;  x = x ^ t ^ (t <<14);
				//
        // // pre-transform y
        // t = (y ^ (y >> 7)) & 0x00AA00AA;  y = y ^ t ^ (t << 7);
        // t = (y ^ (y >>14)) & 0x0000CCCC;  y = y ^ t ^ (t <<14);
				//
        // // final transform
        // t = (x & 0xF0F0F0F0) | ((y >> 4) & 0x0F0F0F0F);
        // y = ((x << 4) & 0xF0F0F0F0) | (y & 0x0F0F0F0F);
        // x = t;
//
// 	B[7*n] = y; y >>= 8;
// 	B[6*n] = y; y >>= 8;
// 	B[5*n] = y; y >>= 8;
// 	B[4*n] = y;
//
//   B[3*n] = x; x >>= 8;
// 	B[2*n] = x; x >>= 8;
// 	B[n] = x; x >>= 8;
// 	B[0] = x;
// 	// B[0]=x>>24;    B[n]=x>>16;    B[2*n]=x>>8;  B[3*n]=x>>0;
// 	// B[4*n]=y>>24;  B[5*n]=y>>16;  B[6*n]=y>>8;  B[7*n]=y>>0;
// }
//
// void transposeLines(Lines & out, Lines & in) {
// 	transpose8<1,2>(in.bytes, out.bytes);
// 	transpose8<1,2>(in.bytes + 8, out.bytes + 1);
// }


/// Unused value
/// @todo Remove?
extern int noise_min;

/// Unused value
/// @todo Remove?
extern int noise_max;

void PicoLED::countFPS(int nFrames) {
	static int br = 0;
	static uint32_t lastframe = 0; // millis();

	if(br++ >= nFrames) {
		uint32_t now = millis();
		now -= lastframe;
		if(now == 0) {
			now = 1; // prevent division by zero below
		}
		nFPS = (br * 1000) / now;
		br = 0;
		lastframe = millis();
	}
}

void PicoLED::setMaxRefreshRate(uint16_t refresh, bool constrain) {
	if(constrain) {
		// if we're constraining, the new value of m_nMinMicros _must_ be higher than previously (because we're only
		// allowed to slow things down if constraining)
		if(refresh > 0) {
			nMinMicros = ((1000000 / refresh) > nMinMicros) ? (1000000 / refresh) : nMinMicros;
		}
	} else if(refresh > 0) {
		nMinMicros = 1000000 / refresh;
	} else {
		nMinMicros = 0;
	}
}

/// Called at program exit when run in a desktop environment. 
/// Extra C definition that some environments may need. 
/// @returns 0 to indicate success
extern "C" int atexit(void (* /*func*/ )()) { return 0; }

#ifdef FASTLED_NEEDS_YIELD
extern "C" void yield(void) { }
#endif

#ifdef NEED_CXX_BITS
namespace __cxxabiv1
{
	#if !defined(ESP8266) && !defined(ESP32)
	extern "C" void __cxa_pure_virtual (void) {}
	#endif

	/* guard variables */

	/* The ABI requires a 64-bit type.  */
	__extension__ typedef int __guard __attribute__((mode(__DI__)));

	extern "C" int __cxa_guard_acquire (__guard *) __attribute__((weak));
	extern "C" void __cxa_guard_release (__guard *) __attribute__((weak));
	extern "C" void __cxa_guard_abort (__guard *) __attribute__((weak));

	extern "C" int __cxa_guard_acquire (__guard *g)
	{
		return !*(char *)(g);
	}

	extern "C" void __cxa_guard_release (__guard *g)
	{
		*(char *)g = 1;
	}

	extern "C" void __cxa_guard_abort (__guard *)
	{

	}
}
#endif


