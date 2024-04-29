#include "TransactionItem.h"
#include <Catalog.h>
#include <ListView.h>
#include <Region.h>
#include <View.h>
#include <stdio.h>
#include <ctime>
#include "Account.h"
#include "CBLocale.h"
#include "Category.h"
#include "Database.h"
#include "MainWindow.h"
#include "Preferences.h"
#include "TransactionData.h"
#include "TransactionLayout.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TransactionItem"


TransactionItem::TransactionItem(const TransactionData& trans)
	: BListItem(),
	  fDate(trans.Date()),
	  fAccount(trans.GetAccount()),
	  fType(trans.Type().Type()),
	  fPayee(trans.Payee()),
	  fAmount(trans.Amount()),
	  fCategory(""),
	  fMemo(trans.Memo()),
	  fStatus(trans.Status()),
	  fID(trans.GetID()),
	  fTimeStamp(trans.GetTimeStamp())
{
	if (trans.CountCategories() > 1)
		fCategory = B_TRANSLATE("Split");
	else
		fCategory = trans.NameAt(0);
}

void
TransactionItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	BString string;
	Locale locale = fAccount->GetLocale();

	BRect r(frame);
	r.right--;

	if (IsSelected()) {
		owner->SetHighUIColor(B_LIST_SELECTED_BACKGROUND_COLOR);
		owner->SetLowUIColor(B_LIST_SELECTED_ITEM_TEXT_COLOR);
		owner->FillRect(frame);
		owner->SetHighUIColor(B_CONTROL_BORDER_COLOR);
		owner->StrokeRect(frame);
		owner->SetHighUIColor(B_LIST_SELECTED_ITEM_TEXT_COLOR);
	} else {
		if (fStatus == TRANS_RECONCILED) {
			owner->SetHighUIColor(B_LIST_BACKGROUND_COLOR, B_DISABLED_MARK_TINT);
			owner->SetLowUIColor(B_LIST_BACKGROUND_COLOR, B_DISABLED_MARK_TINT);
			owner->FillRect(frame);
			owner->SetHighUIColor(B_CONTROL_BORDER_COLOR, B_DISABLED_MARK_TINT);
			owner->StrokeLine(r.LeftBottom(), r.RightBottom());
			owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);
		} else {
			owner->SetHighUIColor(B_LIST_BACKGROUND_COLOR);
			owner->SetLowUIColor(B_LIST_BACKGROUND_COLOR);
			owner->FillRect(frame);
			owner->SetHighUIColor(B_CONTROL_BORDER_COLOR);
			owner->StrokeLine(r.LeftBottom(), r.RightBottom());
		}
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
	locale.DateToString(fDate, string);
	owner->DrawString(string.String(), BPoint(xpos, ypos - 6));
	owner->ConstrainClippingRegion(NULL);

	xpos += TDateWidth();
	owner->SetHighUIColor(B_CONTROL_BORDER_COLOR);

	// Line Between Date & Type
	owner->StrokeLine(BPoint(xpos, ypos - TRowHeight()), BPoint(xpos, ypos));

	owner->StrokeLine(BPoint(0, ypos), BPoint(r.right, ypos));

	// Type
	owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);
	owner->DrawString(fType.String(), BPoint(xpos + 5, ypos - 6));

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

	Fixed balance = fAccount->BalanceAtTransaction(fDate, fPayee.String());
	if (balance.AsFixed() < 0)
		owner->SetHighUIColor(B_FAILURE_COLOR);
	locale.CurrencyToString(balance, string);
	owner->DrawString(string.String(), BPoint(xpos + 5, ypos - 6));

	// Line between Balance and Amount
	owner->SetHighUIColor(B_CONTROL_BORDER_COLOR);
	owner->StrokeLine(BPoint(xpos, ypos - TRowHeight()), BPoint(xpos, ypos));

	// Amount
	xpos -= TAmountWidth();
	cliprect.right = cliprect.left;
	cliprect.left = xpos;
	clip = cliprect;
	owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);
	fAccount->GetLocale().CurrencyToString(fAmount.AbsoluteValue(), string);

	owner->ConstrainClippingRegion(&clip);
	owner->DrawString(string.String(), BPoint(xpos + 5, ypos - 6));
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
	owner->DrawString(fPayee.String(), BPoint(xpos + 5, ypos - 6));
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
	owner->DrawString(fCategory.String(), BPoint(xpos, ypos - 6));
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
		owner->DrawString(fMemo.String(), BPoint(xpos + 5, ypos - 6));
	} else {
		owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR, B_DISABLED_LABEL_TINT);
		owner->DrawString(B_TRANSLATE("No Memo"), BPoint(xpos + 5, ypos - 6));
	}
	owner->ConstrainClippingRegion(NULL);
}

void
TransactionItem::Update(BView* owner, const BFont* finfo)
{
	BListItem::Update(owner, finfo);
	SetHeight(TRowHeight() * 2);
}

void
TransactionItem::SetData(const TransactionData& trans)
{
	fDate = trans.Date();
	fAccount = trans.GetAccount();
	fType = trans.Type().Type();
	fPayee = trans.Payee();
	fAmount = trans.Amount();
	if (trans.CountCategories() > 1)
		fCategory = B_TRANSLATE("Split");
	else
		fCategory = trans.NameAt(0);
	fMemo = trans.Memo();
	fStatus = trans.Status();
	fID = trans.GetID();
}
