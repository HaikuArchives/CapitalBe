#include "App.h"
#include "CBLocale.h"
#include <AppFileInfo.h>
#include <Catalog.h>
#include <DateFormat.h>
#include <Debug.h>
#include <File.h>
#include <OS.h>
#include <Roster.h>
#include <String.h>
#include <Url.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Locale"


Locale::Locale(void)
{
	SetDefaults();
}

Locale::~Locale(void) {}

bool
Locale::operator!=(const Locale& other) const
{
	return !(*this == other);
}

bool
Locale::operator==(const Locale& other) const
{
	if (fCurrencySymbol != other.fCurrencySymbol || fPrefixSymbol != other.fPrefixSymbol
		|| fCurrencySeparator != other.fCurrencySeparator
		|| fCurrencyDecimal != other.fCurrencyDecimal
		|| fCurrencyDecimalPlace != other.fCurrencyDecimalPlace || fUseDST != other.fUseDST) {
		return false;
	}
	return true;
}

void
Locale::Flatten(BFile* file)
{
	BString str("\t<Locale>\n");

	str << "\t\t<DateFormat Value=\"ddmmyyyy\" />\n";

	str << "\t\t<CurrencySymbol Value=\"" << fCurrencySymbol << "\" />\n";
	str << "\t\t<CurrencyDecimal Value=\"" << fCurrencyDecimal << "\" />\n";
	str << "\t\t<CurrencySeparator Value=\"" << fCurrencySeparator << "\" />\n";

	if (!fPrefixSymbol)
		str << "\t\t<CurrencySuffixFlag />\n";
	if (fUseDST)
		str << "\t\t<DaylightSavingsFlag />\n";
	str << "\t</Locale>\n";

	file->Write(str.String(), str.Length());
}

status_t
Locale::CurrencyToString(const Fixed& amount, BString& string)
{
	string = "";

	BString dollars;
	dollars << (amount.IsPositive() ? amount.AsLong() : -amount.AsLong());

	char cents[20];
	BString formatstring("%.");
	formatstring << (int)fCurrencyDecimalPlace << "f";
	sprintf(cents, formatstring.String(), amount.DecimalPart());

	string = cents + ((amount.IsNegative() && amount.DecimalPart() != 0) ? 2 : 1);
	if (fCurrencyDecimal != ".")
		string.ReplaceSet(".", fCurrencyDecimal.String());

	for (int32 i = dollars.CountChars() - 3; i > 0; i -= 3)
		dollars.Insert(fCurrencySeparator, i);

	string.Prepend(dollars);

	if (fPrefixSymbol)
		string.Prepend(fCurrencySymbol);
	else
		string.Append(fCurrencySymbol);

	if (amount < 0)
		string.Prepend("-");

	return B_OK;
}

status_t
Locale::StringToCurrency(const char* string, Fixed& amount)
{
	// This is going to be a royal pain. We have to deal with separators (or lack
	// thereof) and the mere possibility of a cents separator. Yuck.

	if (!string)
		return B_ERROR;

	BString dollars(string), cents;

	dollars.RemoveAll(fCurrencySymbol);
	dollars.RemoveAll(fCurrencySeparator);

	int32 index = dollars.FindFirst(fCurrencyDecimal);
	if (index < 0) {
		// We're working with whole dollars here. :)
		cents = "00";
	} else {
		cents = dollars.String() + index + 1;
		dollars.Truncate(index);
		if (dollars == "-0")
			cents.Prepend("-");
	}

	amount = atol(dollars.String());
	float temp = atol(cents.String());
	if (amount < 0)
		temp = -temp;

	amount.AddPremultiplied(temp);
	return B_OK;
}

status_t
Locale::PremultipliedStringToCurrency(const char* string, Fixed& amount)
{
	// This function is an optimized version of StringToCurrency which expects
	// the input string to represent the premultiplied version of a Fixed value.
	// As a result, everything which is not a number is stripped out, converted
	// to a number and added to the amount parameter
	if (!string)
		return B_ERROR;

	// TODO: See if using the string set version of RemoveAll would be faster

	BString valuestr(string);
	valuestr.RemoveAll(fCurrencySymbol);
	valuestr.RemoveAll(fCurrencySeparator);
	valuestr.RemoveAll(fCurrencyDecimal);

	int32 value = atoi(valuestr.String());
	amount.SetPremultiplied(value);
	return B_OK;
}

status_t
Locale::DateToString(time_t date, BString& string)
{
	BDateFormat dateFormatter;
	return dateFormatter.Format(string, date, B_SHORT_DATE_FORMAT);
}

status_t
Locale::StringToDate(const char* instring, time_t& date)
{
	if (instring == NULL)
		return B_BAD_VALUE;

	BDateFormat dateFormatter;
	BDate parseDate;
	BDateTime dateTime;
	BDateTime now = BDateTime::CurrentDateTime(B_LOCAL_TIME);

	if (dateFormatter.Parse(instring, B_SHORT_DATE_FORMAT, parseDate) == B_OK)
		dateTime.SetDate(parseDate);

	// Use current date if date is 1/1/70, parse failed
	if (dateTime.Time_t() <= 14400)
		date = now.Time_t();
	else
		date = dateTime.Time_t();

	return B_OK;
}

