#include "MainWindow.h"

#include <Catalog.h>
#include <ControlLook.h>
#include <Directory.h>
#include <Entry.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <Messenger.h>
#include <NetPositive.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>
#include <Url.h>
#include <private/interface/AboutWindow.h>

#include <stdlib.h>

#include "AccountSettingsWindow.h"
#include "BudgetWindow.h"
#include "CBLocale.h"
#include "CategoryWindow.h"
#include "DAlert.h"
#include "IconMenuItem.h"
#include "Layout.h"
#include "PrefWindow.h"
#include "Preferences.h"
#include "ReconcileWindow.h"
#include "RegisterView.h"
#include "ReportWindow.h"
#include "ScheduleAddWindow.h"
#include "ScheduleListWindow.h"
#include "ScheduledExecutor.h"
#include "StringView.h"
#include "TextControl.h"
#include "TransactionEditWindow.h"
#include "TransferWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


// #define TEST
// #define NOSAVE

// Internal definition of the Haiku services daemon
#define B_SERVICES_DAEMON_RESTART 'SDRS'


int32 gTextViewHeight = 20;
int32 gStringViewHeight = 20;

MainWindow::MainWindow(BRect frame) : BWindow(frame, "", B_TITLED_WINDOW, 0)
{
	BString temp;
	SetTitle(B_TRANSLATE_SYSTEM_NAME("CapitalBe"));

	// These chunks of code will save a lot of headache later on --
	// we cache the preferred size of BTextControls to make control layout *much* easier.
	// If it gets much more complicated than these couple of chunks, then it should
	// be moved into its own init function that is called from here.
	float pw, ph;

	BTextControl* temptc = new BTextControl(BRect(0, 0, 1, 1), "temptc", "", "", NULL);
	AddChild(temptc);
	temptc->GetPreferredSize(&pw, &ph);
	gTextViewHeight = (int32)ph;
	RemoveChild(temptc);
	delete temptc;

	BStringView* tempsv = new BStringView(BRect(0, 0, 1, 1), "tempsv", "Foo");
	AddChild(tempsv);
	tempsv->GetPreferredSize(&pw, &ph);
	gStringViewHeight = (int32)ph;
	RemoveChild(tempsv);
	delete tempsv;

	fLoadError = false;
	InitSettings();

	AddShortcut(B_HOME, B_COMMAND_KEY, new BMessage(M_FIRST_TRANSACTION));
	AddShortcut(B_END, B_COMMAND_KEY, new BMessage(M_LAST_TRANSACTION));

	SetSizeLimits(520, 30000, 260, 30000);
	if (frame.Width() < 520)
		ResizeBy(520 - frame.Width(), 0);
	if (frame.Height() < 260)
		ResizeBy(0, 260 - frame.Height());

	BRect r(Bounds());
	r.bottom = 20;
	BMenuBar* bar = new BMenuBar("keybar");

	BMenu* menu = new BMenu("");
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Report a bug" B_UTF8_ELLIPSIS), new BMessage(M_REPORT_BUG)));
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Settings" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_OPTIONS_WINDOW), ','));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("About CapitalBe"), new BMessage(M_SHOW_ABOUT)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED), 'Q'));

	IconMenuItem* iconMenu;
	iconMenu = new IconMenuItem(menu, NULL, "application/x-vnd.wgp-CapitalBe", B_MINI_ICON);
	bar->AddItem(iconMenu);

	menu = new BMenu(B_TRANSLATE("File"));
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Categories" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_CATEGORY_WINDOW)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Scheduled transactions" B_UTF8_ELLIPSIS),
		new BMessage(M_SHOW_SCHEDULED_WINDOW)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Import from QIF file" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_IMPORT_PANEL)));
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Export to QIF file" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_EXPORT_PANEL)));
	bar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Account"));
	bar->AddItem(menu);
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Reconcile" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_RECONCILE_WINDOW), 'R'));
	menu->AddSeparatorItem();
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("New" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_NEW_ACCOUNT), 'N'));
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Delete" B_UTF8_ELLIPSIS), new BMessage(M_DELETE_ACCOUNT)));
	fAccountClosedItem = new BMenuItem(B_TRANSLATE("Close"), new BMessage(M_CLOSE_ACCOUNT));
	menu->AddItem(fAccountClosedItem);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Settings" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_ACCOUNT_SETTINGS)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Previous"), new BMessage(M_PREVIOUS_ACCOUNT),
		B_UP_ARROW, B_COMMAND_KEY | B_SHIFT_KEY));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Next"), new BMessage(M_NEXT_ACCOUNT), B_DOWN_ARROW,
		B_COMMAND_KEY | B_SHIFT_KEY));

	menu = new BMenu(B_TRANSLATE("Transaction"));
	bar->AddItem(menu);

	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Edit" B_UTF8_ELLIPSIS), new BMessage(M_EDIT_TRANSACTION), 'E'));
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Enter a transfer" B_UTF8_ELLIPSIS), new BMessage(M_ENTER_TRANSFER), 'T'));
	menu->AddSeparatorItem();
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Delete" B_UTF8_ELLIPSIS), new BMessage(M_DELETE_TRANSACTION)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Schedule this transaction" B_UTF8_ELLIPSIS),
		new BMessage(M_SCHEDULE_TRANSACTION)));
	menu->AddSeparatorItem();
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Previous"), new BMessage(M_PREVIOUS_TRANSACTION), B_UP_ARROW));
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Next"), new BMessage(M_NEXT_TRANSACTION), B_DOWN_ARROW));

	menu = new BMenu(B_TRANSLATE("Tools"));
	bar->AddItem(menu);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Budget"), new BMessage(M_SHOW_BUDGET_WINDOW)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Reports"), new BMessage(M_SHOW_REPORTS_WINDOW)));

	// We load the financial data before we create any of the views because the
	// notifications are not sent and startup time is *significantly* reduced
	LoadData();

	r = Bounds();
	r.top = bar->Frame().bottom + 1;
	fRegisterView = new RegisterView("registerview", B_WILL_DRAW);

	fImportPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false,
		new BMessage(M_IMPORT_ACCOUNT));
	temp = B_TRANSLATE("CapitalBe:");
	temp += " ";
	temp += B_TRANSLATE("Import");
	fImportPanel->Window()->SetTitle(temp.String());
	fExportPanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false,
		new BMessage(M_EXPORT_ACCOUNT));
	temp = B_TRANSLATE("CapitalBe:");
	temp += " ";
	temp += B_TRANSLATE("Export");
	fExportPanel->Window()->SetTitle(temp.String());
	gDatabase.AddObserver(this);

	HandleScheduledTransactions();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0).SetInsets(0).Add(bar).Add(fRegisterView).End();
}

