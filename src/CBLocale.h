#ifndef CBLOCALE_H
#define CBLOCALE_H

#include <File.h>
#include <String.h>
#include <TextControl.h>
#include <time.h>
#include "DAlert.h"
#include "Fixed.h"

class Locale {
public:
	Locale(void);
	~Locale(void);

	bool operator==(const Locale& other) const;
	bool operator!=(const Locale& other) const;

	void Flatten(BFile* file);

	status_t CurrencyToString(const Fixed& amount, BString& string);
	status_t DateToString(time_t date, BString& string);
	status_t StringToCurrency(const char* string, Fixed& amount);
	status_t StringToDate(const char* instring, time_t& date);

	void SetAccountLocale(const char* languageID);
	const char* AccountLocale(void) const { return fAccountLocale.String(); }

	const char* CurrencySymbol(void) const { return fCurrencySymbol.String(); }
	const char* CurrencySeparator(void) const { return fCurrencySeparator.String(); }
	const char* CurrencyDecimal(void) const { return fCurrencyDecimal.String(); }

private:
	friend class CapitalBeParser;

	void SetDefaults(void);

	BString fAccountLocale;
	BString fCurrencySymbol;
	BString fCurrencySeparator;
	BString fCurrencyDecimal;
};

void ShowAlert(const char* header, const char* message, alert_type type = B_INFO_ALERT);
void ShowBug(const char* string);
void GetVersionString(BString& string);
void CapitalizeEachWord(BString& string);
const char* GetCurrencyOnlyMask(void);
void IllegalCharsToEntities(BString* string);

#endif
