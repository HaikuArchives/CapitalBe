#include <Messenger.h>
#include <Window.h>
#include <stdlib.h>

#include "Account.h"
#include "BuildOptions.h"
#include "CategoryBox.h"
#include "CheckNumBox.h"
#include "CheckView.h"
#include "CurrencyBox.h"
#include "DAlert.h"
#include "Database.h"
#include "DateBox.h"
#include "Layout.h"
#include "MainWindow.h"
#include "MsgDefs.h"
#include "NavTextBox.h"
#include "PayeeBox.h"
#include "Preferences.h"
#include "TransactionData.h"
#include "Translate.h"
#include "TimeSupport.h"

enum
{
	M_ENTER_TRANSACTION='entr'
};

CheckView::CheckView(const BRect &frame, const char *name, int32 resize, int32 flags)
 : BView(frame,name,resize,flags | B_FRAME_EVENTS)
{
	BRect r;
	
	r.left = 2;
	r.top = 0;
	r.bottom = gStringViewHeight;
	r.right = 15;
	fDateLabel = new BStringView(r,"datelabel",TRANSLATE("Date"));
	fDateLabel->ResizeToPreferred();
	AddChild(fDateLabel);
	
	float texttop = 0;
	float controltop = fDateLabel->Frame().bottom + 1;
	
	r.left = 0;
	r.top = controltop;
	r.bottom = controltop + gTextViewHeight;
	r.right = r.left + StringWidth("00-00-0000")+15;
	fDate = new DateBox(r,"dateentry","",NULL,new BMessage(M_DATE_CHANGED),
					B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fDate->SetDivider(0);
	AddChild(fDate);
	
	r.left = r.right + 2;
	r.right = r.left + StringWidth("0000")+15;
	r.top = controltop;
	r.bottom = controltop + gTextViewHeight;
	fType = new CheckNumBox(r,"typeentry","",NULL,new BMessage(M_TYPE_CHANGED),
					B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fType->SetDivider(0);
	AddChild(fType);
	
	r.left += 2;
	r.top = texttop;
	r.bottom = texttop + gStringViewHeight;
	fTypeLabel=new BStringView(r,"typelabel",TRANSLATE("Type"));
	fTypeLabel->ResizeToPreferred();
	AddChild(fTypeLabel);
	
	r.left = r.right + 2;
	r.right = Bounds().right - StringWidth("$000,000.00") - 12;
	r.top = controltop;
	r.bottom = controltop + gTextViewHeight;
	fPayee = new PayeeBox(r,"payeeentry","",NULL,new BMessage(M_PAYEE_CHANGED),
				B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fPayee->SetDivider(0);
	AddChild(fPayee);
	
	r.left += 2;
	r.top = texttop;
	r.bottom = gStringViewHeight;
	fPayeeLabel = new BStringView(r,"payeelabel",TRANSLATE("Payee"));
	fPayeeLabel->ResizeToPreferred();
	AddChild(fPayeeLabel);
	
	r.right = Bounds().right;
	r.top = controltop;
	r.bottom = controltop + gTextViewHeight;
	r.left = r.right - StringWidth("$000,000.00") - 10;
	fAmount = new CurrencyBox(r,"amountentry","","",new BMessage(M_AMOUNT_CHANGED),
					B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fAmount->SetDivider(0);
	AddChild(fAmount);
	
	r.left += 2;
	r.top = texttop;
	r.bottom = gStringViewHeight;
	fAmountLabel = new BStringView(r,"amountlabel",TRANSLATE("Amount"));
	fAmountLabel->ResizeToPreferred();
	AddChild(fAmountLabel);

	texttop = fDate->Frame().bottom + 1;
	controltop = texttop + gStringViewHeight + 1;
	
	r.top = texttop;
	r.bottom = texttop + gStringViewHeight;
	r.left = 0;
	r.right = (fAmount->Frame().right - 5) / 3;
	fCategoryLabel = new BStringView(r,"categorylabel",TRANSLATE("Category"));
	AddChild(fCategoryLabel);
	
	r.top = controltop;
	r.bottom = controltop + gTextViewHeight;
	fCategory = new CategoryBox(r,"categoryentry","",NULL,new BMessage(M_CATEGORY_CHANGED),
				B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fCategory->SetDivider(0);
	AddChild(fCategory);
	
	r.left = fCategory->Frame().right + 2;
	r.right = fAmount->Frame().right;
	r.top = texttop;
	r.bottom = texttop + gStringViewHeight;
	fMemoLabel = new BStringView(r,"memolabel",TRANSLATE("Memo"));
	AddChild(fMemoLabel);
	
	r.top = controltop;
	r.bottom = controltop + gTextViewHeight;
	fMemo = new NavTextBox(r,"memoentry","",NULL,new BMessage(M_MEMO_CHANGED),
				B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fMemo->SetDivider(0);
	fMemo->TextView()->DisallowChar(B_ESCAPE);
	fMemo->SetCharacterLimit(21);
	AddChild(fMemo);
	
	prefsLock.Lock();
	BString rechelp = gAppPath;
	prefsLock.Unlock();
	rechelp << "helpfiles/" << gCurrentLanguage->Name() << "/Main Window Help";
	
	BPoint helppoint(fCategory->Frame().LeftBottom());
	helppoint.y += ( (Bounds().bottom-helppoint.y)-16)/2;
	fHelpButton = new HelpButton(helppoint,"rechelp",rechelp.String());
	AddChild(fHelpButton);
	
	fEnter = new BButton(BRect(0,0,1,1),"enterbutton",TRANSLATE("Enter"),
						new BMessage(M_ENTER_TRANSACTION),B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
	
	#ifndef ENTER_NAVIGATION
	fEnter->MakeDefault(true);
	#endif
	
	fEnter->ResizeToPreferred();
	fEnter->MoveTo(Bounds().Width() - fEnter->Frame().Width(),
					fMemo->Frame().bottom + 10);
	
//	#ifndef ENTER_NAVIGATION
//	fEnter->MoveBy(0,-8);
//	#endif
	AddChild(fEnter);
	
	gDatabase.AddObserver(this);
}

CheckView::~CheckView(void)
{
}

void CheckView::AttachedToWindow(void)
{
	SetViewColor(Parent()->ViewColor());
	fDate->GetFilter()->SetMessenger(new BMessenger(this));
	fPayee->GetFilter()->SetMessenger(new BMessenger(this));
	fType->GetFilter()->SetMessenger(new BMessenger(this));
	fAmount->GetFilter()->SetMessenger(new BMessenger(this));
	fCategory->GetFilter()->SetMessenger(new BMessenger(this));
	fMemo->GetFilter()->SetMessenger(new BMessenger(this));
	
	fEnter->SetTarget(this);
}

void CheckView::MessageReceived(BMessage *msg)
{
	int32 start;
	BString string;
	switch(msg->what)
	{
		case M_TYPE_AUTOCOMPLETE:
		{
			msg->FindInt32("start",&start);
			msg->FindString("string",&string);
			fType->SetText(string.String());
			fType->TextView()->Select(start,string.CountChars());
			break;
		}
		case M_PAYEE_AUTOCOMPLETE:
		{
			msg->FindInt32("start",&start);
			msg->FindString("string",&string);
			fPayee->SetText(string.String());
			fPayee->TextView()->Select(start,string.CountChars());
			break;
		}
		case M_CATEGORY_AUTOCOMPLETE:
		{
			msg->FindInt32("start",&start);
			msg->FindString("string",&string);
			fCategory->SetText(string.String());
			fCategory->TextView()->Select(start,string.CountChars());
			break;
		}
		
		#ifdef ENTER_NAVIGATION
		
		case M_ENTER_NAVIGATION:
		{
			if(!fMemo->ChildAt(0)->IsFocus())
			{
				DoNextField();
				break;
			}
			
			// fall through to the next case
		}
		
		#endif
		
		case M_ENTER_TRANSACTION:
		{
			// The text filter sends this message whenever the user hits Enter
			// from the Memo field. The CheckView instance should do whatever is
			// needed to post the transaction into the register
			
			#ifdef DEMO_MODE
				Account *demoacc=gDatabase.CurrentAccount();
				if(demoacc && demoacc->CountTransactions()>=25)
				{
					ShowAlert(TRANSLATE("Demo Mode Limit"),
							TRANSLATE(
							"The Demo Mode limit has been reached on this account.",
							"You can manually enter up to 25 transactions per "
							"account. We hope that you like Capital Be and will "
							"purchase the full version. Have a nice day!"),
							B_IDEA_ALERT);
					MakeEmpty();
					break;
				}
			
			#endif
			
			if( !fDate->Validate() || !fType->Validate() ||
				!fPayee->Validate() || !fAmount->Validate() ||
				!fCategory->Validate() )
				break;
			
			Account *acc=gDatabase.CurrentAccount();
			if(!acc)
				break;
			
			TransactionData trans(acc,fDate->Text(),fType->Text(),
					fPayee->Text(),fAmount->Text(),fCategory->Text(),fMemo->Text(),
					real_time_clock_usecs());
			
			
			gDatabase.AddTransaction(trans);
			acc->SetCurrentTransaction(trans.GetID());
			
			if(trans.Type().TypeCode() == TRANS_NUMERIC)
				acc->SetLastCheckNumber(atol(trans.Type().Type()));
			
			MakeEmpty();
			
			gDatabase.GetTransaction(trans.GetID(),trans);
			fDate->SetDate(trans.Date());
			
			BString str;
			gDefaultLocale.DateToString(fDate->GetDate(),str);
			fDate->SetText(str.String());
			fDate->MakeFocus(true);
			break;
		}
		case M_PREVIOUS_FIELD:
		{
			if(fDate->ChildAt(0)->IsFocus())
			{
				if(fDate->Validate(false))
				{
					if(gDatabase.CurrentAccount() && strlen(fDate->Text())>0)
					{
						time_t date;
						gDefaultLocale.StringToDate(fDate->Text(),date);
						fDate->SetDate(date);
					}
					fMemo->MakeFocus(true);
				}
				break;
			}
			else
			if(fType->ChildAt(0)->IsFocus())
				fDate->MakeFocus(true);
			else
			if(fPayee->ChildAt(0)->IsFocus())
			{
				if(fPayee->Validate(false))
					fType->MakeFocus(true);
			}
			else
			if(fAmount->ChildAt(0)->IsFocus())
			{
				if(fAmount->Validate(false))
					fPayee->MakeFocus(true);
			}
			else
			if(fCategory->ChildAt(0)->IsFocus())
			{
//				if(fCategory->Validate())
					fAmount->MakeFocus(true);
			}
			if(fMemo->ChildAt(0)->IsFocus())
				fCategory->MakeFocus(true);
			break;
		}
		case M_NEXT_FIELD:
		{
			DoNextField();
			break;
		}
		default:
		{
			BView::MessageReceived(msg);
		}
	}
}

void CheckView::SetFields(const char *date,const char *type, const char *payee,
				const char *amount,const char *category,const char *memo)
{
	fDate->SetText(date);
	fType->SetText(type);
	fPayee->SetText(payee);
	fAmount->SetText(amount);
	fCategory->SetText(category);
	fMemo->SetText(memo);
}

void CheckView::HandleNotify(const uint64 &value, const BMessage *msg)
{
	if(value & WATCH_SELECT)
	{
		if(value & WATCH_ACCOUNT)
		{
			MakeEmpty();
			
			Account *account;
			if(msg->FindPointer("item",(void**)&account)==B_OK)
			{
				if(account && !account->HasObserver(this))
					account->AddObserver(this);
			}
			
			if(!account)
			{
				// This happens when the last account is deleted -- there
				// is no longer a current account
				fDate->MakeFocus(true);
				return;
			}
			
			
			if(account->CountTransactions()>0)
			{
				TransactionData data;
				gDatabase.GetTransaction(account->GetID(),data);
				fDate->SetDate(data.Date());
				
				BString text;
				gDefaultLocale.DateToString(data.Date(),text);
				fDate->SetText(text.String());
			}
			else
			{
				BString text;
				gDefaultLocale.DateToString(GetCurrentDate(),text);
				fDate->SetText(text.String());
			}
			
			fDate->MakeFocus(true);
		}
		else
		if(value & WATCH_TRANSACTION)
		{
			uint32 id;
			if(msg->FindInt32("id",(int32*)&id)==B_OK)
			{
				if(gDatabase.CurrentAccount())
				{
					TransactionData data;
					gDatabase.GetTransaction(id,data);
					fDate->SetDate(data.Date());
					
					BString text;
					gDefaultLocale.DateToString(data.Date(),text);
					fDate->SetText(text.String());
				}
			}
		}
	}
}

void CheckView::MakeEmpty(void)
{
	fDate->SetText("");
	fType->SetText("");
	fPayee->SetText("");
	fAmount->SetText("");
	fCategory->SetText("");
	fMemo->SetText("");
}

void CheckView::MakeFocus(bool value)
{
	fDate->MakeFocus(value);
}

void CheckView::FrameResized(float width, float height)
{
	fAmountLabel->MoveTo(fPayee->Frame().right + 2, fAmountLabel->Frame().top);
	fCategoryLabel->ResizeTo(Bounds().Width()/2,fCategoryLabel->Bounds().Height());
	fCategory->ResizeTo((fAmount->Frame().right - 5) / 3,fCategory->Bounds().Height());
	
	fMemo->MoveTo(fCategory->Frame().right + 2, fMemo->Frame().top);
	fMemo->ResizeTo(fAmount->Frame().right - fMemo->Frame().left,
					fMemo->Frame().Height());
	fMemoLabel->MoveTo(fCategory->Frame().right + 2, fMemoLabel->Frame().top);
}

void CheckView::DoNextField(void)
{
	if(fDate->ChildAt(0)->IsFocus())
	{
		if(fDate->Validate(false))
		{
			if(gDatabase.CurrentAccount() && strlen(fDate->Text())>0)
			{
				time_t date;
				gDefaultLocale.StringToDate(fDate->Text(),date);
				fDate->SetDate(date);
			}
		}
		fType->MakeFocus(true);
	}
	else
	if(fType->ChildAt(0)->IsFocus())
		fPayee->MakeFocus(true);
	else
	if(fPayee->ChildAt(0)->IsFocus())
	{
		if(fPayee->Validate(false))
			fAmount->MakeFocus(true);
	}
	else
	if(fAmount->ChildAt(0)->IsFocus())
	{
		if(fAmount->Validate(false))
			fCategory->MakeFocus(true);
	}
	else
	if(fCategory->ChildAt(0)->IsFocus())
	{
		// TODO: don't force entering a transaction when going to the
		// split window via key editing
		if(strcmp(fCategory->Text(),"Split")==0)
		{
			Window()->PostMessage(M_ENTER_TRANSACTION,this);
			Window()->PostMessage(M_EDIT_TRANSACTION);
			return; 
		}
		
		fMemo->MakeFocus(true);
	}
	else
	if(fMemo->ChildAt(0)->IsFocus())
	{
		fEnter->MakeFocus(true);
	}
	else
	{
		// We should *never* be here
		ShowBug("M_NEXT_FIELD received for unknown view in CheckView");
	}
}
