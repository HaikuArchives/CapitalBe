#include <AppFileInfo.h>
#include <Catalog.h>
#include <DateFormat.h>
#include <Debug.h>
#include <File.h>
#include <Locale.h>
#include <NumberFormat.h>
#include <OS.h>
#include <Roster.h>
#include <String.h>
#include <Url.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "App.h"
#include "CBLocale.h"

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
		|| fCurrencyDecimalPlace != other.fCurrencyDecimalPlace) {
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
	if (!fPrefixSymbol)
		str << "\t\t<CurrencySuffixFlag />\n";
	str << "\t</Locale>\n";
	file->Write(str.String(), str.Length());
}

status_t
Locale::CurrencyToString(const Fixed& amount, BString& string)
{
	BNumberFormat numberFormatter;
	BString separator = numberFormatter.GetSeparator(B_DECIMAL_SEPARATOR);

	// Determine the formatted number as a string
	BString num;
	if (strcmp(fCurrencySymbol, "") == 0) {	 // Using default locale
		return numberFormatter.FormatMonetary(string, amount.AsDouble());
	} else {  // Using custom locale
		if (numberFormatter.Format(
				num, fCurrencyDecimalPlace > 0 ? amount.AsDouble() : amount.IntegerPart())
			!= B_OK) {
			return B_ERROR;
		}
	}

	// Check if the number already has a decimal separator
	int32 separatorIndex = num.FindFirst(separator);
	if (separatorIndex == B_ERROR) {
		// No decimal separator found, append one if needed
		if (fCurrencyDecimalPlace > 0) {
			num += separator;
			for (int i = 0; i < fCurrencyDecimalPlace; ++i)
				num += '0';
		}
	} else {
		// Ensure the number has exactly fCurrencyDecimalPlace decimals
		int32 currentDecimals = num.Length() - separatorIndex - 1;
		if (currentDecimals < fCurrencyDecimalPlace) {
			for (int i = 0; i < fCurrencyDecimalPlace - currentDecimals; ++i)
				num += '0';
		} else if (currentDecimals > fCurrencyDecimalPlace) {
			num.Truncate(separatorIndex + 1 + fCurrencyDecimalPlace);
		}
	}

	// Append or prepend the currency symbol
	if (fPrefixSymbol)
		num.Prepend(fCurrencySymbol);
	else
		num.Append(fCurrencySymbol);

	// Prepend a negative sign if the amount is negative
	if (amount < 0)
		string.Prepend("-");

	// Set the final string
	string = num;
	return B_OK;
}


status_t
Locale::StringToCurrency(const char* string, Fixed& amount)
{
	// This is going to be a royal pain. We have to deal with separators (or lack
	// thereof) and the mere possibility of a cents separator. Yuck.

	if (!string)
		return B_ERROR;

	BNumberFormat numberFormatter;
	BString decimalSymbol = numberFormatter.GetSeparator(B_DECIMAL_SEPARATOR);
	BString groupingSeparator = numberFormatter.GetSeparator(B_GROUPING_SEPARATOR);
	BString dollars(string), cents;

	dollars.RemoveAll(groupingSeparator);
	// Remove currency symbol
	if (strcmp(fCurrencySymbol, "") == 0) {
		// Remove currency symbol
		// https://discuss.haiku-os.org/t/parsing-a-currency-formatted-string-to-number/15077/3
		BString format5, format6;
		numberFormatter.FormatMonetary(format5, 5.0);
		numberFormatter.FormatMonetary(format6, 6.0);
		int i = 0;
		while (i < format5.Length() && format5[i] == format6[i])
			i++;
		while (i >= 0 && (format5[i] & 0xc0) == 0x80)
			i--;
		format5.Truncate(i);
		dollars.RemoveFirst(format5);
	} else
		dollars.RemoveAll(fCurrencySymbol);

	int32 index = dollars.FindFirst(decimalSymbol);
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
	if (dateTime.Time_t() == 14400)
		date = now.Time_t();
	else
		date = dateTime.Time_t();

	return B_OK;
}


void
Locale::SetDefaults(void)
{
	fCurrencySymbol = "";
	fPrefixSymbol = true;
	fCurrencyDecimalPlace = 2;
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
Locale::SetCurrencySymbolPrefix(const bool& value)
{
	fPrefixSymbol = value;
}


void
Locale::SetCurrencyDecimalPlace(const uint8 place)
{
	fCurrencyDecimalPlace = place;
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
