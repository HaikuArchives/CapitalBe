#include "TransferWindow.h"
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <ScrollView.h>

#include "CBLocale.h"
#include "CurrencyBox.h"
#include "DAlert.h"
#include "Database.h"
#include "DateBox.h"
#include "Fixed.h"
#include "MsgDefs.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TransferWindow"


#define M_SOURCE_SELECTED 'srsl'
#define M_DEST_SELECTED 'dssl'
#define M_DATE_CHANGED 'dtch'
#define M_AMOUNT_CHANGED 'amch'


TransferWindow::TransferWindow(BHandler* target)
	: BWindow(BRect(100, 100, 500, 350), B_TRANSLATE("Add account transfer"), B_TITLED_WINDOW_LOOK,
		  B_MODAL_APP_WINDOW_FEEL,
		  B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS |
			  B_CLOSE_ON_ESCAPE),
	  fMessenger(target),
	  fMessage(M_CREATE_TRANSFER)
{
	InitObject(NULL, NULL, Fixed(0));
}


TransferWindow::TransferWindow(BHandler* target, Account* src, Account* dest, const Fixed& amount)
	: BWindow(BRect(100, 100, 300, 300), B_TRANSLATE("Edit transfer"), B_TITLED_WINDOW_LOOK,
		  B_MODAL_APP_WINDOW_FEEL,
		  B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS |
			  B_CLOSE_ON_ESCAPE),
	  fMessenger(target)
{
	InitObject(src, dest, amount);
}

void
TransferWindow::InitObject(Account* src, Account* dest, const Fixed& amount)
{
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	BView* back = new BView("back", B_WILL_DRAW);
	back->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0).SetInsets(0).Add(back).End();

	fFromLabel = new BStringView("fromlabel", B_TRANSLATE("From account:"));

	BRect r(Bounds());
	fToLabel = new BStringView("tolabel", B_TRANSLATE("To account:"));

	fOK = new BButton("okbutton", B_TRANSLATE("OK"), new BMessage(M_CREATE_TRANSFER));
	fOK->SetEnabled(false);
	fOK->MakeDefault(true);

	fCancel = new BButton("cancelbutton", B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));

	fMemo = new BTextControl("memobox", B_TRANSLATE("Memo:"), NULL, NULL);

	BString amt;
	BString temp = B_TRANSLATE("Amount");
	temp << ":";
	gCurrentLocale.CurrencyToString(amount, amt);
	fAmount = new CurrencyBox("amountbox", temp.String(), amt.String(), NULL);
	fAmount->GetFilter()->SetMessenger(new BMessenger(this));

	fDate = new DateBox("datebox", B_TRANSLATE("Date:"), "", NULL);
	fDate->GetFilter()->SetMessenger(new BMessenger(this));
	//	fDate->SetEscapeCancel(true);

	if (src && dest) {
		BString datestr;
		gDefaultLocale.DateToString(fDate->GetDate(), datestr);
		fDate->SetText(datestr.String());
	} else {
		BString datestr;
		gDefaultLocale.DateToString(fDate->GetDate(), datestr);
		fDate->SetText(datestr.String());
	}

	fSourceList = new BListView("sourcelist");
	BScrollView* scrollsrc = new BScrollView("sourcescroll", fSourceList, 0, false, true);
	fSourceList->SetSelectionMessage(new BMessage(M_SOURCE_SELECTED));
	scrollsrc->SetViewColor(back->ViewColor());

	fDestList = new BListView("destlist");
	BScrollView* scrolldest = new BScrollView("destscroll", fDestList, 0, false, true);
	fDestList->SetSelectionMessage(new BMessage(M_DEST_SELECTED));
	scrolldest->SetViewColor(back->ViewColor());

	int32 current = -1;
	for (int32 i = 0; i < gDatabase.CountAccounts(); i++) {
		Account* acc = gDatabase.AccountAt(i);
		if (acc && !acc->IsClosed()) {
			fSourceList->AddItem(new AccountListItem(acc));
			fDestList->AddItem(new AccountListItem(acc));
			if (acc == gDatabase.CurrentAccount())
				current = i;
		}
	}

	if (current >= 0) {
		fSourceList->Select(current);
		fDestList->ItemAt(current)->SetEnabled(false);
	}

	if (gDatabase.CountAccounts() == 2) {
		// When there are just 2 accounts, automatically select the other account and set focus
		// to the amount box
		if (fSourceList->CurrentSelection() == 0)
			fDestList->Select(1);
		else
			fDestList->Select(0L);
		fAmount->MakeFocus(true);
	} else
		fDestList->MakeFocus(true);

	BLayoutBuilder::Group<>(back, B_VERTICAL, 0)
		.SetInsets(10)
		.AddGrid(4.0f, 1.0f)
		.Add(fFromLabel, 0, 0)
		.Add(scrollsrc, 0, 1, 2)
		.Add(fDate, 0, 2, 2)
		.Add(fMemo, 0, 3, 2)
		.Add(fToLabel, 2, 0)
		.Add(scrolldest, 2, 1, 2)
		.Add(fAmount, 2, 2, 2)
		.AddGrid(1.0f, 1.0f, 2, 3, 2)
		.AddGlue(0, 0)
		.Add(fCancel, 1, 0)
		.Add(fOK, 2, 0)
		.End()
		.End()
		.End();
}


