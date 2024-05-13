#include "CategoryBox.h"
#include <Catalog.h>
#include "CBLocale.h"
#include "Database.h"
#include "MsgDefs.h"
#include "TimeSupport.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CategoryBox"


CategoryBoxFilter::CategoryBoxFilter(CategoryBox* box)
	: AutoTextControlFilter(box)
{
}

filter_result
CategoryBoxFilter::KeyFilter(const int32& key, const int32& mod)
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
	if (end == (int32)strlen(TextControl()->Text())) {
		TextControl()->TextView()->Delete(start, end);

		BString string;
		if (GetCurrentMessage()->FindString("bytes", &string) != B_OK)
			string = "";
		string.Prepend(TextControl()->Text());

		BString autocomplete = acc->AutocompleteCategory(string.String());

		if (autocomplete.CountChars() > 0) {
			BMessage automsg(M_CATEGORY_AUTOCOMPLETE);
			automsg.AddInt32("start", strlen(TextControl()->Text()) + 1);
			automsg.AddString("string", autocomplete.String());
			SendMessage(&automsg);
		}
	}

	return B_DISPATCH_MESSAGE;
}

CategoryBox::CategoryBox(
	const char* name, const char* label, const char* text, BMessage* msg, uint32 flags)
	: AutoTextControl(name, label, text, msg, flags)
{
	SetFilter(new CategoryBoxFilter(this));
	SetCharacterLimit(32);
}

bool
CategoryBox::Validate(void)
{
	if (strlen(Text()) < 1) {
		DAlert* alert = new DAlert(B_TRANSLATE("Category is missing."),
			B_TRANSLATE("Do you want to add this transaction without a category?"),
			B_TRANSLATE("Add"), B_TRANSLATE("Cancel"), NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		int32 value = alert->Go();
		if (value == 0)
			SetText(B_TRANSLATE("Uncategorized"));
		else
			return false;
	}
	BString string(Text());
	CapitalizeEachWord(string);
	SetText(string.String());
	return true;
}
