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

template <int P>
class wFloat{
public:
	int64_t m;
	int16_t e;

	wFloat(double d);		
	wFloat(float f);
	wFloat(int64_t _m, int16_t _e);
	wFloat(){}

	float toFloat();

	template <int Q>
	friend wFloat<Q> operator+(const wFloat<Q> &num1, const wFloat<Q> &num2);

	template <int Q>
	friend wFloat<Q> operator*(const wFloat<Q> &num1, const wFloat<Q> &num2);

	wFloat<P>& operator=(const wFloat<P> &other);
	wFloat<P>& operator=(const double x);
	wFloat<P>& operator=(const float x);
	wFloat<P>& operator=(const int x);
	wFloat<P>& operator+=(const wFloat<P> &other);
	wFloat<P>& operator*=(const wFloat<P> & other);

	void setBitRepFloat(float f);
	void setBitRepDouble(double d);
};

#include "WordArithmetic4.inl"