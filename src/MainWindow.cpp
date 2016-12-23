#include "MainWindow.h"
#include "RegisterView.h"
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <Messenger.h>
#include <Entry.h>
#include <Directory.h>
#include <Path.h>
#include <Roster.h>
#include <NetPositive.h>

#include "AboutWindow.h"
#include "AccountSettingsWindow.h"
#include "BudgetWindow.h"
#include "BuildOptions.h"
#include "CategoryWindow.h"
#include "DAlert.h"
#include "Layout.h"
#include "CBLocale.h"
#include "LanguageRoster.h"
#include "Preferences.h"
#include "PrefWindow.h"
#include "ReconcileWindow.h"
#include "ReportWindow.h"
#include "ScheduleAddWindow.h"
#include "ScheduleListWindow.h"
#include "ScheduledExecutor.h"
#include "StringView.h"
#include "TextControl.h"
#include "TransactionEditWindow.h"
#include "TransferWindow.h"
#include "Translate.h"

//#define TEST
//#define NOSAVE

// Internal definition of the Haiku services daemon
#define B_SERVICES_DAEMON_RESTART 'SDRS'

Language *gCurrentLanguage = NULL;

int32 gTextViewHeight = 20;
int32 gStringViewHeight = 20;

MainWindow::MainWindow(BRect frame)
 : BWindow(frame,"",B_DOCUMENT_WINDOW, 0)
{
	BString temp;
	// Main window title changes in demo mode
	if(BUILD_MODE == PREVIEW_MODE)
		SetTitle(TRANSLATE("Capital Be Preview Edition"));
	else
	if(BUILD_MODE == BETA_MODE)
		SetTitle("Capital Be: Beta");
	else
		SetTitle("Capital Be");
	
	// These chunks of code will save a lot of headache later on -- 
	// we cache the preferred size of BTextControls to make control layout *much* easier.
	// If it gets much more complicated than these couple of chunks, then it should
	// be moved into its own init function that is called from here.
	float pw, ph;

	BTextControl *temptc = new BTextControl(BRect(0,0,1,1),"temptc","","",NULL);
	AddChild(temptc);
	temptc->GetPreferredSize(&pw,&ph);
	gTextViewHeight = (int32)ph;
	RemoveChild(temptc);
	delete temptc;
	
	BStringView *tempsv = new BStringView(BRect(0,0,1,1),"tempsv","Foo");
	AddChild(tempsv);
	tempsv->GetPreferredSize(&pw,&ph);
	gStringViewHeight = (int32)ph;
	RemoveChild(tempsv);
	delete tempsv;
	
	ReadLanguageSettings();
	if(fLanguage.CountChars()>0)
	{
		language_roster->SetLanguage(fLanguage.String());
		gCurrentLanguage = language_roster->GetLanguage();
	}
	if(!gCurrentLanguage)
		gCurrentLanguage = language_roster->GetLanguage();
	
	fLoadError = false;
	InitSettings();
	
	AddShortcut(B_HOME,B_COMMAND_KEY, new BMessage(M_FIRST_TRANSACTION));
	AddShortcut(B_END,B_COMMAND_KEY, new BMessage(M_LAST_TRANSACTION));
	
	SetSizeLimits(520,30000,260,30000);
	if(frame.Width() < 520)
		ResizeBy(520 - frame.Width(),0);
	if(frame.Height() < 260)
		ResizeBy(0,260 - frame.Height());
	
	BRect r(Bounds());
	r.bottom=20;
	BMenuBar *bar = new BMenuBar("keybar");
	
	#ifdef BETA_MODE
	temp = TRANSLATE("Report a Bug"); temp += "…";
	bar->AddItem(new BMenuItem(temp.String(),
				new BMessage(M_REPORT_BUG)));
	#endif

	BMenu *menu = new BMenu(TRANSLATE("Program"));
	#ifdef DEMO_MODE
	menu->AddItem(new BMenuItem(TRANSLATE("Buy Capital Be"),
					new BMessage(M_PURCHASE_FULL_VERSION)));
	menu->AddSeparatorItem();
	#endif
	
	temp = TRANSLATE("Options"); temp += "…";
	menu->AddItem(new BMenuItem(temp.String(),
				new BMessage(M_SHOW_OPTIONS_WINDOW),','));
	
	// Set up language support. Note that we only show the menu at all if there is
	// more than one language available

	if(language_roster->CountLanguages()>1)
	{
		menu->AddSeparatorItem();
		fLanguageMenu=new BMenu(TRANSLATE("Language"));
		fLanguageMenu->SetRadioMode(true);
		for(int32 i=0; i<language_roster->CountLanguages(); i++)
		{
			Language *language=language_roster->LanguageAt(i);
			BMessage *langmsg=new BMessage(M_SET_LANGUAGE);
			langmsg->AddInt32("index",i);
			fLanguageMenu->AddItem(new BMenuItem(language->Name(),langmsg));
		}
		menu->AddItem(fLanguageMenu);
		
		BMenuItem *markeditem=fLanguageMenu->FindItem(fLanguage.String());
		if(markeditem)
			markeditem->SetMarked(true);
	}
	else
		fLanguageMenu=NULL;
	
	menu->AddSeparatorItem();
	temp = TRANSLATE("About Capital Be"); temp += "…";
	menu->AddItem(new BMenuItem(temp.String(),new BMessage(M_SHOW_ABOUT)));
	bar->AddItem(menu);
	
	menu = new BMenu(TRANSLATE("File"));
	temp = TRANSLATE("Categories"); temp << "…";
	menu->AddItem(new BMenuItem(temp.String(),new BMessage(M_SHOW_CATEGORY_WINDOW)));
	temp = TRANSLATE("Scheduled Transactions"); temp << "…";
	menu->AddItem(new BMenuItem(temp.String(),new BMessage(M_SHOW_SCHEDULED_WINDOW)));
	menu->AddSeparatorItem();
	temp = TRANSLATE("Import from QIF File"); temp << "…";
	menu->AddItem(new BMenuItem(temp.String(),new BMessage(M_SHOW_IMPORT_PANEL)));
	temp = TRANSLATE("Export to QIF File"); temp << "…";
	menu->AddItem(new BMenuItem(temp.String(),new BMessage(M_SHOW_EXPORT_PANEL)));
	
	bar->AddItem(menu);
	
	menu = new BMenu(TRANSLATE("Account"));
	bar->AddItem(menu);
	
	temp = TRANSLATE("Reconcile"); temp << "…";
	menu->AddItem(new BMenuItem(temp.String(),new BMessage(M_SHOW_RECONCILE_WINDOW),'R'));
	menu->AddSeparatorItem();
	temp = TRANSLATE("New"); temp << "…";
	menu->AddItem(new BMenuItem(temp.String(),new BMessage(M_SHOW_NEW_ACCOUNT),'N'));
	temp = TRANSLATE("Delete"); temp << "…";
	menu->AddItem(new BMenuItem(temp.String(),new BMessage(M_DELETE_ACCOUNT)));
	fAccountClosedItem = new BMenuItem(TRANSLATE("Close"),new BMessage(M_CLOSE_ACCOUNT));
	menu->AddItem(fAccountClosedItem);
	menu->AddSeparatorItem();
	temp = TRANSLATE("Settings"); temp << "…";
	menu->AddItem(new BMenuItem(temp.String(),new BMessage(M_SHOW_ACCOUNT_SETTINGS)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(TRANSLATE("Previous"),new BMessage(M_PREVIOUS_ACCOUNT),B_UP_ARROW,
								B_COMMAND_KEY | B_SHIFT_KEY));
	menu->AddItem(new BMenuItem(TRANSLATE("Next"),new BMessage(M_NEXT_ACCOUNT),B_DOWN_ARROW,
								B_COMMAND_KEY | B_SHIFT_KEY));
	menu = new BMenu(TRANSLATE("Transaction"));
	bar->AddItem(menu);
	temp = TRANSLATE("Edit"); temp << "…";
	menu->AddItem(new BMenuItem(temp.String(),new BMessage(M_EDIT_TRANSACTION),
								'E'));
	temp = TRANSLATE("Enter a Transfer"); temp << "…";
	menu->AddItem(new BMenuItem(temp.String(),new BMessage(M_ENTER_TRANSFER),'T'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(TRANSLATE("Delete"),new BMessage(M_DELETE_TRANSACTION)));
	menu->AddSeparatorItem();
	temp = TRANSLATE("Schedule This Transaction"); temp << "…";
	menu->AddItem(new BMenuItem(temp.String(),
				new BMessage(M_SCHEDULE_TRANSACTION)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(TRANSLATE("Previous"),new BMessage(M_PREVIOUS_TRANSACTION),
								B_UP_ARROW));
	menu->AddItem(new BMenuItem(TRANSLATE("Next"),new BMessage(M_NEXT_TRANSACTION),B_DOWN_ARROW));
	
	menu = new BMenu(TRANSLATE("Tools"));
	bar->AddItem(menu);
	temp = TRANSLATE("Budget"); temp << "…";
	menu->AddItem(new BMenuItem(temp.String(),new BMessage(M_SHOW_BUDGET_WINDOW)));
	temp = TRANSLATE("Reports"); temp << "…";
	menu->AddItem(new BMenuItem(temp.String(),new BMessage(M_SHOW_REPORTS_WINDOW)));
	
	// We load the financial data before we create any of the views because the
	// notifications are not sent and startup time is *significantly* reduced
	LoadData();
	
	r = Bounds();
	r.top = bar->Frame().bottom+1;
	fRegisterView = new RegisterView("registerview",B_WILL_DRAW);
	
	fImportPanel = new BFilePanel(B_OPEN_PANEL,new BMessenger(this),NULL,B_FILE_NODE,
									false,new BMessage(M_IMPORT_ACCOUNT));	
	temp = "Capital Be: "; temp += TRANSLATE("Import");
	fImportPanel->Window()->SetTitle(temp.String());
	fExportPanel = new BFilePanel(B_SAVE_PANEL,new BMessenger(this),NULL,B_FILE_NODE,
									false,new BMessage(M_EXPORT_ACCOUNT));	
	temp = "Capital Be: "; temp += TRANSLATE("Export");
	fExportPanel->Window()->SetTitle(temp.String());
	gDatabase.AddObserver(this);
	
	HandleScheduledTransactions();
	
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0)
		.Add(bar)
		.Add(fRegisterView)
	.End();
}

MainWindow::~MainWindow(void)
{
	SavePreferences(PREFERENCES_PATH "/CapitalBeSettings");
	delete fImportPanel;
}

bool MainWindow::QuitRequested(void)
{
#ifndef NOSAVE
	SaveData();
#endif
	prefsLock.Lock();
	gPreferences.RemoveData("mainframe");
	gPreferences.AddRect("mainframe",Frame());
	prefsLock.Unlock();
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void MainWindow::InitSettings(void)
{
	// This loads all the settings from disk and uses sane defaults if a setting
	// is non-existent or invalid
	if(gPreferences.FindString("lastfile",&fLastFile)!=B_OK)
		fLastFile = PREFERENCES_PATH "/MyAccountData";
}


void MainWindow::MessageReceived(BMessage *msg)
{
	Account *acc = gDatabase.CurrentAccount();
	
	switch(msg->what)
	{
		case M_SET_LANGUAGE:
		{
			int32 language;
			if(msg->FindInt32("index",&language)!=B_OK)
				break;
			
			BMenuItem *item = fLanguageMenu->ItemAt(language);
			if(!item)
				break;
			
			fLanguage = item->Label();
			WriteLanguageSettings();
			
			// This will *rock* under Haiku and any other places which are running
			// the daemon
			if(be_roster->IsRunning("application/x-vnd.Haiku-ServicesDaemon"))
			{
				BMessenger msgr("application/x-vnd.Haiku-ServicesDaemon");
				BMessage msg(B_SERVICES_DAEMON_RESTART);
				msg.AddString("signature","application/x-vnd.wgp-CapitalBe");
				msgr.SendMessage(&msg);
				be_app->PostMessage(B_QUIT_REQUESTED);
			}
			else
			{
				ShowAlert(TRANSLATE("Changes will be made on restart"),
					TRANSLATE("Capital Be will be use your language choice the next time it is "
					"started."),B_IDEA_ALERT);
			}
			break;
		}
		case M_REPORT_BUG:
		{
			BString cmdstring("/boot/beos/apps/BeMail mailto:support@capitalbe.com &");
			system(cmdstring.String());
			break;
		}
		case M_SHOW_NEW_ACCOUNT:
		{
			#ifdef PREVIEW_MODE
				if(gDatabase.CountAccounts()>=5)
				{
					ShowAlert(TRANSLATE("Preview Mode Limit"),
							TRANSLATE(
							"You can have up to 5 accounts in Capital Be's demo version. "
							"We hope that you like Capital Be and will "
							"purchase the full version. Have a nice day!"),
							B_IDEA_ALERT);
					break;
				}
			#endif
			
			AccountSettingsWindow *newaccwin = new AccountSettingsWindow(NULL);
			BRect r(Frame());
			newaccwin->MoveTo(r.left + ((Bounds().Width()-newaccwin->Bounds().Width())/2),
							r.top + ((Bounds().Height()-newaccwin->Bounds().Height())/2) );
			newaccwin->Show();
			break;
		}
		case M_SHOW_ACCOUNT_SETTINGS:
		{
			if(!acc)
				break;
				
			AccountSettingsWindow *accwin = new AccountSettingsWindow(acc);
			BRect r(Frame());
			accwin->MoveTo(r.left + ((Bounds().Width()-accwin->Bounds().Width())/2),
							r.top + ((Bounds().Height()-accwin->Bounds().Height())/2) );
			accwin->Show();
			break;
		}
		case M_DELETE_ACCOUNT:
		{
			if(!acc)
				break;
			
			DAlert *alert = new DAlert(TRANSLATE("Really Delete Account?"),
					TRANSLATE("Once deleted, you will not be able to "
					"get back any data on this account."),
				TRANSLATE("Delete"),
				TRANSLATE("Cancel"),NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
			
			if(alert->Go()==0)
			{
				int32 index = gDatabase.IndexOf(acc);
				if(index==0)
				{
					gDatabase.SetCurrentAccount(1);
				}
				else
				if(index>0)
				{
					gDatabase.SetCurrentAccount(index-1);
				}
				gDatabase.RemoveAccount(acc);
			}
			break;
		}
		case M_PREVIOUS_ACCOUNT:
		{
			if(gDatabase.CountAccounts()<2)
				break;
			
			int32 index = gDatabase.IndexOf(acc);
			if(index < 1 || index > gDatabase.CountAccounts()-1)
				break;
			
			// Because there is no way to call BListView::Select without generating
			// a selection notification firing, we have to hack a method to manually
			// do the selecting which then generates the proper notification instead
			// of tweaking the database via SetCurrentAccount. Man, I hate hacks sometimes. :/
			fRegisterView->SelectAccount(index-1);
			break;
		}
		case M_NEXT_ACCOUNT:
		{
			if(gDatabase.CountAccounts()<2)
				break;
			
			int32 index = gDatabase.IndexOf(acc);
			if(index < 0 || index >= gDatabase.CountAccounts()-1)
				break;
			
			// Because there is no way to call BListView::Select without generating
			// a selection notification firing, we have to hack a method to manually
			// do the selecting which then generates the proper notification instead
			// of tweaking the database via SetCurrentAccount. Man, I hate hacks sometimes. :/
			fRegisterView->SelectAccount(index+1);
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

//			BEntry entry("/boot/develop/projects/Capital Be/source/yoderdata.qif");
//			entry.GetRef(&ref);
			
			if(msg->FindRef("refs",&ref)!=B_OK)
				break;
			
			if(!gDatabase.ImportFile(ref))
			{
				BString errmsg(TRANSLATE("Could not import the data in the file %%FILENAME%%."));
				errmsg.ReplaceFirst("%%FILENAME%%",ref.name);
				ShowAlert(errmsg.String(),
						TRANSLATE("This happens when the kind of file is not "
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
			if(msg->FindRef("directory",&dir)!=B_OK ||
					msg->FindString("name",&name)!=B_OK)
				break;
			
			BPath path(&dir);
			name.Prepend("/");
			name.Prepend(path.Path());
			
			BEntry entry(name.String());
			entry.GetRef(&dir);
			
			if(!gDatabase.ExportFile(dir))
			{
				BString errmsg(TRANSLATE("Could not export your financial data to the file %%FILENAME%%."));
				errmsg.ReplaceFirst("%%FILENAME%%",dir.name);
				ShowAlert(errmsg.String(),
						TRANSLATE("This really shouldn't happen, so you probably "
								"should e-mail support about this."));
			}
			break;
		}
		case M_CLOSE_ACCOUNT:
		{
			if(!acc)
				break;
			
			if(acc->IsClosed())
				gDatabase.ReopenAccount(acc);
			else
				gDatabase.CloseAccount(acc);
			break;
		}
		case M_SHOW_ABOUT:
		{
			AboutWindow *abwin = new AboutWindow();
			abwin->Show();
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
			if(!acc)
				break;
			
			uint32 count = acc->CountTransactions();
			if(!count)
				break;
			
			uint32 item = acc->CurrentTransaction();
			
			if(!fRegisterView->SelectPreviousTransaction())
				fRegisterView->SelectNextTransaction();
			gDatabase.RemoveTransaction(item);
			break;
		}
		case M_ENTER_TRANSFER:
		{
			if(gDatabase.CountAccounts()<2)
			{
				ShowAlert(TRANSLATE("Not enough accounts for a transfer."),
						TRANSLATE("You need to have at least 2 accounts to perform a transfer."));
				break;
			}
			
			TransferWindow *tnwin = new TransferWindow(this);
			BRect r(Frame());
			tnwin->MoveTo(r.left + ((Bounds().Width()-tnwin->Bounds().Width())/2),
							r.top + ((Bounds().Height()-tnwin->Bounds().Height())/2) );
			tnwin->Show();
			break;
		}
		case M_EDIT_TRANSACTION:
		{
			if(!acc)
				break;
			
			if(!acc->CurrentTransaction())
			{
				if(!acc->CountTransactions())
					break;
			}
			
			TransactionData data;
			gDatabase.GetTransaction(acc->CurrentTransaction(),data);
			BRect r(Frame());
			r.right = r.left + 400;
			r.bottom = r.top + 300;
			TransactionEditWindow *transwin = new TransactionEditWindow(r,data);
			transwin->MoveTo(r.left + ((Bounds().Width()-transwin->Bounds().Width())/2),
							r.top + ((Bounds().Height()-transwin->Bounds().Height())/2) );
			transwin->Show();
			break;
		}
		case M_SCHEDULE_TRANSACTION:
		{
			if(!acc)
				break;
			
			if(!acc->CurrentTransaction())
			{
				if(!acc->CountTransactions())
					break;
			}
			
			TransactionData data;
			gDatabase.GetTransaction(acc->CurrentTransaction(),data);
			
			if(data.Type().TypeCode()== TRANS_NUMERIC)
			{
				ShowAlert(TRANSLATE("Numbered transactions cannot be scheduled."),
						TRANSLATE("You can schedule transfers, deposits, or ATM transactions."));
				break;
			}
			
			BRect r(Frame());
			r.right = r.left + 400;
			r.bottom = r.top + 300;
			
			ScheduleAddWindow *schedwin = new ScheduleAddWindow(r,data);
			schedwin->MoveTo(r.left + ((Bounds().Width()-schedwin->Bounds().Width())/2),
							r.top + ((Bounds().Height()-schedwin->Bounds().Height())/2) );
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
			PrefWindow *pwin = new PrefWindow(Frame());
			pwin->Show();
			break;
		}
		case M_SHOW_REPORTS_WINDOW:
		{
			// While it seems kind of silly to show the reports window if 
			// there is no data, it seems more like a bug to not bother showing it
			
			ReportWindow *rwin = new ReportWindow(Frame());
			rwin->Show();
			break;
		}
		case M_SHOW_RECONCILE_WINDOW:
		{
			if(!acc)
			{
				if(gDatabase.CountAccounts()<1)
					ShowAlert(TRANSLATE("Oops!"),
							TRANSLATE("You need to have an account created in order to reconcile it."));
				else
					ShowAlert(TRANSLATE("Oops!"),
							TRANSLATE("You need to select an account in order to reconcile it."));
				break;
			}
			
			BRect r(Frame());
			ReconcileWindow *recwin = new ReconcileWindow(BRect(100,100,600,425), acc);
			recwin->MoveTo(r.left + ((Bounds().Width()-recwin->Bounds().Width())/2),
							r.top + ((Bounds().Height()-recwin->Bounds().Height())/2) );
			recwin->Show();
			break;
		}
		case M_SHOW_SCHEDULED_WINDOW:
		{
			BRect r(Frame());
			ScheduleListWindow *schedwin = new ScheduleListWindow(BRect(100,100,600,425));
			schedwin->MoveTo(r.left + ((Bounds().Width()-schedwin->Bounds().Width())/2),
							r.top + ((Bounds().Height()-schedwin->Bounds().Height())/2) );
			schedwin->Show();
			break;
		}
		case M_SHOW_BUDGET_WINDOW:
		{
			BudgetWindow *bwin = new BudgetWindow(Frame());
			bwin->Show();
			break;
		}
		case M_SHOW_CATEGORY_WINDOW:
		{
			BRect r(Frame());
			CategoryWindow *catwin = new CategoryWindow(BRect(100,100,600,425));
			catwin->MoveTo(r.left + ((Bounds().Width()-catwin->Bounds().Width())/2),
							r.top + ((Bounds().Height()-catwin->Bounds().Height())/2) );
			catwin->Show();
			break;
		}
		case M_PURCHASE_FULL_VERSION:
		{
			BMessage msg(B_NETPOSITIVE_OPEN_URL);
			msg.AddString("be:url","http://www.capitalbe.com/");
			be_roster->Launch(B_NETPOSITIVE_APP_SIGNATURE,&msg);
			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}

void MainWindow::LoadData(void)
{
	if(gDatabase.OpenFile(fLastFile.String())!=B_OK)
	{
		BEntry entry(fLastFile.String());
		if(!entry.Exists())
		{
			// TODO: Show Capital Be introduction
// TODO: Show a Create File dialog
			gDatabase.CreateFile(fLastFile.String());
			PostMessage(M_SHOW_NEW_ACCOUNT);
		}
	}
}

void MainWindow::SaveData(void)
{
}

void MainWindow::CreateTransfer(BMessage *msg)
{
	Account *from,*to;
	BString amount,memo,datestr;
	Fixed fixed;
	time_t date;
	
	if( (msg->FindPointer("from",(void**)&from)!=B_OK) ||
		(msg->FindPointer("to",(void**)&to)!=B_OK) ||
		(msg->FindString("amount",&amount)!=B_OK) ||
		(gCurrentLocale.StringToCurrency(amount.String(),fixed)!=B_OK) ||
		(msg->FindString("date",&datestr)!=B_OK) ||
		(gDefaultLocale.StringToDate(datestr.String(),date)!=B_OK) )
		return;
	
	if(msg->FindString("memo",&memo)!=B_OK)
		memo="";
	
	// Now that we've gathered all the data from the message sent to us by TransferWindow,
	// we create the transactions needed for each account.
	BString payee = TRANSLATE("Transfer to %%PAYEE%%");
	payee.ReplaceFirst("%%PAYEE%%",to->Name());
	
	uint32 transid = gDatabase.NextTransactionID();
	TransactionType type("XFER");
	gDatabase.AddTransaction(from->GetID(),transid,date,type,payee.String(),
							fixed.InvertAsCopy(),"Transfer",memo.String());
	
	payee = TRANSLATE("Transfer from %%PAYEE%%");
	payee.ReplaceFirst("%%PAYEE%%",to->Name());
	payee << from->Name();
	gDatabase.AddTransaction(to->GetID(),transid,date,type,payee.String(),
							fixed,"Transfer",memo.String());
}

void MainWindow::HandleNotify(const uint64 &value, const BMessage *msg)
{
	bool lockwin=false;
	if(!IsLocked())
	{
		Lock();
		lockwin=true;
	}
	
	if(value & WATCH_ACCOUNT)
	{
		Account *acc;
		if(msg->FindPointer("item",(void**)&acc)!=B_OK || !acc)
		{
			if(lockwin)
				Unlock();
			return;
		}
		
		if(value & WATCH_SELECT || value & WATCH_CHANGE)
		{
			if(acc->IsClosed())
			{
				fAccountClosedItem->SetLabel(TRANSLATE("Reopen"));
			}
			else
			{
				fAccountClosedItem->SetLabel(TRANSLATE("Close"));
			}
		}
	}

	if(lockwin)
		Unlock();
}

void MainWindow::ReadLanguageSettings(void)
{
	BFile file("/boot/home/config/settings/CapitalBe/CurrentLocale", B_READ_ONLY);
	
	if(file.InitCheck()!=B_OK)
		return;
	
	file.Seek(0,SEEK_END);
	off_t size = file.Position();
	file.Seek(0,SEEK_SET);
	
	char name[size];
	file.Read(name,size);
	file.Unset();
	
	fLanguage=name;
}

void MainWindow::WriteLanguageSettings(void)
{
	BFile file("/boot/home/config/settings/CapitalBe/CurrentLocale",
				B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	
	if(file.InitCheck()!=B_OK)
		return;
	
	file.Seek(0,SEEK_SET);
	file.Write(fLanguage.String(),fLanguage.Length()+1);
	file.Unset();
}
