/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	bitigchi (Emir Sari)
 *	humdinger (Joachim Seemer)
 *	dospuntos (Johan Wagenheim)
 */
#include "DateBox.h"
#include "CBLocale.h"
#include "Database.h"
#include "MsgDefs.h"
#include "TimeSupport.h"

#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TextInput"


DateBoxFilter::DateBoxFilter(DateBox* box)
	:
	AutoTextControlFilter(box)
{
}


filter_result
DateBoxFilter::KeyFilter(const int32& key, const int32& mod)
{
	// Here is where all the *real* work for a date box is done.
	if (mod & B_COMMAND_KEY)
		return B_DISPATCH_MESSAGE;

	if (key == B_TAB) {
		if (!((DateBox*)TextControl())->IsTabFiltering())
			return B_DISPATCH_MESSAGE;

		if (mod & B_SHIFT_KEY)
			SendMessage(new BMessage(M_PREVIOUS_FIELD));
		else
			SendMessage(new BMessage(M_NEXT_FIELD));
		return B_SKIP_MESSAGE;
	}

#ifdef ENTER_NAVIGATION
	if (key == B_ENTER) {
		SendMessage(new BMessage(M_ENTER_NAVIGATION));
		return B_SKIP_MESSAGE;
	}
#endif

	// Weed out navigation keys
	if (key < 32 && key != B_PAGE_UP && key != B_PAGE_DOWN && key != B_UP_ARROW
		&& key != B_DOWN_ARROW)
		return B_DISPATCH_MESSAGE;

	int32 start, end;
	DateBox* box = (DateBox*)TextControl();
	box->TextView()->GetSelection(&start, &end);

	BString string;
	if (key == B_UP_ARROW) {
		if (strlen(box->Text()) > 0) {
			if (mod & B_SHIFT_KEY)
				box->fCurrentDate = IncrementDateByMonth(box->fCurrentDate);
			else
				box->fCurrentDate = IncrementDateByDay(box->fCurrentDate);
		}
	} else if (key == B_DOWN_ARROW) {
		if (strlen(box->Text()) > 0) {
			if (mod & B_SHIFT_KEY)
				box->fCurrentDate = DecrementDateByMonth(box->fCurrentDate);
			else
				box->fCurrentDate = DecrementDateByDay(box->fCurrentDate);
		}
	} else if (key == B_PAGE_UP) {
		if (strlen(box->Text()) > 0)
			box->fCurrentDate = IncrementDateByYear(box->fCurrentDate);
	} else if (key == B_PAGE_DOWN) {
		if (strlen(box->Text()) > 0)
			box->fCurrentDate = DecrementDateByYear(box->fCurrentDate);
	} else
		return B_DISPATCH_MESSAGE;

	gDefaultLocale.DateToString(box->fCurrentDate, string);
	box->SetText(string.String());
	box->TextView()->SelectAll();
	TextControl()->Invoke();
	return B_SKIP_MESSAGE;
}


DateBox::DateBox(const char* name, const char* label, const char* text, BMessage* msg, uint32 flags)
	:
	AutoTextControl(name, label, text, msg, flags),
	fFilterTab(true)
{
	SetFilter(new DateBoxFilter(this));
	fCurrentDate = GetCurrentDate();

	const char date_disallowed[] = "`~!@#$%^&*()_=QWERTYUIOP{[}]|\\ASDFGHJKL;:'\""
								   "ZXCVBNM,<>?qwertyuiopasdfghjklzxcvbnm";
	int32 i = 0;
	while (date_disallowed[i]) {
		TextView()->DisallowChar(date_disallowed[i]);
		i++;
	}

	// Even though dates should have a limit of 10 characters, we want to give
	// the user a little elbow room. ;^)
	SetCharacterLimit(15);
}


bool
DateBox::Validate(const bool& alert)
{
	BString date;
	time_t tdate;
	if (strlen(Text()) < 1) {
		BString date;
		gDefaultLocale.DateToString(fCurrentDate, date);
		SetText(date.String());
	} else if (gDefaultLocale.StringToDate(Text(), tdate) != B_OK) {
		if (alert) {
			ShowAlert(B_TRANSLATE("CapitalBe didn't understand the date you entered"),
				B_TRANSLATE("CapitalBe understands lots of different ways of entering dates. "
							"Apparently, this wasn't one of them. You'll need to change how you "
							"entered this date. Sorry."));
			MakeFocus(true);
		}
		return false;
	} else {
		fCurrentDate = tdate;
		gDefaultLocale.DateToString(fCurrentDate, date);
		SetText(date.String());
	}
	return true;
}
