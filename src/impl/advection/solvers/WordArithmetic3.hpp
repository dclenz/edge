#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <random>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

//assumes little endian
void print_bits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i=size-1;i>=0;i--)
    {
        for (j=7;j>=0;j--)
        {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
        printf(" ");
    }
    puts("");
}

class wFloat{
public:
	int64_t m;
	int16_t e;
	int8_t p;

	wFloat(double d, int8_t _p){
		if (_p > 53 || _p < 1){
			throw "Precision out of bounds";
		}
		p = _p;

		union {double _d; uint64_t i64;} u = {d};
		uint64_t d_b = u.i64;

		uint64_t sMask = 0x8000000000000000;
		uint64_t eMask = 0x7FF0000000000000;
		uint64_t mMask = 0x000FFFFFFFFFFFFF;

		e = ((d_b & eMask) >> 52) - 1023;
		if ( e == -1023 ){
			m = 0;
			e = 0;
		}
		else if ( p == 53 ){
			m = (d_b & mMask) + 0x0010000000000000;
			if (d_b & sMask)
				m *= -1;
		}
		else{
			m = (d_b &mMask) + 0x0010000000000000;
			m += 1 << (52 - p);
			m >>= (52 - p + 1);
			if (d_b & sMask)
				m *= -1;
		}
	}
	wFloat(float f, int8_t _p){
		if (_p > 24 || _p < 1){
			throw "Precision out of bounds";
		}
		p = _p;

		union {float _f; uint32_t i32;} u = {f};
		uint32_t f_b = u.i32;

		uint32_t eMask = 0x7F800000;
		uint32_t mMask = 0x007FFFFF;
		// uint8_t eBias = (1 << (8 - 1)) - 1;

		e = ((f_b & eMask) >> 23) - 127;
		if ( e == -127 ){
			m = 0;
			e = 0;
		}
		else if ( p == 24 ){
			m = (f_b & mMask) + 0x00800000;		
			if (f_b & 0x80000000)
				m *= -1;
		}
		else{
			m = (f_b & mMask) + 0x00800000;
			m += (1 << (23 - p));				// Round
			m >>= (23 - p + 1);					// Truncate
			if (f_b & 0x80000000)
				m *= -1;
		}
	}
	wFloat(int64_t _m, int16_t _e, int8_t _p){
		m = _m;
		e = _e;
		p = _p;
	}
	wFloat(){}

	float toFloat(){
		return m*pow(2, e - (p - 1));
	}

	void setP(int8_t _p){
		if ( _p <= p){			// Decrease Precision
			m >>= (p - _p);
			p = _p;
		}
		else{					// Increase Precision
			m <<= (_p - p);
			p = _p;
		}
	}

	friend wFloat operator+(const wFloat &num1, const wFloat &num2);
	friend wFloat operator*(const wFloat &num1, const wFloat &num2);

	wFloat& operator=(const wFloat &other);
	wFloat& operator+=(const wFloat &other);
	wFloat& operator*=(const wFloat & other);
};

wFloat operator+(const wFloat &num1, const wFloat &num2){
	int64_t m;
	int16_t e, diff, p;

	// e - p + 1 is the "true" exponent (see toFloat())
	if ( num1.e - num1.p == num2.e - num2.p ){
		m = num1.m + num2.m;
		e = max(num1.e, num2.e);
		p = max(num1.p, num2.p);
	}
	else if ( num1.e - num1.p > num2.e - num2.p ){
		diff = (num1.e - num1.p) - (num2.e - num2.p);
		m = num1.m + (num2.m >> diff);
		e = num1.e;
		p = num1.p;
	}
	else{
		diff = (num2.e - num2.p) - (num1.e - num1.p);
		m = (num1.m >> diff) + num2.m;
		e = num2.e;
		p = num2.p;
	}

	// Check for Overflow 
	// This breaks if the sign bit is the most significant bit (p=31/63) [uncomment ^= line]
	if ( ((m^num1.m)&(m^num2.m)) >> p){
		m >>= 1;
//		m ^= 0x800000;
		e++;
	}

	return wFloat(m, e, p);
}


wFloat operator*(const wFloat &num1, const wFloat &num2){
	int64_t m;
	int16_t e, pMin, pMax;

	if ( num1.p < num2.p ){
		pMin = num1.p;
		pMax = num2.p;
	}
	else{
		pMin = num2.p;
		pMax = num1.p;
	}

	// START HERE
	e = num1.e + num2.e - pMax + 1 + pMax;
	int64_t m1 = num1.m;
	int64_t m2 = num2.m;
	// Normalize Result
	// int32_t prod = m1*m2;
	// if ((int)(prod >> 29) == 0 || (int)(prod >> 29) == -1){
	// 	m = (prod + 0x00004000) >> 14;
	// 	e--;
	// }
	// else
	// 	m = (prod + 0x00008000) >> 15;
	
	// Round half to inf
	m = ((m1 * m2) + (1 << (pMax - 1))) >> pMax;
	// m = ((m1 * m2) + 0x400000) >> 23;

	// Truncate
	// m = (m1 * m2) >> 15;

	// int32_t m = 0;
	// int8_t e = 0, p = 2;

	return wFloat(m, e, pMin);
}

wFloat& wFloat::operator=(const wFloat &other){
	m = other.m;
	e = other.e;
	p = other.p;
	return *this;
}

wFloat& wFloat::operator+=(const wFloat &other){
	return *this = *this + other;
}

wFloat& wFloat::operator*=(const wFloat &other){
	return *this = *this * other;
}