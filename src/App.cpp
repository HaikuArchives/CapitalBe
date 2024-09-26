/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	humdinger (Joachim Seemer)
 *	dospuntos (Johan Wagenheim)
 */
#include <AboutWindow.h>
#include <Application.h>
#include <Catalog.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <MimeType.h>
#include <Path.h>
#include <Resources.h>

// #define DEBUG_DATABASE

#ifndef DEBUG_DATABASE

#include <Roster.h>
#include "App.h"
#include "DAlert.h"
#include "Preferences.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "App"


App::App(void)
	: BApplication(kApplicationSignature),
	  fMainWindow(NULL)
{
}


App::~App()
{
	SavePreferences();
}


void
App::AboutRequested()
{
	BAboutWindow* abwin
		= new BAboutWindow(B_TRANSLATE_SYSTEM_NAME("CapitalBe"), kApplicationSignature);
	// clang-format off
	const char* authors[] = {
		"DarkWyrm",
		"Humdinger",
		"Johan Wagenheim",
		"Raefaldhi Amartya Junior",
		"waddlesplash",
		NULL };

	const char* thanks[] = {
		"Davidmp (Catalan translation)",
		"tmtfx (Friaulian translation)",
		"Humdinger (German and Australian/British/Canadian translation)",
		"Johan Wagenheim (Norwegian translation)",
		"Tanausú Gómez, Pintaio and cafeina (Spanish translation)",
		"Emir Sarı (Turkish translation)",
		NULL };
	// clang-format on
	abwin->AddDescription(
		B_TRANSLATE("CapitalBe is a simple application to keep track of your personal finances."));
	abwin->AddCopyright(2009, "DarkWyrm");
	abwin->AddText(B_TRANSLATE("Distributed under the terms of the MIT License."));
	abwin->AddText("https://github.com/HaikuArchives/CapitalBe");
	abwin->AddAuthors(authors);
	abwin->AddSpecialThanks(thanks);
	abwin->Show();
	float width = be_plain_font->StringWidth(
		"CapitalBe is a simple application to keep track of your personal finances.");
	abwin->ResizeTo(width, width);
}


void
App::RefsReceived(BMessage* msg)
{
	entry_ref ref;
	if (msg->FindRef("refs", &ref) == B_OK) {
		BEntry entry(&ref);
		fFile.SetTo(&entry);
		// If we're launching ReadyToRun() is executed
		if (!IsLaunching())
			_ShowMainWindow(fFile.Path());
	}
}


void
App::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_QUIT_NOW:
		{
			Quit();
		} break;

		case M_FILE_NEW:
		{
			entry_ref ref;
			const char* name;
			if (msg->FindRef("directory", &ref) == B_OK && msg->FindString("name", &name) == B_OK) {
				BDirectory directory(&ref);
				BPath path(&directory, name);
				BEntry entry(path.Path());
				if (entry.Exists())
					entry.Remove();
				_ShowMainWindow(path);
			}
		} break;

		case B_SIMPLE_DATA:
		case M_FILE_OPEN:
		{
			entry_ref ref;
			if (msg->FindRef("refs", &ref) == B_OK) {
				BPath path(&ref);
				_ShowMainWindow(path.Path());
			}
		} break;

		default:
			BApplication::MessageReceived(msg);
	}
}

