#include "Database.h"
#include <Entry.h>
#include <FormattingConventions.h>
#include <stdio.h>
#include <stdlib.h>
#include <Locale.h>

#include "Account.h"
#include "Budget.h"
#include "DStringList.h"
#include "Fixed.h"
#include "Import.h"
#include "ScheduledTransData.h"
#include "TimeSupport.h"
#include "TransactionData.h"

// #define LOCK_DATABASE
#ifdef LOCK_DATABASE
#define LOCK fLocker.Lock()
#define UNLOCK fLocker.Unlock()
#else
#define LOCK ;
#define UNLOCK ;
#endif

// This is the system default locale, which is US-style. It is used for storage
// and the default preferred locale
Locale gDefaultLocale;

// This is the locale is set to whatever is used by the active account in the database
Locale gCurrentLocale;

// This global is used to hold all financial data for CapitalBe.
Database gDatabase;

Database::Database(const char* path)
	: fCurrent(NULL),
	  fList(20, true),
	  fPath(path)
{
}

Database::~Database(void) {}

void
Database::CloseAccount(Account* item)
{
	if (!item)
		return;

	LOCK;
	BString command = "update accountlist set status = 'Closed' where accountid = ";
	command << item->GetID() << ";";
	DBCommand(command.String(), "Database::CloseAccount");

	item->SetClosed(true);

	BMessage msg;
	msg.AddPointer("item", (void*)item);
	Notify(WATCH_CHANGE | WATCH_ACCOUNT, &msg);
	UNLOCK;
}

void
Database::RenameAccount(Account* item, const char* name)
{
	if (item && name) {
		LOCK;
		BString command = "update accountlist set name = '";
		command << EscapeIllegalCharacters(name) << "' where accountid = ";
		command << item->GetID() << ";";
		DBCommand(command.String(), "Database::RenameAccount");

		item->SetName(name);

		BMessage msg;
		msg.AddPointer("item", (void*)item);
		Notify(WATCH_CHANGE | WATCH_ACCOUNT, &msg);
		UNLOCK;
	}
}

void
Database::ReopenAccount(Account* item)
{
	if (!item)
		return;

	LOCK;
	BString command = "update accountlist set status = 'Open' where accountid = ";
	command << item->GetID() << ";";
	DBCommand(command.String(), "Database::ReopenAccount");

	item->SetClosed(false);
	BMessage msg;
	msg.AddPointer("item", (void*)item);
	Notify(WATCH_CHANGE | WATCH_ACCOUNT, &msg);
	UNLOCK;
}

void
Database::CreateFile(const char* path)
{
	if (!path)
		return;

	LOCK;
	fDB.close();
	fPath = path;

	try {
		fDB.open(path);
	} catch (...) {
		BString error = "Couldn't create database file '";
		error << path << "'.";
		ShowBug(error.String());
		UNLOCK;
		return;
	}

	// Character-Entry Limits:
	// Category names: 32
	// Account names: 32
	// Currency/Date separators: 2
	// Transaction Status: 10
	// Currency Symbol/Decimal: 2

	DBCommand(
		"create table accountlist (accountid int primary key, name varchar(96), "
		"type varchar(12), status varchar(30));",
		"Database::CreateFile:create accountlist");
	DBCommand(
		"create table accountlocale (accountid int primary key, dateformat varchar(8), "
		"dateseparator char(6), currencysymbol char(6),	"
		"currencyseparator char(6), currencydecimal char(6), "
		"currencyprefix char(1));",
		"Database::CreateFile:create accountlocale");
	DBCommand("create table memorizedlist (transactionid int);",
		"Database::CreateFile:create memorizedlist");
	DBCommand(
		"create table scheduledlist (timestamp int primary key, accountid int, transid int,"
		"date int, type varchar(24), payee varchar(96), amount int,"
		"category varchar(96),memo varchar(63), interval int, count int,"
		"nextdate int);",
		"Database::CreateFile:create scheduledlist");
	DBCommand(
		"create table budgetlist (entryid int primary key, category varchar(96), "
		"amount int, period int(2), isexpense int(1));",
		"Database::CreateFile:create budgetlist");
	DBCommand(
		"create table transactionlist (timestamp int primary key, transid int, "
		"category varchar(96), accountid int);",
		"Database::CreateFile:create transactionlist");
	DBCommand("create table categorylist (name varchar(96), type int(2));",
		"Database::CreateFile:create categorylist");

	// now that the tables have all been created, create the default locale
	DBCommand(
		"create table defaultlocale (dateformat varchar(8), "
		"dateseparator char(6), currencysymbol char(6),	"
		"currencyseparator char(6), currencydecimal char(6), "
		"currencyprefix char(1));",
		"Database::CreateFile:create defaultlocale");
	DBCommand("insert into defaultlocale values('mmddyyyy', '/', '$', ',', '.', 'true');",
		"Database::CreateFile insert defaultlocale values");
	UNLOCK;
}

