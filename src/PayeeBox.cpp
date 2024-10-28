/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	bitigchi (Emir Sari)
 *	dospuntos (Johan Wagenheim)
 *	humdinger (Joachim Seemer)
 *	raefaldhia (Raefaldhi Amartya Junior)
 */
#include "PayeeBox.h"
#include "CBLocale.h"
#include "Database.h"
#include "MsgDefs.h"

#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TextInput"


PayeeBoxFilter::PayeeBoxFilter(PayeeBox* box)
	: AutoTextControlFilter(box)
{
}


filter_result
PayeeBoxFilter::KeyFilter(const int32& key, const int32& mod)
{
	// Here is where all the *real* work for a date box is done.
	if (key == B_TAB) {
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

	//	if(key == B_ESCAPE && !IsEscapeCancel())
	//		return B_SKIP_MESSAGE;

	if (key < 32
		|| ((mod & B_COMMAND_KEY) && !(mod & B_SHIFT_KEY) && !(mod & B_OPTION_KEY)
			&& !(mod & B_CONTROL_KEY)))
		return B_DISPATCH_MESSAGE;

	Account* acc = gDatabase.CurrentAccount();
	if (!acc)
		return B_DISPATCH_MESSAGE;

	int32 start, end;
	TextControl()->TextView()->GetSelection(&start, &end);
	if (end == (int32)strlen(TextControl()->Text())) {
		TextControl()->TextView()->Delete(start, end);
		BString string = "";
		GetCurrentMessage()->FindString("bytes", &string);
		int32 charLength = string.Length();

		string.Prepend(TextControl()->Text());

		BString autocomplete = acc->AutocompletePayee(string.String());
		if (autocomplete.CountChars() == 0)
			autocomplete = string;

		BMessage automsg(M_PAYEE_AUTOCOMPLETE);
		automsg.AddInt32("start", strlen(TextControl()->Text()) + charLength);
		automsg.AddString("string", autocomplete.String());
		SendMessage(&automsg);
	}

	return B_DISPATCH_MESSAGE;
}


PayeeBox::PayeeBox(
	const char* name, const char* label, const char* text, BMessage* msg, uint32 flags)
	: AutoTextControl(name, label, text, msg, flags)
{
	SetFilter(new PayeeBoxFilter(this));
	SetCharacterLimit(32);
}


bool
PayeeBox::Validate(const bool& showalert)
{
	if (showalert && (Text() == NULL || strlen(Text()) < 1)) {
		ShowAlert(B_TRANSLATE("Payee is missing"), B_TRANSLATE("You need to enter a payee."));
		MakeFocus(true);
		return false;
	}

	//	BString text = Text();
	//	CapitalizeEachWord(text);
	//	SetText(text.String());

	return true;
}
