#include "AccountListItem.h"
#include <Catalog.h>
#include <Font.h>
#include <ListView.h>
#include <String.h>
#include <View.h>
#include "Account.h"
#include "CBLocale.h"
#include "Preferences.h"
#include "TransactionLayout.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AccountListItem"


AccountListItem::AccountListItem(Account* acc)
	: BListItem()
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
	// Draw item background
	owner->SetHighUIColor(
		IsSelected() ? B_LIST_SELECTED_BACKGROUND_COLOR : B_LIST_BACKGROUND_COLOR);
	owner->FillRect(frame);

	// Draw item border
	if (IsSelected()) {
		owner->SetHighUIColor(fAccount->IsClosed() ? B_FAILURE_COLOR : B_CONTROL_MARK_COLOR);
		owner->StrokeRect(frame);
	}

	// Draw account title
	owner->SetFont(be_bold_font);
	owner->SetHighUIColor(
		B_LIST_ITEM_TEXT_COLOR, fAccount->IsClosed() ? GetMutedTint(CB_MUTED_TEXT) : B_NO_TINT);

	BFont font;
	owner->DrawString(fAccount->Name(), BPoint(frame.left + 5, frame.top + (font.Size())));

	// Draw Balance (or "Closed")
	owner->SetFont(be_plain_font);
	if (fAccount->IsClosed()) {
		owner->DrawString(
			B_TRANSLATE("Closed"), BPoint(frame.left + 5, frame.top + (font.Size() * 2)));
	} else {
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
