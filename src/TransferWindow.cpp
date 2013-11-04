#include "TransferWindow.h"
#include <MessageFilter.h>
#include "DAlert.h"
#include <ScrollView.h>
#include <Messenger.h>

#include "CurrencyBox.h"
#include "Database.h"
#include "DateBox.h"
#include "Fixed.h"
#include "CBLocale.h"
#include "MsgDefs.h"

#include "TimeSupport.h"
#include "Translate.h"

#define M_SOURCE_SELECTED 'srsl'
#define M_DEST_SELECTED 'dssl'
#define M_DATE_CHANGED 'dtch'
#define M_AMOUNT_CHANGED 'amch'

TransferWindow::TransferWindow(BHandler *target)
 : BWindow(BRect(100,100,500,350),TRANSLATE("Add Account Transfer"),B_TITLED_WINDOW_LOOK,
 			B_MODAL_APP_WINDOW_FEEL,B_NOT_RESIZABLE | B_NOT_MINIMIZABLE |
 			B_NOT_ZOOMABLE),
 	fMessenger(target),
 	fMessage(M_CREATE_TRANSFER)
{
	InitObject(NULL,NULL,Fixed(0));
}

TransferWindow::TransferWindow(BHandler *target, Account *src,  Account *dest, const Fixed &amount)
 : BWindow(BRect(100,100,300,300),TRANSLATE("Edit Transfer"),B_TITLED_WINDOW_LOOK,
 			B_MODAL_APP_WINDOW_FEEL,B_NOT_RESIZABLE | B_NOT_MINIMIZABLE |
 			B_NOT_ZOOMABLE),
 	fMessenger(target)
{
	InitObject(src,dest,amount);
}

