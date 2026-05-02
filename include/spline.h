#pragma once
#include "typedefs.h"

//***************************************************************************
//
// For each destination X, find pair of source points containing it and interpolate the corresponding
// source Y interval to destination Y
//
//***************************************************************************

void lerp_gen (DOUBLE *src_X,  DOUBLE *src_Y,  S32 src_len,
               DOUBLE *dest_X, DOUBLE *dest_Y, S32 dest_len);

//***************************************************************************
//
// Cubic spline interpolators from Wolberg, Digital Image Warping, p. 293
//
// Alternative version (spline_gen) derived from Numerical Recipes 3rd Edition, p. 121
// has slightly better endpoint behavior
//
//***************************************************************************

void spline_gen (DOUBLE *src_X,  DOUBLE *src_Y,  S32 src_len,
                 DOUBLE *dest_X, DOUBLE *dest_Y, S32 dest_len);

void ispline_gen(DOUBLE *X1, DOUBLE *Y1, S32 len1,
                 DOUBLE *X2, DOUBLE *Y2, S32 len2);

void ispline(DOUBLE *Y1, S32 len1,
             DOUBLE *Y2, S32 len2);

void ispline_t(DOUBLE *Y1, S32 len1,
               DOUBLE *Y2, S32 len2);
