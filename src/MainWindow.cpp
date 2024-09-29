/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	bitigchi (Emir Sari)
 *	dospuntos (Johan Wagenheim)
 *	humdinger (Joachim Seemer)
 *	n0toose (Panagiotis Vasilopoulos)
 *	raefaldhia (Raefaldhi Amartya Junior)
 */
#include "MainWindow.h"

#include <Catalog.h>
#include <ControlLook.h>
#include <Directory.h>
#include <Entry.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <Messenger.h>
#include <Path.h>
#include <RecentItems.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <StringFormat.h>
#include <Url.h>

#include <stdlib.h>

#include "AccountSettingsWindow.h"
#include "BudgetWindow.h"
#include "CBLocale.h"
#include "CategoryWindow.h"
#include "FilterView.h"
#include "Help.h"
#include "IconMenuItem.h"
#include "PrefWindow.h"
#include "Preferences.h"
#include "ReconcileWindow.h"
#include "RegisterView.h"
#include "ReportWindow.h"
#include "ScheduleAddWindow.h"
#include "ScheduleListWindow.h"
#include "ScheduledExecutor.h"
#include "TransactionEditWindow.h"
#include "TransferWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


// #define TEST
// #define NOSAVE

// Internal definition of the Haiku services daemon
#define B_SERVICES_DAEMON_RESTART 'SDRS'


bool
LedgerFileFilter::Filter(
	const entry_ref* ref, BNode* node, struct stat_beos* stat, const char* fileType)
{
	BEntry entry(ref, true);  // traverse links
	// allow folders and links of folders
	if (entry.IsDirectory())
		return true;

	if (IsValid(ref, &entry))
		return true;

	return false;
}


bool
LedgerFileFilter::IsValid(const entry_ref* ref, const BEntry* entry)
{
	// allow files with ledger MIME type
	char mimeType[B_MIME_TYPE_LENGTH];
	BNode traversedNode(entry);	 // create a new node from the link-traversed BEntry
	BNodeInfo(&traversedNode).GetType(mimeType);

	if (strncmp(kLedgerMimeType, mimeType, sizeof(kLedgerMimeType)) == 0)
		return true;

	// allow the fixed filename of old CapitalBe versions, just in case...
	char name[B_FILE_NAME_LENGTH];
	entry->GetName(name);
	if (strncmp("MyAccountData", name, 13) == 0)
		return true;

	return false;
}