status_t
Database::OpenFile(const char* path)
{
	if (!path)
		return B_ERROR;

	BEntry entry(path);
	if (entry.InitCheck() != B_OK)
		return entry.InitCheck();

	if (!entry.Exists())
		return B_NAME_NOT_FOUND;

	LOCK;
	fDB.close();
	fPath = path;

	try {
		fDB.open(path);
	} catch (...) {
		UNLOCK;
		ShowAlert("Couldn't open account data",
			"CapitalBe couldn't open your financial data. "
			"If your data is in the old storage format from the Preview Edition 1.0, you "
			"will need to run the conversion program before you can use your data.");
		return B_ERROR;
	}

	gDefaultLocale = GetDefaultLocale();

	// Populate account list
	CppSQLite3Query query =
		DBQuery("select * from accountlist", "Database::OpenFile:get accounts from list");

	while (!query.eof()) {
		uint32 id = query.getIntField(0);
		BString name = DeescapeIllegalCharacters(query.getStringField(1));
		//		BString type = query.getStringField(2);
		BString status = DeescapeIllegalCharacters(query.getStringField(3));

		Account* account = new Account(name.String(), status.ICompare("open") != 0);
		account->SetID(id);
		if (!UsesDefaultLocale(id)) {
			account->UseDefaultLocale(false);
			Locale accountLocale = LocaleForAccount(id);
			account->SetLocale(accountLocale);
		}
		fList.AddItem(account);

		BMessage msg;
		msg.AddPointer("item", (void*)account);
		Notify(WATCH_CREATE | WATCH_ACCOUNT, &msg);

		query.nextRow();
	}
	query.finalize();

	// Set the current transaction and last check number for each account
	for (int32 i = 0; i < fList.CountItems(); i++) {
		Account* acc = fList.ItemAt(i);
		BString command("select transid from account_");
		command << acc->GetID() << " order by date,transid;";

		query = DBQuery(command.String(), "Database::OpenFile:set current transaction");
		if (!query.eof())
			acc->SetCurrentTransaction(query.getInt64Field(0));
		query.finalize();

		acc->SetLastCheckNumber(acc->LookupLastCheckNumber());
	}

	fCurrent = (Account*)fList.ItemAt(0);
	UNLOCK;
	return B_OK;
}

void
Database::CloseFile(void)
{
	LOCK;
	fDB.close();
	UNLOCK;
}

bool
Database::ImportFile(const entry_ref& ref)
{
	LOCK;
	Notify(WATCH_MASS_EDIT, NULL);
	bool value = ImportQIF(ref);
	Notify(WATCH_MASS_EDIT, NULL);
	UNLOCK;

	return value;
}

bool
Database::ExportFile(const entry_ref& ref)
{
	LOCK;
	bool value = ExportQIF(ref);
	UNLOCK;

	return value;
}

Account*
Database::SetCurrentAccount(const int32& index)
{
	// We actually permit a NULL pointer here because sometimes we don't *have* an account
	// to operate on
	LOCK;
	Account* acc = (Account*)fList.ItemAt(index);
	fCurrent = acc;
	gCurrentLocale = acc->GetLocale();

	BMessage msg;
	msg.AddPointer("item", (void*)acc);
	Notify(WATCH_SELECT | WATCH_ACCOUNT, &msg);
	UNLOCK;

	return acc;
}

int32
Database::SetCurrentAccount(Account* account)
{
	if (!account) {
		LOCK;
		SetCurrentAccount(-1);
		UNLOCK;
		return -1;
	}

	// We actually permit a NULL pointer here because sometimes we don't *have* an account
	// to operate on
	if (!fList.HasItem(account))
		return -1;

	LOCK;
	fCurrent = account;
	gCurrentLocale = account->GetLocale();

	BMessage msg;
	msg.AddPointer("item", (void*)account);
	Notify(WATCH_SELECT | WATCH_ACCOUNT, &msg);

	int32 value = fList.IndexOf(account);
	UNLOCK;
	return value;
}

Account*
Database::AddAccount(
	const char* name, const AccountType& type, const char* status, const Locale* locale)
{
	if (!name || !status)
		return NULL;

	LOCK;
	BString ename, estatus;
	ename = EscapeIllegalCharacters(name);
	estatus = EscapeIllegalCharacters(status);

	int32 id = GetLastKey("accountlist", "accountid");
	id++;

	BString command;
	command << "insert into accountlist values(" << id << ", '" << ename << "', '"
			<< AccountTypeToString(type) << "', '" << estatus << "');";
	DBCommand(command.String(), "Database::AddAccount:insert accountlist");

	if (locale != &gDefaultLocale)
		SetAccountLocale(id, *locale);

	command = "";
	command << "create table account_" << id
			<< " (timestamp int primary key, transid int, date int, type varchar(24), "
			   "payee varchar(96), amount int, category varchar(96), memo varchar(63), "
			   "status char(21));";

	DBCommand(command.String(), "Database::AddAccount:create account");

	Account* account = new Account(name);
	account->SetID(id);
	account->UseDefaultLocale(locale == &gDefaultLocale);
	if (strcmp(status, "closed") == 0)
		account->SetClosed(true);
	fList.AddItem(account);

	if (fList.CountItems() == 1)
		fCurrent = account;

	BMessage msg;
	msg.AddPointer("item", (void*)account);
	Notify(WATCH_CREATE | WATCH_ACCOUNT | WATCH_LOCALE, &msg);
	UNLOCK;

	return account;
}