void TransferWindow::InitObject(Account *src,  Account *dest, const Fixed &amount)
{
	BString temp;
	AddShortcut('W',B_COMMAND_KEY,new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q',B_COMMAND_KEY,new BMessage(B_QUIT_REQUESTED));
	
	BView *back = new BView(Bounds(),"back",B_FOLLOW_ALL,B_WILL_DRAW);
	back->SetViewColor(240,240,240);
	AddChild(back);
	
	temp = TRANSLATE("From Account"); temp += ":";
	fFromLabel = new BStringView(BRect(10,10,11,11),"fromlabel",temp.String());
	fFromLabel->ResizeToPreferred();
	back->AddChild(fFromLabel);
	
	BRect r(Bounds());
	r.left = MAX(Bounds().Width()/2,fFromLabel->Frame().right+10);
	r.right -= 10;
	r.top = 10;
	r.bottom = 11;
	temp = TRANSLATE("To Account"); temp += ":";
	fToLabel = new BStringView(r,"tolabel",temp.String());
	fToLabel->ResizeToPreferred();
	back->AddChild(fToLabel);
	
	fOK = new BButton(BRect(0,0,1,1),"okbutton",TRANSLATE("Cancel"),
						new BMessage(M_CREATE_TRANSFER),B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
	fOK->ResizeToPreferred();
	fOK->SetLabel(TRANSLATE("OK"));
	fOK->MoveTo(Bounds().right - 10 - fOK->Frame().Width(),
				Bounds().bottom - 10 - fOK->Frame().Height());
	fOK->SetEnabled(false);
	fOK->MakeDefault(true);
	
	fCancel = new BButton(BRect(0,0,1,1),"cancelbutton",TRANSLATE("Cancel"),
				new BMessage(B_QUIT_REQUESTED),B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
	fCancel->ResizeToPreferred();
	fCancel->MoveTo(fOK->Frame().left - 10 - fCancel->Frame().Width(),
				Bounds().bottom - 10 - fCancel->Frame().Height());
	
	r = fOK->Frame();
	r.top += 5;
	r.left = 10;
	r.right = MIN(fCancel->Frame().left - 10,Bounds().Width()/2);
	temp = TRANSLATE("Memo"); temp += ":";
	fMemo = new BTextControl(r,"memobox",temp.String(),NULL,NULL,B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	fMemo->SetDivider(fMemo->StringWidth(temp.String())+5);
	
	r = fMemo->TextView()->Frame();
	r.OffsetTo(fMemo->Frame().right+10,fOK->Frame().top - r.Height() - 10);
	r.right = Bounds().right - 10;
	BString amt;
	gCurrentLocale.CurrencyToString(amount,amt);
	temp = TRANSLATE("Amount"); temp += ":";
	fAmount = new CurrencyBox(r,"amountbox",temp.String(),amt.String(),NULL,B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	fAmount->GetFilter()->SetMessenger(new BMessenger(this));
	fAmount->SetDivider(fAmount->StringWidth(temp.String())+5);
	
	r.right = r.left - 10;
	r.left = 10;
	temp = TRANSLATE("Date"); temp += ":";
	fDate = new DateBox(r,"datebox",temp.String(),"",NULL,B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	fDate->GetFilter()->SetMessenger(new BMessenger(this));
	fDate->SetDivider(fDate->StringWidth(temp.String())+5);
//	fDate->SetEscapeCancel(true);
	
	if(src && dest)
	{
		BString datestr;
		gDefaultLocale.DateToString(fDate->GetDate(),datestr);
		fDate->SetText(datestr.String());
	}
	else
	{
		BString datestr;
		gDefaultLocale.DateToString(fDate->GetDate(),datestr);
		fDate->SetText(datestr.String());
	}
	
	r.left = fFromLabel->Frame().left;
	r.top = fFromLabel->Frame().bottom + 5;
	r.right = fToLabel->Frame().left - 10 - B_V_SCROLL_BAR_WIDTH;
	r.bottom = fAmount->Frame().top - 10;
	
	fSourceList = new BListView(r,"sourcelist");
	BScrollView *scrollsrc = new BScrollView("sourcescroll",fSourceList, B_FOLLOW_LEFT | B_FOLLOW_TOP,
											0,false,true);
	back->AddChild(scrollsrc);
	fSourceList->SetSelectionMessage(new BMessage(M_SOURCE_SELECTED));
	scrollsrc->SetViewColor(back->ViewColor());
	
	r.left = fToLabel->Frame().left;
	r.right = Bounds().right - 10 - B_V_SCROLL_BAR_WIDTH;
	fDestList = new BListView(r,"destlist");
	BScrollView *scrolldest = new BScrollView("destscroll",fDestList,B_FOLLOW_LEFT | B_FOLLOW_TOP,
											0,false,true);
	back->AddChild(scrolldest);
	fDestList->SetSelectionMessage(new BMessage(M_DEST_SELECTED));
	scrolldest->SetViewColor(back->ViewColor());
	
	back->AddChild(fDate);
	back->AddChild(fAmount);
	back->AddChild(fMemo);
	back->AddChild(fCancel);
	back->AddChild(fOK);
	
	int32 current=-1;
	for(int32 i=0; i<gDatabase.CountAccounts(); i++)
	{
		Account *acc = gDatabase.AccountAt(i);
		if(acc)
		{
			fSourceList->AddItem(new AccountListItem(acc));
			fDestList->AddItem(new AccountListItem(acc));
			if(acc == gDatabase.CurrentAccount())
				current = i;
		}
	}
	
	if(current>=0)
	{
		fSourceList->Select(current);
		fDestList->ItemAt(current)->SetEnabled(false);
	}
	
	if(gDatabase.CountAccounts()==2)
	{
		// When there are just 2 accounts, automatically select the other account and set focus
		// to the amount box
		if(fSourceList->CurrentSelection()==0)
			fDestList->Select(1);
		else
			fDestList->Select(0L);
		fAmount->MakeFocus(true); 
	}
	else
		fDestList->MakeFocus(true);
}

void TransferWindow::SetMessage(BMessage msg)
{
	fMessage = msg;
}

void TransferWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_SOURCE_SELECTED:
		{
			for(int32 i=0; i<fDestList->CountItems(); i++)
			{
				AccountListItem *item = (AccountListItem *)fDestList->ItemAt(i);
				if(item && !item->IsEnabled())
				{
					item->SetEnabled(true);
					fDestList->InvalidateItem(i);
				}
			}
			if(fSourceList->CurrentSelection()>=0)
			{
				fDestList->ItemAt(fSourceList->CurrentSelection())->SetEnabled(false);
				fDestList->InvalidateItem(fSourceList->CurrentSelection());
			}
			break;
		}
		case M_DEST_SELECTED:
		{
			HandleOKButton();
			break;
		}
		case M_PREVIOUS_FIELD:
		{
			// This message is received from the text filter in order to
			// use the Enter key to change from one entry field to another but in the
			// reverse order from M_NEXT_FIELD
			if(fDate->ChildAt(0)->IsFocus())
			{
				fDate->Validate(false);
				fOK->MakeFocus(true);
			}
			else
			if(fAmount->ChildAt(0)->IsFocus())
			{
				fAmount->Validate(false);
				fDate->MakeFocus(true);
			}
			HandleOKButton();
			break;
		}
		case M_NEXT_FIELD:
		{
			// This message is received from the text filter in order to
			// use the Enter key to change from one entry field to another
			if(fDate->ChildAt(0)->IsFocus())
			{
				fDate->Validate(false);
				fAmount->MakeFocus(true);
			}
			else
			if(fAmount->ChildAt(0)->IsFocus())
			{
				fAmount->Validate(false);
				fMemo->MakeFocus(true);
			}
			HandleOKButton();
			break;
		}
		case M_CREATE_TRANSFER:
		{
			if(!fDate->Validate())
				break;
			
			if(!fAmount->Validate())
				break;
			
			AccountListItem *sitem = (AccountListItem*)fSourceList->ItemAt(fSourceList->CurrentSelection());
			if(!sitem)
				break;
			
			AccountListItem *ditem = (AccountListItem*)fDestList->ItemAt(fDestList->CurrentSelection());
			if(!ditem)
				break;
			
			Fixed amount;
			gCurrentLocale.StringToCurrency(fAmount->Text(),amount);
			if(amount==0)
			{
				ShowAlert(TRANSLATE("Not Transferring Any Money"),
						TRANSLATE("If you intend to transfer money, it will need to "
						"be an amount that is not zero."));
				break;
			}
			
			fMessage.AddPointer("from",sitem->GetAccount());
			fMessage.AddPointer("to",ditem->GetAccount());
			fMessage.AddString("amount",fAmount->Text());
			fMessage.AddString("memo",fMemo->Text());
			fMessage.AddString("date",fDate->Text());
			fMessenger.SendMessage(&fMessage);
			fMessage.MakeEmpty();
			
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}

void TransferWindow::HandleOKButton(void)
{
	if(fSourceList->CurrentSelection()>=0)
	{
		AccountListItem *item = (AccountListItem *)fDestList->ItemAt(fDestList->CurrentSelection());
		if(item && item->IsEnabled())
		{
			fOK->SetEnabled(true);
			return;
		}
	}
	
	if(fOK->IsEnabled())
		fOK->SetEnabled(false);
}
