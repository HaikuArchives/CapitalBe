/*
 * Copyright 2009. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 */
#ifndef FIXED_H
#define FIXED_H

#include <SupportDefs.h>

// Fixed point class accurate to 2 decimal places

class Fixed {
public:
	Fixed();
	Fixed(double val);
	Fixed(long val, bool premult = false);
	Fixed(int val);

	float AsFloat() const;
	double AsDouble() const;
	long AsLong() const;
	long AsFixed() const;
	long IntegerPart() const;
	double DecimalPart() const;
	void SetPremultiplied(long value);
	void AddPremultiplied(long value);
	void SubtractPremultiplied(long value);
	Fixed InvertAsCopy() const;
	void Invert();
	Fixed AbsoluteValue() const;
	void Round();

	bool IsNegative() const;
	bool IsPositive() const;
	bool IsZero() const;

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

#endif // FIXED_H