MainWindow::MainWindow(BRect frame, BPath lastFile)
	: BWindow(frame, NULL, B_DOCUMENT_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS),
	  fLastFile(lastFile.Path()),
	  fFrameBeforeZoom(frame),
	  fIsZoomed(false)
{
	BString title = B_TRANSLATE_SYSTEM_NAME("CapitalBe");
	title << ": " << lastFile.Leaf();
	SetTitle(title);

	if (gPreferences.FindColor("negativecolor", &gNegativeColor) != B_OK)
		gNegativeColor = ui_color(B_FAILURE_COLOR);

	int32 selectAcc = 0;
	_GetFileSettings(&fFrameBeforeZoom, &selectAcc);

	AddShortcut(B_HOME, B_COMMAND_KEY, new BMessage(M_FIRST_TRANSACTION));
	AddShortcut(B_END, B_COMMAND_KEY, new BMessage(M_LAST_TRANSACTION));

	SetSizeLimits(520, 30000, 290, 30000);
	if (frame.Width() < 520)
		ResizeBy(520 - frame.Width(), 0);
	if (frame.Height() < 260)
		ResizeBy(0, 260 - frame.Height());

	BMenuBar* bar = new BMenuBar("keybar");

	BMenu* menu = new BMenu("");
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Categories" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_CATEGORY_WINDOW)));
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Settings" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_OPTIONS_WINDOW), ','));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Go to filter"), new BMessage(M_FOCUS_FILTER), 'F'));
	BMenuItem* clearFilter
		= new BMenuItem(B_TRANSLATE("Clear filter"), new BMessage(M_CLEAR_FILTER), 'L');
	menu->AddItem(clearFilter);
	menu->AddSeparatorItem();
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Report a bug" B_UTF8_ELLIPSIS), new BMessage(M_REPORT_BUG)));
	BMenuItem* item
		= new BMenuItem(B_TRANSLATE("About CapitalBe"), new BMessage(B_ABOUT_REQUESTED));
	item->SetTarget(be_app);
	menu->AddItem(item);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED), 'Q'));

	IconMenuItem* iconMenu;
	iconMenu = new IconMenuItem(menu, NULL, kApplicationSignature, B_MINI_ICON);
	bar->AddItem(iconMenu);

	menu = new BMenu(B_TRANSLATE("File"));

	menu->AddItem(
		new BMenuItem(B_TRANSLATE("New ledger" B_UTF8_ELLIPSIS), new BMessage(M_FILE_NEW)));
	BMenuItem* openItem
		= new BMenuItem(BRecentFilesList::NewFileListMenu(B_TRANSLATE("Open" B_UTF8_ELLIPSIS), NULL,
							NULL, be_app, 9, true, NULL, kApplicationSignature),
			new BMessage(M_FILE_OPEN));
	openItem->SetShortcut('O', 0);
	menu->AddItem(openItem);
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
		new BMenuItem(B_TRANSLATE("New" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_NEW_ACCOUNT)));
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
	menu->AddItem(new BMenuItem(B_TRANSLATE("Use as filter"), new BMessage(M_USE_FOR_FILTER), 'U'));
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Use as new transaction"), new BMessage(M_USE_TRANSACTION), 'N'));
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Enter a transfer" B_UTF8_ELLIPSIS), new BMessage(M_ENTER_TRANSFER), 'T'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Delete"), new BMessage(M_DELETE_TRANSACTION)));
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
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Scheduled transactions"), new BMessage(M_SHOW_SCHEDULED_WINDOW)));

	// Help icon menu
	BMenuBar* helpBar = new BMenuBar("helpbar");
	helpBar->AddItem(new IconMenuItem("", new BMessage(M_HELP), getHelpIcon(), B_MINI_ICON));

	// We load the financial data before we create any of the views because the
	// notifications are not sent and startup time is *significantly* reduced
	_LoadData();

	fRegisterView = new RegisterView("registerview", selectAcc);

	// File panels
	BString temp = B_TRANSLATE_SYSTEM_NAME("CapitalBe");
	temp << ": ";

	fNewPanel = new BFilePanel(
		B_SAVE_PANEL, new BMessenger(be_app), NULL, B_FILE_NODE, false, new BMessage(M_FILE_NEW));
	BString label = temp;
	fNewPanel->Window()->SetTitle(label << B_TRANSLATE("New ledger"));

	fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(be_app), NULL, B_FILE_NODE, false,
		new BMessage(M_FILE_OPEN), new LedgerFileFilter());
	label = temp;
	fOpenPanel->Window()->SetTitle(label << B_TRANSLATE("Open ledger"));

	fImportPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false,
		new BMessage(M_IMPORT_ACCOUNT));
	label = temp;
	fImportPanel->Window()->SetTitle(label << B_TRANSLATE("Import"));

	fExportPanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false,
		new BMessage(M_EXPORT_ACCOUNT));
	label = temp;
	fExportPanel->Window()->SetTitle(label << B_TRANSLATE("Export"));

	gDatabase.AddObserver(this);

	// clang-format off
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(0)
		.AddGroup(B_HORIZONTAL, 0.0f)
			.Add(bar, 1.0f)
			.Add(helpBar, 0.0f)
		.End()
		.Add(fRegisterView)
		.End();
	// clang-format on

	clearFilter->SetTarget(FindView("registerview")->FindView("filterview"));

	MoveTo(fFrameBeforeZoom.LeftTop());
	ResizeTo(fFrameBeforeZoom.Width(), fFrameBeforeZoom.Height());

	// If file was zoomed when closing last, zoom it again at launch
	if (fIsZoomed) {
		fIsZoomed = false;
		BWindow::Zoom();
	}

	HandleScheduledTransactions();
	BMessage message(M_RUN_SCHEDULED_TRANSACTIONS);
	fRunner = new BMessageRunner(this, &message, 30 * 1000 * 1000);	 // Every 30 seconds
}


MainWindow::~MainWindow()
{
	_SetFileSettings();
	_SetMime();
	_SetFileLock(false);

	delete fNewPanel;
	delete fOpenPanel;
	delete fImportPanel;
	delete fExportPanel;
}


