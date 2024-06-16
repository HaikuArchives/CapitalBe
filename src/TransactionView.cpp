#include "TransactionView.h"
#include <Bitmap.h>
#include <Box.h>
#include <LayoutBuilder.h>
#include <ScrollBar.h>
#include <StringView.h>
#include <TranslationUtils.h>
#include <stdio.h>
#include <stdlib.h>
#include "Database.h"
#include "MainWindow.h"
#include "TransactionItem.h"
#include "TransactionLayout.h"

TransactionView::TransactionView()
	: BView("transactionview", B_WILL_DRAW | B_SUBPIXEL_PRECISE | B_FRAME_EVENTS),
	  fCurrent(NULL)
{
	InitTransactionItemLayout(this);

	fListView = new BListView("RegisterList", B_SINGLE_SELECTION_LIST,
		B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE);
	fListView->SetSelectionMessage(new BMessage(M_TRANSACTION_SELECTED));
	fListView->SetInvocationMessage(new BMessage(M_TRANSACTION_INVOKED));
	fScrollView = new BScrollView("scrollregister", fListView, 0, false, true);

	fItemList = new BObjectList<TransactionItem>(20, true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0).SetInsets(0).Add(fScrollView).End();
}

TransactionView::~TransactionView(void)
{
	fListView->MakeEmpty();
	delete fItemList;
}

void
TransactionView::AttachedToWindow(void)
{
	SetViewColor(Parent()->ViewColor());
	fListView->SetTarget(this);

	// So the scrollbar initially starts with the proper steps
	BScrollBar* bar = fScrollView->ScrollBar(B_VERTICAL);

	float big, small;
	bar->GetSteps(&small, &big);
	big = (int32)(fScrollView->Frame().Height() * .9);
	bar->SetSteps(small, big);

	for (int32 i = 0; i < gDatabase.CountAccounts(); i++) {
		Account* account = gDatabase.AccountAt(i);
		account->AddObserver(this);
	}
}

void
TransactionView::SetAccount(Account* acc)
{
	// We accept NULL pointers because sometimes we're given them

	// According to the BeBook, calling BListView::MakeEmpty() doesn't free
	// the BListItems, so we have to do this manually. :(
	fListView->MakeEmpty();
	fItemList->MakeEmpty();

	if (!acc)
		return;

	BList itemlist;
	BString command;
	command << ("SELECT * FROM account_") << acc->GetID() << " ORDER BY date,payee";
	CppSQLite3Query query = gDatabase.DBQuery(command.String(), "TransactionView::SetAccount");

	Locale locale = acc->GetLocale();

	uint32 currentid = 0, newid = 0;
	if (!query.eof()) {
		TransactionData data;

		currentid = query.getIntField(1);
		newid = query.getIntField(1);
		data.SetID(currentid);
		data.SetDate(atol(query.getStringField(2)));
		data.SetType(DeescapeIllegalCharacters(query.getStringField(3)).String());
		data.SetPayee(DeescapeIllegalCharacters(query.getStringField(4)).String());
		data.SetAccount(acc);

		Fixed f;
		f.SetPremultiplied(atol(query.getStringField(5)));
		data.AddCategory(DeescapeIllegalCharacters(query.getStringField(6)).String(), f, true);

		if (!query.fieldIsNull(7))
			data.SetMemoAt(data.CountCategories() - 1,
				DeescapeIllegalCharacters(query.getStringField(7)).String());

		BString status = query.getStringField(8);
		if (status.ICompare("Reconciled") == 0)
			data.SetStatus(TRANS_RECONCILED);
		else if (status.ICompare("Cleared") == 0)
			data.SetStatus(TRANS_CLEARED);
		else
			data.SetStatus(TRANS_OPEN);

		query.nextRow();

		while (!query.eof()) {
			newid = query.getIntField(1);

			if (currentid != newid) {
				if (data.CountCategories() == 1)
					data.SetMemo(data.MemoAt(0));

				itemlist.AddItem(new TransactionItem(data));
				data.MakeEmpty();

				currentid = newid;
				newid = query.getIntField(1);
				data.SetID(currentid);
				data.SetDate(atol(query.getStringField(2)));
				data.SetType(DeescapeIllegalCharacters(query.getStringField(3)).String());
				data.SetPayee(DeescapeIllegalCharacters(query.getStringField(4)).String());
				data.SetAccount(acc);
			}

			f.SetPremultiplied(atol(query.getStringField(5)));
			data.AddCategory(DeescapeIllegalCharacters(query.getStringField(6)).String(), f, true);

			if (!query.fieldIsNull(7))
				data.SetMemoAt(data.CountCategories() - 1,
					DeescapeIllegalCharacters(query.getStringField(7)).String());

			status = query.getStringField(8);
			if (status.ICompare("Reconciled") == 0)
				data.SetStatus(TRANS_RECONCILED);
			else if (status.ICompare("Cleared") == 0)
				data.SetStatus(TRANS_CLEARED);
			else
				data.SetStatus(TRANS_OPEN);

			query.nextRow();
		}

		// Add the last transaction data to the item list
		if (data.CountCategories() == 1)
			data.SetMemo(data.MemoAt(0));

		itemlist.AddItem(new TransactionItem(data));
		fListView->AddList(&itemlist);
	}

	if (fListView->CurrentSelection() < 0) {
		fListView->Select(fListView->CountItems() - 1);
		fListView->ScrollToSelection();
	}
}

