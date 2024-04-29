#include "ReconcileItem.h"
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


ReconcileItem::ReconcileItem(const TransactionData& trans) : BStringItem(""), fTransaction(trans)
{
	fValue = fTransaction.Status();
}

ReconcileItem::~ReconcileItem(void) {}

void
ReconcileItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	if (IsSelected()) {
		owner->SetHighUIColor(B_LIST_SELECTED_BACKGROUND_COLOR);
		owner->SetLowUIColor(B_LIST_SELECTED_BACKGROUND_COLOR);
	} else {
		owner->SetHighUIColor(B_LIST_BACKGROUND_COLOR);
		owner->SetLowUIColor(B_LIST_BACKGROUND_COLOR);
	}
	owner->FillRect(frame);
	if (IsSelected()) {
		owner->SetHighUIColor(B_LIST_SELECTED_BACKGROUND_COLOR);
		owner->StrokeRect(frame);
	}

	if (IsReconciled())
		owner->SetFont(be_bold_font);
	else
		owner->SetFont(be_plain_font);

	owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);

	// Draw amount first
	BString string;

	if (fTransaction.Amount().IsNegative())
		gCurrentLocale.CurrencyToString(fTransaction.Amount().InvertAsCopy(), string);
	else
		gCurrentLocale.CurrencyToString(fTransaction.Amount(), string);

	float width = owner->StringWidth(string.String());
	owner->DrawString(string.String(), BPoint(frame.right - width, frame.bottom));


	// Draw Payee next
	BRect r(frame);
	r.right -= width + 5;
	BRegion region(r);
	owner->ConstrainClippingRegion(&region);
	owner->DrawString(fTransaction.Payee(), BPoint(frame.left + 1, frame.bottom - 2));
	owner->ConstrainClippingRegion(NULL);
}

void
ReconcileItem::SetReconciled(bool value)
{
	fTransaction.SetStatus(value ? TRANS_RECONCILED : TRANS_OPEN);
}

bool
ReconcileItem::IsReconciled(void) const
{
	return (fTransaction.Status() == TRANS_RECONCILED);
}

void
ReconcileItem::SyncToTransaction(void)
{
	if (fTransaction.Status() == TRANS_RECONCILED)
		gDatabase.SetTransactionStatus(fTransaction.GetID(), TRANS_RECONCILED);
	else
		gDatabase.SetTransactionStatus(fTransaction.GetID(), TRANS_OPEN);
}
