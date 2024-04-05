#ifndef CBLOCALE_H
#define CBLOCALE_H

#include <File.h>
#include <String.h>
#include <TextControl.h>
#include <time.h>
#include "DAlert.h"
#include "Fixed.h"

typedef enum
{
	DATE_MDY = 1,
	DATE_DMY = 2
} date_format;

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
	status_t PremultipliedStringToCurrency(const char* string, Fixed& amount);
	status_t StringToDate(const char* instring, time_t& date);
	void NumberToCurrency(const Fixed& number, BString& string);

	void SetDateFormat(const date_format& format);
	date_format DateFormat(void) const { return fDateFormat; }

	void SetCurrencySymbol(const char* symbol);
	const char* CurrencySymbol(void) const { return fCurrencySymbol.String(); }

	void SetCurrencySeparator(const char* symbol);
	const char* CurrencySeparator(void) const { return fCurrencySeparator.String(); }

	void SetCurrencyDecimal(const char* symbol);
	const char* CurrencyDecimal(void) const { return fCurrencyDecimal.String(); }

	void SetCurrencySymbolPrefix(const bool& value);
	bool IsCurrencySymbolPrefix(void) const { return fPrefixSymbol; }

	void SetCurrencyDecimalPlace(const uint8& place);
	uint8 CurrencyDecimalPlace(void) const { return fCurrencyDecimalPlace; }

	void SetDateSeparator(const char* symbol);
	const char* DateSeparator(void) const { return fDateSeparator.String(); }

	void SetDST(const bool& value);
	bool UseDST(void) const { return fUseDST; }

private:
	friend class CapitalBeParser;

	void SetDefaults(void);
	status_t ConstructDateStringMDY(const char* in, BString& out);
	status_t ConstructDateStringDMY(const char* in, BString& out);

	// Like BeFinancial, we just use two different date formats: mmddyyyy (0) and
	// ddmmyyyy (1)
	date_format fDateFormat;
	BString fCurrencySymbol;
	bool fPrefixSymbol;
	BString fCurrencySeparator;
	BString fCurrencyDecimal;
	BString fDateSeparator;
	uint8 fCurrencyDecimalPlace;
	bool fUseDST;
};

void ShowAlert(const char* header, const char* message, alert_type type = B_INFO_ALERT);
void ShowBug(const char* string);
void GetVersionString(BString& string);
void CapitalizeEachWord(BString& string);
const char* GetCurrencyOnlyMask(void);
const char* GetDateOnlyMask(void);
void IllegalCharsToEntities(BString* string);

#endif