MainWindow::~MainWindow(void)
{
	SavePreferences(PREFERENCES_PATH "/CapitalBeSettings");
	delete fImportPanel;
}

void
MainWindow::OpenAbout(void)
{
	BAboutWindow* abwin = new BAboutWindow("CapitalBe", "application/x-vnd.wgp-CapitalBe");

	const char* authors[] = {"DarkWyrm", "Jérôme Duval", "Panagiotis Vasilopoulos",
		"Raefaldhi Amartya Junior", "Thomas Schmidt", "waddlesplash", NULL};

	const char* thanks[] = {"Tanausú Gómez (Spanish translation)", NULL};

	abwin->AddCopyright(2009, "DarkWyrm");
	abwin->AddText("Distributed under the terms of the MIT License");
	abwin->AddText("https://github.com/HaikuArchives/CapitalBe");
	abwin->AddAuthors(authors);
	abwin->AddSpecialThanks(thanks);
	abwin->CenterIn(Frame());
	abwin->Show();
}

bool
MainWindow::QuitRequested(void)
{
#ifndef NOSAVE
	SaveData();
#endif
	prefsLock.Lock();
	gPreferences.RemoveData("mainframe");
	gPreferences.AddRect("mainframe", Frame());
	prefsLock.Unlock();

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void
MainWindow::InitSettings(void)
{
	// This loads all the settings from disk and uses sane defaults if a setting
	// is non-existent or invalid
	if (gPreferences.FindString("lastfile", &fLastFile) != B_OK)
		fLastFile = PREFERENCES_PATH "/MyAccountData";
}


void
MainWindow::MessageReceived(BMessage* msg)
{
	Account* acc = gDatabase.CurrentAccount();

	switch (msg->what) {
		case M_REPORT_BUG:
		{
			BUrl("https://github.com/HaikuArchives/CapitalBe/issues/")
				.OpenWithPreferredApplication();
			break;
		}
		case M_SHOW_NEW_ACCOUNT:
		{
			AccountSettingsWindow* newaccwin = new AccountSettingsWindow(NULL);
			BRect r(Frame());
			newaccwin->MoveTo(r.left + ((Bounds().Width() - newaccwin->Bounds().Width()) / 2),
				r.top + ((Bounds().Height() - newaccwin->Bounds().Height()) / 2));
			newaccwin->Show();
			break;
		}
		case M_SHOW_ACCOUNT_SETTINGS:
		{
			if (!acc)
				break;

			AccountSettingsWindow* accwin = new AccountSettingsWindow(acc);
			BRect r(Frame());
			accwin->MoveTo(r.left + ((Bounds().Width() - accwin->Bounds().Width()) / 2),
				r.top + ((Bounds().Height() - accwin->Bounds().Height()) / 2));
			accwin->Show();
			break;
		}
		case M_DELETE_ACCOUNT:
		{
			if (!acc)
				break;

			DAlert* alert = new DAlert(B_TRANSLATE("Really delete account?"),
				B_TRANSLATE("Once deleted, you will not be able to "
							"get back any data on this account."),
				B_TRANSLATE("Delete"), B_TRANSLATE("Cancel"), NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);

			if (alert->Go() == 0) {
				int32 index = gDatabase.IndexOf(acc);
				if (index == 0) {
					gDatabase.SetCurrentAccount(1);
				} else if (index > 0) {
					gDatabase.SetCurrentAccount(index - 1);
				}
				gDatabase.RemoveAccount(acc);
			}
			break;
		}
		case M_PREVIOUS_ACCOUNT:
		{
			if (gDatabase.CountAccounts() < 2)
				break;

			int32 index = gDatabase.IndexOf(acc);
			if (index < 1 || index > gDatabase.CountAccounts() - 1)
				break;

			// Because there is no way to call BListView::Select without generating
			// a selection notification firing, we have to hack a method to manually
			// do the selecting which then generates the proper notification instead
			// of tweaking the database via SetCurrentAccount. Man, I hate hacks sometimes. :/
			fRegisterView->SelectAccount(index - 1);
			break;
		}
		case M_NEXT_ACCOUNT:
		{
			if (gDatabase.CountAccounts() < 2)
				break;

			int32 index = gDatabase.IndexOf(acc);
			if (index < 0 || index >= gDatabase.CountAccounts() - 1)
				break;

			// Because there is no way to call BListView::Select without generating
			// a selection notification firing, we have to hack a method to manually
			// do the selecting which then generates the proper notification instead
			// of tweaking the database via SetCurrentAccount. Man, I hate hacks sometimes. :/
			fRegisterView->SelectAccount(index + 1);
			break;
		}
		case M_SHOW_IMPORT_PANEL:
		{
			fImportPanel->Show();
			break;
		}
		case M_IMPORT_ACCOUNT:
		{
			entry_ref ref;

			//			BEntry entry("/boot/develop/projects/CapitalBe/source/yoderdata.qif");
			//			entry.GetRef(&ref);

			if (msg->FindRef("refs", &ref) != B_OK)
				break;

			if (!gDatabase.ImportFile(ref)) {
				BString errmsg(B_TRANSLATE("Could not import the data in the file %%FILENAME%%."));
				errmsg.ReplaceFirst("%%FILENAME%%", ref.name);
				ShowAlert(errmsg.String(),
					B_TRANSLATE("This happens when the kind of file is not "
								"supported, when the file's data is damaged, or when you feed "
								"it a recipe for quiche."));
			}
			break;
		}
		case M_SHOW_EXPORT_PANEL:
		{
			fExportPanel->Show();
			break;
		}
		case M_EXPORT_ACCOUNT:
		{
			entry_ref dir;
			BString name;
			if (msg->FindRef("directory", &dir) != B_OK || msg->FindString("name", &name) != B_OK)
				break;

			BPath path(&dir);
			name.Prepend("/");
			name.Prepend(path.Path());

			BEntry entry(name.String());
			entry.GetRef(&dir);

			if (!gDatabase.ExportFile(dir)) {
				BString errmsg(
					B_TRANSLATE("Could not export your financial data to the file %%FILENAME%%."));
				errmsg.ReplaceFirst("%%FILENAME%%", dir.name);
				ShowAlert(
					errmsg.String(), B_TRANSLATE("This really shouldn't happen, so you probably "
												 "should e-mail support about this."));
			}
			break;
		}
		case M_CLOSE_ACCOUNT:
		{
			if (!acc)
				break;

			if (acc->IsClosed())
				gDatabase.ReopenAccount(acc);
			else
				gDatabase.CloseAccount(acc);
			break;
		}
		case M_SHOW_ABOUT:
		{
			OpenAbout();
			break;
		}
		case M_PREVIOUS_TRANSACTION:
		{
			fRegisterView->SelectPreviousTransaction();
			break;
		}
		case M_NEXT_TRANSACTION:
		{
			fRegisterView->SelectNextTransaction();
			break;
		}
		case M_FIRST_TRANSACTION:
		{
			fRegisterView->SelectFirstTransaction();
			break;
		}
		case M_LAST_TRANSACTION:
		{
			fRegisterView->SelectLastTransaction();
			break;
		}
		case M_DELETE_TRANSACTION:
		{
			if (!acc)
				break;

			uint32 count = acc->CountTransactions();
			if (!count)
				break;

			uint32 item = acc->CurrentTransaction();

			if (!fRegisterView->SelectPreviousTransaction())
				fRegisterView->SelectNextTransaction();
			gDatabase.RemoveTransaction(item);
			break;
		}
		case M_ENTER_TRANSFER:
		{
			if (gDatabase.CountAccounts() < 2) {
				ShowAlert(B_TRANSLATE("Not enough accounts for a transfer."),
					B_TRANSLATE("You need to have at least 2 accounts to perform a transfer."));
				break;
			}

			TransferWindow* tnwin = new TransferWindow(this);
			BRect r(Frame());
			tnwin->MoveTo(r.left + ((Bounds().Width() - tnwin->Bounds().Width()) / 2),
				r.top + ((Bounds().Height() - tnwin->Bounds().Height()) / 2));
			tnwin->Show();
			break;
		}
		case M_EDIT_TRANSACTION:
		{
			if (!acc)
				break;

			if (!acc->CurrentTransaction()) {
				if (!acc->CountTransactions())
					break;
			}

			TransactionData data;
			gDatabase.GetTransaction(acc->CurrentTransaction(), data);
			BRect r(Frame());
			r.right = r.left + 400;
			r.bottom = r.top + 300;
			TransactionEditWindow* transwin = new TransactionEditWindow(r, data);
			transwin->MoveTo(r.left + ((Bounds().Width() - transwin->Bounds().Width()) / 2),
				r.top + ((Bounds().Height() - transwin->Bounds().Height()) / 2));
			transwin->Show();
			break;
		}
		case M_SCHEDULE_TRANSACTION:
		{
			if (!acc)
				break;

			if (!acc->CurrentTransaction()) {
				if (!acc->CountTransactions())
					break;
			}

			TransactionData data;
			gDatabase.GetTransaction(acc->CurrentTransaction(), data);

			if (data.Type().TypeCode() == TRANS_NUMERIC) {
				ShowAlert(B_TRANSLATE("Numbered transactions cannot be scheduled."),
					B_TRANSLATE("You can schedule transfers, deposits, or ATM transactions."));
				break;
			}

			BRect r(Frame());
			r.right = r.left + 400;
			r.bottom = r.top + 300;

			ScheduleAddWindow* schedwin = new ScheduleAddWindow(r, data);
			schedwin->MoveTo(r.left + ((Bounds().Width() - schedwin->Bounds().Width()) / 2),
				r.top + ((Bounds().Height() - schedwin->Bounds().Height()) / 2));
			schedwin->Show();
			break;
		}
		case M_CREATE_TRANSFER:
		{
			CreateTransfer(msg);
			break;
		}
		case M_SHOW_OPTIONS_WINDOW:
		{
			PrefWindow* pwin = new PrefWindow(Frame());
			pwin->Show();
			break;
		}
		case M_SHOW_REPORTS_WINDOW:
		{
			// While it seems kind of silly to show the reports window if
			// there is no data, it seems more like a bug to not bother showing it

			ReportWindow* rwin = new ReportWindow(Frame());
			rwin->Show();
			break;
		}
		case M_SHOW_RECONCILE_WINDOW:
		{
			if (!acc) {
				if (gDatabase.CountAccounts() < 1)
					ShowAlert(B_TRANSLATE("Oops!"),
						B_TRANSLATE(
							"You need to have an account created in order to reconcile it."));
				else
					ShowAlert(B_TRANSLATE("Oops!"),
						B_TRANSLATE("You need to select an account in order to reconcile it."));
				break;
			}

			ReconcileWindow* recwin = new ReconcileWindow(BRect(100, 100, 700, 425), acc);
			recwin->CenterIn(Frame());
			recwin->Show();
			break;
		}
		case M_SHOW_SCHEDULED_WINDOW:
		{
			BRect r(Frame());
			ScheduleListWindow* schedwin = new ScheduleListWindow(BRect(100, 100, 600, 425));
			schedwin->MoveTo(r.left + ((Bounds().Width() - schedwin->Bounds().Width()) / 2),
				r.top + ((Bounds().Height() - schedwin->Bounds().Height()) / 2));
			schedwin->Show();
			break;
		}
		case M_SHOW_BUDGET_WINDOW:
		{
			BudgetWindow* bwin = new BudgetWindow(Frame());
			bwin->Show();
			break;
		}
		case M_SHOW_CATEGORY_WINDOW:
		{
			BRect r(Frame());
			CategoryWindow* catwin = new CategoryWindow(BRect(100, 100, 600, 425));
			catwin->MoveTo(r.left + ((Bounds().Width() - catwin->Bounds().Width()) / 2),
				r.top + ((Bounds().Height() - catwin->Bounds().Height()) / 2));
			catwin->Show();
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
MainWindow::LoadData(void)
{
	if (gDatabase.OpenFile(fLastFile.String()) != B_OK) {
		BEntry entry(fLastFile.String());
		if (!entry.Exists()) {
			// TODO: Show CapitalBe introduction
			// TODO: Show a Create File dialog
			gDatabase.CreateFile(fLastFile.String());
			PostMessage(M_SHOW_NEW_ACCOUNT);
		}
	}
}

void
MainWindow::SaveData(void)
{
}

void
MainWindow::CreateTransfer(BMessage* msg)
{
	Account *from, *to;
	BString amount, memo, datestr;
	Fixed fixed;
	time_t date;

	if ((msg->FindPointer("from", (void**)&from) != B_OK) ||
		(msg->FindPointer("to", (void**)&to) != B_OK) ||
		(msg->FindString("amount", &amount) != B_OK) ||
		(gCurrentLocale.StringToCurrency(amount.String(), fixed) != B_OK) ||
		(msg->FindString("date", &datestr) != B_OK) ||
		(gDefaultLocale.StringToDate(datestr.String(), date) != B_OK))
		return;

	if (msg->FindString("memo", &memo) != B_OK)
		memo = "";

	// Now that we've gathered all the data from the message sent to us by TransferWindow,
	// we create the transactions needed for each account.
	BString payee = B_TRANSLATE("Transfer to %%PAYEE%%");
	payee.ReplaceFirst("%%PAYEE%%", to->Name());

	uint32 transid = gDatabase.NextTransactionID();
	TransactionType type("XFER");
	gDatabase.AddTransaction(from->GetID(), transid, date, type, payee.String(),
		fixed.InvertAsCopy(), "Transfer", memo.String());

	payee = B_TRANSLATE("Transfer from %%PAYEE%%");
	payee.ReplaceFirst("%%PAYEE%%", to->Name());
	payee << from->Name();
	gDatabase.AddTransaction(
		to->GetID(), transid, date, type, payee.String(), fixed, "Transfer", memo.String());
}

void
MainWindow::HandleNotify(const uint64& value, const BMessage* msg)
{
	bool lockwin = false;
	if (!IsLocked()) {
		Lock();
		lockwin = true;
	}

	if (value & WATCH_ACCOUNT) {
		Account* acc;
		if (msg->FindPointer("item", (void**)&acc) != B_OK || !acc) {
			if (lockwin)
				Unlock();
			return;
		}

		if (value & WATCH_SELECT || value & WATCH_CHANGE) {
			if (acc->IsClosed()) {
				fAccountClosedItem->SetLabel(B_TRANSLATE("Reopen"));
			} else {
				fAccountClosedItem->SetLabel(B_TRANSLATE("Close"));
			}
		}
	}

	if (lockwin)
		Unlock();
}