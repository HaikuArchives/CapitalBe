#include "RegisterView.h"
#include <Font.h>
#include <ScrollView.h>
#include <StringView.h>

#include "Account.h"
#include "AccountListItem.h"
#include "CheckView.h"
#include "Database.h"
#include "MainWindow.h"
#include "QuickTrackerItem.h"
#include "Translate.h"

enum
{
	M_SELECT_ACCOUNT='slac',
	M_SELECT_CURRENT
};

RegisterView::RegisterView(BRect frame, const char *name, int32 resize, int32 flags)
 : BView(frame, name, resize, flags| B_FRAME_EVENTS)
{
	SetViewColor(240,240,240);
	BRect r(Bounds());
	
	BStringView *accountlabel = new BStringView(BRect(12,2,6,6),"accountlabel",
												TRANSLATE("Accounts"));
	accountlabel->ResizeToPreferred();
	AddChild(accountlabel);
	
	r.left = 15;
	r.top = accountlabel->Frame().bottom + 3;
	r.right = r.left + be_plain_font->StringWidth("AccountName");
	r.bottom = r.top + 110;
	
//	fAccountView = new DragListView(r,"accountview");
	fAccountView = new BListView(r,"accountview");
	fAccountView->SetSelectionMessage(new BMessage(M_SELECT_ACCOUNT));
	fAccountView->SetInvocationMessage(new BMessage(M_SHOW_ACCOUNT_SETTINGS));
	fAccountScroller = new BScrollView("accountscroll",fAccountView,0,true,true,
									B_FOLLOW_TOP_BOTTOM|B_FOLLOW_LEFT);
	AddChild(fAccountScroller);
	fAccountScroller->SetViewColor(ViewColor());
	
	r.left = fAccountScroller->Frame().right + 10;
	r.right = Bounds().right - 15;
	r.bottom = Bounds().bottom - 15;
	r.top = r.bottom - 122;
	fCheckView = new CheckView(r, "checkview", B_FOLLOW_LEFT_RIGHT | 
								B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fCheckView->MoveTo(fAccountScroller->Frame().right + 10,
						Bounds().bottom - fCheckView->Bounds().Height() - 15);
	gDatabase.AddObserver(fCheckView);
	
	for(int32 i=0; i<gDatabase.CountAccounts(); i++)
	{
		Account *acc = gDatabase.AccountAt(i);
		fAccountView->AddItem(new AccountListItem(acc));
		acc->AddObserver(this);
	}
	
	r.left = fAccountScroller->Frame().right + 10;
	r.right = Bounds().right - 15;
	r.top = fAccountScroller->Frame().top;
	r.bottom = fCheckView->Frame().top - 10;
	fTransactionView = new TransactionView(r);
	AddChild(fTransactionView);
	gDatabase.AddObserver(fTransactionView);
	gDatabase.AddObserver(this);
	
	// Add this after the transaction view to ensure that Tab navigation order is correct
	AddChild(fCheckView);

	// QuickTracker items need to be added here
	r = fAccountScroller->Frame();
	r.top = r.bottom + 10;
	r.bottom = Bounds().bottom - 10;
	
	fTrackBox = new BBox(r, "qtbox", B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	fTrackBox->SetLabel(TRANSLATE("QuickTracker"));
	AddChild(fTrackBox);
	
	// We will add a view to force the clipping of the child views
	
	QTNetWorthItem *item;
	
	r = fTrackBox->Bounds().InsetByCopy(2,0);
	r.top = 10;
	
	// This is a quick hack to approximate the amount of room needed to display everything
	r.bottom = r.top + (be_plain_font->Size() * 2.4);
	
	item = new QTNetWorthItem(r,"networth");
	fTrackBox->AddChild(item);
}

RegisterView::~RegisterView(void)
{
}

void RegisterView::AttachedToWindow(void)
{
	fAccountView->SetTarget(this);
	
	// If the selection done is before being attached to the window, the message is
	// never received.
	bool selected = false;
	for(int32 i=0; i<gDatabase.CountAccounts();i++)
	{
		Account *acc = gDatabase.AccountAt(i);
		if(acc && !acc->IsClosed())
		{
			fAccountView->Select(i);
			selected = true;
		}
	}
	if(!selected)
		fAccountView->Select(0);

	fCheckView->MakeFocus(true);
}

void RegisterView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_SELECT_ACCOUNT:
		{
			if(fAccountView->CurrentSelection()<0)
				break;
			gDatabase.SetCurrentAccount(fAccountView->CurrentSelection());
			break;
		}
		case M_SHOW_ACCOUNT_SETTINGS:
		{
			if(Window())
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

void RegisterView::HandleNotify(const uint64 &value, const BMessage *msg)
{
	bool lockwin=false;
	if(!Window()->IsLocked())
	{
		Window()->Lock();
		lockwin=true;
	}
	
	if(value & WATCH_ACCOUNT)
	{
		Account *acc;
		if(msg->FindPointer("item",(void**)&acc)!=B_OK)
		{
			if(lockwin)
				Window()->Unlock();
			return;
		}
		
		if(value & WATCH_CREATE)
		{
			fAccountView->AddItem(new AccountListItem(acc));
			if(fAccountView->CountItems()==1)
				fAccountView->Select(0);
			acc->AddObserver(this);
		}
		else
		if(value & WATCH_DELETE)
		{
			AccountListItem *item = (AccountListItem*)fAccountView->RemoveItem(gDatabase.IndexOf(acc));
			delete item;
			fAccountView->Select(0);
		}
		else
		if(value & WATCH_CHANGE)
		{
			for(int32 i=0; i<fAccountView->CountItems(); i++)
			{
				AccountListItem *listitem = (AccountListItem*)fAccountView->ItemAt(i);
				if(listitem && listitem->GetAccount()==acc)
				{
					fAccountView->InvalidateItem(i);
					break;
				}
			}
		}
		
		// Adjust the horizontal scroll bar every time there is a change
		
		float maxwidth = 0;
		for(int32 i=0; i<gDatabase.CountAccounts(); i++)
		{
			acc = gDatabase.AccountAt(i);
			
			float namewidth = be_bold_font->StringWidth(acc->Name()) + B_V_SCROLL_BAR_WIDTH + 10;
			maxwidth = (namewidth > maxwidth) ? namewidth : maxwidth;
		}
		
		float range = maxwidth - fAccountScroller->Bounds().Width();
		if(range < 0)
			range = 0;
		
		BScrollBar *bar = fAccountScroller->ScrollBar(B_HORIZONTAL);
		bar->SetRange(0,range);

	}
	else
	if(value & WATCH_TRANSACTION)
	{
		if(value & WATCH_CREATE || value & WATCH_DELETE || value & WATCH_CHANGE)
			fAccountView->Invalidate();
	}
	else
	if(value & WATCH_LOCALE)
	{
		for(int32 i=0; i<fAccountView->CountItems(); i++)
		{
			AccountListItem *listitem = (AccountListItem*)fAccountView->ItemAt(i);
			if(listitem)
				fAccountView->InvalidateItem(i);
		}
	}
	if(lockwin)
		Window()->Unlock();
}

void RegisterView::SelectAccount(const int32 &index)
{
	if(index < 0 || index > fAccountView->CountItems()-1)
		return;
	
	fAccountView->Select(index);
}