bool
MainWindow::QuitRequested()
{
	prefsLock.Lock();
	gPreferences.RemoveData("mainframe");
	gPreferences.AddRect("mainframe", Frame());
	gPreferences.RemoveData("lastfile");
	gPreferences.AddString("lastfile", fLastFile);
	prefsLock.Unlock();

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
MainWindow::MessageReceived(BMessage* msg)
{
	Account* acc = gDatabase.CurrentAccount();

	switch (msg->what) {
		case M_FOCUS_FILTER:
		{
			BView* target = FindView("registerview")->FindView("filterview")->FindView("payee");
			target->MakeFocus(true);
			break;
		}
		case M_HELP:
		{
			openDocumentation("start.html", NULL);
			break;
		}
		case M_REPORT_BUG:
		{
			BUrl("https://github.com/HaikuArchives/CapitalBe/issues/")
				.OpenWithPreferredApplication();
			break;
		}
		case M_SHOW_NEW_ACCOUNT:
		{
			AccountSettingsWindow* newaccwin = new AccountSettingsWindow(NULL);
			newaccwin->CenterIn(Frame());
			newaccwin->Show();
			break;
		}
		case M_SHOW_ACCOUNT_SETTINGS:
		{
			if (!acc)
				break;

			if (acc->IsClosed()) {
				ShowAlert(B_TRANSLATE("Closed account"),
					B_TRANSLATE("This account is closed, and the settings cannot be edited. Please "
								"reopen the account if you need to make changes."),
					B_WARNING_ALERT);
				break;
			}

			AccountSettingsWindow* accwin = new AccountSettingsWindow(acc);
			accwin->CenterIn(Frame());
			accwin->Show();
			break;
		}
		case M_DELETE_ACCOUNT:
		{
			if (!acc)
				break;

			uint32 scheduledTransactions = gDatabase.CountScheduledTransactions(acc->GetID());
			BString msg;
			if (scheduledTransactions > 0) {
				static BStringFormat format(B_TRANSLATE(
					"{0, plural,"
					"one{This account has # scheduled transaction that will be removed.}"
					"other{This account has # scheduled transactions that will be removed.}}"));
				format.Format(msg, scheduledTransactions);
				msg << "\n\n";
			}

			msg << B_TRANSLATE(
				"Once deleted, you will not be able to get back any data on this account.");

			BAlert* alert = new BAlert(B_TRANSLATE("Really delete account?"), msg.String(),
				B_TRANSLATE("Delete"), B_TRANSLATE("Cancel"), NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);

			if (alert->Go() == 0) {
				int32 index = gDatabase.IndexOf(acc);

				if (gDatabase.CountAccounts() < 2) {  // deleting the last account
					gDatabase.SetCurrentAccount(-1);
					gDatabase.CurrentAccount()->SetCurrentTransaction(-1);
				} else if (index == 0)
					gDatabase.SetCurrentAccount(1);
				else if (index > 0)
					gDatabase.SetCurrentAccount(index - 1);

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
		case B_SIMPLE_DATA:
		{
			be_app->PostMessage(msg);
			break;
		}
		case M_FILE_NEW:
		{
			fNewPanel->Show();
			break;
		}
		case M_FILE_OPEN:
		{
			fOpenPanel->Show();
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
				BString errmsg(B_TRANSLATE("Could not import the data in the file '%%FILENAME%%'"));
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
					B_TRANSLATE("Could not export your financial data to the file '%%FILENAME%%'"));
				errmsg.ReplaceFirst("%%FILENAME%%", dir.name);
				ShowAlert(
					errmsg.String(), B_TRANSLATE("This really shouldn't happen, so you probably "
												 "should report a bug for this."));
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
				ShowAlert(B_TRANSLATE("Not enough accounts for a transfer"),
					B_TRANSLATE("You need to have at least 2 accounts to perform a transfer."));
				break;
			}

			TransferWindow* tnwin = new TransferWindow(this);
			tnwin->CenterIn(Frame());
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

			if (acc->IsClosed()) {
				ShowAlert(B_TRANSLATE("Closed account"),
					B_TRANSLATE("This transaction belongs to a closed account and cannot be "
								"edited. Please reopen the account if you need to make changes."),
					B_WARNING_ALERT);
				break;
			}

			TransactionData data;
			gDatabase.GetTransaction(acc->CurrentTransaction(), acc->GetID(), data);
			TransactionEditWindow* transwin = new TransactionEditWindow(data);
			transwin->CenterIn(Frame());
			transwin->Show();
			break;
		}
		case M_USE_TRANSACTION:
		{
			if (!acc)
				break;

			if (!acc->CurrentTransaction()) {
				if (!acc->CountTransactions())
					break;
			}

			TransactionData data;
			gDatabase.GetTransaction(acc->CurrentTransaction(), acc->GetID(), data);
			fRegisterView->SetCheckFields(data);
			break;
		}
		case M_USE_FOR_FILTER:
		{
			BMessage message(M_SET_FILTER);
			BView* target = fRegisterView->FindView("filterview");
			target->MessageReceived(&message);
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

			if (acc->IsClosed()) {
				ShowAlert(B_TRANSLATE("Closed account"),
					B_TRANSLATE("You cannot schedule transactions on a closed account."),
					B_WARNING_ALERT);
				break;
			}

			TransactionData data;
			gDatabase.GetTransaction(acc->CurrentTransaction(), acc->GetID(), data);

			if (data.Type().TypeCode() == TRANS_XFER) {
				int32 destAccount = -1;
				destAccount = gDatabase.GetTransferDestination(data.GetID(), acc->GetID());
				if (destAccount < 0) {
					ShowAlert(B_TRANSLATE("Scheduling error"),
						B_TRANSLATE(
							"Transfers generated by scheduling cannot be scheduled themselves."));
					break;
				}

				// Scheduling from destination, get the corresponding transaction before scheduling
				if (data.Amount().IsPositive())
					gDatabase.GetTransaction(acc->CurrentTransaction(), destAccount, data);
			}

			BRect r(Frame());
			r.right = r.left + 400;
			r.bottom = r.top + 300;

			ScheduleAddWindow* schedwin = new ScheduleAddWindow(r, data);
			schedwin->CenterIn(Frame());
			schedwin->Show();
			break;
		}
		case M_RUN_SCHEDULED_TRANSACTIONS:
		{
			HandleScheduledTransactions();
			break;
		}
		case M_CREATE_TRANSFER:
		{
			_CreateTransfer(msg);
			break;
		}
		case M_SHOW_OPTIONS_WINDOW:
		{
			PrefWindow* pwin = new PrefWindow(Frame(), this);
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
				if (gDatabase.CountAccounts() < 1) {
					ShowAlert(B_TRANSLATE("Oops!"),
						B_TRANSLATE(
							"You need to have an account created in order to reconcile it."));
				} else {
					ShowAlert(B_TRANSLATE("Oops!"),
						B_TRANSLATE("You need to select an account in order to reconcile it."));
				}
				break;
			}

			ReconcileWindow* recwin = new ReconcileWindow(BRect(0, 0, 700, 550), acc);
			recwin->CenterIn(Frame());
			recwin->Show();
			break;
		}
		case M_SHOW_SCHEDULED_WINDOW:
		{
			ScheduleListWindow* schedwin = new ScheduleListWindow(BRect(100, 100, 600, 425));
			schedwin->CenterIn(Frame());
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
			CategoryWindow* catwin = new CategoryWindow(BRect(100, 100, 600, 425));
			catwin->CenterIn(Frame());
			catwin->Show();
			break;
		}
		case M_NEG_COLOR_CHANGED:
		{
			BView* view = fRegisterView->FindView("transactionview");
			if (view != NULL) {
				view = view->FindView("RegisterList");
				if (view != NULL)
					view->Invalidate();
			}
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
MainWindow::Zoom(BPoint /*origin*/, float /*width*/, float /*height*/)
{
	BRect frame = Frame();
	if (!fIsZoomed) {
		fIsZoomed = true;
		fFrameBeforeZoom = frame;
		BScreen screen(this);
		BRect screenFrame(screen.Frame());

		MoveTo(BPoint(fFrameBeforeZoom.left, 0));
		ResizeTo(fFrameBeforeZoom.Width(), screenFrame.Height());
		MoveOnScreen(B_MOVE_IF_PARTIALLY_OFFSCREEN);
	} else {
		fIsZoomed = false;
		MoveTo(BPoint(frame.left, fFrameBeforeZoom.top));
		ResizeTo(frame.Width(), fFrameBeforeZoom.Height());
	}
}


void
MainWindow::_GetFileSettings(BRect* winFrame, int32* selectAcc)
{
	BNode node(fLastFile);
	if (node.InitCheck() != B_OK)
		return;

	node.ReadAttr("window_zoom", B_BOOL_TYPE, 0, &fIsZoomed, sizeof(bool));

	ssize_t bytesRead;
	BRect frame;
	bytesRead = node.ReadAttr("window_frame", B_RECT_TYPE, 0, &frame, sizeof(BRect));
	if (bytesRead == sizeof(BRect))
		*winFrame = frame;

	int32 select;
	bytesRead = node.ReadAttr("select_account", B_INT32_TYPE, 0, &select, sizeof(int32));
	if (bytesRead == sizeof(int32))
		*selectAcc = select;
}

void
MainWindow::_SetFileSettings()
{
	BNode node(fLastFile);
	if (node.InitCheck() != B_OK)
		return;

	int32 select = 0;
	BListView* accList = (BListView*)fRegisterView->FindView("AccountList");
	if (accList != NULL)
		select = accList->CurrentSelection();

	BRect frame;
	if (fIsZoomed)
		frame = fFrameBeforeZoom;
	else
		frame = Frame();

	node.WriteAttr("window_zoom", B_BOOL_TYPE, 0, &fIsZoomed, sizeof(bool));
	node.WriteAttr("window_frame", B_RECT_TYPE, 0, &frame, sizeof(BRect));
	node.WriteAttr("select_account", B_INT32_TYPE, 0, &select, sizeof(int32));
}


void
MainWindow::_LoadData()
{
	if (gDatabase.OpenFile(fLastFile.String()) != B_OK) {
		BEntry entry(fLastFile.String());
		if (!entry.Exists()) {
			gDatabase.CreateFile(fLastFile.String());
			PostMessage(M_SHOW_NEW_ACCOUNT);
		}
	}
	_SetFileLock(true);
}


void
MainWindow::_SetFileLock(bool state)
{
	BNode node(fLastFile);
	if (node.InitCheck() != B_OK)
		return;

	node.WriteAttr("filelock", B_BOOL_TYPE, 0, &state, sizeof(bool));
}


void
MainWindow::_SetMime()
{
	// Just set the MIME type
	BNode node(fLastFile.String());
	if (node.InitCheck() == B_OK) {
		BNodeInfo nodeInfo(&node);
		nodeInfo.SetType(kLedgerMimeType);
	}
}


void
MainWindow::_CreateTransfer(BMessage* msg)
{
	Account *from, *to;
	BString amount, memo, datestr;
	Fixed fixed;
	time_t date;

	if ((msg->FindPointer("from", (void**)&from) != B_OK)
		|| (msg->FindPointer("to", (void**)&to) != B_OK)
		|| (msg->FindString("amount", &amount) != B_OK)
		|| (gCurrentLocale.StringToCurrency(amount.String(), fixed) != B_OK)
		|| (msg->FindString("date", &datestr) != B_OK)
		|| (gDefaultLocale.StringToDate(datestr.String(), date) != B_OK))
		return;

	if (msg->FindString("memo", &memo) != B_OK)
		memo = "";

	// Now that we've gathered all the data from the message sent to us by TransferWindow,
	// we create the transactions needed for each account.
	BString payee = B_TRANSLATE("Transfer to '%%PAYEE%%'");
	payee.ReplaceFirst("%%PAYEE%%", to->Name());

	uint32 transid = gDatabase.NextTransactionID();
	TransactionType type("XFER");
	gDatabase.AddTransaction(from->GetID(), transid, date, type, payee.String(),
		fixed.InvertAsCopy(), B_TRANSLATE_CONTEXT("Transfer", "CommonTerms"), memo.String());

	payee = B_TRANSLATE("Transfer from '%%PAYEE%%'");
	payee.ReplaceFirst("%%PAYEE%%", from->Name());
	gDatabase.AddTransaction(to->GetID(), transid, date, type, payee.String(), fixed,
		B_TRANSLATE_CONTEXT("Transfer", "CommonTerms"), memo.String());
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
			BMessage message(M_START_FILTER);
			BView* target = fRegisterView->FindView("filterview");
			target->MessageReceived(&message);
			if (acc->IsClosed())
				fAccountClosedItem->SetLabel(B_TRANSLATE("Reopen"));
			else
				fAccountClosedItem->SetLabel(B_TRANSLATE("Close"));
		}
	}

	if (lockwin)
		Unlock();
}
