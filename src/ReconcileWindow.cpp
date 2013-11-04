#include <String.h>
#include <ScrollView.h>
#include <MessageFilter.h>

#include "CurrencyBox.h"
#include "DAlert.h"
#include "Database.h"
#include "DateBox.h"
#include "CBLocale.h"
#include "MsgDefs.h"
#include "ReconcileWindow.h"
#include "ReconcileItem.h"
#include "Preferences.h"
#include "TimeSupport.h"
#include "Transaction.h"
#include "Translate.h"

enum
{
	M_TOGGLE_DEPOSIT='tgdp',
	M_TOGGLE_CHECK,
	M_TOGGLE_CHARGE,
	M_SET_BALANCES,
	M_RECONCILE,
	M_RESET,
	M_AUTORECONCILE
};

/*
class ReconcileFilter : public BMessageFilter
{
public:
	ReconcileFilter(ReconcileWindow *checkview);
	~ReconcileFilter(void);
	virtual filter_result Filter(BMessage *msg, BHandler **target);

private:
	ReconcileWindow *fWindow;
};
*/

ReconcileWindow::ReconcileWindow(const BRect frame, Account *account)
 : BWindow(frame,"",B_DOCUMENT_WINDOW_LOOK,B_NORMAL_WINDOW_FEEL,B_NOT_MINIMIZABLE |	
 	B_NOT_ZOOMABLE)
{
	BString temp;
	fCurrentDate = GetCurrentDate();
//	AddCommonFilter(new ReconcileFilter(this));
	
	SetSizeLimits(500,30000,235,30000);
	if(frame.Width() < 500)
		ResizeBy(500 - frame.Width(),0);
	if(frame.Height() < 235)
		ResizeBy(0,235 - frame.Height());
	
	if(account)
	{
		temp = TRANSLATE("Reconcile");
		temp << ": " << account->Name();
		SetTitle(temp.String());
		gDatabase.AddObserver(this);
	}
	fAccount = account;
	
	
	AddShortcut('W',B_COMMAND_KEY,new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q',B_COMMAND_KEY,new BMessage(B_QUIT_REQUESTED));
	
	BView *back = new BView(Bounds(),"backview",B_FOLLOW_ALL,B_WILL_DRAW);
	AddChild(back);
	back->SetViewColor(240,240,240);
	
	BRect r;
	
	
	temp = TRANSLATE("Date"); temp += ":";
	float width = back->StringWidth(temp.String());
	
	r.left = 10;
	r.top = 10;
	r.bottom = 40;
	r.right = r.left + width + back->StringWidth("00-00-0000") + 20;
	BString datestr;
	gDefaultLocale.DateToString(GetCurrentDate(),datestr);
	fDate = new DateBox(r,"dateentry",temp.String(),datestr.String(),NULL,B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fDate->SetDivider(width+5);
//	fDate->SetEscapeCancel(true);
	
	back->AddChild(fDate);
	fDate->GetFilter()->SetMessenger(new BMessenger(this));
	
	fDateMultiplier = fDate->Bounds().Width() / Bounds().Width();
	
	temp = TRANSLATE("Starting Balance"); temp += ": ";
	width = back->StringWidth(temp.String());
	r = fDate->TextView()->Bounds().OffsetByCopy(10,10);
	r.left = fDate->Frame().right + 10;
	r.right = r.left + width + back->StringWidth("0000000") + 20;
	fOpening = new CurrencyBox(r,"starting",temp.String(),NULL,new BMessage(M_SET_BALANCES),
								B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fOpening->SetDivider(width + 5);
	back->AddChild(fOpening);
	fOpening->GetFilter()->SetMessenger(new BMessenger(this));
	fOpeningMultiplier = fOpening->Bounds().Width() / Bounds().Width();
	
	temp = TRANSLATE("Ending Balance"); temp += ":";
	width = back->StringWidth(temp.String());
	r = fDate->TextView()->Bounds().OffsetByCopy(10,10);
	r.left = fOpening->Frame().right + 10;
	r.right = Bounds().right - 10;
	fClosing = new CurrencyBox(r,"closing",temp.String(),NULL,
								new BMessage(M_SET_BALANCES), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fClosing->SetDivider(width + 5);
	back->AddChild(fClosing);
	fClosing->GetFilter()->SetMessenger(new BMessenger(this));
	fClosingMultiplier = fClosing->Bounds().Width() / Bounds().Width();
	
	r.OffsetTo(10,fDate->Frame().bottom + 10);
	temp = TRANSLATE("Bank Charges"); temp += ":";
	width = back->StringWidth(temp.String());
	r.right = (Bounds().Width()/2)-15;
	fCharges = new CurrencyBox(r,"charges",temp.String(),NULL,NULL,
								B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fCharges->SetDivider(width + 5);
	back->AddChild(fCharges);
	fCharges->GetFilter()->SetMessenger(new BMessenger(this));
	
	r.OffsetTo(fCharges->Frame().right + 10,r.top);
	temp = TRANSLATE("Interest Earned"); temp += ":";
	width = back->StringWidth(temp.String());
	r.right = Bounds().right - 10;
	fInterest = new CurrencyBox(r,"interest",temp.String(),NULL,NULL,
								B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fInterest->SetDivider(width + 5);
	back->AddChild(fInterest);
	fInterest->GetFilter()->SetMessenger(new BMessenger(this));
	
	// 10 pixels padding in between each listview
	float listwidth = (Bounds().Width() - (B_V_SCROLL_BAR_WIDTH*3) - 40)/3;
	
	r.OffsetTo(10,r.bottom+30);
	r.right = r.left + listwidth;
	r.bottom = Bounds().bottom - 105;
	fDepositList = new BListView(r,"depositlist",B_SINGLE_SELECTION_LIST,B_FOLLOW_ALL);
	fDepositList->SetFlags(fDepositList->Flags() | B_FULL_UPDATE_ON_RESIZE);
	fDepositList->SetInvocationMessage(new BMessage(M_TOGGLE_DEPOSIT));
	fDepScroll = new BScrollView("fDepScroll",fDepositList,B_FOLLOW_TOP_BOTTOM|B_FOLLOW_LEFT,
													0,false,true);
	back->AddChild(fDepScroll);
	fDepScroll->SetViewColor(back->ViewColor());
	
	r.OffsetTo(fDepScroll->Frame().right + 10,r.top);
	fCheckList = new BListView(r,"checklist",B_SINGLE_SELECTION_LIST,B_FOLLOW_ALL);
	fCheckList->SetFlags(fDepositList->Flags() | B_FULL_UPDATE_ON_RESIZE);
	fCheckList->SetInvocationMessage(new BMessage(M_TOGGLE_CHECK));
	fCheckScroll = new BScrollView("fCheckScroll",fCheckList,B_FOLLOW_TOP_BOTTOM|B_FOLLOW_LEFT,
													0,false,true);
	back->AddChild(fCheckScroll);
	fCheckScroll->SetViewColor(back->ViewColor());
	
	r.OffsetTo(fCheckScroll->Frame().right + 10,r.top);
	fChargeList = new BListView(r,"chargelist",B_SINGLE_SELECTION_LIST,B_FOLLOW_ALL);
	fChargeList->SetFlags(fDepositList->Flags() | B_FULL_UPDATE_ON_RESIZE);
	fChargeList->SetInvocationMessage(new BMessage(M_TOGGLE_CHARGE));
	fChargeScroll = new BScrollView("fChargeScroll",fChargeList,B_FOLLOW_TOP_BOTTOM|B_FOLLOW_LEFT,
													0,false,true);
	back->AddChild(fChargeScroll);
	fChargeScroll->SetViewColor(back->ViewColor());
	
	float pwidth, pheight;
	BString label;
	
	gCurrentLocale.CurrencyToString(fDepositTotal,label);
	temp = TRANSLATE("Total Deposits");
	temp << ": " << label;
	
	r.left = fDepScroll->Frame().left;
	r.top = fDepScroll->Frame().bottom+5;
	fDepLabel = new BStringView(r,"deplabel",temp.String(),B_FOLLOW_BOTTOM|B_FOLLOW_LEFT);
	fDepLabel->GetPreferredSize(&pwidth, &pheight);
	fDepLabel->ResizeTo(fDepScroll->Frame().Width(),pheight);
	back->AddChild(fDepLabel);
	fDepLabel->SetAlignment(B_ALIGN_RIGHT);
		
	r.left = fCheckScroll->Frame().left;
	gCurrentLocale.CurrencyToString(fCheckTotal,label);
	temp = TRANSLATE("Total Checks");
	temp << ": " << label;
	
	fCheckLabel = new BStringView(r,"checklabel",temp.String(),
								B_FOLLOW_BOTTOM|B_FOLLOW_LEFT);
	fCheckLabel->GetPreferredSize(&pwidth, &pheight);
	fCheckLabel->ResizeTo(fCheckScroll->Frame().Width(),pheight);
	back->AddChild(fCheckLabel);
	fCheckLabel->SetAlignment(B_ALIGN_RIGHT);
	
	r.left = fChargeScroll->Frame().left;
	gCurrentLocale.CurrencyToString(fChargeTotal,label);
	temp = TRANSLATE("Total Charges");
	temp << ": " << label;
	fChargeLabel = new BStringView(r,"chargelabel",temp.String(),
									B_FOLLOW_BOTTOM|B_FOLLOW_LEFT);
	fChargeLabel->GetPreferredSize(&pwidth, &pheight);
	fChargeLabel->ResizeTo(fChargeScroll->Frame().Width(),pheight);
	back->AddChild(fChargeLabel);
	fChargeLabel->SetAlignment(B_ALIGN_RIGHT);
	
	fReconcile = new BButton(BRect(0,0,1,1),"reconcile",TRANSLATE("Reconcile"),
							new BMessage(M_RECONCILE),B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fReconcile->ResizeToPreferred();
	fReconcile->MoveTo(Bounds().right - 12 - fReconcile->Frame().Width(),
						Bounds().bottom - 15 - fReconcile->Frame().Height());
	
	fCancel = new BButton(BRect(0,0,1,1),"cancel",TRANSLATE("Cancel"),
							new BMessage(B_QUIT_REQUESTED),B_FOLLOW_RIGHT |	B_FOLLOW_BOTTOM);
	fCancel->ResizeToPreferred();
	fCancel->MoveTo(fReconcile->Frame().left - 10 - fCancel->Frame().Width(),
					fReconcile->Frame().top);
	
	fReset = new BButton(BRect(0,0,1,1),"reset",TRANSLATE("Reset"),new BMessage(M_RESET),
							B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
	fReset->ResizeToPreferred();
	fReset->MoveTo(fCancel->Frame().left - 30 - fReset->Frame().Width(),
					fReconcile->Frame().top);
	
	fAutoReconcile = new BButton(BRect(0,0,1,1),"autoreconcile",
							TRANSLATE("Quick Balance"),new BMessage(M_AUTORECONCILE),
							B_FOLLOW_LEFT |	B_FOLLOW_BOTTOM);
	fAutoReconcile->ResizeToPreferred();
	fAutoReconcile->MoveTo(10, Bounds().bottom - 15 - fAutoReconcile->Frame().Height());
	
	prefsLock.Lock();
	BString rechelp = gAppPath;
	prefsLock.Unlock();
	rechelp << "helpfiles/" << gCurrentLanguage->Name() << "/Reconcile Window Help";
	BPoint helppoint(fAutoReconcile->Frame().RightTop());
	helppoint.x += 10;
	helppoint.y += (fAutoReconcile->Frame().Height()-16)/2;
	fHelpButton = new HelpButton(helppoint,"rechelp",rechelp.String());
	fHelpButton->SetResizingMode(B_FOLLOW_BOTTOM);
	
	back->AddChild(fAutoReconcile);
	back->AddChild(fHelpButton);
	back->AddChild(fReset);
	back->AddChild(fCancel);
	back->AddChild(fReconcile);
	
	r = fDepLabel->Frame();
	r.OffsetBy(0,r.Height()+15);
	temp = TRANSLATE("Unreconciled Total"); temp += ":";
	fTotalLabel = new BStringView(r,"totallabel",temp.String(),
									B_FOLLOW_BOTTOM|B_FOLLOW_LEFT);
	fTotalLabel->GetPreferredSize(&pwidth, &pheight);
	fTotalLabel->ResizeTo(fReset->Frame().left - 10 - fTotalLabel->Frame().left,pheight);
	back->AddChild(fTotalLabel);
	
	account->DoForEachTransaction(AddReconcileItems,this);
		
	fDate->MakeFocus(true);
}

ReconcileWindow::~ReconcileWindow(void)
{
	prefsLock.Lock();
	gPreferences.RemoveData("reconcileframe");
	gPreferences.AddRect("reconcileframe",Frame());
	prefsLock.Unlock();
}

void ReconcileWindow::FrameResized(float w, float h)
{
	// We implement our own resizing routines because all the controls need to be resized in a proportional
	// manner of the window being resized, such as the 3 listviews each taking up just a little less than 1/3
	// of the window's width
	fDate->ResizeTo(w * fDateMultiplier, fDate->Frame().Height());
	fOpening->ResizeTo(w * fOpeningMultiplier, fOpening->Frame().Height());
	fOpening->MoveTo(fDate->Frame().right + 10,fOpening->Frame().top);
	fClosing->MoveTo(fOpening->Frame().right + 10,fClosing->Frame().top);
	fClosing->ResizeTo(w - 10 - fClosing->Frame().left, fClosing->Frame().Height());
	
	
	fCharges->ResizeTo((w/2)-15,fCharges->Frame().Height());
	fInterest->MoveTo(fCharges->Frame().right + 10,fInterest->Frame().top);
	fInterest->ResizeTo(w - 10 - fInterest->Frame().left,fInterest->Frame().Height());
	
	float listwidth = (Bounds().Width() - 40)/3;
	float height = fDepScroll->Frame().Height();
	
	fDepScroll->ResizeTo(listwidth, height);
	fCheckScroll->ResizeTo(listwidth, height);
	fChargeScroll->ResizeTo(listwidth, height);
	
	float top = fCheckList->Parent()->Frame().top;
	fCheckScroll->MoveTo(fDepScroll->Frame().right + 10,top);
	fChargeScroll->MoveTo(fCheckScroll->Frame().right + 10,top);
	
	fDepLabel->MoveTo(fDepScroll->Frame().left, fDepScroll->Frame().bottom+5);
	fCheckLabel->MoveTo(fCheckScroll->Frame().left, fCheckScroll->Frame().bottom+5);
	fChargeLabel->MoveTo(fChargeScroll->Frame().left, fChargeScroll->Frame().bottom+5);
	
	fDepLabel->ResizeTo(fDepScroll->Frame().Width(),fDepLabel->Frame().Height());
	fCheckLabel->ResizeTo(fCheckScroll->Frame().Width(),fCheckLabel->Frame().Height());
	fChargeLabel->ResizeTo(fChargeScroll->Frame().Width(),fChargeLabel->Frame().Height());
}

void ReconcileWindow::MessageReceived(BMessage *msg)
{
	int32 index;
	ReconcileItem *selection;
	BString label,temp;
	
	switch(msg->what)
	{
		case M_PREVIOUS_FIELD:
		{
			if(fDate->TextView()->IsFocus())
				fReconcile->MakeFocus(true);
			else
			if(fOpening->TextView()->IsFocus())
				fDate->MakeFocus(true);
			else
			if(fClosing->TextView()->IsFocus())
				fOpening->MakeFocus(true);
			else
			if(fCharges->TextView()->IsFocus())
				fClosing->MakeFocus(true);
			else
			if(fInterest->TextView()->IsFocus())
				fCharges->MakeFocus(true);
			break;
		}
		case M_NEXT_FIELD:
		{
			if(fDate->TextView()->IsFocus())
				fOpening->MakeFocus(true);
			else
			if(fOpening->TextView()->IsFocus())
				fClosing->MakeFocus(true);
			else
			if(fClosing->TextView()->IsFocus())
				fCharges->MakeFocus(true);
			else
			if(fCharges->TextView()->IsFocus())
				fInterest->MakeFocus(true);
			else
			if(fInterest->TextView()->IsFocus())
				fDepositList->MakeFocus(true);
				
			break;
		}
		case M_RECONCILE:
		{
			ApplyChargesAndInterest();
			
			int32 i;
			ReconcileItem *item;
			for(i=0; i<fDepositList->CountItems(); i++)
			{
				item = (ReconcileItem*)fDepositList->ItemAt(i);
				if(item->IsReconciled())
					item->SyncToTransaction();
			}
			
			for(i=0; i<fCheckList->CountItems(); i++)
			{
				item = (ReconcileItem*)fCheckList->ItemAt(i);
				if(item->IsReconciled())
					item->SyncToTransaction();
			}
			
			for(i=0; i<fChargeList->CountItems(); i++)
			{
				item = (ReconcileItem*)fChargeList->ItemAt(i);
				if(item->IsReconciled())
					item->SyncToTransaction();
			}
			
			BMessage notify;
			fAccount->Notify(WATCH_REDRAW | WATCH_ACCOUNT,&notify);
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case M_RESET:
		{
			int32 i;
			ReconcileItem *item;
			for(i=0; i<fDepositList->CountItems(); i++)
			{
				item = (ReconcileItem*)fDepositList->ItemAt(i);
				if(item->IsReconciled())
				{
					item->RevertTransaction();
					fDepositList->InvalidateItem(i);
				}
			}
			
			for(i=0; i<fCheckList->CountItems(); i++)
			{
				item = (ReconcileItem*)fCheckList->ItemAt(i);
				if(item->IsReconciled())
				{
					item->RevertTransaction();
					fCheckList->InvalidateItem(i);
				}
			}
			
			for(i=0; i<fChargeList->CountItems(); i++)
			{
				item = (ReconcileItem*)fChargeList->ItemAt(i);
				if(item->IsReconciled())
				{
					item->RevertTransaction();
					fChargeList->InvalidateItem(i);
				}
			}
			break;
		}
		case M_TOGGLE_DEPOSIT:
		{
			index = fDepositList->CurrentSelection();
			selection = (ReconcileItem*)fDepositList->ItemAt(index);
			if(selection)
			{
				if(selection->IsReconciled())
				{
					selection->SetReconciled(false);
					fDepositTotal -= selection->GetTransaction()->Amount();
					fTotal -= selection->GetTransaction()->Amount();
				}
				else
				{
					selection->SetReconciled(true);
					fDepositTotal += selection->GetTransaction()->Amount();
					fTotal += selection->GetTransaction()->Amount();
				}
				fDepositList->InvalidateItem(index);
				
				fAccount->GetLocale().CurrencyToString(fDepositTotal,label);
				temp << TRANSLATE("Total Deposits") << ": " << label;
				fDepLabel->SetText(label.String());
				
				fAccount->GetLocale().CurrencyToString(fTotal + fDifference,label);
				temp = "";
				temp << TRANSLATE("Unreconciled Total") << ": " << label;
				fTotalLabel->SetText(label.String());
				
				if( (fTotal+fDifference) == 0)
					fReconcile->SetEnabled(true);
				else
					fReconcile->SetEnabled(false);
			}
			break;
		}
		case M_TOGGLE_CHECK:
		{
			index = fCheckList->CurrentSelection();
			selection = (ReconcileItem*)fCheckList->ItemAt(index);
			if(selection)
			{
				if(selection->IsReconciled())
				{
					selection->SetReconciled(false);
					fCheckTotal += selection->GetTransaction()->Amount();
					fTotal -= selection->GetTransaction()->Amount();
				}
				else
				{
					selection->SetReconciled(true);
					fCheckTotal -= selection->GetTransaction()->Amount();
					fTotal += selection->GetTransaction()->Amount();
				}
				fCheckList->InvalidateItem(index);
				
				fAccount->GetLocale().CurrencyToString(fCheckTotal,label);
				temp << TRANSLATE("TotaChecks") << ": " << label;
				fCheckLabel->SetText(label.String());
				
				fAccount->GetLocale().CurrencyToString(fTotal + fDifference,label);
				temp = "";
				temp << TRANSLATE("Unreconciled Total") << ": " << label;
				fTotalLabel->SetText(label.String());
		
				if( (fTotal+fDifference) == 0)
					fReconcile->SetEnabled(true);
				else
					fReconcile->SetEnabled(false);
			}
			break;
		}
		case M_TOGGLE_CHARGE:
		{
			index = fChargeList->CurrentSelection();
			selection = (ReconcileItem*)fChargeList->ItemAt(index);
			if(selection)
			{
				if(selection->IsReconciled())
				{
					selection->SetReconciled(false);
					fChargeTotal += selection->GetTransaction()->Amount();
					fTotal -= selection->GetTransaction()->Amount();
				}
				else
				{
					selection->SetReconciled(true);
					fChargeTotal -= selection->GetTransaction()->Amount();
					fTotal += selection->GetTransaction()->Amount();
				}
				fChargeList->InvalidateItem(index);
				
				fAccount->GetLocale().CurrencyToString(fChargeTotal,label);
				temp << TRANSLATE("Total Charges") << ": " << label;
				fChargeLabel->SetText(label.String());
				
				fAccount->GetLocale().CurrencyToString(fTotal + fDifference,label);
				temp = "";
				temp << TRANSLATE("Unreconciled Total") << ": " << label;
				fTotalLabel->SetText(label.String());
				
				if( (fTotal+fDifference) == 0)
					fReconcile->SetEnabled(true);
				else
					fReconcile->SetEnabled(false);
			}
			break;
		}
		case M_SET_BALANCES:
		{
			Fixed fixed,fixed2;
			if(gCurrentLocale.StringToCurrency(fOpening->Text(),fixed)!=B_OK ||
				gCurrentLocale.StringToCurrency(fClosing->Text(),fixed2)!=B_OK )
				break;
			
			fDifference = fixed - fixed2;
			
			gCurrentLocale.CurrencyToString(fTotal + fDifference,label);
			label.Prepend(" ");
			label.Prepend("Unreconciled Total:");
			fTotalLabel->SetText(label.String());
				
			if( (fTotal+fDifference) == 0)
				fReconcile->SetEnabled(true);
			else
				fReconcile->SetEnabled(false);
			break;
		}
		case M_AUTORECONCILE:
		{
			AutoReconcile();
			BMessage notify;
			fAccount->Notify(WATCH_REDRAW | WATCH_ACCOUNT,&notify);
			
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

void ReconcileWindow::HandleNotify(const uint64 &value, const BMessage *msg)
{
	bool unlock=false;
	if(!IsLocked())
	{
		unlock = true;
		Lock();
	}
	
	// This should prevent a rather spectacular slowdown if the user decides to import some accounts
	// while reconciling. Bizarre and unthinkable, but people is people. :P
	if(value & WATCH_MASS_EDIT)
	{
		if(IsWatching(WATCH_TRANSACTION))
		{
			RemoveWatch(WATCH_ALL);
			AddWatch(WATCH_MASS_EDIT);
		}
		else
		{
			AddWatch(WATCH_ALL);
		}
		
		if(unlock)
			Unlock();
		return;
	}
	
	Account *acc = NULL;
	if( (value & WATCH_ACCOUNT) && (value & WATCH_DELETE) )
	{
		if( (msg->FindPointer("item",(void**)&acc)==B_OK) && (acc == fAccount) )
			PostMessage(B_QUIT_REQUESTED);
	}
	else
	if( (value & WATCH_TRANSACTION) )
	{
		if(value & WATCH_DELETE)
		{
			uint32 id;
			if( msg->FindInt32("id",(int32*)&id)==B_OK)
			{
				ReconcileItem *deleteditem;
				BListView *itemlist;
				
				deleteditem = FindItemForID(fDepositList, id);
				if(deleteditem)
					itemlist = fDepositList;
				else
				{
					deleteditem = FindItemForID(fCheckList, id);
					if(deleteditem)
						itemlist = fCheckList;
					else
					{
						deleteditem = FindItemForID(fChargeList, id);
						if(deleteditem)
							itemlist = fChargeList;
						else
						{
							ShowBug("No list for ID in ReconcileWindow::HandleNotify");
							if(unlock)
								Unlock();
							return;
						}
					}
				}
				
				itemlist->RemoveItem(deleteditem);
				delete deleteditem;
			}
		}
		else
		if(value & WATCH_CREATE)
		{
			uint32 accountid;
			if(msg->FindInt32("accountid",(int32*)&accountid)!=B_OK || accountid != fAccount->GetID())
			{	
				if(unlock)
					Unlock();
				return;
			}
			
			TransactionData *data;
			if(msg->FindPointer("item",(void**)&data)==B_OK)
			{
				ReconcileItem *newitem = new ReconcileItem(*data);
				
				if(data->Type().TypeCode()==TRANS_DEP)
					InsertTransactionItem(fDepositList,newitem);
				else
				if(data->Type().TypeCode()==TRANS_NUMERIC)
					InsertTransactionItem(fCheckList,newitem);
				else
					InsertTransactionItem(fChargeList,newitem);
			}
		}
		
	}
	if(unlock)
		Unlock();
}

bool ReconcileWindow::QuitRequested(void)
{
	gDatabase.RemoveObserver(this);
	return true;
}

void ReconcileWindow::ApplyChargesAndInterest(void)
{
	Fixed charge;
	if(strlen(fCharges->Text())>0 && gCurrentLocale.StringToCurrency(fCharges->Text(), charge)==B_OK)
	{
		TransactionData chargetrans(fAccount, fDate->Text(),"ATM",TRANSLATE("Bank Charge"),
									fCharges->Text(),TRANSLATE("Bank Charge"),NULL,
									TRANS_RECONCILED);
		gDatabase.AddTransaction(chargetrans);
	}
	
	Fixed interest;
	if(strlen(fInterest->Text())>0 && gCurrentLocale.StringToCurrency(fInterest->Text(), interest)==B_OK)
	{
		TransactionData interesttrans(fAccount,fDate->Text(),TRANSLATE("DEP"),
									TRANSLATE("Account Interest"),
									fInterest->Text(),TRANSLATE("Account Interest"),NULL,
									TRANS_RECONCILED);
		gDatabase.AddTransaction(interesttrans);
	}
}

bool ReconcileWindow::AutoReconcile(void)
{
	// We are going to attempt to automatically reconcile the account. We will do
	// this by adding up the values of all transactions unreconciled before the
	// statement date. If they balance, we can notify the user that we were successful
	// and we've saved him/her quite a bit of time in finance handling. If not,
	// we can tell the user that it failed and the conditions under which it works.
	time_t statdate;
	int32 i;
	ReconcileItem *item;
	
	if(gDefaultLocale.StringToDate(fDate->Text(),statdate)!=B_OK)
	{
		// Do we have an empty date box?
		if(strlen(fDate->Text())<1)
		{
			ShowAlert(TRANSLATE("Date is missing."),
					TRANSLATE("You need to enter the date for the statement to Quick Balance."));
			return false;
		}
	}
	
	Fixed dep,chrg,chk,bankchrg,interest;
	BList list;
	
	if(strlen(fCharges->Text())>0)
	{
		if(gCurrentLocale.StringToCurrency(fCharges->Text(),bankchrg)==B_OK)
			bankchrg.Invert();
		else
		{
			ShowAlert(TRANSLATE("Capital Be didn't understand the amount for Bank Charges."),
						TRANSLATE("There may be a typo or the wrong kind of currency symbol "
						"for this account."));
			return false;
		}
	}
	
	if(strlen(fInterest->Text())>0)
	{
		if(gCurrentLocale.StringToCurrency(fInterest->Text(),interest)!=B_OK)
		{
			ShowAlert(TRANSLATE("Capital Be didn't understand the amount for Interest Earned."),
						TRANSLATE("There may be a typo or the wrong kind of currency symbol "
						"for this account."));
			return false;
		}
	}
	
	for(i=0; i<fDepositList->CountItems(); i++)
	{
		item = (ReconcileItem*)fDepositList->ItemAt(i);
		if(item->GetTransaction()->Date()>=statdate)
			break;
		
		if(!item->IsReconciled())
		{
			dep += item->GetTransaction()->Amount();
			list.AddItem(item);
		}
	}
	
	for(i=0; i<fCheckList->CountItems(); i++)
	{
		item = (ReconcileItem*)fCheckList->ItemAt(i);
		if(item->GetTransaction()->Date()>=statdate)
			break;
		
		if(!item->IsReconciled())
		{
			chk += item->GetTransaction()->Amount();
			list.AddItem(item);
		}
	}
	
	for(i=0; i<fChargeList->CountItems(); i++)
	{
		item = (ReconcileItem*)fChargeList->ItemAt(i);
		if(item->GetTransaction()->Date()>=statdate)
			break;
		
		if(!item->IsReconciled())
		{
			chrg += item->GetTransaction()->Amount();
			list.AddItem(item);
		}
	}
			
	if( dep + chk + chrg + bankchrg + interest + fDifference == 0)
	{
		for(i=0; i<list.CountItems(); i++)
		{
			item = (ReconcileItem*)list.ItemAt(i);
			item->SetReconciled(true);
		}
		ApplyChargesAndInterest();
		ShowAlert(TRANSLATE("Success!"),TRANSLATE("Quick Balance successful!"),B_IDEA_ALERT);
		PostMessage(B_QUIT_REQUESTED);
		return true;
	}
	
	ShowAlert(TRANSLATE("Couldn't Quick Balance."),
		TRANSLATE("Quick Balance failed. This doesn't mean "
		"that you did something wrong - it's just that Quick Balance works on "
		"simpler cases in balancing an account than this one. Sorry."));
	return false;
}

ReconcileItem *ReconcileWindow::FindItemForID(BListView *target, const uint32 &id)
{
	for(int32 i=0; i<target->CountItems(); i++)
	{
		ReconcileItem *temp = (ReconcileItem*)target->ItemAt(i);
		if(temp->GetTransaction()->GetID() == id)
			return temp;
	}
	return NULL;
}

void ReconcileWindow::InsertTransactionItem(BListView *target, ReconcileItem *item)
{
	TransactionData *itemdata = item->GetTransaction();
	
	for(int32 i=0; i<target->CountItems(); i++)
	{
		ReconcileItem *temp = (ReconcileItem*)target->ItemAt(i);
		TransactionData *tempdata = temp->GetTransaction();
		
		if(itemdata->Date() < tempdata->Date() || 
			(itemdata->Date() == tempdata->Date() && 
			strcmp(itemdata->Payee(),tempdata->Payee())<1) )
		{
			target->AddItem(item,i);
			return;
		}
	}

	target->AddItem(item);
}

void AddReconcileItems(const TransactionData &data, void *ptr)
{
	if(data.Status()==TRANS_RECONCILED)
		return;
	
	ReconcileWindow *win = (ReconcileWindow *)ptr;
	
	switch(data.Type().TypeCode())
	{
		case TRANS_NUMERIC:
		{
			win->fCheckList->AddItem(new ReconcileItem(data));
			break;
		}
		
		case TRANS_DEP:
		{
			win->fDepositList->AddItem(new ReconcileItem(data));
			break;
		}
		
		case TRANS_ATM:
		case TRANS_XFER:
		default:
		{
			win->fChargeList->AddItem(new ReconcileItem(data));
			break;
		}
	}
}

/*
ReconcileFilter::ReconcileFilter(ReconcileWindow *win)
 : BMessageFilter(B_PROGRAMMED_DELIVERY, B_ANY_SOURCE,B_KEY_DOWN),
 	fWindow(win)
{
}

ReconcileFilter::~ReconcileFilter(void)
{
}

filter_result ReconcileFilter::Filter(BMessage *msg, BHandler **target)
{
	int32 mod;
	if(msg->FindInt32("modifiers",&mod)==B_OK)
	{
		if( (mod & B_COMMAND_KEY) || (mod & B_COMMAND_KEY) ||
			(mod & B_OPTION_KEY) )
			return B_DISPATCH_MESSAGE;
	}
		
	BView *v=dynamic_cast<BView*>(*target);
	if(!v || strcmp("_input_",v->Name())!=0)
		return B_DISPATCH_MESSAGE;
	
	BTextControl *text = dynamic_cast<BTextControl*>(v->Parent());
	if(!text)
		return B_DISPATCH_MESSAGE;
	
	if(text==fWindow->fDate)
	{
		int32 rawchar;
		msg->FindInt32("raw_char",&rawchar);
//		if(rawchar==B_ENTER)
//		{
//			fWindow->PostMessage(M_NEXT_FIELD);
//			return B_SKIP_MESSAGE;
//		}
	
		// Weed out navigation keys
		if(rawchar<32 && rawchar!=B_PAGE_UP && rawchar!=B_PAGE_DOWN)
			return B_DISPATCH_MESSAGE;
		
		int32 start, end;
		text->TextView()->GetSelection(&start,&end);
		
		BString string;
		if(rawchar=='+')
		{
			if(strlen(text->Text())>0)
				fWindow->fCurrentDate = IncrementDateByDay(fWindow->fCurrentDate);
			
			gDefaultLocale.DateToString(fWindow->fCurrentDate,string);
			text->SetText(string.String());
			text->TextView()->SelectAll();
			return B_SKIP_MESSAGE;
		}
		else
		if(rawchar=='-')
		{
			if(strlen(text->Text())>0)
				fWindow->fCurrentDate = DecrementDateByDay(fWindow->fCurrentDate);
			gDefaultLocale.DateToString(fWindow->fCurrentDate,string);
			text->SetText(string.String());
			text->TextView()->SelectAll();
			return B_SKIP_MESSAGE;
		}
		else
		if(rawchar==B_PAGE_UP)
		{
			if(strlen(text->Text())>0)
			{
				if(mod & B_SHIFT_KEY)
					fWindow->fCurrentDate = IncrementDateByYear(fWindow->fCurrentDate);
				else
					fWindow->fCurrentDate = IncrementDateByMonth(fWindow->fCurrentDate);
			}
			gDefaultLocale.DateToString(fWindow->fCurrentDate,string);
			text->SetText(string.String());
			text->TextView()->SelectAll();
			return B_SKIP_MESSAGE;
		}
		else
		if(rawchar==B_PAGE_DOWN)
		{
			if(strlen(text->Text())>0)
			{
				if(mod & B_SHIFT_KEY)
					fWindow->fCurrentDate = DecrementDateByYear(fWindow->fCurrentDate);
				else
					fWindow->fCurrentDate = DecrementDateByMonth(fWindow->fCurrentDate);
			}
			gDefaultLocale.DateToString(fWindow->fCurrentDate,string);
			text->SetText(string.String());
			text->TextView()->SelectAll();
			return B_SKIP_MESSAGE;
		}
	}
	
	return B_DISPATCH_MESSAGE;
}
*/