void
App::ReadyToRun()
{
	_InstallMimeType();

	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	path.Append("CapitalBe");
	create_directory(path.Path(), 0755);
	gSettingsPath = path;

	LoadPreferences();

	if (fFile.InitCheck() == B_OK) {
		_ShowMainWindow(fFile);
		return;
	}

	BString alertText;
	BString lastFile;

	if (gPreferences.FindString("lastfile", &lastFile) == B_OK) {
		BEntry entry(lastFile.String());
		if (entry.Exists()) {
			BPath path(&entry);
			_ShowMainWindow(path);
		} else {
			alertText
				= B_TRANSLATE("CapitalBe couldn't find the last open ledger at '%filename%'.\n");
			alertText.ReplaceFirst("%filename%", lastFile.String());
			_ShowAlert(alertText);
		}
	} else {  // "lastfile" wasn't set in previous CapitalBe versions
		lastFile << path.Path() << "/MyAccountData";  // this file was used back then
		BEntry entry(lastFile.String());
		if (entry.Exists()) {
			BPath path(&entry);
			_ShowMainWindow(path);
		} else {  // looks like it's our first launch
			alertText = B_TRANSLATE(
				"Welcome to CapitalBe!\n\n"
				"There appears to be no 'ledger' where all your accounts and transactions "
				"are saved. You can open an existing ledger, or create a new ledger.\n");
			_ShowAlert(alertText);
		}
	}
}


void
App::_InstallMimeType()
{
	// install mime type of documents
	BMimeType mime(kLedgerMimeType);
	if (mime.InitCheck() < B_OK)
		return;

	mime.Delete();

	status_t ret = mime.Install();
	if (ret < B_OK) {
		fprintf(stderr, "Could not install mime type: %s.\n", strerror(ret));
		return;
	}

	// set preferred app
	if (mime.SetPreferredApp(kApplicationSignature) < B_OK)
		fprintf(stderr, "Could not set preferred app!\n");

	// set descriptions
	if (mime.SetShortDescription(B_TRANSLATE_COMMENT(
			"CapitalBe ledger", "MIME type short description"))
		< B_OK)
		fprintf(stderr, "Could not set short description of mime type!\n");
	if (mime.SetLongDescription(B_TRANSLATE_COMMENT(
			"CapitalBe ledger containing accounts, transactions etc.",
			"MIME type long description"))
		!= B_OK)
		fprintf(stderr, "Could not set long description of mime type!\n");

	// set document icons
	BResources* resources = AppResources();
	// does not need to be freed (belongs to BApplication base)
	if (resources == NULL) {
		fprintf(stderr, "Could not find app resources.\n");
		return;
	}

	size_t size;
	const void* iconData;

	iconData = resources->LoadResource('VICN', kLedgerMimeType, &size);
	if (iconData && mime.SetIcon((uint8*)iconData, size) < B_OK)
		fprintf(stderr, "Could not set vector icon of mime type.\n");
}


bool
App::_IsFileLocked(BPath path)
{
	BNode node(path.Path());
	if (node.InitCheck() != B_OK)
		return false;

	bool state = false;
	node.ReadAttr("filelock", B_BOOL_TYPE, 0, &state, sizeof(bool));

	return state;
}


void
App::_ShowAlert(BString text)
{
	DAlert* alert = new DAlert(B_TRANSLATE_SYSTEM_NAME("CapitalBe"), text, B_TRANSLATE("Cancel"),
		B_TRANSLATE("Open ledger"), B_TRANSLATE("Create new ledger"), B_WIDTH_AS_USUAL,
		B_OFFSET_SPACING, B_INFO_ALERT);
	alert->SetShortcut(0, B_ESCAPE);

	switch (alert->Go()) {
		case 0:
		{
			PostMessage(B_QUIT_REQUESTED);
		} break;

		case 1:
		{
			BFilePanel* openLedger = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL,
				B_FILE_NODE, false, new BMessage(M_FILE_OPEN));
			openLedger->Window()->SetTitle(B_TRANSLATE("CapitalBe: Open ledger"));
			openLedger->Show();
		} break;

		case 2:
		{
			BFilePanel* newLedger = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), NULL,
				B_FILE_NODE, false, new BMessage(M_FILE_NEW));
			newLedger->Window()->SetTitle(B_TRANSLATE("CapitalBe: Create new ledger"));
			newLedger->Show();
		} break;
	}
}