TransactionItem*
TransactionView::AddTransaction(const TransactionData& trans, const int32& index)
{
	int32 itemindex = 0;
	if (index < 0)
		itemindex = FindIndexForDate(trans.Date(), trans.Payee());
	else
		itemindex = index;

	TransactionItem* transitem = new TransactionItem(trans);
	fListView->AddItem(transitem, itemindex);
	fItemList->AddItem(transitem, itemindex);
	return (TransactionItem*)transitem;
}

void
TransactionView::DeleteTransaction(const int32& index)
{
	// This is totally bizarre. If we remove the first TransactionItem from the
	// actual fListView, it removes *all* of the items. Does commenting out the line
	// create a memory leak? BDB doesn't report any leaks FWIW. That won't work as
	// a fix, though, because whenever a transaction is deleted anywhere else, it
	// causes a segfault. Grrr....
	TransactionItem* item = (TransactionItem*)fListView->RemoveItem(index);
	delete item;
	fItemList->RemoveItemAt(index);
}

void
TransactionView::Draw(BRect updateRect)
{
	BRect frame = Frame();
	frame.OffsetTo(0, 0);
	frame.bottom = frame.top + 46;
	SetHighColor(156, 156, 156);
	StrokeRect(frame);
}

void
TransactionView::EditTransaction(void)
{
	int32 cs = fListView->CurrentSelection();
	fListView->InvalidateItem(cs);
}

void
TransactionView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case M_TRANSACTION_SELECTED:
		{
			Account* acc = gDatabase.CurrentAccount();
			if (acc) {
				TransactionItem* titem
					= (TransactionItem*)fListView->ItemAt(fListView->CurrentSelection());
				if (titem)
					acc->SetCurrentTransaction(titem->GetID());
			}
			break;
		}
		case M_TRANSACTION_INVOKED:
		{
			Window()->PostMessage(M_EDIT_TRANSACTION);
			break;
		}
		default:
		{
			BView::MessageReceived(message);
			break;
		}
	}
}

