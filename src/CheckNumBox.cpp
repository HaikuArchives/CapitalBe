#include "CheckNumBox.h"
#include "CBLocale.h"
#include "Database.h"
#include "MsgDefs.h"
#include "TimeSupport.h"

#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CheckNumBox"

CheckNumBoxFilter::CheckNumBoxFilter(CheckNumBox* box) : AutoTextControlFilter(box) {}

filter_result
CheckNumBoxFilter::KeyFilter(const int32& key, const int32& mod)
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

	if (key < 32 || ((mod & B_COMMAND_KEY) && !(mod & B_SHIFT_KEY) && !(mod & B_OPTION_KEY) &&
					 !(mod & B_CONTROL_KEY)))
		return B_DISPATCH_MESSAGE;

	Account* acc = gDatabase.CurrentAccount();
	if (!acc)
		return B_DISPATCH_MESSAGE;

	int32 start, end;
	TextControl()->TextView()->GetSelection(&start, &end);
	BString string, autocomplete;
	int32 length = strlen(TextControl()->Text());

	BString keystring;
	GetCurrentMessage()->FindString("bytes", &keystring);

	if (keystring == "+") {
		if (strlen(TextControl()->Text()) > 0) {
			uint16 num = acc->LastCheckNumber() + 1;
			acc->SetLastCheckNumber(num);

			string << (num + 1);
		}
		else
			string << acc->LastCheckNumber() + 1;

		TextControl()->SetText(string.String());
		TextControl()->TextView()->SelectAll();
		return B_SKIP_MESSAGE;
	}
	else if (keystring == "-") {
		//		if(strlen(TextControl()->Text())>0)
		if (length > 0) {
			uint16 num = acc->LastCheckNumber() - 1;
			acc->SetLastCheckNumber(num);

			string << (num + 1);
		}
		else
			string << acc->LastCheckNumber() + 1;

		TextControl()->SetText(string.String());
		TextControl()->TextView()->SelectAll();
		return B_SKIP_MESSAGE;
	}
	else
		//	if(end == (int32)strlen(TextControl()->Text()))
		if (end == length) {
			TextControl()->TextView()->Delete(start, end);

			if (GetCurrentMessage()->FindString("bytes", &string) != B_OK)
				string = "";
			string.Prepend(TextControl()->Text());

			autocomplete = acc->AutocompleteType(string.String());
			if (autocomplete.CountChars() > 0) {
				BMessage automsg(M_TYPE_AUTOCOMPLETE);
				automsg.AddInt32("start", strlen(TextControl()->Text()) + 1);
				automsg.AddString("string", autocomplete.String());
				SendMessage(&automsg);
			}
		}

	return B_DISPATCH_MESSAGE;
}

CheckNumBox::CheckNumBox(
	const char* name, const char* label, const char* text, BMessage* msg, uint32 flags
)
	: AutoTextControl(name, label, text, msg, flags)
{
	SetFilter(new CheckNumBoxFilter(this));

	const char type_disallowed[] = "`~!@#$%^&*()_-+={[}]|\\;:'\"<>?";
	int32 i = 0;
	while (type_disallowed[i]) {
		TextView()->DisallowChar(type_disallowed[i]);
		i++;
	}

	SetCharacterLimit(8);
}

bool
CheckNumBox::Validate(void)
{
	if (strlen(Text()) < 1) {
		ShowAlert(
			B_TRANSLATE("Transaction type is missing."),
			B_TRANSLATE("You need to enter a check number or transaction type, such as "
						"ATM (for debit card transactions and the like), DEP (for deposits), "
						"or your own code for some other kind of expense.")
		);
		MakeFocus(true);
		return false;
	}
	return true;
}
