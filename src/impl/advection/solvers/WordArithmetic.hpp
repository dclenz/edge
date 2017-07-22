#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h> 	// for uint32_t
#include <random>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

union word
{
	struct word_w16 {
		int e : 8;
		int s : 8;
		int m : 16;
	} w16;

	struct word_m24 {
		int e: 8;
		int m : 24;
	} w24;

	int32_t word_field;
};

// Mantissa of output will have a 1 in bit 15 (bit 16 is sign)
word f2w(float f){
	word w;

	// Get float into int format
	union {float _f; uint32_t i32;} u = {f};
	uint32_t f_b = u.i32;

	w.w16.e = ((f_b & 0x7F800000) >> 23) - 127;
	if( w.w16.e == -127){
		w.w16.e = 0;
		w.w24.m = 0;
		return w;	// If w.w16.e == -127, the float was 0 (or denormalized), so return 0.
	}

	// ********************************************************

	// ************************
	// **  Cast to 16b int
	// ************************

	// // extract mantissa, add implicit bit & round, then truncate
	// int16_t mantissa = ((f_b & 0x007FFFFF) + 0x00800100) >> (8 + 1);

	// // Adjust sign 
	// // NOTE: Multiplication (or similar) is necessary to ensure that
	// // 		 the two's complement is computed
	// if ( f_b & 0x80000000 )
	// 	w.w16.m = -1 * mantissa;
	// else
	// 	w.w16.m = mantissa;
	// w.w16.s = 0;

	// ********************************************************

	// *************************
	// **  Cast to 24b int
	// *************************

	int32_t mantissa = ((f_b & 0x007FFFFF) + 0x00800000) >> 1;	// Would rounding (instead of truncating) help here? probably doesn't matter
	if (f_b & 0x80000000){
		w.w24.m = -1 * mantissa;
	}
	else
		w.w24.m = mantissa;

	// ********************************************************

	return w;
}

float w2f(word w){
	return w.w24.m*pow(2, w.w24.e - 14 - 8);
//	return w.w16.m*pow(2, w.w16.e - 14);
}

word w_add(word a, word b){
	word r;

	// Ensure that a has exponent at least as large as b
	if( a.w16.e < b.w16.e ){
		word temp = a;
		a = b;
		b = temp;
	}

	// Guaranteed to be nonnegative
	int16_t e_diff = a.w16.e - b.w16.e;

	// Warning: No rounding before the (eventual?)
	// truncation to 16 bits
	r.w24.e = 0;
	r.w24.m = (a.w24.m) + (b.w24.m >> (e_diff));
	if (((r.w16.m^a.w16.m)&(r.w16.m^b.w16.m))>>15){ // If the addition overflowed
		r.w24.m >>= 1;								// Then recover the 
		r.w24.m ^= 0x800000;						//		overflowed bit (discarding low bit)
		r.w24.e = 1;								// And increment exponent
	}

	r.w24.e += a.w24.e;								// Add in larger exponent

	return r;
}


/// Fixed truncation from 32 bits (no shifting before multiplication)
word w_mul(word a, word b){
	word r;

	// Final bit shift truncates 15 bits
	// Extra -14 due to both operands having a scale factor of 2^-14
	// r.e = a.e * 2 + (15 - e_diff - 14);
	r.w16.e = a.w16.e + b.w16.e + 15 - 14;

	// There should be a better way to do this
	// Almost half of the time spent in multiplication is wasted
	// Consider _mm_mulhi_pi16 (or modern equivalent) in the future?
	//r.m = (((int32_t)a.m * (int32_t)(b.m >> e_diff)) >> 16 ) & 0x0000FFFF;
	int32_t am, bm, rm, zm;
	int16_t x;
	am = (int32_t)a.w16.m;
	bm = (int32_t)b.w16.m;
	rm = (am*bm);

	// print_bits(4, &rm);
	// printf("\n");

	// the addition here rounds half toward +inf
	zm = (rm + 0x00004000) >> 15;
	x = zm & 0x0000FFFF;

	r.w16.m = x;

	return r;
}