void
TransactionView::HandleNotify(const uint64& value, const BMessage* msg)
{
	bool lockwin = false;
	if (!Window()->IsLocked()) {
		lockwin = true;
		Window()->Lock();
	}

	if (value & WATCH_MASS_EDIT) {
		if (IsWatching(WATCH_TRANSACTION)) {
			RemoveWatch(WATCH_ALL);
			AddWatch(WATCH_MASS_EDIT);
		} else {
			AddWatch(WATCH_ALL);

			// need to reset all transactions for the current account
			SetAccount(gDatabase.CurrentAccount());
		}
	}

	if (value & WATCH_TRANSACTION) {
		uint32 accountid = 0;
		if (msg->FindInt32("accountid", (int32*)&accountid) != B_OK) {
			if (lockwin)
				Window()->Unlock();
			return;
		}

		if (accountid == gDatabase.CurrentAccount()->GetID()) {
			if (value & WATCH_CREATE) {
				TransactionData* trans = NULL;
				if (msg->FindPointer("item", (void**)&trans) != B_OK) {
					if (lockwin)
						Window()->Unlock();
					return;
				}

				TransactionItem* item = AddTransaction(*trans);
				fListView->Select(fListView->IndexOf(item));
				fListView->ScrollToSelection();
			} else if (value & WATCH_DELETE) {
				uint32 id = 0;
				if (msg->FindInt32("id", (int32*)&id) != B_OK) {
					if (lockwin)
						Window()->Unlock();
					return;
				}

				int32 index = FindItemForID(id);
				if (index < 0) {
					if (lockwin)
						Window()->Unlock();
					return;
				}

				DeleteTransaction(index);
			} else if (value & WATCH_CHANGE) {
				TransactionData* data;
				if (msg->FindPointer("item", (void**)&data) == B_OK) {
					int32 index = FindItemForID(data->GetID());
					TransactionItem* item = (TransactionItem*)fListView->ItemAt(index);
					if (item) {
						item->SetData(*data);
						fListView->InvalidateItem(index);
					}
				}
			}
/*			else
			if(value & WATCH_SELECT)
			{
				fListView->Select(index);
				fListView->ScrollToSelection();
			}
*/		}
	} else if (value & WATCH_ACCOUNT) {
		if (value & WATCH_REDRAW) {
			fListView->Invalidate();
		}

		Account* acc;
		if (msg->FindPointer("item", (void**)&acc) != B_OK) {
			if (lockwin)
				Window()->Unlock();
			return;
		}

		if (value & WATCH_SELECT) {
			SetAccount(gDatabase.CurrentAccount());
		} else if (value & WATCH_CREATE) {
			acc->AddObserver(this);
		} else if (value & (WATCH_CHANGE | WATCH_LOCALE)) {
			if (acc == gDatabase.CurrentAccount())
				fListView->Invalidate();
		}
	} else if (value & (WATCH_CHANGE | WATCH_LOCALE)) {
		// this case handles updates to the default locale
		// when the default locale is updated
		Locale* locale;
		if (msg->FindPointer("locale", (void**)&locale) == B_OK) {
			if (locale == &gDefaultLocale)
				fListView->Invalidate();
		}
	}

	if (lockwin)
		Window()->Unlock();
}

void
TransactionView::FrameResized(float width, float height)
{
	BScrollBar* bar = fScrollView->ScrollBar(B_VERTICAL);

	float big, small;
	bar->GetSteps(&small, &big);
	big = (int32)(fScrollView->Frame().Height() * .9);
	bar->SetSteps(small, big);
}

bool
TransactionView::SelectNext(void)
{
	int32 index = fListView->CurrentSelection();
	if (index < 0 || index >= fListView->CountItems() - 1)
		return false;
	fListView->Select(index + 1);
	fListView->ScrollToSelection();
	return true;
}

bool
TransactionView::SelectPrevious(void)
{
	int32 index = fListView->CurrentSelection();
	if (index <= 0)
		return false;
	fListView->Select(index - 1);
	fListView->ScrollToSelection();
	return true;
}

bool
TransactionView::SelectFirst(void)
{
	if (fListView->CountItems() <= 0)
		return false;
	fListView->Select(0L);
	fListView->ScrollToSelection();
	return true;
}

bool
TransactionView::SelectLast(void)
{
	if (fListView->CountItems() <= 0)
		return false;
	fListView->Select(fListView->CountItems() - 1);
	fListView->ScrollToSelection();
	return true;
}

int32
TransactionView::FindItemForID(const uint32& id)
{
	for (int32 i = 0; i < fListView->CountItems(); i++) {
		TransactionItem* item = (TransactionItem*)fListView->ItemAt(i);
		if (item->GetID() == id)
			return i;
	}
	return -1;
}

int32
TransactionView::FindIndexForDate(const time_t& time, const char* payee)
{
	// We need to find the appropriate index based on the date of
	// the transaction and insert it there
	int32 count = fListView->CountItems();

	if (count < 0)
		return 0;

	int32 i = 0;
	while (i < count) {
		TransactionItem* item = (TransactionItem*)fListView->ItemAt(i);

		if (time < item->GetDate()
			|| (time == item->GetDate() && strcmp(payee, item->GetPayee()) < 1))
			return i;

		i++;
	}
	return fListView->CountItems();
}
