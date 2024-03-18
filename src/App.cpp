#include "Database.h"
#include <Application.h>

// #define DEBUG_DATABASE

#ifndef DEBUG_DATABASE

#include "App.h"
#include "BuildOptions.h"
#include "CBLocale.h"
#include "LanguageRoster.h"
#include "MainWindow.h"
#include "Preferences.h"
#include "TimeSupport.h"
#include <Roster.h>

#include "ReportWindow.h"

bool gRestartApp = false;

App::App(void) : BApplication("application/x-vnd.wgp-CapitalBe")
{
	// Load preferences and then initialize the translation system
	LoadPreferences(PREFERENCES_PATH "/CapitalBeSettings");

	BString languagepath = gAppPath.String();
	languagepath += "R5LanguageFiles";
	language_roster = new LanguageRoster(languagepath.String());

	// We can skip locking because nothing else is open at this point :)
	BRect winframe;
	if (gPreferences.FindRect("mainframe", &winframe) == B_OK)
		ConstrainWindowFrameToScreen(&winframe);
	else
		winframe.Set(100, 100, 620, 360);

	MainWindow* win = new MainWindow(winframe);
	win->Show();
}

App::~App(void)
{
	delete language_roster;
	language_roster = NULL;
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
	// Attempt to load the default data file

	BEntry entry("/boot/home/config/settings/CapitalBe");
	BFile file;

	if (!entry.Exists())
		create_directory(PREFERENCES_PATH, 0777);

	entry.SetTo(PREFERENCES_PATH "/CapitalBeSettings");
	if (!entry.Exists()) {
		file.SetTo(PREFERENCES_PATH "/CapitalBeSettings", B_READ_WRITE | B_CREATE_FILE);
		file.Unset();
	}

	App* app = new App;
	app->Run();
	delete app;

	return 0;
}

#else

// Test code

#include "CBLocale.h"
#include "Fixed.h"
#include "Import.h"
#include "Transaction.h"
#include <Entry.h>

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
#endif // end disabled test code
