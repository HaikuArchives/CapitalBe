#include "Import.h"
#include <File.h>
#include <TypeConstants.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include "Account.h"
#include "Budget.h"
#include "CBLocale.h"
#include "Category.h"
#include "DStringList.h"
#include "Database.h"
#include "ObjectList.h"
#include "TextFile.h"
#include "Transaction.h"
#include "TransactionData.h"

#define DEBUG_IMPORT

#ifdef DEBUG_IMPORT
#define STRACE(x) printf x
#else
#define STRACE(x) /* */
#endif

bool debuggerflag = false;

// These functions are called by the global database object to remove all the
// messy import code from the rest of the otherwise-clean OOP code. This code is
// not likely to be pretty, especially the code for importing BeFinancial files

/*
	Undocumented types:
		!Type:InvItem
		!Type:Template
	All I know is that they exist when Business Lists are exported from Q2004.
*/

BString ReadCategories(BObjectList<Category>& list, TextFile& file);
BString ReadTransactions(Account* account, TextFile& file);
BString ReadAccounts(BObjectList<Account>& list, TextFile& file);

BString DateToQIFDate(const time_t& date);
BString MakeCategoryString(const DStringList& list, const bool& isexpense);

bool
ImportQIF(const entry_ref& ref)
{
	TextFile file(ref, B_READ_ONLY);

	STRACE(("Importing QIF file\n"));
	BObjectList<Category> catlist(20);
	BObjectList<Account> accountlist(20);
	int32 accountindex = 0;

	BString string = file.ReadLine();

	while (!file.IsEOF()) {
		Account* currentaccount;
		BString accountname;

		if (string.CountChars() < 1)
			break;

		string.RemoveAll("\r");

		if (string.FindFirst("!Type:Cat") != B_ERROR) {
			string = ReadCategories(catlist, file);
		} else if (string.FindFirst("!Type:Bank") != B_ERROR) {
			STRACE(("Bank Account\n"));
			accountindex++;
			accountname = "Bank Account ";
			accountname << accountindex;

			currentaccount = gDatabase.AddAccount(accountname.String(), ACCOUNT_BANK, "open");
			string = ReadTransactions(currentaccount, file);
		} else if (string.FindFirst("!Type:Cash") != B_ERROR) {
			accountindex++;
			accountname = "Cash Account ";
			accountname << accountindex;

			currentaccount = gDatabase.AddAccount(accountname.String(), ACCOUNT_CASH, "open");
			string = ReadTransactions(currentaccount, file);
		} else if (string.FindFirst("!Type:CCard") != B_ERROR) {
			accountindex++;
			accountname = "Credit Card Account ";
			accountname << accountindex;

			currentaccount = gDatabase.AddAccount(accountname.String(), ACCOUNT_CREDIT, "open");
			string = ReadTransactions(currentaccount, file);
		}
		/*		else
				if(string=="!Account")
				{
					if(accountlist.CountItems()<1)
						string = ReadAccounts(accountlist,file);
					else
					{
						string = ReadTransactions(accountlist.ItemAt(accountindex),file);
						accountindex++;
					}

				}
				else
				if(string=="!Type:Memorized")
				{
					printf("DEBUG: importing Memorized transactions unimplemented\n");
					string = file.ReadLine();
				}
		*/
		else {
			//			STRACE(("Line %s\n",string.String()));
			string = file.ReadLine();
		}
	}

	return true;
}

BString
ReadCategories(BObjectList<Category>& list, TextFile& file)
{
	STRACE(("Importing category list\n"));
	BString catdata = file.ReadLine();
	catdata.RemoveAll("\r");

	Category* cat = new Category();
	while (catdata.ByteAt(0) != '!' && !file.IsEOF()) {
		/*		switch(catdata.ByteAt(0))
				{
					case 'N':
					{
						// Name
						cat->SetNameAt(0,catdata.String()+1);
						break;
					}
					case 'B':
					{
						// Budget amounts are listed in the file in order
						// and for any category which is used for budgeting

						// TODO: Implement Budget amounts in QIF import
						break;
					}
					case '^':
					{
						list.AddItem(cat);
						cat = new Category();
						break;
					}
					case '!':
					{
						STRACE(("Added %ld categories\n",list.CountItems()));
						delete cat;
						return catdata;
						break;
					}
					default:
						break;
				}
		*/
		catdata = file.ReadLine();
		catdata.RemoveAll("\r");
	}
	delete cat;

	STRACE(("Added %ld categories\n", list.CountItems()));
	return catdata;
}

