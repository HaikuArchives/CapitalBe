#include <Application.h>
#include "Database.h"

//#define DEBUG_DATABASE

#ifndef DEBUG_DATABASE

#include <Roster.h>
#include "App.h"
#include "MainWindow.h"
#include "CBLocale.h"
#include "BuildOptions.h"
#include "Preferences.h"
#include "TimeSupport.h"
#include "LanguageRoster.h"

#include "ReportWindow.h"

bool gRestartApp=false;

App::App(void)
	:	BApplication("application/x-vnd.wgp-CapitalBe")
{
	#ifdef BETA_MODE
	
	// To create the UNIX number, run the Terminal command
	// date --date="25 Dec 2006" +%s
	// and change the date accordingly
	if(GetCurrentDate() > 1167022800)
	{
		ShowAlert("This beta version has expired.",
			"This is only a test version of Capital Be and is intended to work only "
			"for a short period of time to allow the community to help find bugs and "
			"make Capital Be the best financial manager possible.\n\nYou can download "
			"a new copy of Capital Be from http://www.capitalbe.com/");
		be_app->PostMessage(B_QUIT_REQUESTED);
		return;
	}
	
	#endif
	
	// Load preferences and then initialize the translation system
	LoadPreferences(PREFERENCES_PATH "/CapitalBeSettings");
	
	BString languagepath = gAppPath.String();
	languagepath += "R5LanguageFiles";
	language_roster = new LanguageRoster(languagepath.String());
	
	
	// We can skip locking because nothing else is open at this point :)
	BRect winframe;
	if(gPreferences.FindRect("mainframe",&winframe)==B_OK)
		ConstrainWindowFrameToScreen(&winframe);
	else
		winframe.Set(100,100,620,360);
	
	MainWindow *win = new MainWindow(winframe);
	win->Show();
	
	#ifdef PREVIEW_MODE
	
	ShowAlert("Welcome to Capital Be's Technology Preview!",
		"Welcome and thank you for trying Capital Be, what will be the foremost "
		"personal finance manager for BeOS, Zeta, and Haiku.\n\n"
		"This preview version may contain bugs and is not feature complete, but "
		"will give you an idea of what the full version will be like when released.\n\n"
		"Please feel free to experiment and send any feedback to capitalbe@earthlink.net",
		B_IDEA_ALERT);
	
	#endif
}

App::~App(void)
{
	delete language_roster;
	language_roster = NULL;
	
}

void App::MessageReceived(BMessage *msg)
{
	if(msg->what==M_QUIT_NOW)
		Quit();
	else
		BApplication::MessageReceived(msg);
}

int main(void)
{
	// Attempt to load the default data file
	
	BEntry entry("/boot/home/config/settings/CapitalBe");
	BFile file;
	
	if(!entry.Exists())
		create_directory(PREFERENCES_PATH,0777);

	entry.SetTo(PREFERENCES_PATH "/CapitalBeSettings");
	if(!entry.Exists())
	{
		file.SetTo(PREFERENCES_PATH "/CapitalBeSettings",B_READ_WRITE|B_CREATE_FILE);
		file.Unset();
	}
	
	App *app = new App;
	app->Run();
	delete app;
		
	return 0;
}

#else

// Test code

#include <Entry.h>
#include "Fixed.h"
#include "Transaction.h"
#include "CBLocale.h"
#include "Import.h"

void StartFile(void)
{

	BEntry entry("/boot/develop/projects/Capital Be/cbsql/testdb");
	if(entry.Exists())
		entry.Remove();
	gDatabase.CreateFile("/boot/develop/projects/Capital Be/cbsql/testdb");

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

int main(void)
{
//	gDatabase.OpenFile("/boot/develop/projects/Capital Be/cbsql/testdb");
	
	StartFile();
}
#endif // end disabled test code
