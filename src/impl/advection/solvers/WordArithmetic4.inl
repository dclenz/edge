#ifndef W_ARITH4_INL
#define W_ARITH4_INL

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

template <int P>
void wFloat<P>::setBitRepFloat(float f){
	if (P > 24 || P < 1){
		throw "Precision out of bounds";
	}

	union {float _f; uint32_t i32;} u = {f};
	uint32_t f_b = u.i32;

	uint32_t sMask = 0x80000000;
	uint32_t eMask = 0x7F800000;
	uint32_t mMask = 0x007FFFFF;
	// uint8_t eBias = (1 << (8 - 1)) - 1;

	e = ((f_b & eMask) >> 23) - 127;
	if ( e == -127 ){
		m = 0;
		e = 0;
	}
	else if ( P == 24 ){
		m = (f_b & mMask) + 0x00800000;		
		if (f_b & sMask)
			m *= -1;
	}
	else{
		m = (f_b & mMask) + 0x00800000;
		m += (1LL << (23 - P));				// Round
		m >>= (23 - P + 1);					// Truncate
		if (f_b & sMask)
			m *= -1;
	}
}

template <int P>
void wFloat<P>::setBitRepDouble(double d){
	if (P > 31 || P < 1){
		throw "Precision out of bounds";
	}

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
	else if ( P == 53 ){
		m = (d_b & mMask) + 0x0010000000000000;
		if (d_b & sMask)
			m *= -1;
	}
	else{
		m = (d_b &mMask) + 0x0010000000000000;
		m += 1LL << (52 - P);
		m >>= (52 - P + 1);
		if (d_b & sMask)
			m *= -1;
	}
}

template <int P>
wFloat<P>::wFloat(double d){
	setBitRepDouble(d);
}

template <int P>
wFloat<P>::wFloat(float f){
	setBitRepFloat(f);
}

template <int P>
wFloat<P>::wFloat(int i){
	m = i;
	e = P-1;
}

template <int P>
wFloat<P>::wFloat(int64_t _m, int16_t _e){
	m = _m;
	e = _e;
}

template <int P>
float wFloat<P>::toFloat(){
	return m*pow(2, e - (P - 1));
}

template <int Q>
wFloat<Q> operator+(const wFloat<Q> &num1, const wFloat<Q> &num2){
	int64_t m;
	int16_t e, diff;

	// e - p + 1 is the "true" exponent (see toFloat())
	if ( num1.e == num2.e ){
		m = num1.m + num2.m;
		e = max(num1.e, num2.e);
	}
	else if ( num1.e > num2.e ){
		diff = num1.e - num2.e;
		m = num1.m + (num2.m >> diff);
		e = num1.e;
	}
	else{
		diff = num2.e - num1.e;
		m = (num1.m >> diff) + num2.m;
		e = num2.e;
	}

	// Check for Overflow 
	// This breaks if the sign bit is the most significant bit (p=31/63) [uncomment ^= line]
	if ( ((m^num1.m)&(m^num2.m)) >> Q){
		m >>= 1;
//		m ^= 0x800000;
		e++;
	}

	return wFloat<Q>(m, e);
}

template <int Q>
wFloat<Q> operator*(const wFloat<Q> &num1, const wFloat<Q> &num2){
	int64_t m;
	int16_t e;

	// START HERE
	e = num1.e + num2.e + 1;
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
	m = ((m1 * m2) + (1LL << (Q - 1))) >> Q;
	// m = ((m1 * m2) + 0x400000) >> 23;

	// Truncate
	// m = (m1 * m2) >> 15;

	// int32_t m = 0;
	// int8_t e = 0, p = 2;

	return wFloat<Q>(m, e);
}

template <int P>
wFloat<P>& wFloat<P>::operator=(const wFloat<P> &other){
	m = other.m;
	e = other.e;
	return *this;
}

template <int P>
wFloat<P>& wFloat<P>::operator=(const double d){
	setBitRepDouble(d);
	return *this;
}

template <int P>
wFloat<P>& wFloat<P>::operator=(const float f){
	setBitRepFloat(f);
	return *this;
}

template <int P>
wFloat<P>& wFloat<P>::operator+=(const wFloat<P> &other){
	return *this = *this + other;
}

template <int P>
wFloat<P>& wFloat<P>::operator*=(const wFloat<P> &other){
	return *this = *this * other;
}

#endif