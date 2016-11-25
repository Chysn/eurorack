/*
 * chromatic_quantize.h
 *
 *  Created on: Sep 25, 2016
 *  Author: Jason Justian
 */

#ifndef PEAKS_CHROMATIC_QUANTIZE_H_
#define PEAKS_CHROMATIC_QUANTIZE_H_

namespace peaks {

/* How many steps (positive and negative)? */
#define BEIGEMAZE_CHROMATIC_RANGE 13

/* Control value at one volt (positive and negative) */
#define BEIGEMAZE_CHROMATIC_VOLT 4200

inline int16_t chromatic_quantize(int16_t raw) {
	int8_t ix = raw / (32768 / BEIGEMAZE_CHROMATIC_RANGE);
	int32_t note; // Make it big for the fixed-point arithmetic to follow
	if (ix < 0) ix = -ix;
	if (ix > 12) ix = 12; // Range-checking

	/* (((12th root of 2) ** n) - 1) * 100000, where n = note, 0 = tonic */
    /* These are arithmetically-determined coefficients based on
           (((12th root of 2) ** n) - 1) * 100000, where n = note, 0 = tonic
       But they aren't very accurate so see below
	uint32_t coeff [] = {      0,   5946,  12246,  18921,
						   25992,  33484,  41421,  48930,
						   58740,  68179,  78180,  88775,
						  100000 };
    */
	/* These are experimentally-calibrated coefficients: */
	uint32_t coeff [] = {      0,   7300,  16000,  25592,
						   33484,  41421,  49200,  57500,
						   67000,  75500,  84000,  91500,
						  100000 };
	note = (coeff[ix] * BEIGEMAZE_CHROMATIC_VOLT) / coeff[12];
	if (raw < 0) note = -note;

	return static_cast<int16_t>(note);
}

} // namespace peaks

#endif /* PEAKS_CHROMATIC_QUANTIZE_H_ */