bool
Database::RemoveAccount(const int& accountid)
{
	LOCK;
	BString command;
	command << "select accountid from accountlist where accountid = " << accountid << ";";
	CppSQLite3Query query = DBQuery(command.String(), "Database::RemoveAccount:accountid check");

	if (!query.eof()) {
		query.finalize();

		Account* item = AccountByID(accountid);
		if (item) {
			BMessage msg;
			msg.AddPointer("item", (void*)item);
			Notify(WATCH_DELETE | WATCH_ACCOUNT, &msg);
		}

		command = "delete from accountlist where accountid = ";
		command << accountid << ";";
		DBCommand(command.String(), "Database::RemoveAccount:delete accountlist item");

		command = "delete from accountlocale where accountid = ";
		command << accountid << ";";
		DBCommand(command.String(), "Database::RemoveAccount:delete accountlocale item");

		command = "drop table account_";
		command << accountid;
		DBCommand(command.String(), "Database::RemoveAccount:drop account table");

		fList.RemoveItem(item);
		if (fList.CountItems() == 0)
			fCurrent = 0;

		UNLOCK;
		return true;
	}
	query.finalize();
	UNLOCK;
	return false;
}

void
Database::RemoveAccount(Account* account)
{
	if (account)
		RemoveAccount(account->GetID());
}

Account*
Database::AccountByName(const char* name)
{
	if (!name)
		return NULL;

	for (int32 i = 0; i < fList.CountItems(); i++) {
		Account* acc = fList.ItemAt(i);
		if (acc && acc->Name() && strcmp(name, acc->Name()) == 0)
			return acc;
	}
	return NULL;
}

Account*
Database::AccountByID(const uint32& accountid)
{
	for (int32 i = 0; i < fList.CountItems(); i++) {
		Account* acc = fList.ItemAt(i);
		if (acc && acc->GetID() == accountid)
			return acc;
	}
	return NULL;
}

void
Database::AddBudgetEntry(const BudgetEntry& entry)
{
	if (entry.name.CountChars() < 1 || !HasCategory(entry.name.String()))
		return;

	LOCK;
	BString ecategory = EscapeIllegalCharacters(entry.name.String());

	// See if the budget list already has the category and if it does, update the entry
	// to the new value. Otherwise, add the entry to the list
	BString command;
	command << "select entryid from budgetlist where category = '" << ecategory << "';";
	CppSQLite3Query query = DBQuery(command.String(), "Database::AddBudgetEntry:check existing");

	int value = -1;
	if (!query.eof())
		value = query.getIntField(0);
	query.finalize();

	if (value >= 0) {
		command = "update budgetlist set amount = ";
		command << entry.amount.AsFixed() << " where entryid = " << value << ";";
		DBCommand(command.String(), "Database::AddBudgetEntry:update budgetlist amount");

		command = "update budgetlist set period = ";
		command << (int)entry.period << " where entryid = " << value << ";";
		DBCommand(command.String(), "Database::AddBudgetEntry:update budgetlist period");
		return;
	}

	// We got this far, so we just add the entry to the list
	value = GetLastKey("budgetlist", "entryid");
	value++;
	command = "insert into budgetlist values(";
	command << value << ", '" << ecategory << "', " << entry.amount.AsFixed() << ", "
			<< (int)entry.period << ", " << (entry.isexpense ? 1 : 0) << ");";
	DBCommand(command.String(), "Database::AddBudgetEntry:insert into budgetlist");
	UNLOCK;
}

bool
Database::RemoveBudgetEntry(const char* category)
{
	if (!category || !HasBudgetEntry(category))
		return false;

	LOCK;
	BString ecategory = EscapeIllegalCharacters(category);

	BString command;
	command << "delete from budgetlist where category = '" << ecategory << "';";
	DBCommand(command.String(), "Database::RemoveBudgetEntry");
	UNLOCK;
	return true;
}

bool
Database::HasBudgetEntry(const char* category)
{
	if (!category)
		return false;

	LOCK;
	BString ecategory = EscapeIllegalCharacters(category);
	BString command;
	command << "select entryid from budgetlist where category = '" << ecategory << "' order by 1;";
	CppSQLite3Query query = DBQuery(command.String(), "Database::HasBudgetEntry");

	bool value = query.eof();
	UNLOCK;
	return value;
}

bool
Database::GetBudgetEntry(const char* name, BudgetEntry& entry)
{
	if (!name)
		return false;

	BString escaped = EscapeIllegalCharacters(name);
	BString command = "select amount,period,isexpense from budgetlist where category = '";
	command << escaped << "';";
	CppSQLite3Query query = gDatabase.DBQuery(command.String(), "Database::GetBudgetEntry");

	if (!query.eof()) {
		entry.name = name;
		entry.amount.SetPremultiplied(query.getInt64Field(0));
		entry.period = (BudgetPeriod)query.getIntField(1);
		entry.isexpense = (query.getIntField(2) == 1);
		return true;
	}
	return false;
}

int32
Database::CountBudgetEntries(void)
{
	CppSQLite3Query query =
		gDatabase.DBQuery("select category from budgetlist", "Database::CountBudgetEntries");

	int32 count = 0;
	while (!query.eof()) {
		count++;
		query.nextRow();
	}
	return count;
}