BString
ReadAccounts(BObjectList<Account>& list, TextFile& file)
{
	STRACE(("Importing accounts\n"));
	BString accdata = file.ReadLine();

	BString accname = "";

	while (accdata.ByteAt(0) != '!' && !file.IsEOF()) {
		accdata.RemoveAll("\r");
		switch (accdata.ByteAt(0)) {
			case 'N':
			{
				accname = accdata.String() + 1;
				break;
			}
			case '^':
			{
				if (accname.CountChars() < 1)
					break;

				Account* acc = gDatabase.AddAccount(accname.String(), ACCOUNT_BANK);
				list.AddItem(acc);
				accname = "";
				break;
			}
			default:
				break;
		}

		accdata = file.ReadLine();
	}
	return accdata;
}

BString
ReadTransactions(Account* account, TextFile& file)
{
	gDatabase.SetNotify(false);
	gDatabase.DBCommand("BEGIN EXCLUSIVE", "Import:Turn off autocommit");
	STRACE(("Importing bank transactions to %s\n", account ? account->Name() : "<unnamed>"));

	time_t date;
	TransactionData data;
	data.SetAccount(account);

	BString transdata = file.ReadLine();

	bool splitmode = false;
	while (transdata.ByteAt(0) != '!' && !file.IsEOF()) {
		transdata.RemoveAll("\r");

		switch (transdata.ByteAt(0)) {
			case 'D':
			{
				// Date. Note that there are two formats.
				// The antequated format is immediately compatible with
				// StringToDate - MM/DD/YY. The other is downright
				// funky: MM/DD/' F . F is an offset from the year 2000,
				// so a line reading D1/23' 5 is 1/23/2005.
				transdata.ReplaceFirst("' ", "/200");
				if (gDefaultLocale.StringToDate(transdata.String() + 1, date) == B_OK)
					data.SetDate(date);
				break;
			}
			case 'C':
			{
				// apparently a check can be cleared or not - no reconciliation
				// done :/
				data.SetStatus(TRANS_RECONCILED);
				break;
			}
			case 'M':
			{
				data.SetMemo(transdata.String() + 1);
				break;
			}
			case 'N':
			{
				data.SetType(transdata.String() + 1);
				break;
			}
			case 'P':
			{
				// Payee
				data.SetPayee(transdata.String() + 1);
				break;
			}
			case 'S':
			{
				if (splitmode == false) {
					data.SetCategory("");
					splitmode = true;
				}

				BString splitamount = file.ReadLine();
				BString splitmemo;
				if (splitamount.ByteAt(0) == 'E') {
					splitmemo = splitamount.String() + 1;
					splitmemo.RemoveAll("\r");

					splitamount = file.ReadLine();
					splitamount.RemoveAll("\r");
				}

				if (splitamount.ByteAt(0) != '$')
					break;

				Fixed splitfixed;
				if (gCurrentLocale.StringToCurrency(splitamount.String() + 1, splitfixed) != B_OK)
					break;

				data.AddCategory(transdata.String() + 1, splitfixed, false);
				if (splitmemo.CountChars() > 0)
					data.SetMemoAt(data.CountCategories() - 1, splitmemo.String());

				break;
			}
			case 'U':
			case 'T':
			{
				Fixed amount;
				if (gCurrentLocale.StringToCurrency(transdata.String() + 1, amount) == B_OK)
					data.SetAmount(amount);

				if (amount.IsPositive())
					data.SetType("DEP");
				else {
					if (data.Type().TypeCode() == TRANS_INIT)
						data.SetType("ATM");
				}
				break;
			}
			case 'L':
			{
				// Category
				data.SetCategory(transdata.String() + 1);
				break;
			}
			case '^':
			{
				if (strlen(data.Payee()) < 1) {
					// There shouldn't be a transaction without a payee. Apparently, though,
					// things aren't so pretty, necessarily. We will handle this with
					// two conditions - skip the thing entirely if the amount is 0
					// and use [No Payee Entered] otherwise
					if (data.Amount().AsLong() == 0)
						break;
					else
						data.SetPayee("[No Payee Entered]");
				}

				if (data.CountCategories() < 1)
					data.SetCategory("Uncategorized");


				gDatabase.AddTransaction(data);
				STRACE(("Added transaction %s\n", data.Payee()));
				data.MakeEmpty();
				data.SetAccount(account);
				splitmode = false;
				break;
			}
			default:
				break;
		}
		transdata = file.ReadLine();
	}
	gDatabase.SetNotify(true);
	gDatabase.DBCommand("COMMIT", "turn on autocommit");
	return transdata;
}

