/*
 * Copyright 2009. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 */
#ifndef CBLOCALE_H
#define CBLOCALE_H

#include "Fixed.h"

#include <Alert.h>
#include <File.h>
#include <String.h>
#include <TextControl.h>

#include <time.h>

class Locale {
public:
	Locale();
	~Locale();

	bool operator==(const Locale& other) const;
	bool operator!=(const Locale& other) const;

	void Flatten(BFile* file);

	status_t CurrencyToString(const Fixed& amount, BString& string);
	status_t DateToString(time_t date, BString& string);
	status_t StringToCurrency(const char* string, Fixed& amount);
	status_t PremultipliedStringToCurrency(const char* string, Fixed& amount);
	status_t StringToDate(const char* instring, time_t& date);
	void NumberToCurrency(const Fixed& number, BString& string);

	void SetCurrencySymbol(const char* symbol);
	const char* CurrencySymbol() const { return fCurrencySymbol.String(); }

	void SetCurrencySymbolPrefix(const bool& value);
	bool IsCurrencySymbolPrefix() const { return fPrefixSymbol; }

	void SetCurrencyDecimalPlace(const uint8 place);
	uint8 CurrencyDecimalPlace() const { return fCurrencyDecimalPlace; }

private:
	friend class CapitalBeParser;

	void _SetDefaults();

	BString fCurrencySymbol;
	bool fPrefixSymbol;
	uint8 fCurrencyDecimalPlace;
};

void ShowAlert(const char* header, const char* message, alert_type type = B_INFO_ALERT);
void ShowBug(const char* string);
void GetVersionString(BString& string);
void CapitalizeEachWord(BString& string);
const char* GetCurrencyOnlyMask();
void IllegalCharsToEntities(BString* string);

#endif	// CBLOCALE_H
