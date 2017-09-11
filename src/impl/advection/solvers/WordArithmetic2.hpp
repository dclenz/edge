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

class word16{
	int16_t m;
	int8_t e;
public:	
	word16(float f){
		union {float _f; uint32_t i32;} u = {f};
		uint32_t f_b = u.i32;

		e = ((f_b & 0x7F800000) >> 23) - 127;
		if ( e == -127 ){
			m = 0;
			e = 0;
		}
		else{
			m = ((f_b & 0x007FFFFF) + 0x00800100) >> 9;		// Add implicit bit and round half to +inf
			if ( f_b & 0x80000000 )						// Adjust sign
				m *= -1;
		}
	}
	word16(int16_t _m, int8_t _e){
		m = _m;
		e = _e;
	}
	word16(){}

	float toFloat(){
		return m*pow(2, e - 14);
	}
	
	friend word16 operator+(const word16 &num1, const word16 &num2);
	friend word16 operator*(const word16 &num1, const word16 &num2);

	word16& operator=(const word16 &other);
	word16& operator+=(const word16 &other);
	word16& operator*=(const word16 &other);

};

word16 operator+(const word16 &num1, const word16 &num2){
	int16_t m;
	int8_t e, e_diff;

	if ( num1.e >= num2.e ){
		e_diff = num1.e - num2.e;
		m = num1.m + (num2.m >> e_diff);
		e = num1.e;
	}
	else{
		e_diff = num2.e - num1.e;
		m = (num1.m >> e_diff) + num2.m;
		e = num2.e;
	}

	// Check for Overflow
	if ( ((m^num1.m)&(m^num2.m)) >> 15){
		// m += (m & 0x0002) >> 1;						// Round to even  //***** This is causing sub-float performance earlier on. Why?
		m >>= 1;
		m ^= 0x8000;
		e++;
	}

	return word16(m, e);
}

word16 operator*(const word16 &num1, const word16 &num2){
	int16_t m;
	int8_t e;

	e = num1.e + num2.e + 15 - 14;
	int32_t m1 = num1.m;
	int32_t m2 = num2.m;
	// Normalize Result
	// int32_t prod = m1*m2;
	// if ((int)(prod >> 29) == 0 || (int)(prod >> 29) == -1){
	// 	m = (prod + 0x00004000) >> 14;
	// 	e--;
	// }
	// else
	// 	m = (prod + 0x00008000) >> 15;
	
	// Round half to inf
	m = ((m1 * m2) + 0x00004000) >> 15;

	// Truncate
	// m = (m1 * m2) >> 15;

	return word16(m, e);
}

word16& word16::operator=(const word16 &other){
	m = other.m;
	e = other.e;
	return *this;
}

word16& word16::operator+=(const word16 &other){
	return *this = *this + other;
}

word16& word16::operator*=(const word16 &other){
	return *this = *this * other;
}


class word24{
	int32_t m;
	int8_t e;
public:
	word24(float f){
		union {float _f; uint32_t i32;} u = {f};
		uint32_t f_b = u.i32;

		e = ((f_b & 0x7F800000) >> 23) - 127;
		if ( e == -127 ){
			m = 0;
			e = 0;
		}
		else{
			m = (f_b & 0x007FFFFF) + 0x00800000;
			m = (m + 0x00000001) >> 1;		// Round
			
			if ( f_b & 0x80000000 )						// Adjust sign
				m *= -1;
		}
	}
	word24(int32_t _m, int8_t _e){
		m = _m;
		e = _e;
	}
	word24(){}

	float toFloat(){
		return m*pow(2, e - 22);
	}
	
	friend word24 operator+(const word24 &num1, const word24 &num2);
	friend word24 operator*(const word24 &num1, const word24 &num2);

	word24& operator=(const word24 &other);
	word24& operator+=(const word24 &other);
	word24& operator*=(const word24 &other);
};

word24 operator+(const word24 &num1, const word24 &num2){
	int32_t m;
	int8_t e, e_diff;

	if ( num1.e >= num2.e ){
		e_diff = num1.e - num2.e;
		m = num1.m + (num2.m >> e_diff);
		e = num1.e;
	}
	else{
		e_diff = num2.e - num1.e;
		m = (num1.m >> e_diff) + num2.m;
		e = num2.e;
	}

	// Check for Overflow
	if ( ((m^num1.m)&(m^num2.m)) >> 23){
		// m += (m & 0x0002) >> 1;						// Round to even  //***** This is causing sub-float performance earlier on. Why?
		m >>= 1;
//		m ^= 0x800000;
		e++;
	}

	return word24(m, e);
}

word24 operator*(const word24 &num1, const word24 &num2){
	int32_t m;
	int8_t e;

	e = num1.e + num2.e + 23 - 22;
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
	m = ((m1 * m2) + 0x400000) >> 23;

	// Truncate
	// m = (m1 * m2) >> 15;

	return word24(m, e);
}

word24& word24::operator=(const word24 &other){
	m = other.m;
	e = other.e;
	return *this;
}

word24& word24::operator+=(const word24 &other){
	return *this = *this + other;
}

word24& word24::operator*=(const word24 &other){
	return *this = *this * other;
}