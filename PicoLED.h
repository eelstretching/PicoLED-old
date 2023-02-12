#ifndef __INC_PICOLED_H
#define __INC_PICOLED_H

/// @file FastLED.h
/// central include file for FastLED, defines the CFastLED class/object

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)
#define FASTLED_HAS_PRAGMA_MESSAGE
#endif

/// Current FastLED version number, as an integer.
/// E.g. 3005000 for version "3.5.0", with:
/// * 1 digit for the major version
/// * 3 digits for the minor version
/// * 3 digits for the patch version
#define FASTLED_VERSION 3005000
#ifndef FASTLED_INTERNAL
#  ifdef  FASTLED_SHOW_VERSION
#    ifdef FASTLED_HAS_PRAGMA_MESSAGE
#      pragma message "FastLED version 3.005.000"
#    else
#      warning FastLED version 3.005.000  (Not really a warning, just telling you here.)
#    endif
#  endif
#endif

#include <stdint.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <pico/types.h>

// Utility functions
#include "picoled_delay.h"
#include "bitswap.h"

#include "lib8tion.h"
#include "pixeltypes.h"
#include "hsv2rgb.h"
#include "colorutils.h"
#include "pixelset.h"
#include "colorpalettes.h"
#include "controller.h"

#include "noise.h"
#include "power_mgt.h"

#if !defined(CLOCKLESS_FREQUENCY)
    #define CLOCKLESS_FREQUENCY F_CPU
#endif
#define FMUL (CLOCKLESS_FREQUENCY/8000000)

#define BINARY_DITHER 0x01

#define FL_PROGMEM
#define FL_PGM_READ_BYTE_NEAR(x)  (*((const  uint8_t*)(x)))
#define FL_PGM_READ_WORD_NEAR(x)  (*((const uint16_t*)(x)))
#define FL_PGM_READ_DWORD_NEAR(x) (*((const uint32_t*)(x)))

/// WS2811 controller class @ 800 KHz.
/// @copydetails WS2812Controller800Khz
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class WS2811Controller800Khz : public ClocklessController<DATA_PIN, 3 * FMUL, 4 * FMUL, 3 * FMUL, RGB_ORDER> {};

/// WS2812 controller class @ 800 KHz.
/// @tparam DATA_PIN the data pin for these LEDs
/// @tparam RGB_ORDER the RGB ordering for these LEDs
template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class WS2812Controller800Khz : public ClocklessController<DATA_PIN, 2 * FMUL, 5 * FMUL, 3 * FMUL, RGB_ORDER> {};

template<uint8_t DATA_PIN, EOrder RGB_ORDER> class WS2811 : public WS2811Controller800Khz<DATA_PIN, RGB_ORDER> {};               ///< @copydoc WS2811Controller800Khz

/// LED controller for WS2812 LEDs with GRB color order
/// @see WS2812Controller800Khz
template<uint8_t DATA_PIN> class NEOPIXEL : public WS2812Controller800Khz<DATA_PIN, GRB> {};

/// Unknown NUM_CONTROLLERS definition. Unused elsewhere in the library?
/// @todo Remove?
#define NUM_CONTROLLERS 8

/// Typedef for a power consumption calculation function. Used within
/// CFastLED for rescaling brightness before sending the LED data to
/// the strip with CFastLED::show().
/// @param scale the initial brightness scale value
/// @param data max power data, in milliwatts
/// @returns the brightness scale, limited to max power
typedef uint8_t (*power_func)(uint8_t scale, uint32_t data);

///
/// A function to return an approximation of the number of microseconds since the 
/// program has been running.
/// @returns the number of microseconds since the platform has been running
uint32_t micros() {
	return to_us_since_boot(get_absolute_time());
}

///
/// A function to return an approximation of the number of milliseconds since the 
/// program has been running.
/// @returns the number of milliseconds since the platform has been running
uint32_t millis() {
	return to_ms_since_boot(get_absolute_time());
}

/// High level controller interface for FastLED.
/// This class manages controllers, global settings, and trackings such as brightness
/// and refresh rates, and provides access functions for driving led data to controllers
/// via the show() / showColor() / clear() methods.
/// This is instantiated as a global object with the name FastLED.
/// @nosubgrouping
class PicoLED {
	// int m_nControllers;
	uint8_t  scale;         ///< the current global brightness scale setting
	uint16_t nFPS;          ///< tracking for current frames per second (FPS) value
	uint32_t nMinMicros;    ///< minimum µs between frames, used for capping frame rates
	uint32_t nPowerData;    ///< max power use parameter
	power_func pPowerFunc;  ///< function for overriding brightness when using FastLED.show();

public:
	PicoLED();