bool
ExportQIF(const entry_ref& ref)
{
	BFile file(&ref, B_CREATE_FILE | B_READ_WRITE | B_FAIL_IF_EXISTS);
	if (file.InitCheck() != B_OK)
		return false;

	BString text;
	CppSQLite3Query query;
	BString command;

	// Export categories and budget
	DStringList incomelist, expenselist;
	query = gDatabase.DBQuery("SELECT * FROM categorylist ORDER BY name",
		"ExportQIF:get category list");
	while (!query.eof()) {
		if (query.getIntField(1) == 1)
			incomelist.AddItem(query.getStringField(0));
		else
			expenselist.AddItem(query.getStringField(0));
		query.nextRow();
	}
	query.finalize();

	if (incomelist.CountItems() > 0 || expenselist.CountItems() > 0)
		text << "!Type:Cat\n";

	text << MakeCategoryString(incomelist, false);
	text << MakeCategoryString(expenselist, true);

	if (text.CountChars() > 0)
		file.Write(text.String(), text.CountChars());
	text = "";

	// Export account list
	text << "!Account\n";
	for (int32 i = 0; i < gDatabase.CountAccounts(); i++) {
		Account* account = gDatabase.AccountAt(i);
		text << "N" << account->Name() << "\nBank\n^\n";
	}

	file.Write(text.String(), text.CountChars());
	text = "";

	// Export accounts
	text << "!Account\n";
	for (int32 i = 0; i < gDatabase.CountAccounts(); i++) {
		Account* account = gDatabase.AccountAt(i);
		text << "N" << account->Name() << "\nTBank\n^\n!Type:Bank\n";

		command = "SELECT * FROM account_";
		command << account->GetID() << " ORDER BY date;";
		query = gDatabase.DBQuery(command.String(), "ExportQIF:get transactions from accounts");

		while (!query.eof()) {
			time_t date = query.getInt64Field(2);
			text << "D" << DateToQIFDate(date) << "\n";

			int32 amount = query.getInt64Field(5);
			Fixed fixed(amount, true);
			BString temp;
			gDefaultLocale.CurrencyToString(fixed, temp);
			temp.RemoveFirst(gDefaultLocale.CurrencySymbol());
			text << "U" << temp << "\nT" << temp << "\n";

			text << "N" << DeescapeIllegalCharacters(query.getStringField(3)) << "\n";
			text << "P" << DeescapeIllegalCharacters(query.getStringField(4)) << "\n";

			temp = DeescapeIllegalCharacters(query.getStringField(7));
			if (temp.CountChars() > 0)
				text << "M" << temp << "\n";

			text << "L" << DeescapeIllegalCharacters(query.getStringField(6)) << "\n^\n";
			query.nextRow();
		}

		file.Write(text.String(), text.CountChars());
		text = "";
	}
	query.finalize();

	// TODO: Export memorized transactions

	return true;
}

BString
DateToQIFDate(const time_t& date)
{
	struct tm* timestruct;

	timestruct = localtime(&date);

	BString qifdate;
	if (timestruct) {
		if (timestruct->tm_year < 100) {
			qifdate << (timestruct->tm_mon + 1) << "/" << timestruct->tm_mday << "/"
					<< timestruct->tm_year;
		} else {
			qifdate << (timestruct->tm_mon + 1) << "/";

			if (timestruct->tm_mday < 10)
				qifdate += " ";
			qifdate << timestruct->tm_mday << "'";

			if ((timestruct->tm_year - 100) < 10)
				qifdate += " ";
			qifdate << (timestruct->tm_year - 100);
		}
	}

	return qifdate;
}

BString
MakeCategoryString(const DStringList& list, const bool& isexpense)
{
	BString text;
	CppSQLite3Query query;

	for (int32 i = 0; i < list.CountItems(); i++) {
		BString* category = list.ItemAt(i);
		BString unescaped(DeescapeIllegalCharacters(category->String()));
		text << "N" << unescaped << "\nD" << unescaped << "\n" << (isexpense ? "E" : "I") << "\n";

		// See if the category is in the budget and, if so, write it to disk if nonzero
		BudgetEntry entry;
		if (gDatabase.GetBudgetEntry(unescaped.String(), entry)) {
			Fixed f = entry.amount;

			bool error = false;
			switch (entry.period) {
				case BUDGET_WEEKLY:
				{
					f *= 52;
					f /= 12;
					break;
				}
				case BUDGET_MONTHLY:
				{
					break;
				}
				case BUDGET_QUARTERLY:
				{
					f /= 3;
					break;
				}
				case BUDGET_ANNUALLY:
				{
					f /= 12;
					break;
				}
				default:
				{
					error = true;
					break;
				}
			}

			BString temp;
			gDefaultLocale.CurrencyToString(f, temp);
			temp.RemoveFirst(gDefaultLocale.CurrencySymbol());
			temp.Prepend("B");
			temp += "\n";

			for (int32 i = 0; i < 12; i++)
				text += temp;
		}

		text << "^\n";
	}

	return text;
}