void
Database::SetAccountLocale(const uint32& accountid, const Locale& data)
{
	gCurrentLocale = data;

	LOCK;
	BString command("select accountid from accountlocale where accountid = ");
	command << accountid << ";";
	CppSQLite3Query query = DBQuery(command.String(), "Database::SetAccountLocale:find accountid");

	// Todo: remove table columns for date format
	if (query.eof()) {
		command << "insert into accountlocale values(" << accountid << ",'" << data.AccountLocale()
				<< "','-','-','-','-','-');";
		DBCommand(command.String(), "Database::SetAccountLocale:insert into accountlocale");
		UNLOCK;
		return;
	}

	query.finalize();

	// This already has the locale data in the table, so we'll just update it and return
	command << "update accountlocale set dateformat = '" << data.AccountLocale() << "' where accountid = " << accountid	<< ";";
	DBCommand(command.String(), "Database::SetAccountLocale:update dateformat");
	UNLOCK;
}


Locale
Database::LocaleForAccount(const uint32& id)
{
	LOCK;
	BString command;
	command << "select * from accountlocale where accountid = " << id << ";";
	CppSQLite3Query query = DBQuery(command.String(), "Database::LocaleForAccount:find account");

	Locale locale;
	if (query.eof()) {
		UNLOCK;
		return locale;
	}

	BString temp;
	locale.SetAccountLocale(query.getStringField(1));
	locale.SetCurrencySymbol(query.getStringField(3));
	locale.SetCurrencySeparator(query.getStringField(4));
	locale.SetCurrencyDecimal(query.getStringField(5));
	temp = query.getStringField(6);
	locale.SetCurrencySymbolPrefix(temp.Compare("true") == 0 ? true : false);
	UNLOCK;
	return locale;
}

void
Database::SetDefaultLocale(const Locale& data)
{
	// TODO: remove
	return;
}

Locale
Database::GetDefaultLocale(void)
{
	//TODO: remove?

	Locale locale;
	return locale;
}

bool
Database::UsesDefaultLocale(const uint32& id)
{
	LOCK;

	BString command = "select accountid from accountlocale where accountid = ";
	command << id << ";";

	CppSQLite3Query query = DBQuery(command.String(), "Database::UsesDefaultLocale");
	return query.eof();

	UNLOCK;
}

bool
Database::AddTransaction(const uint32& accountid, const uint32& id, const time_t& date,
	const TransactionType& type, const char* payee, const Fixed& amount, const char* category,
	const char* memo, const uint8& status)
{
	if (!payee || !category)
		return false;

	LOCK;
	bigtime_t timestamp = real_time_clock_usecs();

	AddCategory(category, amount.IsNegative());

	BString ecategory = EscapeIllegalCharacters(category);
	BString epayee = EscapeIllegalCharacters(payee);
	BString ememo = EscapeIllegalCharacters(memo);

	BString command = "insert into transactionlist values(";
	command << timestamp << ", " << id << ", '" << ecategory << "', " << accountid << ");";
	DBCommand(command.String(), "Database::AddTransaction:insert into transactionlist");

	command = "insert into account_";
	command << accountid << " values(" << timestamp << ", " << id << ", " << date << ",'"
			<< type.Type() << "', '" << epayee << "', " << amount.AsFixed() << ", '" << ecategory;
	if (memo)
		command << "', '" << ememo << "', '";
	else
		command << "', '', ";

	if (status == TRANS_CLEARED)
		command << "cleared');";
	else if (status == TRANS_RECONCILED)
		command << "reconciled');";
	else
		command << "open');";

	DBCommand(command.String(), "Database::AddTransaction:insert into account");

	Account* account = AccountByID(accountid);
	if (account) {
		if (account->CurrentTransaction() == 0) {
			// It appears that the account is empty. Make sure and, if so, set the just-added
			// transaction to the current one
			command = "select * from account_";
			command << accountid << ";";
			CppSQLite3Query query =
				DBQuery(command.String(), "Database::AddTransaction:get current transaction");
			if (query.eof()) {
				// account is empty. make it the current one.
				account->SetCurrentTransaction(id);
			}
		}
	}

	if (IsNotifying()) {
		BMessage msg;
		TransactionData data;
		GetTransaction(id, data);
		msg.AddInt32("accountid", accountid);
		msg.AddPointer("item", (void*)&data);
		Notify(WATCH_CREATE | WATCH_TRANSACTION, &msg);
	}
	UNLOCK;

	return true;
}

bool
Database::AddTransaction(TransactionData& data, const bool& newid)
{
	if (!data.IsValid())
		return false;

	LOCK;
	uint32 id = 0;

	if (data.CountCategories() == 1) {
		if (newid)
			id = NextTransactionID();
		else
			id = data.GetID();

		AddTransaction(data.GetAccount()->GetID(), id, data.Date(), data.Type(), data.Payee(),
			data.Amount(), data.NameAt(0), data.Memo(), data.Status());
	} else {
		if (newid) {
			id = NextTransactionID();
			data.SetID(id);
		}

		// We are disabling notifications for the moment so that we don't end up with
		// multiple single-category transaction entries in the transaction view.
		SetNotify(false);
		for (int32 i = 0; i < data.CountCategories() - 1; i++) {
			// We have to be careful here because we have the potential to be adding
			// multiple categories.
			AddTransaction(data.GetAccount()->GetID(), id, data.Date(), data.Type(), data.Payee(),
				data.AmountAt(i), data.NameAt(i), data.MemoAt(i), data.Status());
		}

		// We now re-enable notifications and add the final transaction entry. This will
		// cause everyone to be notified of what really is just 1 transaction that has
		// multiple categories
		SetNotify(true);
		int32 index = data.CountCategories() - 1;
		AddTransaction(data.GetAccount()->GetID(), id, data.Date(), data.Type(), data.Payee(),
			data.AmountAt(index), data.NameAt(index), data.MemoAt(index), data.Status());
	}
	UNLOCK;
	return true;
}