void
Locale::NumberToCurrency(const Fixed& number, BString& string)
{
	string = "";
	string << number.IntegerPart();

	for (int32 i = string.CountChars() - 3; i > 0; i -= 3)
		string.Insert(fCurrencySeparator, i);

	string << fCurrencyDecimal;

	// We have to do this to eliminate the leading zero and the decimal point
	BString decimal;
	decimal << (float)number.DecimalPart();

	if (number.IsNegative())
		string += decimal.String() + 3;
	else
		string += decimal.String() + 2;
}

void
Locale::SetDefaults(void)
{
	fCurrencySymbol = "$";
	fPrefixSymbol = true;
	fCurrencySeparator = ",";
	fCurrencyDecimal = ".";
	fCurrencyDecimalPlace = 2;
	fUseDST = false;
}


void
ShowAlert(const char* header, const char* message, alert_type type)
{
	DAlert* alert = new DAlert(header, message, "OK", NULL, NULL, B_WIDTH_AS_USUAL, type);
	alert->Go();
}

void
GetVersionString(BString& string)
{
	app_info ai;
	version_info vi;
	be_app->GetAppInfo(&ai);
	BFile file(&ai.ref, B_READ_ONLY);
	BAppFileInfo appinfo(&file);
	appinfo.GetVersionInfo(&vi, B_APP_VERSION_KIND);

	BString variety;
	switch (vi.variety) {
		case 0:
			variety = "Development";
			break;
		case 1:
			variety = "Alpha";
			break;
		case 2:
			variety = "Beta";
			break;
		case 3:
			variety = "Gamma";
			break;
		case 4:
			variety = "Release Candidate";
			break;
		default:
			variety = "Final";
			break;
	}

	string = "";
	string << vi.major << "." << vi.middle << " ";

	if (variety != "Final")
		string << variety << " " << vi.internal;
	else
		string << "Final";
}

// This function saves typing and makes debug messages more friendly and easier to
// test
void
ShowBug(const char* string)
{
	BString message = B_TRANSLATE(
		"CapitalBe has run into a bug. This shouldn't happen, but it has.\n"
		"Would you like to:\n\n1) Save the bug to a text file for uploading to\n"
		"CapitalBe's issue tracker (https://github.com/HaikuArchives/CapitalBe/issues)\n\n"
		"2) Just quit and do nothing\n");

	DAlert* alert = new DAlert(B_TRANSLATE("Agh! Bug!"), message.String(),
		B_TRANSLATE("Save bugreport"), B_TRANSLATE("Quit"));
	int32 value = alert->Go();

	if (value == 1) {
		be_app->PostMessage(M_QUIT_NOW);
		return;
	}

	// Generate the report text
	message = "CapitalBe Bug Report\n\n";

	BString version;
	GetVersionString(version);
	message << "Version: " << version << "\n";
	message << "Error: " << string << "\n";

	if (value == 0) {
		// Generate a text file of the bug on the Desktop
		BString filename("/boot/home/Desktop/CapitalBe Bug Report ");
		filename << real_time_clock() << ".txt";
		BFile file(filename.String(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		file.Write(message.String(), message.Length());
		file.Unset();

		// Open GitHub in browser
		BUrl("https://github.com/HaikuArchives/CapitalBe/issues/").OpenWithPreferredApplication();
	}

	be_app->PostMessage(M_QUIT_NOW);
}

// Code from Haiku's String.cpp
void
CapitalizeEachWord(BString& string)
{
	if (string.CountChars() < 1)
		return;

	char* index = string.LockBuffer(string.Length());

	int32 count = 0;
	int32 length = string.Length();

	do {
		// Find the first alphabetical character...
		for (; count < length; count++) {
			if (isalpha(index[count])) {
				// ...found! Convert it to uppercase.
				index[count] = toupper(index[count]);
				count++;
				break;
			}
		}
		// Now find the first non-alphabetical character,
		// and meanwhile, turn to lowercase all the alphabetical ones
		for (; count < length; count++) {
			if (isalpha(index[count]))
				index[count] = tolower(index[count]);
			else
				break;
		}
	} while (count < length);

	string.UnlockBuffer();
}


void
Locale::SetCurrencySymbol(const char* symbol)
{
	if (symbol)
		fCurrencySymbol = symbol;
}

void
Locale::SetCurrencySeparator(const char* symbol)
{
	if (symbol)
		fCurrencySeparator = symbol;
}

void
Locale::SetCurrencyDecimal(const char* symbol)
{
	if (symbol)
		fCurrencyDecimal = symbol;
}

void
Locale::SetCurrencySymbolPrefix(const bool& value)
{
	fPrefixSymbol = value;
}

void
Locale::SetCurrencyDecimalPlace(const uint8& place)
{
	fCurrencyDecimalPlace = place;
}


void
Locale::SetDST(const bool& value)
{
	fUseDST = value;
}

const char*
GetCurrencyOnlyMask(void)
{
	return "zxcvbnm/"
		   "<>?asdfghjkl;':\"qwertyuiop[]\\{}|`-=~!@#%^&*()_"
		   "à¡™∞ô¶•«»–≠œ∑é®†øπ‘¬å∫∂ƒ©Ωﬁ∆◊æäñ≈ç√ßñµ…÷";
}


void
IllegalCharsToEntities(BString* string)
{
	if (!string)
		return;

	string->ReplaceAll("<", "&lt;");
	string->ReplaceAll(">", "&gt;");
	string->ReplaceAll("&", "&amp;");
	string->ReplaceAll("'", "&apos;");
	string->ReplaceAll("\"", "&quot;");
}
