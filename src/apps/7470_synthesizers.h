#ifndef HP7470_SYNTHESIZERS_H
#define HP7470_SYNTHESIZERS_H

#include "typedefs.h"

//
// Synthesis functions to convert binary trace data from various instruments
// to HP-GL/2 format
//

C8 *synthesize_492P(S32 device_address);
C8 *synthesize_856xA(S32 device_address, bool is_278X);
C8 *synthesize_3585A(S32 device_address);
C8 *synthesize_generic_SA(S32 device_address, bool is_SCPI);

#endif
