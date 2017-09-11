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

struct w_pair{
	int16_t m;
	int8_t e;
};

// Mantissa of output will have a 1 in bit 15 (bit 16 is sign)
w_pair f2w(float f){
	w_pair w = {0, 0};

	// may need some finesse here - this violates C's strict aliasing rule, which says (generally)
	// that objects can't be referenced by lvalues of different types. An exception to this
	// is character types; that is, I could cast f to a char array. Unfortunately, then I would 
	// have to loop through the array to for each of my bit operations. Shifts also become hard.
	// Should strict aliasing be enforced? Or go for something more direct?
	
	// uint32_t f_b = *(uint32_t*)&f;

	// another option - type punning with unions should be allowed, however the value retrieved is
	// said to be unspecified (not undefined). However, the case where both members of a union
	// have the same size seems to be the safest, which is the case here.
	union {float _f; uint32_t i32;} u = {f};
	uint32_t f_b = u.i32;

	// unbiased exponents of finite floats lie in range 0 <--> 254
	w.e = ((f_b & 0x7F800000) >> 23) - 127;
	if( w.e == -127){
		w.e = 0;
		return w;	// If w.e == -127, the float was 0, so return 0.
	}

	// extract mantissa, add implicit bit & round, then truncate
	int16_t mantissa = ((f_b & 0x007FFFFF) + 0x00800100) >> (8 + 1);

	// Adjust sign 
	// NOTE: Multiplication (or similar) is necessary to ensure that
	// 		 the two's complement is computed
	if ( f_b & 0x80000000 )
		w.m = -1 * mantissa;
	else
		w.m = mantissa;

//	printf("%f\n", f);

	return w;
}

float w2f(w_pair w){
	return w.m*pow(2, w.e - 14);
}

w_pair w_add(w_pair a, w_pair b){
	w_pair r;

	// Ensure that a has exponent at least as large as b
	if( a.e < b.e ){
		w_pair temp = a;
		a = b;
		b = temp;
	}

	// Guaranteed to be nonnegative
	int16_t e_diff = a.e - b.e;
	// if( e_diff > 14){
	// 	printf("WARNING: e_diff > 14 (add)\n");
	// 	printf("e_diff = %i\n", e_diff);
	// 	printf("%f\n", w2f(a));
	// 	printf("%f\n", w2f(b));
	// }


	// Tests for overflow
	// Worthwhile to consider more efficient tests
	// Also, builtin fxn may not be portable to icc
	if( __builtin_add_overflow(a.m, b.m >> e_diff, &r.m) ){
		r.m = (a.m >> 1) + (b.m >> (e_diff + 1));
		r.e = a.e + 1;
	}
	else{
		// __builtin_add_overflow stores sum in r.m during test
		r.e = a.e;
	}

	// if( e_diff > 14 )
	// 	printf("%f\n\n", w2f(r));


	return r;
}

// Fixed truncation from 32 bits (no shifting before multiplication)
w_pair w_mul(w_pair a, w_pair b){
	w_pair r;

	// Final bit shift truncates 15 bits
	// Extra -14 due to both operands having a scale factor of 2^-14
	// r.e = a.e * 2 + (15 - e_diff - 14);
	r.e = a.e + b.e + 15 - 14;

	// There should be a better way to do this
	// Almost half of the time spent in multiplication is wasted
	// Consider _mm_mulhi_pi16 (or modern equivalent) in the future?
	//r.m = (((int32_t)a.m * (int32_t)(b.m >> e_diff)) >> 16 ) & 0x0000FFFF;
	int32_t am, bm, rm, zm;
	int16_t x;
	am = (int32_t)a.m;
	bm = (int32_t)b.m;
	rm = (am*bm);

	// print_bits(4, &rm);
	// printf("\n");

	// the addition here rounds half toward +inf
	// Changed 0x00004000 to 0x00008000
	zm = (rm + 0x00008000) >> 15;
	x = zm & 0x0000FFFF;

	r.m = x;

	return r;
}