	/// Add a CLEDController instance to the world.  Exposed to the public to allow people to implement their own
	/// CLEDController objects or instances.  There are two ways to call this method (as well as the other addLeds()
	/// variations). The first is with 3 arguments, in which case the arguments are the controller, a pointer to
	/// led data, and the number of leds used by this controller.  The second is with 4 arguments, in which case
	/// the first two arguments are the same, the third argument is an offset into the CRGB data where this controller's
	/// CRGB data begins, and the fourth argument is the number of leds for this controller object.
	/// @param pLed the led controller being added
	/// @param data base pointer to an array of CRGB data structures
	/// @param nLedsOrOffset number of leds (3 argument version) or offset into the data array
	/// @param nLedsIfOffset number of leds (4 argument version)
	/// @returns a reference to the added controller
	static CLEDController &addLeds(CLEDController *pLed, struct CRGB *data, int nLedsOrOffset, int nLedsIfOffset = 0);


	/// Set the global brightness scaling
	/// @param scale a 0-255 value for how much to scale all leds before writing them out
	void setBrightness(uint8_t scale) { scale = scale; }

	/// Get the current global brightness setting
	/// @returns the current global brightness value
	uint8_t getBrightness() { return scale; }

	/// Set the maximum power to be used, given in volts and milliamps.
	/// @param volts how many volts the leds are being driven at (usually 5)
	/// @param milliamps the maximum milliamps of power draw you want
	inline void setMaxPowerInVoltsAndMilliamps(uint8_t volts, uint32_t milliamps) { setMaxPowerInMilliWatts(volts * milliamps); }

	/// Set the maximum power to be used, given in milliwatts
	/// @param milliwatts the max power draw desired, in milliwatts
	inline void setMaxPowerInMilliWatts(uint32_t milliwatts) { pPowerFunc = &calculate_max_brightness_for_power_mW; nPowerData = milliwatts; }

	/// Update all our controllers with the current led colors, using the passed in brightness
	/// @param scale the brightness value to use in place of the stored value
	void show(uint8_t scale);

	/// Update all our controllers with the current led colors
	void show() { show(scale); }

	/// Clear the leds, wiping the local array of data. Optionally you can also
	/// send the cleared data to the LEDs.
	/// @param writeData whether or not to write out to the leds as well
	void clear(bool writeData = false);

	/// Clear out the local data array
	void clearData();

	/// Set all leds on all controllers to the given color/scale.
	/// @param color what color to set the leds to
	/// @param scale what brightness scale to show at
	void showColor(const struct CRGB & color, uint8_t scale);

	/// Set all leds on all controllers to the given color
	/// @param color what color to set the leds to
	void showColor(const struct CRGB & color) { showColor(color, scale); }

	/// Delay for the given number of milliseconds.  Provided to allow the library to be used on platforms
	/// that don't have a delay function (to allow code to be more portable). 
	/// @note This will call show() constantly to drive the dithering engine (and will call show() at least once).
	/// @param ms the number of milliseconds to pause for
	void delay(uint32_t ms);

	/// Set a global color temperature.  Sets the color temperature for all added led strips,
	/// overriding whatever previous color temperature those controllers may have had.
	/// @param temp A CRGB structure describing the color temperature
	void setTemperature(const struct CRGB & temp);

	/// Set a global color correction.  Sets the color correction for all added led strips,
	/// overriding whatever previous color correction those controllers may have had.
	/// @param correction A CRGB structure describin the color correction.
	void setCorrection(const struct CRGB & correction);

	/// Set the dithering mode.  Sets the dithering mode for all added led strips, overriding
	/// whatever previous dithering option those controllers may have had.
	/// @param ditherMode what type of dithering to use, either BINARY_DITHER or DISABLE_DITHER
	void setDither(uint8_t ditherMode = BINARY_DITHER);

	/// Set the maximum refresh rate.  This is global for all leds.  Attempts to
	/// call show() faster than this rate will simply wait.
	/// @note The refresh rate defaults to the slowest refresh rate of all the leds added through addLeds().
	/// If you wish to set/override this rate, be sure to call setMaxRefreshRate() _after_
	/// adding all of your leds.
	/// @param refresh maximum refresh rate in hz
	/// @param constrain constrain refresh rate to the slowest speed yet set
	void setMaxRefreshRate(uint16_t refresh, bool constrain=false);

	/// For debugging, this will keep track of time between calls to countFPS(). Every
	/// `nFrames` calls, it will update an internal counter for the current FPS.
	/// @todo Make this a rolling counter
	/// @param nFrames how many frames to time for determining FPS
	void countFPS(int nFrames=25);

	/// Get the number of frames/second being written out
	/// @returns the most recently computed FPS value
	uint16_t getFPS() { return nFPS; }

	/// Get how many controllers have been registered
	/// @returns the number of controllers (strips) that have been added with addLeds()
	int count();

	/// Get a reference to a registered controller
	/// @returns a reference to the Nth controller
	CLEDController & operator[](int x);

	/// Get the number of leds in the first controller
	/// @returns the number of LEDs in the first controller
	int size() { return (*this)[0].size(); }

	/// Get a pointer to led data for the first controller
	/// @returns pointer to the CRGB buffer for the first controller
	CRGB *leds() { return (*this)[0].leds(); }
};


/// Global LED strip management instance
extern PicoLED GPicoLED;

#endif
