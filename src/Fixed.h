#ifndef FIXED_H
#define FIXED_H

#include <SupportDefs.h>

// Fixed point class accurate to 2 decimal places

class Fixed {
public:
	Fixed(void);
	Fixed(double val);
	Fixed(long val, bool premult = false);
	Fixed(int val);

	float AsFloat(void) const;
	double AsDouble(void) const;
	long AsLong(void) const;
	long AsFixed(void) const;
	long IntegerPart(void) const;
	double DecimalPart(void) const;
	void SetPremultiplied(long value);
	void AddPremultiplied(long value);
	void SubtractPremultiplied(long value);
	Fixed InvertAsCopy(void) const;
	void Invert(void);
	Fixed AbsoluteValue(void) const;
	void Round(void);

	bool IsNegative(void) const;
	bool IsPositive(void) const;
	bool IsZero(void) const;

	Fixed operator+(const Fixed& from);
	Fixed operator-(const Fixed& from);
	bool operator<(const Fixed& from) const;
	bool operator>(const Fixed& from) const;
	bool operator<=(const Fixed& from) const;
	bool operator>=(const Fixed& from) const;
	bool operator!=(const Fixed& from) const;
	bool operator==(const Fixed& from) const;

	Fixed operator+(const double& from);
	Fixed operator-(const double& from);
	Fixed operator+=(const double& from);
	Fixed operator-=(const double& from);
	Fixed operator*=(const double& from);
	Fixed operator/=(const double& from);
	bool operator<(const long& from) const;
	bool operator>(const long& from) const;
	bool operator==(const long& from) const;

	Fixed& operator+=(const Fixed& from);
	Fixed& operator-=(const Fixed& from);
	Fixed& operator=(const long& from);

private:
	int32 fValue;
};

#endif
