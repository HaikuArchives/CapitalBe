/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	dospuntos (Johan Wagenheim)
 *	humdinger (Joachim Seemer)
 */
#include "ReconcileItem.h"

#include <ListView.h>
#include <Region.h>
#include <View.h>

#include <ctime>
#include <stdio.h>

#include "CBLocale.h"
#include "Database.h"
#include "Preferences.h"
#include "TransactionData.h"


ReconcileItem::ReconcileItem(const TransactionData& trans)
	:
	BStringItem(""),
	fTransaction(trans)
{
	fValue = fTransaction.Status();
}


ReconcileItem::~ReconcileItem()
{
}


void
ReconcileItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	BListView* list = (BListView*)owner;
	int32 index = list->IndexOf(this);

	if (IsSelected()) {
		owner->SetHighUIColor(B_LIST_SELECTED_BACKGROUND_COLOR);
		owner->FillRect(frame);
		owner->SetHighUIColor(B_CONTROL_HIGHLIGHT_COLOR);
		owner->StrokeRect(frame);
	} else {
		if (index % 2 == 1) { // darken odd row
			owner->SetHighUIColor(B_LIST_BACKGROUND_COLOR,
				GetMutedTint(ui_color(B_LIST_BACKGROUND_COLOR), CB_ALT_ROW));
		} else
			owner->SetHighUIColor(B_LIST_BACKGROUND_COLOR);

		owner->FillRect(frame);
	}

	owner->SetFont(IsReconciled() ? be_bold_font : be_plain_font);
	owner->SetHighUIColor(IsSelected() ? B_LIST_SELECTED_ITEM_TEXT_COLOR : B_LIST_ITEM_TEXT_COLOR);

	// Compute vertical alignment factor
	font_height fh;
	owner->GetFontHeight(&fh);
	float fontFactor = ceilf(fh.ascent + fh.descent + fh.leading) / 4 + 1;

	// Draw amount first
	BString string;

	if (fTransaction.Amount().IsNegative())
		gCurrentLocale.CurrencyToString(fTransaction.Amount().InvertAsCopy(), string);
	else
		gCurrentLocale.CurrencyToString(fTransaction.Amount(), string);

	float width = owner->StringWidth(string.String());
	owner->DrawString(string.String(), BPoint(frame.right - width - 5, frame.bottom - fontFactor));

	// Draw Payee next
	BRect r(frame);
	r.right -= width + 5;
	BRegion region(r);
	owner->ConstrainClippingRegion(&region);
	owner->DrawString(fTransaction.Payee(), BPoint(frame.left + 5, frame.bottom - fontFactor));
	owner->ConstrainClippingRegion(NULL);
}


void
ReconcileItem::SetReconciled(bool value)
{
	fTransaction.SetStatus(value ? TRANS_RECONCILED : TRANS_OPEN);
}


bool
ReconcileItem::IsReconciled() const
{
	return (fTransaction.Status() == TRANS_RECONCILED);
}


void
ReconcileItem::SyncToTransaction()
{
	if (fTransaction.Status() == TRANS_RECONCILED)
		gDatabase.SetTransactionStatus(fTransaction.GetID(), TRANS_RECONCILED);
	else
		gDatabase.SetTransactionStatus(fTransaction.GetID(), TRANS_OPEN);
}
