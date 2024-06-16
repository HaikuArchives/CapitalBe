#include "NumBox.h"
#include "MsgDefs.h"


NumBoxFilter::NumBoxFilter(NumBox* box)
	: AutoTextControlFilter(box)
{
}

filter_result
NumBoxFilter::KeyFilter(const int32& key, const int32& mod)
{
	// Here is where all the *real* work for a date box is done.
	if (key == B_TAB) {
		if (!((NumBox*)TextControl())->IsTabFiltering())
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

	//	if(key == B_ESCAPE && !IsEscapeCancel())
	//		return B_SKIP_MESSAGE;

	return B_DISPATCH_MESSAGE;
}

NumBox::NumBox(const char* name, const char* label, const char* text, BMessage* msg, uint32 flags)
	: AutoTextControl(name, label, text, msg, flags),
	  fFilterTab(true),
	  fAllowNegatives(true)
{
	SetFilter(new NumBoxFilter(this));

	const char num_disallowed[]
		= " `~!@#%^&*()_+=QWERTYUIOP{[}]|\\ASDFGHJKL;:'\""
		  "ZXCVBNM<>?/qwertyuiopasdfghjklzxcvbnm$¥£";
	int32 i = 0;
	while (num_disallowed[i]) {
		TextView()->DisallowChar(num_disallowed[i]);
		i++;
	}
}

bool
NumBox::Validate(bool alert)
{
	if (strlen(Text()) < 1)
		SetText("0");

	return true;
}

void
NumBox::AllowNegatives(const bool& value)
{
	if (fAllowNegatives != value) {
		fAllowNegatives = value;
		if (fAllowNegatives)
			TextView()->AllowChar('-');
		else
			TextView()->DisallowChar('-');
	}
}
