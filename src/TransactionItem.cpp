#include "TransactionItem.h"
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

	float textTint = B_NO_TINT;
	if (fAccount->IsClosed()) {
		// Gray out all transactions from closed accounts
		// Todo: disable editing of transactions from closed accounts
		textTint = GetMutedTint(CB_MUTED_TEXT);
	}

	BRect r(frame);
	r.right--;

	if (IsSelected()) {
		owner->SetHighUIColor(B_LIST_SELECTED_BACKGROUND_COLOR);
		owner->FillRect(frame);
		owner->SetHighUIColor(
			fStatus == TRANS_RECONCILED ? B_TOOL_TIP_BACKGROUND_COLOR : B_CONTROL_HIGHLIGHT_COLOR);
		owner->StrokeRect(frame);
	} else if (fStatus == TRANS_RECONCILED) {
		owner->SetHighUIColor(B_MENU_BACKGROUND_COLOR, GetMutedTint(CB_MUTED_BG));
		owner->FillRect(frame);
		owner->SetHighUIColor(B_CONTROL_TEXT_COLOR);
		owner->StrokeLine(r.LeftBottom(), r.RightBottom());
	} else {
		owner->SetHighUIColor(B_CONTROL_TEXT_COLOR);
		owner->StrokeLine(r.LeftBottom(), r.RightBottom());
	}

	owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR, textTint);

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

	// Line Between Date & Type
	owner->SetHighUIColor(B_CONTROL_BORDER_COLOR);
	owner->StrokeLine(BPoint(xpos, ypos - TRowHeight()), BPoint(xpos, ypos));

	// Type
	owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR, textTint);
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

	owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR, textTint);
	Fixed balance = fAccount->BalanceAtTransaction(fDate, fPayee.String());
	if (balance.AsFixed() < 0)
		owner->SetHighUIColor(B_FAILURE_COLOR, textTint);
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
	owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR, textTint);
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

	owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR, textTint);
	clip = payee_rect;
	owner->ConstrainClippingRegion(&clip);
	owner->DrawString(fPayee.String(), BPoint(xpos + 5, ypos - 6));
	owner->ConstrainClippingRegion(NULL);

	owner->SetHighUIColor(B_CONTROL_BORDER_COLOR);
	owner->StrokeLine(BPoint(r.left, ypos), BPoint(r.right, ypos));

	// Category
	owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR, textTint);
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
	owner->StrokeLine(BPoint(xpos, ypos - TRowHeight()), BPoint(xpos, ypos - 2));

	// Memo
	clip = cliprect;
	owner->ConstrainClippingRegion(&clip);
	if (fMemo.CountChars() > 0) {
		owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR, textTint);
		owner->DrawString(fMemo.String(), BPoint(xpos + 5, ypos - 6));
	} else {
		owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR, GetMutedTint(CB_MUTED_TEXT));
		owner->DrawString(B_TRANSLATE("No memo"), BPoint(xpos + 5, ypos - 6));
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
