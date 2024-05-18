#include "CurrencyBox.h"
#include "CBLocale.h"
#include "Database.h"
#include "MsgDefs.h"
#include "TimeSupport.h"

#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TextInput"


CurrencyBoxFilter::CurrencyBoxFilter(CurrencyBox* box)
	: NavTextBoxFilter(box)
{
}

filter_result
CurrencyBoxFilter::KeyFilter(const int32& key, const int32& mod)
{
	// Here is where all the *real* work for a date box is done.
	if (key == B_TAB && ((NavTextBox*)TextControl())->IsTabFiltering()) {
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

	return B_DISPATCH_MESSAGE;
}

CurrencyBox::CurrencyBox(
	const char* name, const char* label, const char* text, BMessage* msg, uint32 flags)
	: NavTextBox(name, label, text, msg, flags)
{
	SetFilter(new CurrencyBoxFilter(this));

	const char amount_disallowed[] =
		" `~!@#%^&*()_-+=QWERTYUIOP{[}]|\\ASDFGHJKL;:'\""
		"ZXCVBNM<>?/qwertyuiopasdfghjklzxcvbnm";
	int32 i = 0;
	while (amount_disallowed[i]) {
		TextView()->DisallowChar(amount_disallowed[i]);
		i++;
	}
	SetCharacterLimit(20);
}


bool
CurrencyBox::Validate(bool alert)
{
	if (strlen(Text()) < 1)
		SetText("0");

	Fixed amount;
	if (gCurrentLocale.StringToCurrency(Text(), amount) != B_OK) {
		if (alert) {
			ShowAlert(B_TRANSLATE_CONTEXT("CapitalBe didn't understand the amount", "TextInput"),
				B_TRANSLATE_CONTEXT("There may be a typo or the wrong kind of currency symbol "
									"for this account.",
					"TextInput"));
			MakeFocus(true);
		}
		return false;
	} else {
		BString string;
		gCurrentLocale.CurrencyToString(amount, string);
		SetText(string.String());
	}

	return true;
}
