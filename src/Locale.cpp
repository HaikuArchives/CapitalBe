#include <AppFileInfo.h>
#include <Debug.h>
#include <E-mail.h>
#include <File.h>
#include <OS.h>
#include <Roster.h>
#include <String.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "App.h"
#include "CBLocale.h"
#include <Catalog.h>

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
	if (fDateFormat != other.fDateFormat || fCurrencySymbol != other.fCurrencySymbol ||
		fPrefixSymbol != other.fPrefixSymbol || fCurrencySeparator != other.fCurrencySeparator ||
		fCurrencyDecimal != other.fCurrencyDecimal || fDateSeparator != other.fDateSeparator ||
		fCurrencyDecimalPlace != fCurrencyDecimalPlace || fUseDST != other.fUseDST)
		return false;
	return true;
}

void
Locale::Flatten(BFile* file)
{
	BString str("\t<Locale>\n");

	if (fDateFormat == DATE_MDY)
		str << "\t\t<DateFormat Value=\"mmddyyyy\" />\n";
	else
		str << "\t\t<DateFormat Value=\"ddmmyyyy\" />\n";

	str << "\t\t<CurrencySymbol Value=\"" << fCurrencySymbol << "\" />\n";
	str << "\t\t<CurrencyDecimal Value=\"" << fCurrencyDecimal << "\" />\n";
	str << "\t\t<CurrencySeparator Value=\"" << fCurrencySeparator << "\" />\n";
	str << "\t\t<DateSeparator Value=\"" << fDateSeparator << "\" />\n";

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
	char buffer[10];
	struct tm* timestruct = localtime(&date);
	BString datestr;

	if (fDateFormat == DATE_MDY)
		strftime(buffer, 10, "%m", timestruct);
	else if (fDateFormat == DATE_DMY)
		strftime(buffer, 10, "%d", timestruct);

	datestr += buffer;
	datestr += fDateSeparator;
	if (fDateFormat == DATE_MDY)
		strftime(buffer, 10, "%d", timestruct);
	else if (fDateFormat == DATE_DMY)
		strftime(buffer, 10, "%m", timestruct);

	datestr += buffer;
	datestr += fDateSeparator;
	strftime(buffer, 10, "%Y", timestruct);
	datestr += buffer;
	string = datestr;

	return B_OK;
}