bool
Database::RemoveTransaction(const uint32& transid)
{
	if (!HasTransaction(transid))
		return false;

	// Timestamps are not used. They are used as unique identifiers for each transaction,
	// but transid's are not unique. This is because a transaction that the user has entered
	// in may be split across multiple categories and the transid is what links them all together.

	LOCK;
	DStringList stringlist;
	BString command;
	BObjectList<uint32> idlist(20, true);

	command << "select accountid from transactionlist where transid = " << transid << ";";
	CppSQLite3Query query =
		DBQuery(command.String(), "Database::RemoveTransaction:find transaction");

	while (!query.eof()) {
		idlist.AddItem(new uint32(query.getInt64Field(0)));
		query.nextRow();
	}

	query.finalize();

	if (idlist.CountItems() < 1) {
		UNLOCK;
		return false;
	}

	for (int32 i = 0; i < idlist.CountItems(); i++) {
		uint32 accountid = *(idlist.ItemAt(i));

		BMessage msg;
		msg.AddInt32("accountid", accountid);
		msg.AddInt32("id", transid);
		Notify(WATCH_TRANSACTION | WATCH_DELETE, &msg);

		command = "delete from account_";
		command << accountid << " where transid = " << transid << ";";
		DBCommand(command.String(), "Database::RemoveTransaction:delete from account");

		// determine if the account is empty and set the current to -1 if it is.
		Account* account = AccountByID(accountid);
		if (account) {
			command = "select * from account_";
			command << accountid << ";";
			query = DBQuery(command.String(), "Database::RemoveTransaction:check account empty");
			if (query.eof())
				account->SetCurrentTransaction(-1);
			query.finalize();
		}
	}

	command = "delete from transactionlist where transid = ";
	command << transid << ";";
	DBCommand(command.String(), "Database::RemoveTransaction:delete from transactionlist");

	UNLOCK;
	return true;
}

uint32
Database::NextTransactionID(void)
{
	LOCK;
	uint32 key = GetLastKey("transactionlist", "transid");
	key++;
	UNLOCK;
	return key;
}

bool
Database::HasTransaction(const uint32& transid)
{
	if (transid < 0)
		return false;

	LOCK;
	BString command;
	command << "select transid from transactionlist where transid = " << transid << ";";
	CppSQLite3Query query = DBQuery(command.String(), "Database::HasTransaction");

	bool value = !query.eof();
	UNLOCK;
	return value;
}

bool
Database::GetTransaction(const uint32& transid, const uint32& accountid, TransactionData& data)
{
	LOCK;

	BString command;
	CppSQLite3Query query;

	// Make sure the account exists. We might not have all tables from account_0 to account_N,
	// if an account was deleted.
	command << "SELECT accountid FROM accountlist WHERE accountid = " << accountid << ";";
	query = DBQuery(command.String(), "Database::GetTransaction: check for account table");
	if (query.eof()) {
		UNLOCK;
		return false;
	}

	command = "select date,payee,amount,category,memo,type,timestamp from account_";
	command << accountid << " where transid = " << transid << ";";
	query = DBQuery(command.String(), "Database::GetTransaction:get transaction data");

	if (query.eof()) {
		UNLOCK;
		return false;
	}

	Locale loc = LocaleForAccount(accountid);

	data.MakeEmpty();
	data.SetAccount(AccountByID(accountid));
	data.SetDate(atol(query.getStringField(0)));
	data.SetPayee(DeescapeIllegalCharacters(query.getStringField(1)).String());
	data.SetType(DeescapeIllegalCharacters(query.getStringField(5)).String());
	data.SetID(transid);
	while (!query.eof()) {
		Fixed f;
		f.SetPremultiplied(atol(query.getStringField(2)));

		data.AddCategory(DeescapeIllegalCharacters(query.getStringField(3)).String(), f, true);

		if (!query.fieldIsNull(4))
			data.SetMemoAt(data.CountCategories() - 1, query.getStringField(4));

		query.nextRow();
	}

	if ((data.CountCategories() == 1) && strlen(data.MemoAt(0)) > 0)
		data.SetMemo(DeescapeIllegalCharacters(data.MemoAt(0)));

	UNLOCK;
	return true;
}

bool
Database::GetTransaction(const uint32& transid, TransactionData& data)
{
	LOCK;

	BString command;
	CppSQLite3Query query;

	command << "select accountid from transactionlist where transid = " << transid << ";";
	query = DBQuery(command.String(), "Database::GetTransaction:get accountid");

	uint32 accountid = query.getIntField(0);
	query.finalize();
	UNLOCK;

	return GetTransaction(transid, accountid, data);
}

