/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	bitigchi (Emir Sari)
 *	dospuntos (Johan Wagenheim)
 *	humdinger (Joachim Seemer)
 *	raefaldhia (Raefaldhi Amartya Junior)
 */
#include "RegisterView.h"

#include <Catalog.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <StringView.h>

#include "Account.h"
#include "AccountListView.h"
#include "CheckView.h"
#include "Database.h"
#include "FilterView.h"
#include "MainWindow.h"
#include "Preferences.h"
#include "QuickTrackerItem.h"
#include "TimeSupport.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RegisterView"


RegisterView::RegisterView(const char* name, int32 flags)
	: BView(name, flags | B_FRAME_EVENTS)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	// Accounts
	BStringView* accountLabel = new BStringView("accountlabel", B_TRANSLATE("Accounts"));
	accountLabel->SetFont(be_bold_font);
	accountLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	//	fAccountView = new DragListView(r,"accountview");
	fAccountView = new AccountList();
	fAccountView->SetSelectionMessage(new BMessage(M_SELECT_ACCOUNT));
	fAccountView->SetInvocationMessage(new BMessage(M_SHOW_ACCOUNT_SETTINGS));

	fAccountScroller = new BScrollView("accountscroll", fAccountView, 0, false, true);
	fAccountScroller->SetViewColor(ViewColor());

	// Checkview
	fCheckView = new CheckView("checkview", B_WILL_DRAW);
	gDatabase.AddObserver(fCheckView);

	for (int32 i = 0; i < gDatabase.CountAccounts(); i++) {
		Account* acc = gDatabase.AccountAt(i);
		fAccountView->AddItem(new AccountListItem(acc));
		acc->AddObserver(this);
	}

	// Transactions list
	fTransactionlabel = new BStringView("transactionlabel", B_TRANSLATE("Transactions"));
	fTransactionlabel->SetFont(be_bold_font);
	fTransactionlabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fTransactionView = new TransactionView();
	gDatabase.AddObserver(fTransactionView);
	gDatabase.AddObserver(this);

	// Filter
	BBox* filterBox = new BBox("filterbox");
	filterBox->SetLabel(B_TRANSLATE("Filter"));
	fFilterView = new FilterView("filterview", B_WILL_DRAW);

	// QuickTracker
	BBox* qtBox = new BBox("qtbox");
	qtBox->SetLabel(B_TRANSLATE("QuickTracker"));
	QTNetWorthItem* qtItem = new QTNetWorthItem("networth");

	// clang-format off
	BLayoutBuilder::Group<>(filterBox, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_BIG_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fFilterView)
		.End();

	BLayoutBuilder::Group<>(qtBox, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_BIG_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(qtItem)
		.AddGlue(10)
		.End();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING, 0, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL)
			.AddGroup(B_VERTICAL, 0)
				.Add(accountLabel)
				.Add(fAccountScroller, 2)
				.AddStrut(B_USE_DEFAULT_SPACING)
				.Add(filterBox)
				.AddStrut(B_USE_DEFAULT_SPACING)
				.Add(qtBox)
				.End()
			.AddGroup(B_VERTICAL, 0)
				.Add(fTransactionlabel)
				.Add(fTransactionView)
				.Add(fCheckView)
				.End()
			.End()
		.End();
}
// clang-format on

RegisterView::~RegisterView()
{
	prefsLock.Lock();
	gPreferences.RemoveData("selected-account");
	gPreferences.AddInt32("selected-account", fAccountView->CurrentSelection());
	prefsLock.Unlock();
}


void
RegisterView::AttachedToWindow()
{
	fAccountView->SetTarget(this);
	fFilterView->SetMessenger(new BMessenger(this));

	int32 index;
	if (index > fAccountView->CountItems() || index < 0
		|| gPreferences.FindInt32("selected-account", &index) != B_OK)
		index = 0;

	fAccountView->Select(index);

	fCheckView->MakeFocus(true);
}


void
RegisterView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_SELECT_ACCOUNT:
		{
			if (fAccountView->CurrentSelection() < 0)
				break;
			gDatabase.SetCurrentAccount(fAccountView->CurrentSelection());
			fCheckView->SetFieldsEnabled(!gDatabase.CurrentAccount()->IsClosed());

			break;
		}
		case M_FILTER:
		{
			if (fFilterView->IsEmpty())
				fTransactionlabel->SetText(B_TRANSLATE("Transactions"));
			else
				fTransactionlabel->SetText(B_TRANSLATE("Transactions (filtered)"));
			fTransactionView->MessageReceived(msg);
			break;
		}
		case M_SHOW_ACCOUNT_SETTINGS:
		{
			if (Window())
				Window()->PostMessage(M_SHOW_ACCOUNT_SETTINGS);
			break;
		}
		default:
		{
			BView::MessageReceived(msg);
			break;
		}
	}
}


void
RegisterView::HandleNotify(const uint64& value, const BMessage* msg)
{
	bool lockwin = false;
	if (!Window()->IsLocked()) {
		Window()->Lock();
		lockwin = true;
	}

	if (value & WATCH_ACCOUNT) {
		Account* acc;
		if (msg->FindPointer("item", (void**)&acc) != B_OK) {
			if (lockwin)
				Window()->Unlock();
			return;
		}

		if (value & WATCH_CREATE) {
			fAccountView->AddItem(new AccountListItem(acc));
			if (fAccountView->CountItems() == 1)
				fAccountView->Select(0);
			acc->AddObserver(this);
		} else if (value & WATCH_DELETE) {
			AccountListItem* item
				= (AccountListItem*)fAccountView->RemoveItem(gDatabase.IndexOf(acc));
			delete item;
			fAccountView->Select(0);
		} else if (value & WATCH_CHANGE) {
			for (int32 i = 0; i < fAccountView->CountItems(); i++) {
				AccountListItem* listitem = (AccountListItem*)fAccountView->ItemAt(i);
				if (listitem && listitem->GetAccount() == acc) {
					fAccountView->InvalidateItem(i);
					break;
				}
			}
			fCheckView->SetFieldsEnabled(!acc->IsClosed());
		}
	} else if (value & WATCH_TRANSACTION) {
		if (value & WATCH_CREATE || value & WATCH_DELETE || value & WATCH_CHANGE)
			fAccountView->Invalidate();
	} else if (value & WATCH_LOCALE) {
		for (int32 i = 0; i < fAccountView->CountItems(); i++) {
			AccountListItem* listitem = (AccountListItem*)fAccountView->ItemAt(i);
			if (listitem)
				fAccountView->InvalidateItem(i);
		}
	}
	if (lockwin)
		Window()->Unlock();
}


void
RegisterView::SelectAccount(const int32& index)
{
	if (index < 0 || index > fAccountView->CountItems() - 1)
		return;

	fAccountView->Select(index);
}


void
RegisterView::SetCheckFields(TransactionData data)
{
	BString amount;
	gCurrentLocale.CurrencyToString(data.Amount().AbsoluteValue(), amount);

	BString date;
	gDefaultLocale.DateToString(GetCurrentDate(), date);

	fCheckView->SetFields(
		date, data.Type().Type(), data.Payee(), amount.String(), data.NameAt(0), data.MemoAt(0));

	BTextControl* dateBox = (BTextControl*)fCheckView->FindView("dateentry");
	dateBox->TextView()->MakeFocus(true);
}