status_t
Locale::ConstructDateStringDMY(const char* in, BString& out)
{
	// This constructs a proper date string given a list of numbers
	if (!in)
		return B_ERROR;

	int32 length = strlen(in);
	int num = 0, num2 = 0;
	char charstring[3];
	charstring[1] = '\0';
	charstring[2] = '\0';

	switch (length) {
		case 2:	 // DM
		{
			out = in;
			out.Insert(fDateSeparator, 1);
			break;
		}
		case 3:
		case 5:
		case 7:
		{
			// DDM - 0: 0-3, 1: 0-9 (0-1 if digit 0 == 3), 2: 1-9
			// DMM - 0: 1-9, 1: 0-1, 2: 0-9 (0-2 if digit 1 == 1)
			// DDMYYYY - see above + year
			// DMMYYYY - see above + year
			bool ddm = false;
			charstring[0] = in[0];
			num = atoi(charstring);

			if (num > 3)
				ddm = false;
			else if (num < 1)
				ddm = true;
			else {
				// [1-3]xx.
				charstring[0] = in[1];
				num2 = atoi(charstring);

				if (num == 3) {
					if (num2 > 1)
						ddm = false;
					else
						ddm = true;
				} else if (num2 > 1)
					ddm = true;
				else if (num2 == 0) {
					// Although we can't say for sure, it's unlikely that the user entered a leading
					// zero for the month.
					ddm = true;
				} else {
					// [1-2][0-1]x.
					charstring[0] = in[2];
					num = atoi(charstring);

					if (num == 0)
						ddm = false;
					else if (num > 2)
						ddm = true;
					else {
						// Getting this far means that [1-2]1[1-2] or stuff like 1/12, 2/12, 11/2,
						// or 21/2 As a result, we will choose the earlier result - ddm.
						ddm = true;
					}
				}
			}

			out = in;
			if (ddm)
				out.Insert(fDateSeparator, 2);
			else
				out.Insert(fDateSeparator, 1);
			break;
		}
		case 4:
		{
			// DDMM - 0: 0-3, 1: 0-9 (0-1, if digit 0 is 3), 2: 0-1, 3: 0-9
			// DMYY - 0: 1-9, 1: 1-9, 2-3: 0-9
			bool ddmm = false;
			charstring[0] = in[0];
			num = atoi(charstring);

			if (num > 3)
				ddmm = false;
			else if (num == 0)
				ddmm = true;
			else {
				charstring[0] = in[1];
				num2 = atoi(charstring);

				if (num == 3) {
					if (num2 > 1)
						ddmm = false;
					else
						ddmm = true;
				} else if (num2 == 0)
					ddmm = true;
				else {
					// [1-2][1-2]xx
					charstring[0] = in[2];
					num = atoi(charstring);

					// at this point, it's tough to figure which, but it's more likely that
					// if digit 2 is 0 or 9, it's a year and DDMM otherwise
					if (num == 0 || num == 9)
						ddmm = false;
					else
						ddmm = true;
				}
			}

			out = in;
			if (ddmm)
				out.Insert(fDateSeparator, 2);
			else {
				out.Insert(fDateSeparator, 1);
				out.Insert(fDateSeparator, 2);
			}

			break;
		}
		case 6:
		{
			// DDMMYY - 0: 0-3, 1: 0-9 (0-1 if digit 0 == 3), 2: 0-1, 3: 0-9 (0-2 if digit 2 == 1),
			//			4-5: 0-9
			// DMYYYY - 0: 1-9, 1: 1-9, 2-5: 0-9
			bool ddmm = false;
			charstring[0] = in[0];
			num = atoi(charstring);

			if (num == 0)
				ddmm = false;
			else {
				charstring[0] = in[1];
				num2 = atoi(charstring);

				if (num == 3) {
					if (num2 > 1)
						ddmm = false;
					else if (num2 == 0)
						ddmm = true;
					else {
						// We're in one of those "can't really tell" places, but we can
						// guess based on the the next two numbers. If they are '19' or '20',
						// we'll go with dmyyyy.
						if ((in[2] == '1' && in[3] == '9') || (in[2] == '2' && in[3] == '0')) {
							ddmm = false;
						} else
							ddmm = true;
					}
				} else {
					// Getting this far, we know that [1-9][1-9]xxxx
					charstring[0] = in[2];
					num = atoi(charstring);
					if (num > 1)
						ddmm = false;
					else {
						charstring[0] = in[3];
						num2 = atoi(charstring);

						if (num == 1) {
							if (num2 > 2)
								ddmm = false;
							else
								ddmm = true;
						} else {
							// At this point, we have [1-9][1-9]0xxx. We'll check to see if
							// digit 3 is 0, but if it happens to be nonzero, it's a little
							// hard to definitively know which the user meant. Of course, it
							// is probably ddmmyy format.
							if (num2 == 0)
								ddmm = false;
							else
								ddmm = true;
						}
					}
				}
			}

			out = in;
			if (ddmm) {
				out.Insert(fDateSeparator, 2);
				out.Insert(fDateSeparator, 5);
			} else {
				out.Insert(fDateSeparator, 1);
				out.Insert(fDateSeparator, 3);
			}
			break;
		}
		case 8:	 // DDMMYYYY. Simple to handle
		{
			out = in;
			out.Insert(fDateSeparator, 2);
			out.Insert(fDateSeparator, 5);
			break;
		}
		default:
		{
			return B_ERROR;
			break;
		}
	}
	return B_OK;
}

