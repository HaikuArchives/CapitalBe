#include "ScheduledTransItem.h"
#include <Catalog.h>
#include <ListView.h>
#include <Region.h>
#include <View.h>
#include <stdio.h>
#include <ctime>
#include "Account.h"
#include "CBLocale.h"
#include "Database.h"
#include "Preferences.h"
#include "TransactionData.h"
#include "TransactionLayout.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ScheduledTransItem"


ScheduledTransItem::ScheduledTransItem(const ScheduledTransData& data)
	: BListItem(),
	  fAccount(data.GetAccount()),
	  fType(data.Type().Type()),
	  fPayee(data.Payee()),
	  fAmount(""),
	  fCategory(""),
	  fMemo(data.Memo()),
	  fDate(""),
	  fID(data.GetID())
{
	Locale locale = data.GetAccount()->GetLocale();
	locale.CurrencyToString(data.Amount().AbsoluteValue(), fAmount);
	gDefaultLocale.DateToString(data.Date(), fDate);

	if (data.CountCategories() > 1)
		fCategory = B_TRANSLATE("Split");
	else
		fCategory = data.NameAt(0);
}

void
ScheduledTransItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	BString string;
	Locale locale = fAccount->GetLocale();

	BRect r(frame);
	r.right--;

	if (IsSelected()) {
		owner->SetHighUIColor(B_LIST_SELECTED_BACKGROUND_COLOR);
		owner->FillRect(frame);
		owner->SetHighUIColor(B_CONTROL_HIGHLIGHT_COLOR);
		owner->StrokeRect(frame);
	} else {
		owner->SetHighUIColor(B_LIST_BACKGROUND_COLOR);
		owner->FillRect(frame);
		owner->SetHighUIColor(B_CONTROL_BORDER_COLOR);
		owner->StrokeLine(r.LeftBottom(), r.RightBottom());
	}

	owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);

	BRect cliprect;
	BRegion clip(cliprect);
	float xpos = TLeftPadding();
	float ypos = r.top + TRowHeight();

	// Date
	cliprect.left = xpos;
	cliprect.right = xpos + TDateWidth();
	cliprect.top = ypos - TRowHeight();
	cliprect.bottom = ypos;

	clip = cliprect;
	owner->ConstrainClippingRegion(&clip);
	owner->DrawString(fDate.String(), BPoint(xpos, ypos - 3));
	owner->ConstrainClippingRegion(NULL);

	xpos += TDateWidth();
	owner->SetHighUIColor(B_CONTROL_BORDER_COLOR);

	// Line Between Date & Type
	owner->StrokeLine(BPoint(xpos, ypos - TRowHeight()), BPoint(xpos, ypos));

	owner->StrokeLine(BPoint(0, ypos), BPoint(r.right, ypos));
	owner->SetHighColor(0, 0, 0);

	// Type
	owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);
	owner->DrawString(fType.String(), BPoint(xpos + 5, ypos - 3));

	// Line between Type and Payee
	xpos += TNumWidth();
	owner->SetHighUIColor(B_CONTROL_BORDER_COLOR);
	owner->StrokeLine(BPoint(xpos, ypos - TRowHeight()), BPoint(xpos, ypos));

	// Calculate the rectangle for the payee, but this field depends on the
	// width of the view, so we can't yet easily calculate the right coordinate
	// of the rectangle just yet
	BRect payee_rect(xpos, ypos, xpos, ypos - TRowHeight());


	// Balance
	xpos = r.right - TAmountWidth();
	cliprect.right = r.right;
	cliprect.left = xpos;
	clip = cliprect;
	owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);

	/*	Fixed balance = fAccount->BalanceAt(fDate);
		if(balance.AsFixed()<0)
			owner->SetHighColor(150, 0, 0);
		locale.CurrencyToString(balance,string);
		owner->DrawString(string.String(), BPoint(xpos + 5, ypos - 3));
	*/
	// Line between Balance and Amount
	owner->SetHighUIColor(B_CONTROL_BORDER_COLOR);
	owner->StrokeLine(BPoint(xpos, ypos - TRowHeight()), BPoint(xpos, ypos));

	// Amount
	xpos -= TAmountWidth();
	cliprect.right = cliprect.left;
	cliprect.left = xpos;
	clip = cliprect;
	owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);

	owner->ConstrainClippingRegion(&clip);
	owner->DrawString(fAmount.String(), BPoint(xpos + 5, ypos - 3));
	owner->ConstrainClippingRegion(NULL);

	// Line between Amount and Payee
	owner->SetHighUIColor(B_CONTROL_BORDER_COLOR);
	owner->StrokeLine(BPoint(xpos, ypos - TRowHeight()), BPoint(xpos, ypos));

	// Payee
	payee_rect.right = xpos;
	payee_rect.top = ypos - TRowHeight();
	payee_rect.bottom = ypos;
	xpos = payee_rect.left;

	owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);
	clip = payee_rect;
	owner->ConstrainClippingRegion(&clip);
	owner->DrawString(fPayee.String(), BPoint(xpos + 5, ypos - 3));
	owner->ConstrainClippingRegion(NULL);

	owner->SetHighUIColor(B_CONTROL_BORDER_COLOR);
	owner->StrokeLine(BPoint(r.left, ypos), BPoint(r.right, ypos));

	// Category
	owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);
	ypos += TRowHeight();
	xpos = TLeftPadding();
	cliprect.left = TLeftPadding();
	cliprect.right = r.right / 2;
	cliprect.top = cliprect.bottom;
	cliprect.bottom += TRowHeight();
	clip = cliprect;
	owner->ConstrainClippingRegion(&clip);
	owner->DrawString(fCategory.String(), BPoint(xpos, ypos - 3));
	owner->ConstrainClippingRegion(NULL);

	xpos = r.right / 2;
	cliprect.left = xpos;
	cliprect.right = r.right;

	// Line between Category and Memo
	owner->SetHighUIColor(B_CONTROL_BORDER_COLOR);
	owner->StrokeLine(BPoint(xpos, ypos - TRowHeight()), BPoint(xpos, ypos));

	// Memo
	clip = cliprect;
	owner->ConstrainClippingRegion(&clip);
	if (fMemo.CountChars() > 0) {
		owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);
		owner->DrawString(fMemo.String(), BPoint(xpos + 5, ypos - 3));
	} else {
		owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR,
			GetMutedTint(ui_color(B_LIST_ITEM_TEXT_COLOR), CB_MUTED_TEXT));
		owner->DrawString(B_TRANSLATE("No memo"), BPoint(xpos + 5, ypos - 3));
	}
	owner->ConstrainClippingRegion(NULL);
}

void
ScheduledTransItem::Update(BView* owner, const BFont* finfo)
{
	BListItem::Update(owner, finfo);
	SetHeight(TRowHeight() * 2);
}

void
ScheduledTransItem::SetData(const TransactionData& trans)
{
	fAccount = trans.GetAccount();
	Locale locale = fAccount->GetLocale();

	gDefaultLocale.DateToString(trans.Date(), fDate);
	fType = trans.Type().Type();
	fPayee = trans.Payee();
	locale.CurrencyToString(trans.Amount().AbsoluteValue(), fAmount);
	if (trans.CountCategories() > 1)
		fCategory = B_TRANSLATE("Split");
	else
		fCategory = trans.NameAt(0);
	fMemo = trans.Memo();
	fID = trans.GetID();
}
