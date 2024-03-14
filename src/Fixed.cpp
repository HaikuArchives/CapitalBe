#include "Fixed.h"

Fixed::Fixed(void)
{
	fValue = 0;
}

Fixed::Fixed(double val)
{
	fValue = (int32)(val * 100.0);
}

Fixed::Fixed(long val, bool premult)
{
	fValue = premult ? val : val * 100;
}

Fixed::Fixed(int val)
{
	fValue = val * 100;
}

float
Fixed::AsFloat(void) const
{
	return float(fValue / 100.0);
}

double
Fixed::AsDouble(void) const
{
	return double(fValue / 100.0);
}

long
Fixed::AsLong(void) const
{
	return fValue / 100;
}

long
Fixed::AsFixed(void) const
{
	return fValue;
}

long
Fixed::IntegerPart(void) const
{
	return fValue / 100;
}

double
Fixed::DecimalPart(void) const
{
	return (fValue / 100.0) - (double)(fValue / 100);
}

Fixed
Fixed::AbsoluteValue(void) const
{
	Fixed f(*this);
	if (f.IsNegative())
		f.Invert();
	return f;
}

void
Fixed::Round(void)
{
	int32 integerpart = (fValue / 100) * 100;
	int32 decimalpart;

	decimalpart = fValue - integerpart;
	fValue -= decimalpart;
	if (fValue > 0) {
		if (decimalpart > 49)
			fValue += 100;
	} else {
		if (decimalpart < -49)
			fValue -= 100;
	}
}

void
Fixed::SetPremultiplied(long value)
{
	fValue = value;
}

void
Fixed::AddPremultiplied(long value)
{
	fValue += value;
}

void
Fixed::SubtractPremultiplied(long value)
{
	fValue += value;
}

Fixed
Fixed::InvertAsCopy(void) const
{
	Fixed f;
	f.SetPremultiplied(-fValue);
	return f;
}

void
Fixed::Invert(void)
{
	fValue = -fValue;
}

bool
Fixed::IsNegative(void) const
{
	return fValue < 0;
}

bool
Fixed::IsPositive(void) const
{
	return fValue > 0;
}

bool
Fixed::IsZero(void) const
{
	return fValue == 0;
}

bool
Fixed::operator!=(const Fixed& from) const
{
	return (fValue != from.fValue);
}

bool
Fixed::operator==(const Fixed& from) const
{
	return (fValue == from.fValue);
}

Fixed
Fixed::operator+(const Fixed& from)
{
	return Fixed(fValue + from.fValue, true);
}

Fixed
Fixed::operator-(const Fixed& from)
{
	return Fixed(fValue - from.fValue, true);
}

Fixed
Fixed::operator+(const double& from)
{
	return Fixed((double(fValue) / 100.0) + from);
}

Fixed
Fixed::operator-(const double& from)
{
	return Fixed((double(fValue) / 100.0) - from);
}

Fixed
Fixed::operator+=(const double& from)
{
	return Fixed(fValue += int32(from * 100));
}

Fixed
Fixed::operator-=(const double& from)
{
	return Fixed(fValue -= int32(from * 100));
}

Fixed
Fixed::operator*=(const double& from)
{
	// Use 64-bit processing to prevent out-of-range errors
	int64 start = fValue * 100;
	int64 mult = int64(from);

	fValue = int32((start * mult) / 100);
	return *this;
}

Fixed
Fixed::operator/=(const double& from)
{
	// Use 64-bit processing to prevent out-of-range errors
	int64 start = fValue * 100;
	int64 div = int64(from * 100.0);

	fValue = int32(start / div);
	return *this;
}

bool
Fixed::operator<(const long& from) const
{
	return fValue < (from * 100);
}

bool
Fixed::operator>(const long& from) const
{
	return fValue > (from * 100);
}

bool
Fixed::operator<(const Fixed& from) const
{
	return fValue < from.fValue;
}

bool
Fixed::operator>(const Fixed& from) const
{
	return fValue > from.fValue;
}

bool
Fixed::operator<=(const Fixed& from) const
{
	return fValue <= from.fValue;
}

bool
Fixed::operator>=(const Fixed& from) const
{
	return fValue >= from.fValue;
}

bool
Fixed::operator==(const long& from) const
{
	return fValue == (from * 100);
}

Fixed&
Fixed::operator+=(const Fixed& from)
{
	fValue += from.fValue;
	return *this;
}

Fixed&
Fixed::operator-=(const Fixed& from)
{
	fValue -= from.fValue;
	return *this;
}

Fixed&
Fixed::operator=(const long& from)
{
	fValue = from * 100;
	return *this;
}
