#include <AboutWindow.h>
#include <Application.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <Path.h>

// #define DEBUG_DATABASE

#ifndef DEBUG_DATABASE

#include <Roster.h>
#include "App.h"
#include "MainWindow.h"
#include "Preferences.h"

bool gRestartApp = false;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "App"


App::App(void)
	: BApplication(kApplicationSignature)
{
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	path.Append("CapitalBe");
	create_directory(path.Path(), 0755);
	gSettingsPath = path;

	LoadPreferences();

	// We can skip locking because nothing else is open at this point :)
	BRect winframe;
	if (gPreferences.FindRect("mainframe", &winframe) != B_OK)
		winframe.Set(100, 100, 720, 660);

	MainWindow* win = new MainWindow(winframe);
	// Make sure the window is visible on screen
	win->MoveOnScreen();
	win->Show();
}

App::~App(void)
{
	SavePreferences();
}

void
App::AboutRequested(void)
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
App::MessageReceived(BMessage* msg)
{
	if (msg->what == M_QUIT_NOW)
		Quit();
	else
		BApplication::MessageReceived(msg);
}

int
main(void)
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
StartFile(void)
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
main(void)
{
	//	gDatabase.OpenFile("/boot/develop/projects/CapitalBe/cbsql/testdb");

	StartFile();
}
#endif	// end disabled test code
