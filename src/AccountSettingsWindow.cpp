#include "AccountSettingsWindow.h"
#include "Fixed.h"
#include <MessageFilter.h>

#include "AutoTextControl.h"
#include "CBLocale.h"
#include "Database.h"
#include "EscapeCancelFilter.h"
#include "PrefWindow.h"
#include "Translate.h"

#define	M_EDIT_ACCOUNT_SETTINGS 'east'
#define M_NAME_CHANGED 'nmch'
#define M_TOGGLE_USE_DEFAULT 'tgud'

AccountSettingsWindow::AccountSettingsWindow(Account *account)
 : BWindow(BRect(100,100,300,300),TRANSLATE("Account Settings"),B_FLOATING_WINDOW_LOOK,
 			B_MODAL_APP_WINDOW_FEEL,B_NOT_RESIZABLE | B_NOT_MINIMIZABLE |
 			B_NOT_ZOOMABLE),
 	fAccount(account)
{
	AddCommonFilter(new EscapeCancelFilter);
	
	BString temp;
	AddShortcut('W',B_COMMAND_KEY,new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q',B_COMMAND_KEY,new BMessage(B_QUIT_REQUESTED));
	
	BView *back = new BView(Bounds(),"back",B_FOLLOW_ALL,B_WILL_DRAW);
	back->SetViewColor(240,240,240);
	AddChild(back);
	
	BRect r(Bounds());
	r.InsetBy(10,10);
	
	temp = TRANSLATE("Account Name"); temp += ":";
	fAccountName = new AutoTextControl(r,"accname",temp.String(),
									(fAccount ? fAccount->Name() : NULL ),
									new BMessage(M_NAME_CHANGED));
	fAccountName->ResizeToPreferred();
	fAccountName->SetCharacterLimit(32);
	back->AddChild(fAccountName);
	r = fAccountName->Frame();
	
	fAccountName->MakeFocus(true);
	fAccountName->SetDivider(fAccountName->StringWidth(temp.String())+3);
	
	r.OffsetBy(0,r.Height()+10);
	fUseDefault = new BCheckBox(r,"usedefault","Use Default Currency Settings",
						new BMessage(M_TOGGLE_USE_DEFAULT));
	fUseDefault->ResizeToPreferred();
	if(!fAccount || fAccount->IsUsingDefaultLocale())
		fUseDefault->SetValue(B_CONTROL_ON);
	back->AddChild(fUseDefault);
	
	Locale templocale;
	if(fAccount)
		templocale = fAccount->GetLocale();
	fPrefView = new CurrencyPrefView(Bounds(),"prefview",&templocale);
	fPrefView->MoveTo(0,fUseDefault->Frame().bottom + 10);
	fPrefView->SetResizingMode(B_FOLLOW_TOP);
	
	fOK = new BButton(BRect(0,0,1,1),"okbutton",TRANSLATE("Cancel"),
						new BMessage(M_EDIT_ACCOUNT_SETTINGS),
						B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
	fOK->ResizeToPreferred();
	fOK->SetLabel(TRANSLATE("OK"));
	fOK->MoveTo(fPrefView->Frame().right - fOK->Frame().Width() - 10,
				fPrefView->Frame().bottom + 10);
	
	if(strlen(fAccountName->Text())<1)
		fOK->SetEnabled(false);
	
	fCancel = new BButton(BRect(0,0,1,1),"cancelbutton",TRANSLATE("Cancel"),
				new BMessage(B_QUIT_REQUESTED),B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
	fCancel->ResizeToPreferred();
	fCancel->MoveTo(fOK->Frame().left - 10 - fCancel->Frame().Width(), fOK->Frame().top);
	
	ResizeTo(fOK->Frame().right + 10,fOK->Frame().bottom + 10);
	fAccountName->ResizeTo(Bounds().Width() - 20, r.Height());
	
	back->AddChild(fPrefView);
	back->AddChild(fCancel);
	back->AddChild(fOK);
	
	SetDefaultButton(fOK);
	
	if(!fAccount || fAccount->IsUsingDefaultLocale())
	{
			ResizeTo(Frame().Width(),fUseDefault->Frame().bottom + 20 + fOK->Bounds().Height());
			fPrefView->Hide();
	}
}

void AccountSettingsWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_EDIT_ACCOUNT_SETTINGS:
		{
			Locale temp;
			
			if(!fAccount)
			{
				temp = gDefaultLocale;
				fPrefView->GetSettings(temp);
				gDatabase.AddAccount(fAccountName->Text(),ACCOUNT_BANK,"Open",&temp);
			}
			else
			{
				if(strcmp(fAccountName->Text(),fAccount->Name())!=0)
					gDatabase.RenameAccount(fAccount,fAccountName->Text());
				
				temp = fAccount->GetLocale();
				fPrefView->GetSettings(temp);
				if(temp!=fAccount->GetLocale())
					fAccount->SetLocale(temp);
			}
			
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case M_TOGGLE_USE_DEFAULT:
		{
			if(fUseDefault->Value()==B_CONTROL_ON)
			{
				ResizeTo(Frame().Width(),fUseDefault->Frame().bottom + 20 + fOK->Bounds().Height());
				fPrefView->Hide();
				fAccount->UseDefaultLocale(true);
			}
			else
			{
				ResizeTo(Frame().Width(),
						fPrefView->Frame().bottom + 20 + fOK->Bounds().Height());
				fPrefView->Show();
				fAccount->UseDefaultLocale(false);
			}
			
			break;
		}
		case M_NAME_CHANGED:
		{
			if(strlen(fAccountName->Text())<1)
				fOK->SetEnabled(false);
			else
				fOK->SetEnabled(true);
			
			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}
