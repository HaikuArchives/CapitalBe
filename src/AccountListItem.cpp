#include "AccountListItem.h"
#include "Account.h"
#include "CBLocale.h"
#include "Database.h"
#include "Preferences.h"
#include "TransactionLayout.h"

#include <Catalog.h>
#include <Font.h>
#include <ListView.h>
#include <String.h>
#include <View.h>

#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AccountListItem"

AccountListItem::AccountListItem(Account* acc) : BListItem()
{
	fAccount = acc;
	fEnabled = true;
}

void
AccountListItem::SetEnabled(bool enabled)
{
	fEnabled = enabled;
}

void
AccountListItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	if (IsSelected()) {
		if (IsEnabled()) {
			if (fAccount->IsClosed()) {
				owner->SetHighColor(GetColor(BC_SELECTION_NOFOCUS));
				owner->SetLowColor(GetColor(BC_SELECTION_NOFOCUS));
			}
			else {
				owner->SetHighColor(GetColor(BC_SELECTION_FOCUS));
				owner->SetLowColor(GetColor(BC_SELECTION_FOCUS));
			}
		}
		else {
			owner->SetHighColor(230, 230, 230);
			owner->SetLowColor(230, 230, 230);
		}
	}
	else {
		if (fAccount->IsClosed()) {
			owner->SetHighColor(240, 240, 240, 128);
			owner->SetLowColor(240, 240, 240, 128);
		}
		else {
			owner->SetHighColor(255, 255, 255, 128);
			owner->SetLowColor(255, 255, 255, 128);
		}
	}
	owner->FillRect(frame);

	if (IsSelected()) {
		owner->SetHighColor(100, 100, 100);
		owner->StrokeRect(frame);
	}

	if (IsEnabled())
		owner->SetHighColor(0, 0, 0);
	else
		owner->SetHighColor(200, 200, 200);
	owner->SetFont(be_bold_font);
	BFont font;
	owner->DrawString(fAccount->Name(), BPoint(frame.left + 5, frame.top + (font.Size())));
	owner->SetFont(be_plain_font);

	if (fAccount->IsClosed()) {
		owner->DrawString(
			B_TRANSLATE("Closed"), BPoint(frame.left + 5, frame.top + (font.Size() * 2))
		);
	}
	else {
		BString text;
		fAccount->GetLocale().CurrencyToString(fAccount->Balance(), text);
		owner->DrawString(text.String(), BPoint(frame.left + 5, frame.top + (font.Size() * 2)));
	}
}

void
AccountListItem::Update(BView* owner, const BFont* finfo)
{
	BListItem::Update(owner, finfo);

	// We can afford to make this call because the row height is just a sane
	// value based on the height of be_plain_font, which we are also using here
	SetHeight(TRowHeight() * 2);
}
