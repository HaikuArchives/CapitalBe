#include "App.h"
#include "CBLocale.h"
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

#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>

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
	if (fAccountLocale != other.fAccountLocale)
		return false;
	return true;
}

void
Locale::Flatten(BFile* file)
{
	BString str("\t<Locale>\n");

	str << "\t\t<AccountLocale Value=\"" << fAccountLocale << "\" />\n";

	str << "\t</Locale>\n";

	file->Write(str.String(), str.Length());
}

status_t
Locale::CurrencyToString(const Fixed& amount, BString& string)
{
	BFormattingConventions conv(fAccountLocale.String());
	BLanguage* language = new BLanguage(fAccountLocale.String());
	BLocale* locale = new BLocale(language, &conv);
	BNumberFormat numberFormatter(locale);
	BString curstr;

	return numberFormatter.FormatMonetary(string, amount.AsDouble());
}

status_t
Locale::StringToCurrency(const char* string, Fixed& amount)
{
	if (!string)
		return B_ERROR;

	std::string input(string);
	std::string digits;
	bool decimalPointSeen = false;

	for (char ch : input) {
		if (std::isdigit(ch)) {
			digits += ch;
		} else if ((ch == ',' || ch == '.') && !decimalPointSeen) {
			decimalPointSeen = true;
			digits += '.';
		}
	}

	if (digits.empty())
		return B_ERROR;

	double value = std::stod(digits) * 100;
	long premultipliedValue = static_cast<long>(std::round(value));

	if (premultipliedValue < 0)
		premultipliedValue = -premultipliedValue;

	amount.SetPremultiplied(premultipliedValue);

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
Locale::SetDefaults(void)
{
	fCurrencySymbol = "$";
	fCurrencySeparator = ",";
	fCurrencyDecimal = ".";

	BLanguage lang;
	fAccountLocale = lang.ID();
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
Locale::SetAccountLocale(const char* languageID)
{
	if (languageID != NULL)
		fAccountLocale = languageID;
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