void
Database::SetTransactionStatus(const uint32& transid, const uint8& status)
{
	LOCK;

	BString command;
	command << "select accountid from transactionlist where transid = " << transid << ";";
	CppSQLite3Query query =
		DBQuery(command.String(), "Database::SetTransactionStatus:get accountid");

	uint32 accountid = query.getIntField(0);
	query.finalize();

	command = "update account_";
	command << accountid << " set status = ";

	if (status == TRANS_OPEN)
		command << "'Open' ";
	else if (status == TRANS_RECONCILED)
		command << "'Reconciled' ";
	else
		command << "'Cleared'";

	command << "where transid = " << transid << ";";
	DBCommand(command.String(), "Database::SetTransactionStatus:set status");

	BMessage msg;
	TransactionData data;
	GetTransaction(transid, data);
	msg.AddInt32("accountid", accountid);
	msg.AddPointer("item", (void*)&data);
	Notify(WATCH_CHANGE | WATCH_TRANSACTION, &msg);

	UNLOCK;
}

bool
Database::GetTransferCounterpart(const uint32& transid, TransactionData& data)
{
	LOCK;

	BString command;
	command << "select accountid from transactionlist where transid = " << transid << " and "
			<< "accountid != " << data.GetAccount()->GetID() << ";";
	CppSQLite3Query query =
		DBQuery(command.String(), "Database::SetTransferCounterpart:get accountid");

	uint32 accountid = query.getIntField(0);
	query.finalize();

	UNLOCK;

	data.SetAccount(AccountByID(accountid));

	return GetTransaction(transid, data);
}

void
Database::AddScheduledTransaction(const ScheduledTransData& data, const bool& newid)
{
	if (!data.IsValid())
		return;

	LOCK;

	uint32 id;
	if (newid) {
		id = GetLastKey("scheduledlist", "transid");
		id++;
	} else
		id = data.GetID();

	time_t nextdate = data.GetNextDueDate();
	if (nextdate == 0) {
		switch (data.GetInterval()) {
			case SCHEDULED_MONTHLY:
			{
				nextdate = IncrementDateByMonth(data.Date());
				break;
			}
			case SCHEDULED_WEEKLY:
			{
				// TODO: Add weekly scheduling support
				//				data.SetNextDueDate(IncrementDateByMonth(data.Date()));
				ShowBug(
					"Unimplemented Weekly scheduling support in "
					"Database::AddScheduledTransaction()");
				break;
			}
			case SCHEDULED_QUARTERLY:
			{
				nextdate = IncrementDateByQuarter(data.Date());
				break;
			}
			case SCHEDULED_ANNUALLY:
			{
				nextdate = IncrementDateByYear(data.Date());
				break;
			}
			default:
			{
				ShowBug("Unknown scheduled value in Database::AddScheduledTransaction()");
				break;
			}
		}
	}

	if (data.CountCategories() == 1) {
		InsertSchedTransaction(id, data.GetAccount()->GetID(), data.Date(), data.Type(),
			data.Payee(), data.Amount(), data.NameAt(0), data.Memo(), data.GetInterval(), nextdate,
			data.GetCount());
	} else {
		// We are disabling notifications for the moment so that we don't end up with
		// multiple single-category transaction entries in the transaction view.
		SetNotify(false);
		for (int32 i = 0; i < data.CountCategories() - 1; i++) {
			// We have to be careful here because we have the potential to be adding
			// multiple categories.
			InsertSchedTransaction(id, data.GetAccount()->GetID(), data.Date(), data.Type(),
				data.Payee(), data.AmountAt(i), data.NameAt(i), data.MemoAt(i), data.GetInterval(),
				nextdate, data.GetCount());
		}

		// We now re-enable notifications and add the final transaction entry. This will
		// cause everyone to be notified of what really is just 1 transaction that has
		// multiple categories
		SetNotify(true);
		int32 index = data.CountCategories() - 1;
		InsertSchedTransaction(id, data.GetAccount()->GetID(), data.Date(), data.Type(),
			data.Payee(), data.AmountAt(index), data.NameAt(index), data.MemoAt(index),
			data.GetInterval(), nextdate, data.GetCount());
	}
	UNLOCK;
}

void
Database::RemoveScheduledTransaction(const uint32& id)
{
	LOCK;
	BString command = "delete from scheduledlist where transid = ";
	command << id << ";";
	DBCommand(command.String(), "Database::RemoveScheduledTransactionStatus");

	// TODO: add notification?

	UNLOCK;
}

bool
Database::GetScheduledTransaction(const uint32& transid, ScheduledTransData& data)
{
	LOCK;

	BString command;
	CppSQLite3Query query;

	command =
		"select accountid,date,payee,amount,category,memo,type,nextdate,"
		"count,interval from scheduledlist where transid = ";
	command << transid << ";";
	query = DBQuery(command.String(), "Database::GetScheduledTransaction:get transaction data");

	if (query.eof()) {
		UNLOCK;
		return false;
	}

	data.MakeEmpty();
	data.SetAccount(AccountByID(query.getIntField(0)));
	Locale loc = LocaleForAccount(data.GetAccount()->GetID());

	data.SetID(transid);
	data.SetDate(query.getInt64Field(1));
	data.SetPayee(DeescapeIllegalCharacters(query.getStringField(2)).String());
	data.SetType(DeescapeIllegalCharacters(query.getStringField(6)).String());
	data.SetNextDueDate(query.getInt64Field(7));
	data.SetCount(query.getIntField(8));
	data.SetInterval((TransactionInterval)query.getIntField(9));
	while (!query.eof()) {
		Fixed f;
		f.SetPremultiplied(atol(query.getStringField(3)));

		data.AddCategory(DeescapeIllegalCharacters(query.getStringField(4)).String(), f, true);

		if (!query.fieldIsNull(4))
			data.SetMemoAt(data.CountCategories() - 1, query.getStringField(5));

		query.nextRow();
	}

	if ((data.CountCategories() == 1) && strlen(data.MemoAt(0)) > 0)
		data.SetMemo(data.MemoAt(0));

	UNLOCK;
	return true;
}

