#include "SplitViewFilter.h"
#include "Account.h"
#include "AutoTextControl.h"
#include "CategoryBox.h"
#include "Database.h"
#include "MsgDefs.h"
#include "TimeSupport.h"

#include <Catalog.h>
#include <Button.h>
#include <TextControl.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SplitFilterView"


SplitViewFilter::SplitViewFilter(SplitView *checkview)
	: BMessageFilter(B_PROGRAMMED_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN), fView(checkview) {}

SplitViewFilter::~SplitViewFilter(void) {}

filter_result
SplitViewFilter::Filter(BMessage *msg, BHandler **target) {
	int32 mod;
	if (msg->FindInt32("modifiers", &mod) == B_OK) {
		if ((mod & B_COMMAND_KEY) || (mod & B_COMMAND_KEY) || (mod & B_OPTION_KEY))
			return B_DISPATCH_MESSAGE;
	}

	BView *v = dynamic_cast<BView *>(*target);
	if (!v)
		return B_DISPATCH_MESSAGE;

	BTextControl *text = dynamic_cast<BTextControl *>(v->Parent());
	BButton *button = dynamic_cast<BButton *>(v);
	if (!text && !button)
		return B_DISPATCH_MESSAGE;

	int32 rawchar;
	msg->FindInt32("raw_char", &rawchar);

	if (rawchar == B_TAB) {
		if (mod & B_SHIFT_KEY)
			fView->fMessenger->SendMessage(new BMessage(M_PREVIOUS_FIELD));
		else
			fView->fMessenger->SendMessage(new BMessage(M_NEXT_FIELD));
		return B_SKIP_MESSAGE;
	}

	//		Enter Key Navigation Behavior:
	//
	//		If the transaction is not split, Enter jumps between fields until it hits the
	//		memo field. From the memo field, it is as if the Enter button was clicked.
	//
	//		If the transaction is split, it jumps from the memo field to the split
	//		category field. The regular category box is disabled while the split list is
	//		shown. When Enter is pressed from the split memo field, it is also as if
	//		the Enter button was pushed
	if (rawchar == B_ENTER) {
		if (button) {
			if (button == fView->fEnter) {
				fView->fMessenger->SendMessage(new BMessage(M_ENTER_TRANSACTION));
				return B_DISPATCH_MESSAGE;
			}

#ifdef ENTER_NAVIGATION

			else if (button == fView->fSplit) {
				fView->ToggleSplit();
				fView->fMessenger->SendMessage(new BMessage(M_NEXT_FIELD));
				return B_SKIP_MESSAGE;
			} else {
				fView->fMessenger->SendMessage(new BMessage(M_NEXT_FIELD));
				return B_SKIP_MESSAGE;
			}

#endif
		}

		if (!fView->fSplitContainer->IsHidden()) {
			// Enter transaction if the last split item amount box
			//			if(text==fView->fSplitCategory || text==fView->fSplitAmount)
			//			{
			//				fView->fMessenger->SendMessage(new BMessage(M_NEXT_FIELD));
			//			}
			//			else
			//			{
			//				if(text==fView->fSplitMemo)
			//					fView->fMessenger->SendMessage(new BMessage(M_ENTER_TRANSACTION));
			//			}

			if (text == fView->fSplitMemo)
				fView->fMessenger->SendMessage(new BMessage(M_ENTER_TRANSACTION));

#ifdef ENTER_NAVIGATION

			else
				fView->fMessenger->SendMessage(new BMessage(M_NEXT_FIELD));

#endif
		}
		return B_DISPATCH_MESSAGE;
	}

	// Weed out navigation keys
	//	if(text!=fView->fSplitCategory && text!=fView->fSplitAmount && text!=fView->fSplitMemo)
	//	if(rawchar<32 || (button && rawchar==B_SPACE))
	switch (rawchar) {
	case B_UP_ARROW:
	case B_DOWN_ARROW:
	case B_LEFT_ARROW:
	case B_RIGHT_ARROW:
	case B_HOME:
	case B_END: {
		return B_DISPATCH_MESSAGE;
	}
	case B_SPACE: {
		if (button)
			return B_DISPATCH_MESSAGE;
	}
	default:
		break;
	}

	int32 start, end;
	text->TextView()->GetSelection(&start, &end);

	Account *acc = gDatabase.CurrentAccount();

	if (text == fView->fSplitCategory || text == fView->fSplitAmount || text == fView->fSplitMemo) {
		switch (rawchar) {
		case B_UP_ARROW:
		case B_DOWN_ARROW:
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW: {
			break;
		}
		case B_DELETE:
		case B_BACKSPACE: {
			// handle this case because of UTF-8 handling needed

			BMessage editmsg(M_EDIT_KEY);

			if (text == fView->fSplitCategory) {
				editmsg.AddInt32("command", M_SPLIT_CATEGORY_CHANGED);
			} else if (text == fView->fSplitAmount) {
				editmsg.AddInt32("command", M_SPLIT_AMOUNT_CHANGED);
			} else if (text == fView->fSplitMemo) {
				editmsg.AddInt32("command", M_SPLIT_MEMO_CHANGED);
			}

			fView->fMessenger->SendMessage(&editmsg);
			break;
		}
		default: {
			if (text == fView->fSplitAmount) {
				BString amount_disallowed("`~!@#$%^&*()_-+=QWERTYUIOP{[}]|\\ASDFGHJKL;:'\""
										  "ZXCVBNM<>?qwertyuiopasdfghjklzxcvbnm");
				if (amount_disallowed.FindFirst(rawchar) != B_ERROR)
					return B_SKIP_MESSAGE;
			}

			BMessage keymsg(M_EDIT_KEY);
			if (text == fView->fSplitCategory) {
				keymsg.AddInt32("command", M_SPLIT_CATEGORY_CHANGED);
			} else if (text == fView->fSplitAmount) {
				keymsg.AddInt32("command", M_SPLIT_AMOUNT_CHANGED);
			} else if (text == fView->fSplitMemo) {
				keymsg.AddInt32("command", M_SPLIT_MEMO_CHANGED);
			}

			fView->fMessenger->SendMessage(&keymsg);
			break;
		}
		}
		if (end == (int32)strlen(text->Text())) {
			text->TextView()->Delete(start, end);

			BString string;
			if (msg->FindString("bytes", &string) != B_OK)
				string = "";
			string.Prepend(text->Text());

			BString autocomplete = acc->AutocompleteCategory(string.String());

			if (autocomplete.CountChars() > 0 && autocomplete != B_TRANSLATE("Split")) {
				BMessage automsg(M_CATEGORY_AUTOCOMPLETE);
				automsg.AddInt32("start", strlen(text->Text()) + 1);
				automsg.AddString("string", autocomplete.String());
				fView->fMessenger->SendMessage(&automsg);
			}
		}
	}

	return B_DISPATCH_MESSAGE;
}