status_t
Locale::ConstructDateStringMDY(const char* in, BString& out)
{
	// This constructs a proper date string given a list of numbers
	if (!in)
		return B_ERROR;

	int32 length = strlen(in);

	/*
		Possible date formats (with numbers only):

		M, D - almost impossible to differentiate. Not supported
		DD - impossible to differentiate from MD. Not supported.

		MD - all dates acceptable. Very easy.

		MDD - 0: 1-9, 1: 1-3, 2: 0-9 (0-1 when digit 1 is 3)
		MMD - 0: 0-1, 1: 0-2, 2: 1-9

		MMDD - 0: 0-1, 1: 0-2, 3: 1-3, 4: 0-9
		MDYY - 0: 1-9, 1: 1-9, 3: 0-9, 4: 0-9

		MDDYY
		MMDYY

		MMDDYY
		MDYYYY

		MDDYYYY
		MMDYYYY

		MMDDYYYY
	*/
	int num = 0;
	char charstring[3];
	charstring[1] = '\0';
	charstring[2] = '\0';

	out = in;

	switch (length) {
		case 2:	 // MD
		{
			out.Insert(fDateSeparator, 1);
			break;
		}
		case 3:	 // MDD, MMD. Overlap when [1][1-2][0-9]. When there is overlap, assume the earlier
				 // date.
		case 5:	 // MDDYY, MMDYY
		case 7:	 // MMDYYYY, MDDYYYY
		{
			// MDD - 0: 1-9, 1: 1-3, 2: 0-9 (0-1 if digit 1 == 3)
			// MMD - 0: 0-1, 1: 0-9 (1-9 if digit 0 == 0), 2: 0-9
			bool mdd = false;
			charstring[0] = in[0];
			num = atoi(charstring);

			if (num > 1)
				mdd = true;
			else if (num < 1)
				mdd = false;
			else {
				// So far, the first digit is a one, so it could be either.
				charstring[0] = in[1];
				num = atoi(charstring);

				if (num == 0)
					mdd = false;
				else if (num == 3)
					mdd = true;
				else {
					// We got this far, so there is no possible way to know for sure which it is.
					// Possible dates: 1/1x, 1/2x, 11/x, 12/x
					// As a result, we will choose the earlier date, which means mdd=true.
					mdd = true;
				}
			}

			if (mdd)
				out.Insert(fDateSeparator, 1);
			else
				out.Insert(fDateSeparator, 2);

			break;
		}
		case 4:	 // MDYY, MMDD
		{
			// MMDD - 0: 0-1, 1: 0-2, 3: 1-3, 4: 0-9 (0-1 if digit 3 == 3)
			// MDYY - 0: 1-9, 1: 1-9, 3: 0-9, 4: 0-9
			bool mdyy = false;

			charstring[0] = in[0];
			num = atoi(charstring);

			if (num > 1)
				mdyy = true;
			else if (num == 0)
				mdyy = false;
			else {
				// First number = 1
				charstring[0] = in[1];
				num = atoi(charstring);

				if (num > 2)
					mdyy = true;
				else if (num == 0)
					mdyy = false;
				else {
					// Second number is 1 or 2.
					charstring[0] = in[2];
					num = atoi(charstring);
					if (num == 0 || num > 3)
						mdyy = true;
					else if (num == 3) {
						// Date is 1[1,2]3x. Chances are that we can't tell, but if x>1, it's mdyy.
						charstring[0] = in[3];
						num = atoi(charstring);
						if (num > 1)
							mdyy = true;
						else {
							// Getting this far means we can't tell. Date is in format 1[1,2]3[0,1]
							// This means we have no idea. Considering the liklihood that the user
							// is entering a date in the year 1930 or 1931 is relatively small, we
							// choose mmdd
							mdyy = false;
						}
					} else {
						// Getting this far means we can't tell. Date is in format 1[1,2][1,2]x
						// Example dates: 1124 could be either 1/1/24 or 11/24. Just as above, the
						// user probably means MM/DD because of when this code is written.
						mdyy = false;
					}
				}
			}

			if (mdyy) {
				out.Insert(fDateSeparator, 1);
				out.Insert(fDateSeparator, 3);
			} else
				out.Insert(fDateSeparator, 2);
			break;
		}
		case 6:	 // MDYYYY, MMDDYY
		{
			// MDYYYY - 0: 1-9, 1: 1-9, 2-5: 0-9
			// MMDDYY - 0: 0-1, 1: 0-9 (1-9 if digit 0 == 0), 2: 0-3, 3: 0-9 (0-1 if digit 2 == 3),
			// 4-5: 0-9
			bool mmddyy = false;
			charstring[0] = in[0];
			num = atoi(charstring);

			if (num > 1)
				mmddyy = false;
			else {
				// [0-1]xxxxx
				charstring[0] = in[1];
				num = atoi(charstring);

				if (num == 0)
					mmddyy = true;
				else {
					// At this point we're dealing with 01xxxx or 11xxxx. We need to see if digits 2
					// & 3 are a date.
					charstring[0] = in[2];
					num = atoi(charstring);
					if (num > 3)
						mmddyy = false;
					else if (num == 3) {
						// Either 013xxx or 113xxx. If digit 3 > 1, then it's mdyyyy
						charstring[0] = in[3];
						num = atoi(charstring);
						if (num > 1)
							mmddyy = false;
						else
							mmddyy = true;
					} else if (num == 0) {
						// It's *much* less likely that the user will type in a year with a leading
						// zero, so choose mmddyy
						mmddyy = true;
					} else {
						// 01[1-2]xxx or 11[1-2]xxx. At this point, it's not possible to tell for
						// sure which it is, but probability points to x119xx or x120xx if it's a
						// year. Otherwise, we'll assume mmddyy
						charstring[0] = in[3];
						uint8 num2 = atoi(charstring);
						if (num == 1) {
							if (num2 == 9)
								mmddyy = false;
							else
								mmddyy = true;
						} else if (num == 2) {
							if (num2 == 0)
								mmddyy = false;
							else
								mmddyy = true;
						} else
							mmddyy = true;
					}
				}
			}

			if (mmddyy) {
				out.Insert(fDateSeparator, 2);
				out.Insert(fDateSeparator, 5);
			} else {
				out.Insert(fDateSeparator, 1);
				out.Insert(fDateSeparator, 4);
			}
			break;
		}
		case 8:	 // MMDDYYYY. Simple to handle
		{
			out = in;
			out.Insert(fDateSeparator, 2);
			out.Insert(fDateSeparator, 5);
			break;
		}
		default:  // M, D, other unsupported lengths
		{
			return B_ERROR;
			break;
		}
	}

	return B_OK;
}