uint32
Database::CountScheduledTransactions(void)
{
	CppSQLite3Query query = gDatabase.DBQuery("select count(*) from scheduledlist",
		"ScheduleListView::RefreshScheduleList: count transactions");
	if (query.eof())
		return 0;

	return query.getInt64Field(0);
}

bool
Database::InsertSchedTransaction(const uint32& id, const uint32& accountid, const time_t& startdate,
	const TransactionType& type, const char* payee, const Fixed& amount, const char* category,
	const char* memo, const TransactionInterval& interval, const time_t& nextdate,
	const int32& count)
{
	// Internal method. No locking required
	if (!payee || !category)
		return false;

	LOCK;
	bigtime_t timestamp = real_time_clock_usecs();

	BString ecategory = EscapeIllegalCharacters(category);
	BString epayee = EscapeIllegalCharacters(payee);
	BString ememo = EscapeIllegalCharacters(memo);

	BString command = "insert into scheduledlist values(";
	command << timestamp << ", " << accountid << ", " << id << ", " << startdate << ",'"
			<< type.Type() << "', '" << epayee << "', " << amount.AsFixed() << ", '" << ecategory;
	if (memo)
		command << "', '" << ememo << "', ";
	else
		command << "', '', ";

	command << (int)interval << ", " << count << ", " << nextdate << ");";

	DBCommand(command.String(), "Database::InsertSchedTransaction:insert into table");

	return true;
}

int32
Database::GetLastKey(const char* table, const char* column)
{
	// Internal method. No locking required
	if (!table || !column)
		return B_ERROR;

	BString command;
	command << "select " << column << " from " << table << " order by 1;";
	CppSQLite3Query query = DBQuery(command.String(), "Database::GetLastKey");

	int value = B_ERROR;
	while (!query.eof()) {
		value = query.getIntField(0);
		query.nextRow();
	}
	return value;
}

void
Database::AddCategory(const char* name, const bool& isexpense)
{
	if (!name || HasCategory(name))
		return;

	if (strcasecmp(name, "split") == 0)
		return;

	BString command("insert into categorylist values('");
	command << EscapeIllegalCharacters(name) << "', " << (isexpense ? 0 : 1) << ");";
	DBCommand(command.String(), "Database::AddCategory");
}

void
Database::RemoveCategory(const char* name)
{
	if (!name || !HasCategory(name))
		return;

	BString command("delete from categorylist where name = '");
	command << EscapeIllegalCharacters(name) << "';";
	DBCommand(command.String(), "Database::RemoveCategory");
}

bool
Database::RenameCategory(const char* oldname, const char* newname)
{
	if ((!oldname && !newname) || strcmp(oldname, newname) == 0)
		return false;

	if (!HasCategory(oldname) || HasCategory(newname))
		return false;

	BString command("update categorylist set name = '");
	if (newname)
		command << EscapeIllegalCharacters(newname);

	command << "' where name = '";

	if (oldname)
		command << EscapeIllegalCharacters(oldname);

	command << "';";

	DBCommand(command.String(), "Database::RenameCategory");
	return true;
}

bool
Database::HasCategory(const char* name)
{
	BString command;
	CppSQLite3Query query;

	if (!name || strlen(name) == 0) {
		command = "select * from categorylist;";
		query = DBQuery(command.String(), "Database::HasCategory:find category");

		while (!query.eof()) {
			BString cat = query.getStringField(0);
			if (cat.CountChars() == 0)
				return true;

			query.nextRow();
		}

		return false;
	}

	command << "select name from categorylist where name = '" << EscapeIllegalCharacters(name)
			<< "';";
	query = DBQuery(command.String(), "Database::HasCategory:find category");

	return !query.eof();
}

bool
Database::IsCategoryExpense(const char* name)
{
	if (!name)
		return false;

	if (!HasCategory(name))
		ShowBug("Called IsCategoryExpense on a nonexistent category");

	BString command("select type from categorylist where name = '");
	command << EscapeIllegalCharacters(name) << "';";
	CppSQLite3Query query = DBQuery(command.String(), "Database::IsCategoryExpense");

	if (query.eof())
		ShowBug("Called IsCategoryExpense and search had no results");

	int value = query.getIntField(0);
	return (value == 0);
}

void
Database::SetCategoryExpense(const char* name, const bool& isexpense)
{
	if (!name || !HasCategory(name))
		return;

	BString command("update categorylist set type = ");
	command << (isexpense ? 0 : 1) << " where name = '" << EscapeIllegalCharacters(name) << "';";
	command << EscapeIllegalCharacters(name) << "';";
	DBCommand(command.String(), "Database::SetCategoryExpense");
}