void
TransferWindow::SetMessage(BMessage msg)
{
	fMessage = msg;
}

void
TransferWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_SOURCE_SELECTED:
		{
			for (int32 i = 0; i < fDestList->CountItems(); i++) {
				AccountListItem* item = (AccountListItem*)fDestList->ItemAt(i);
				if (item && !item->IsEnabled()) {
					item->SetEnabled(true);
					fDestList->InvalidateItem(i);
				}
			}
			if (fSourceList->CurrentSelection() >= 0) {
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
			if (fDate->ChildAt(0)->IsFocus()) {
				fDate->Validate(false);
				fOK->MakeFocus(true);
			} else if (fAmount->ChildAt(0)->IsFocus()) {
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
			if (fDate->ChildAt(0)->IsFocus()) {
				fDate->Validate(false);
				fAmount->MakeFocus(true);
			} else if (fAmount->ChildAt(0)->IsFocus()) {
				fAmount->Validate(false);
				fMemo->MakeFocus(true);
			}
			HandleOKButton();
			break;
		}
		case M_CREATE_TRANSFER:
		{
			if (!fDate->Validate())
				break;

			if (!fAmount->Validate())
				break;

			AccountListItem* sitem =
				(AccountListItem*)fSourceList->ItemAt(fSourceList->CurrentSelection());
			if (!sitem)
				break;

			AccountListItem* ditem =
				(AccountListItem*)fDestList->ItemAt(fDestList->CurrentSelection());
			if (!ditem)
				break;

			Fixed amount;
			gCurrentLocale.StringToCurrency(fAmount->Text(), amount);
			if (amount == 0) {
				ShowAlert(B_TRANSLATE("Not transferring any money"),
					B_TRANSLATE("If you intend to transfer money, it will need to "
								"be an amount that is not zero."));
				break;
			}

			fMessage.AddPointer("from", sitem->GetAccount());
			fMessage.AddPointer("to", ditem->GetAccount());
			fMessage.AddString("amount", fAmount->Text());
			fMessage.AddString("memo", fMemo->Text());
			fMessage.AddString("date", fDate->Text());
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

void
TransferWindow::HandleOKButton(void)
{
	if (fSourceList->CurrentSelection() >= 0) {
		AccountListItem* item = (AccountListItem*)fDestList->ItemAt(fDestList->CurrentSelection());
		if (item && item->IsEnabled()) {
			fOK->SetEnabled(true);
			return;
		}
	}

	if (fOK->IsEnabled())
		fOK->SetEnabled(false);
}