void
App::_ShowMainWindow(BPath path)
{
	if (fMainWindow != NULL) {
		BMessenger messengerMain(fMainWindow);
		if (messengerMain.IsValid() && messengerMain.LockTarget())
			fMainWindow->Quit();
		fMainWindow = NULL;
	}

	if (_IsFileLocked(path)) {
		BString text(B_TRANSLATE("The ledger '%filename%' appears to be already open.\n"
			"Entering data into the same ledger in parallel will probably lead to data loss.\n"));
		text.ReplaceFirst("%filename%", path.Leaf());
		DAlert* alert = new DAlert(B_TRANSLATE_SYSTEM_NAME("CapitalBe"), text,
			B_TRANSLATE("Cancel"), B_TRANSLATE("Open ledger"), NULL,
			B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);

		if (alert->Go() == 0) {
			PostMessage(B_QUIT_REQUESTED);
			return;
		}
	}

	BRect winFrame;
	if (gPreferences.FindRect("mainframe", &winFrame) != B_OK)
		winFrame.Set(100, 100, 720, 660);

	fMainWindow = new MainWindow(winFrame, path);
	fMainWindow->MoveOnScreen();
	fMainWindow->Show();
}


int
main()
{
	App* app = new App;
	app->Run();
	delete app;

	return 0;
}

#else

// Test code

#include <Entry.h>
#include "CBLocale.h"
#include "Fixed.h"
#include "Import.h"
#include "Transaction.h"


void
StartFile()
{
	BEntry entry("/boot/develop/projects/CapitalBe/cbsql/testdb");
	if (entry.Exists())
		entry.Remove();
	gDatabase.CreateFile("/boot/develop/projects/CapitalBe/cbsql/testdb");

	/*	gDatabase.AddAccount("Test Account 1",ACCOUNT_BANK);
		gDatabase.AddAccount("Test Account 2",ACCOUNT_BANK);

		Locale ld;
		ld.SetCurrencySymbol("£");
		ld.SetCurrencySymbolPrefix(false);
		gDatabase.SetAccountLocale(1,ld);

		gDatabase.AddBudgetEntry("Misc",Fixed(0L));
		gDatabase.AddBudgetEntry("Auto_Fuel",Fixed(75));
		gDatabase.AddBudgetEntry("Misc",Fixed(150));
		gDatabase.AddBudgetEntry("RemoveMe",Fixed(15.50));
		gDatabase.RemoveBudgetEntry("RemoveMe");

		gDatabase.RemoveAccount(1);
		gDatabase.AddAccount("Test Account 2",ACCOUNT_BANK);

		uint32 id = gDatabase.NextTransactionID();
		gDatabase.AddTransaction(0,id,real_time_clock(),TransactionType("DEP"),"Paychecks",
							Fixed(1000),"Salary",NULL);

		id = gDatabase.NextTransactionID();
		Fixed f;
		f.SetPremultiplied(5097);
		gDatabase.AddTransaction(0,id,real_time_clock(),TransactionType("ATM"),"Stuff-Mart",
							f,"Misc",NULL);
		f.SetPremultiplied(1234);
		gDatabase.AddTransaction(0,id,real_time_clock(),TransactionType("ATM"),"Stuff-Mart",
							f,"Clothing",NULL);
		f.SetPremultiplied(4321);
		gDatabase.AddTransaction(0,id,real_time_clock(),TransactionType("ATM"),"Stuff-Mart",
							f,"Computer",NULL);

		id = gDatabase.NextTransactionID();
		f.SetPremultiplied(1934);
		gDatabase.AddTransaction(0,id,real_time_clock(),TransactionType("ATM"),"Gas",
							f,"Auto_Fuel",NULL);
		gDatabase.RemoveTransaction(1);


		entry.SetTo("/darkstar/Quicken Data/QIF/import.qif");
		entry_ref ref;
		entry.GetRef(&ref);
		ImportQIF(ref);
	*/
}


int
main()
{
	//	gDatabase.OpenFile("/boot/develop/projects/CapitalBe/cbsql/testdb");

	StartFile();
}
#endif	// end disabled test code
