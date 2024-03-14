#include "AccountSettingsWindow.h"
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MessageFilter.h>
#include "Fixed.h"

#include "AutoTextControl.h"
#include "CBLocale.h"
#include "Database.h"
#include "EscapeCancelFilter.h"
#include "Fixed.h"
#include "PrefWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AccountSettingsWindow"


#define M_EDIT_ACCOUNT_SETTINGS 'east'
#define M_NAME_CHANGED 'nmch'
#define M_TOGGLE_USE_DEFAULT 'tgud'

AccountSettingsWindow::AccountSettingsWindow(Account* account)
	: BWindow(BRect(0, 0, 1, 1), B_TRANSLATE("Account settings"), B_FLOATING_WINDOW_LOOK,
		  B_MODAL_APP_WINDOW_FEEL,
		  B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	  fAccount(account)
{
	AddCommonFilter(new EscapeCancelFilter);

	BString temp;
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	BView* back = new BView("back", B_WILL_DRAW);
	back->SetViewColor(240, 240, 240);

	temp = B_TRANSLATE("Account name:");
	fAccountName = new AutoTextControl("accname", temp.String(),
		(fAccount ? fAccount->Name() : NULL), new BMessage(M_NAME_CHANGED));
	fAccountName->SetCharacterLimit(32);

	fAccountName->MakeFocus(true);
	fAccountName->SetDivider(fAccountName->StringWidth(temp.String()) + 3);

	fUseDefault = new BCheckBox("usedefault", B_TRANSLATE("Use default currency settings"),
		new BMessage(M_TOGGLE_USE_DEFAULT));
	if (!fAccount || fAccount->IsUsingDefaultLocale())
		fUseDefault->SetValue(B_CONTROL_ON);

	Locale templocale;
	if (fAccount)
		templocale = fAccount->GetLocale();
	fPrefView = new CurrencyPrefView("prefview", &templocale);

	fOK = new BButton("okbutton", B_TRANSLATE("Cancel"), new BMessage(M_EDIT_ACCOUNT_SETTINGS));
	fOK->SetLabel(B_TRANSLATE("OK"));

	if (strlen(fAccountName->Text()) < 1)
		fOK->SetEnabled(false);

	fCancel = new BButton("cancelbutton", B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));

	SetDefaultButton(fOK);

	if (!fAccount || fAccount->IsUsingDefaultLocale()) {
		fPrefView->Hide();
	}

	BLayoutBuilder::Group<>(back, B_VERTICAL, 0.0f)
		.SetInsets(10)
		.AddGroup(B_VERTICAL, 0.0f)
		.Add(fAccountName)
		.Add(fUseDefault)
		.Add(fPrefView)
		.End()
		.AddGrid(0.0f, 0.0f)
		.AddGlue(0, 0)
		.Add(fCancel, 1, 0)
		.Add(fOK, 2, 0)
		.End()
		.End();
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f).SetInsets(0).Add(back).End();
}

void
AccountSettingsWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_EDIT_ACCOUNT_SETTINGS:
		{
			Locale temp;

			if (!fAccount) {
				temp = gDefaultLocale;
				fPrefView->GetSettings(temp);
				gDatabase.AddAccount(fAccountName->Text(), ACCOUNT_BANK, "Open", &temp);
			} else {
				if (strcmp(fAccountName->Text(), fAccount->Name()) != 0)
					gDatabase.RenameAccount(fAccount, fAccountName->Text());

				temp = fAccount->GetLocale();
				fPrefView->GetSettings(temp);
				if (temp != fAccount->GetLocale())
					fAccount->SetLocale(temp);
			}

			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case M_TOGGLE_USE_DEFAULT:
		{
			bool useDefault = fUseDefault->Value() == B_CONTROL_ON;

			if (useDefault)
				fPrefView->Hide();
			else
				fPrefView->Show();

			if (fAccount != NULL)
				fAccount->UseDefaultLocale(useDefault);

			break;
		}
		case M_NAME_CHANGED:
		{
			if (strlen(fAccountName->Text()) < 1)
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
