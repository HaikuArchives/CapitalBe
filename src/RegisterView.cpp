#include "RegisterView.h"

#include <Catalog.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <StringView.h>

#include "Account.h"
#include "AccountListItem.h"
#include "CheckView.h"
#include "Database.h"
#include "MainWindow.h"
#include "QuickTrackerItem.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RegisterView"

enum { M_SELECT_ACCOUNT = 'slac', M_SELECT_CURRENT };

RegisterView::RegisterView(const char* name, int32 flags) : BView(name, flags | B_FRAME_EVENTS)
{
	SetViewColor(240, 240, 240);

	BStringView* accountlabel = new BStringView("accountlabel", B_TRANSLATE("Accounts"));

	//	fAccountView = new DragListView(r,"accountview");
	fAccountView = new BListView("accountview", B_SINGLE_SELECTION_LIST);
	fAccountView->SetSelectionMessage(new BMessage(M_SELECT_ACCOUNT));
	fAccountView->SetInvocationMessage(new BMessage(M_SHOW_ACCOUNT_SETTINGS));
	fAccountScroller = new BScrollView("accountscroll", fAccountView, 0, true, true);
	fAccountScroller->SetViewColor(ViewColor());

	fCheckView = new CheckView("checkview", B_WILL_DRAW);
	gDatabase.AddObserver(fCheckView);

	for (int32 i = 0; i < gDatabase.CountAccounts(); i++) {
		Account* acc = gDatabase.AccountAt(i);
		fAccountView->AddItem(new AccountListItem(acc));
		acc->AddObserver(this);
	}

	fTransactionView = new TransactionView();
	gDatabase.AddObserver(fTransactionView);
	gDatabase.AddObserver(this);

	fTrackBox = new BBox("qtbox");
	fTrackBox->SetLabel(B_TRANSLATE("QuickTracker"));

	QTNetWorthItem* item;
	item = new QTNetWorthItem("networth");

	BFont font;
	BLayoutBuilder::Group<>(fTrackBox, B_VERTICAL, 0)
		.SetInsets(10, font.Size() * 1.3, 10, 10)
		.Add(item)
		.AddGlue(1024 * 1024 * 2014)
		.End();
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(10)
		.Add(accountlabel)
		.AddGroup(B_HORIZONTAL)
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING, 1)
		.Add(fAccountScroller)
		.Add(fTrackBox)
		.End()
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING, 3)
		.Add(fTransactionView)
		.Add(fCheckView)
		.End()
		.End()
		.End();
}

RegisterView::~RegisterView(void) {}

void
RegisterView::AttachedToWindow(void)
{
	fAccountView->SetTarget(this);

	// If the selection done is before being attached to the window, the message is
	// never received.
	bool selected = false;
	for (int32 i = 0; i < gDatabase.CountAccounts(); i++) {
		Account* acc = gDatabase.AccountAt(i);
		if (acc && !acc->IsClosed()) {
			fAccountView->Select(i);
			selected = true;
		}
	}
	if (!selected)
		fAccountView->Select(0);

	fCheckView->MakeFocus(true);
}

void
RegisterView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case M_SELECT_ACCOUNT: {
		if (fAccountView->CurrentSelection() < 0)
			break;
		gDatabase.SetCurrentAccount(fAccountView->CurrentSelection());
		break;
	}
	case M_SHOW_ACCOUNT_SETTINGS: {
		if (Window())
			Window()->PostMessage(M_SHOW_ACCOUNT_SETTINGS);
		break;
	}
	default: {
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
		}
		else if (value & WATCH_DELETE) {
			AccountListItem* item =
				(AccountListItem*)fAccountView->RemoveItem(gDatabase.IndexOf(acc));
			delete item;
			fAccountView->Select(0);
		}
		else if (value & WATCH_CHANGE) {
			for (int32 i = 0; i < fAccountView->CountItems(); i++) {
				AccountListItem* listitem = (AccountListItem*)fAccountView->ItemAt(i);
				if (listitem && listitem->GetAccount() == acc) {
					fAccountView->InvalidateItem(i);
					break;
				}
			}
		}

		// Adjust the horizontal scroll bar every time there is a change

		float maxwidth = 0;
		for (int32 i = 0; i < gDatabase.CountAccounts(); i++) {
			acc = gDatabase.AccountAt(i);

			float namewidth = be_bold_font->StringWidth(acc->Name()) + B_V_SCROLL_BAR_WIDTH + 10;
			maxwidth = (namewidth > maxwidth) ? namewidth : maxwidth;
		}

		float range = maxwidth - fAccountScroller->Bounds().Width();
		if (range < 0)
			range = 0;

		BScrollBar* bar = fAccountScroller->ScrollBar(B_HORIZONTAL);
		bar->SetRange(0, range);
	}
	else if (value & WATCH_TRANSACTION) {
		if (value & WATCH_CREATE || value & WATCH_DELETE || value & WATCH_CHANGE)
			fAccountView->Invalidate();
	}
	else if (value & WATCH_LOCALE) {
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