status_t
Locale::StringToDate(const char* instring, time_t& date)
{
	//	Input recognized for MM/DD/YY:
	//	1) MM/DD
	//	2) MM/DD/YYYY
	//	3) MM/DD//YY
	//	4) M/D/YY
	//  5) M/DD/YYYY

	if (!instring || (fDateFormat != DATE_MDY && fDateFormat != DATE_DMY))
		return B_BAD_VALUE;

	BString monthstr, daystr, yearstr, string = instring;
	int32 index;
	if (fDateFormat == DATE_MDY) {
		monthstr = string;
		index = monthstr.FindFirst(fDateSeparator);
		if (index < 0) {
			ConstructDateStringMDY(instring, string);
			monthstr = string;
			index = monthstr.FindFirst(fDateSeparator);
			if (index < 0)
				return index;
		}

		monthstr.Truncate(index);

		daystr = string.String() + index + 1;
		index = daystr.FindFirst(fDateSeparator);
		if (index < 0) {
			// This probably means that the user didn't tack the year
			// onto the end. Check to see if there are 1 or 2 characters
			// beyond the point of the last separator. If so, then chances are
			// that it is a valid date using the current year.
			if (daystr.CountChars() != 1 && daystr.CountChars() != 2)
				return B_ERROR;
			yearstr = "";
		} else {
			daystr.Truncate(index);
			yearstr = daystr.String() + index + 1;
		}
	} else {
		daystr = string;
		index = daystr.FindFirst(fDateSeparator);
		if (index < 0) {
			ConstructDateStringDMY(instring, string);
			monthstr = string;
			index = monthstr.FindFirst(fDateSeparator);
			if (index < 0)
				return index;
		}

		daystr.Truncate(index);

		monthstr = string.String() + index + 1;
		index = monthstr.FindFirst(fDateSeparator);
		if (index < 0) {
			// This probably means that the user didn't tack the year
			// onto the end. Check to see if there are 1 or 2 characters
			// beyond the point of the last separator. If so, then chances are
			// that it is a valid date using the current year.
			if (monthstr.CountChars() != 1 && monthstr.CountChars() != 2)
				return B_ERROR;
			yearstr = "";
		} else {
			yearstr = monthstr.String() + index + 1;
			monthstr.Truncate(index);
		}
	}

	struct tm timestruct;

	time_t rawtime;
	time(&rawtime);
	timestruct = *localtime(&rawtime);

	if (yearstr.Length() > 0) {
		if (yearstr.CountChars() <= 2) {
			timestruct.tm_year = atoi(yearstr.String());
			if (timestruct.tm_year < 10)
				timestruct.tm_year += 100;
		} else
			timestruct.tm_year = atoi(yearstr.String()) - 1900;
	}
	timestruct.tm_sec = 1;
	timestruct.tm_min = 1;
	timestruct.tm_hour = 1;
	timestruct.tm_mon = atoi(monthstr.String()) - 1;
	timestruct.tm_mday = atoi(daystr.String());
	timestruct.tm_isdst = (fUseDST) ? 1 : 0;
	time_t datestruct = mktime(&timestruct);

	if (datestruct == -1) {
		//		debugger("Error handling for StringToDate unimplemented");
		return B_ERROR;
	}

	date = datestruct;
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
	fDateFormat = DATE_MDY;
	fCurrencySymbol = "$";
	fPrefixSymbol = true;
	fCurrencySeparator = ",";
	fCurrencyDecimal = ".";
	fDateSeparator = "/";
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
	BString message =
		B_TRANSLATE("CapitalBe has run into a bug. This shouldn't happen, but it has.\n"
		"Would you like to:\n\n1) Have CapitalBe make an e-mail to send to Support\n"
		"2) Save the bug to a text file for e-mailing later\n"
		"3) Just quit and do nothing\n");

	DAlert* alert =
		new DAlert(B_TRANSLATE("Agh! Bug!"), message.String(), B_TRANSLATE("Make an E-mail"),
		B_TRANSLATE("Save to File"), B_TRANSLATE("Quit"));
	int32 value = alert->Go();

	if (value == 0) {
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
		// Make an e-mail to myself. :D

		BString cmdstring("/boot/beos/apps/BeMail mailto:support@capitalbe.com ");
		cmdstring << "-subject 'CapitalBe Bug Report' -body '" << message << "'";
		cmdstring << " &";
		system(cmdstring.String());
	} else if (value == 1) {
		// Generate a text file of the bug on the Desktop
		BString filename("/boot/home/Desktop/CapitalBe Bug Report ");
		filename << real_time_clock() << ".txt";
		BFile file(filename.String(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		file.Write(message.String(), message.Length());
		file.Unset();
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
Locale::SetDateFormat(const date_format& format)
{
	fDateFormat = format;
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
Locale::SetDateSeparator(const char* symbol)
{
	if (symbol)
		fDateSeparator = symbol;
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

const char*
GetDateOnlyMask(void)
{
	return "`~!@#$%^&*()_=QWERTYUIOP{[}]|\\ASDFGHJKL;:'\"ZXCVBNM,.<>?qwertyuiopasdfghjklzxcvbnm";
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