void
Database::RecategorizeTransactions(const char* from, const char* to)
{
	if (!HasCategory(from))
		return;

	if (!HasCategory(to))
		AddCategory(to, IsCategoryExpense(from));

	Notify(WATCH_MASS_EDIT, NULL);

	BString command;
	for (int32 i = 0; i < CountAccounts(); i++) {
		Account* acc = AccountAt(i);
		if (!acc)
			continue;

		command = "update account_";
		command << acc->GetID() << " set category = '";

		if (to)
			command << EscapeIllegalCharacters(to);
		command << "' where category = '";

		if (from)
			command << EscapeIllegalCharacters(from);
		command << "';";
		DBCommand(command.String(), "Database::RecategorizeTransactions");
	}

	Notify(WATCH_MASS_EDIT, NULL);
}

BString
AccountTypeToString(const AccountType& type)
{
	switch (type) {
		case ACCOUNT_BANK:
			return BString("Bank");
		case ACCOUNT_CREDIT:
			return BString("Credit");
		default:
			return BString("Unknown");
	}
}

// This will prevent SQL injection attacks

static const char* sIllegalCharacters[] = {"!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "-",
	"+", "=", "{", "}", "[", "]", "\\", "|", ";", ":", "'", "\"", "<", ">", ",", ".", "/", "?", "`",
	"~", " ", NULL};
static const char* sReplacementCharacters[] = {"£21£", "£40£", "£23£", "£24£", "£25£", "£5e£",
	"£26£", "£2a£", "£28£", "£29£", "£2d£", "£2b£", "£3d£", "£7b£", "£7d£", "£5b£", "£5d£", "£5c£",
	"£7c£", "£3b£", "£3a£", "£27£", "£22£", "£3c£", "£3e£", "£2c£", "£2e£", "£2f£", "£3f£", "£60£",
	"£7e£", "£20£", NULL};

static const char* sIllegalWords[] = {" select ", " drop ", " create ", " delete ", " where ",
	" update ", " order ", " by ", " and ", " or ", " in ", " between ", " aliases ", " join ",
	" union ", " alter ", " functions ", " group ", " into ", " view ", NULL};
static const char* sReplacementWords[] = {" ¥select ", " ¥drop ", " ¥create ", " ¥delete ",
	" ¥where ", " ¥update ", " ¥order ", " ¥by ", " ¥and ", " ¥or ", " ¥in ", " ¥between ",
	" ¥aliases ", " ¥join ", " ¥union ", " ¥alter ", " ¥functions ", " ¥group ", " ¥into ",
	" ¥view ", NULL};

BString
EscapeIllegalCharacters(const char* instr)
{
	// Because the £ symbol isn't allowed in a category but is a valid database character,
	// we'll use it as the escape character for illegal characters

	BString string(instr);
	if (string.CountChars() < 1)
		return string;

	string.RemoveAll("£");
	string.RemoveAll("¥");

	int32 i = 0;
	while (sIllegalCharacters[i]) {
		string.ReplaceAll(sIllegalCharacters[i], sReplacementCharacters[i]);
		i++;
	}

	// Just to make sure that reserved words aren't used, we'll prefix them with the ¥ character
	// for the same reasons that we used £ with bad characters
	i = 0;
	while (sIllegalWords[i]) {
		string.ReplaceAll(sIllegalWords[i], sReplacementWords[i]);
		i++;
	}
	return string;
}

BString
DeescapeIllegalCharacters(const char* instr)
{
	BString string(instr);
	if (string.CountChars() < 1)
		return string;

	int32 i = 0;
	while (sIllegalCharacters[i]) {
		string.ReplaceAll(sReplacementCharacters[i], sIllegalCharacters[i]);
		i++;
	}

	// Just to make sure that reserved words aren't used, we'll prefix them with the ¥ character
	// for the same reasons that we used £ with bad characters
	i = 0;
	while (sIllegalWords[i]) {
		string.ReplaceAll(sReplacementWords[i], sIllegalWords[i]);
		i++;
	}
	return string;
}

void
Database::DBCommand(const char* command, const char* functionname)
{
	if (!command)
		ShowBug("NULL database command in Database::DBCommand");
	if (!functionname)
		ShowBug("NULL function name in Database::DBCommand");

	try {
		fDB.execDML(command);
	} catch (CppSQLite3Exception& e) {
		BString msg("Database Exception in ");
		msg << functionname << ".\n\n"
			<< e.errorMessage() << "\n\nDatabase Exception Command: " << command << "\n";
		printf("%s\n", msg.String());
		ShowBug(msg.String());
	}
}

CppSQLite3Query
Database::DBQuery(const char* query, const char* functionname)
{
	if (!query)
		ShowBug("NULL database command in Database::DBQuery");
	if (!functionname)
		ShowBug("NULL function name in Database::DBQuery");

	try {
		return fDB.execQuery(query);
	} catch (CppSQLite3Exception& e) {
		BString msg("Database Exception in ");
		msg << functionname << ".\n\n"
			<< e.errorMessage() << "\n\nDatabase Exception Query: " << query << "\n";
		printf("%s\n", msg.String());
		ShowBug(msg.String());
	}
	// this will never be reached - just to shut up the compiler
	return CppSQLite3Query();
}
