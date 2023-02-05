#ifndef __INC_COLORPALETTES_H
#define __INC_COLORPALETTES_H

#include "PicoLED.h"
#include "colorutils.h"

/// @file colorpalettes.h
/// Declarations for the predefined color palettes supplied by FastLED.

// Have Doxygen ignore these declarations
/// @cond



extern const TProgmemRGBPalette16 CloudColors_p;
extern const TProgmemRGBPalette16 LavaColors_p;
extern const TProgmemRGBPalette16 OceanColors_p;
extern const TProgmemRGBPalette16 ForestColors_p;

extern const TProgmemRGBPalette16 RainbowColors_p;

/// Alias of RainbowStripeColors_p
#define RainbowStripesColors_p RainbowStripeColors_p
extern const TProgmemRGBPalette16 RainbowStripeColors_p;

extern const TProgmemRGBPalette16 PartyColors_p;

extern const TProgmemRGBPalette16 HeatColors_p;


DECLARE_GRADIENT_PALETTE( Rainbow_gp);



/// @endcond

#endif
