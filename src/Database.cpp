/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	bitigchi (Emir Sari)
 *	humdinger (Joachim Seemer)
 *	dospuntos (Johan Wagenheim)
 */
#include "Database.h"

#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include "Account.h"
#include "Budget.h"
#include "DStringList.h"
#include "Fixed.h"
#include "Import.h"
#include "Preferences.h"
#include "ScheduledTransData.h"
#include "TimeSupport.h"
#include "TransactionData.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <vector>

// #define LOCK_DATABASE
#ifdef LOCK_DATABASE
#define LOCK fLocker.Lock()
#define UNLOCK fLocker.Unlock()
#else
#define LOCK ;
#define UNLOCK ;
#endif

#define DB_VERSION 1

// This is the default locale. It uses the system settings from the Haiku locale kit
// for currency and date formatting.
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


Database::~Database() {}


void
Database::CloseAccount(Account* item)
{
	if (!item)
		return;

	LOCK;
	BString command;
	command << "UPDATE accountlist SET status = 'Closed' WHERE accountid = " << item->GetID()
			<< ";";
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
		CppSQLite3Buffer bufSQL;
		bufSQL.format(
			"UPDATE accountlist SET name = %Q WHERE accountid = %i;", name, item->GetID());
		DBCommand(bufSQL, "Database::RenameAccount");

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
	BString command;
	command << "UPDATE accountlist SET status = 'Open' WHERE accountid = " << item->GetID() << ";";
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

	// Clear account list
	fList.MakeEmpty();
	fCurrent = NULL;

	// Character-Entry Limits:
	// Category names: 32
	// Account names: 32
	// Currency/Date separators: 2
	// Transaction Status: 10
	// Currency Symbol/Decimal: 2

	DBCommand("CREATE TABLE db_version (version INTEGER PRIMARY KEY);",
		"DatabaseCreateFile:create db_version");
	BString command;
	command << "INSERT INTO db_version (version) VALUES (" << DB_VERSION << ");";
	DBCommand(command.String(), "Database::CreateFile:create db_version");

	DBCommand(
		"CREATE TABLE accountlist (accountid INT PRIMARY KEY, name VARCHAR(96), "
		"type VARCHAR(12), status VARCHAR(30));",
		"Database::CreateFile:create accountlist");
	DBCommand(
		"CREATE TABLE accountlocale (accountid INT PRIMARY KEY, "
		"currencysymbol CHAR(6), currencydecimal CHAR(6), currencyprefix CHAR(1));",
		"Database::CreateFile:create accountlocale");
	DBCommand("CREATE TABLE memorizedlist (transactionid INT);",
		"Database::CreateFile:create memorizedlist");
	DBCommand(
		"CREATE TABLE scheduledlist (timestamp INT PRIMARY KEY, accountid INT, transid INT,"
		"date INT, type VARCHAR(24), payee VARCHAR(96), amount INT,"
		"category VARCHAR(96),memo VARCHAR(63), interval INT, count INT,"
		"nextdate INT, destination INT);",
		"Database::CreateFile:create scheduledlist");
	DBCommand(
		"CREATE TABLE budgetlist (entryid INT PRIMARY KEY, category VARCHAR(96), "
		"amount INT, period INT(2), isexpense INT(1));",
		"Database::CreateFile:create budgetlist");
	DBCommand(
		"CREATE TABLE transactionlist (timestamp INT PRIMARY KEY, transid INT, "
		"category VARCHAR(96), accountid INT);",
		"Database::CreateFile:create transactionlist");
	DBCommand("CREATE TABLE categorylist (name VARCHAR(96), type INT(2));",
		"Database::CreateFile:create categorylist");
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

	// Check for and apply DB migrations
	if (_ApplyMigrations() != B_OK) {
		UNLOCK;
		ShowBug("Database::ApplyMigrations() failed");
		return B_ERROR;
	}

	// Clear account list
	fList.MakeEmpty();
	fCurrent = NULL;

	// Populate account list
	CppSQLite3Query query
		= DBQuery("SELECT * FROM accountlist", "Database::OpenFile:get accounts from list");

	while (!query.eof()) {
		uint32 id = query.getIntField(0);
		BString name = query.getStringField(1);
		BString status = query.getStringField(3);

		Account* account = new Account(name.String(), status.ICompare("Open") != 0);
		account->SetID(id);
		if (!UsesDefaultLocale(id)) {
			Locale accountLocale = LocaleForAccount(id);
			account->UseDefaultLocale(false);
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
		BString command;
		command << "SELECT transid FROM account_" << acc->GetID() << " ORDER BY date,transid;";
		query = DBQuery(command.String(), "Database::OpenFile:set current transaction");

		if (!query.eof())
			acc->SetCurrentTransaction(query.getInt64Field(0));
		query.finalize();
	}

	fCurrent = (Account*)fList.ItemAt(0);
	UNLOCK;
	return B_OK;
}


void
Database::CloseFile()
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
	if (index < 0)
		fCurrent = NULL;
	else {
		Account* acc = (Account*)fList.ItemAt(index);
		fCurrent = acc;
		gCurrentLocale = acc->GetLocale();
	}
	BMessage msg;
	msg.AddPointer("item", (void*)fCurrent);
	Notify(WATCH_SELECT | WATCH_ACCOUNT, &msg);
	UNLOCK;

	return fCurrent;
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

	int32 id = _GetLastKey("accountlist", "accountid");
	id++;

	CppSQLite3Buffer bufSQL;
	bufSQL.format("INSERT INTO accountlist VALUES(%i, %Q, %Q, %Q);", id, name,
		AccountTypeToString(type).String(), status);
	DBCommand(bufSQL, "Database::AddAccount:insert accountlist");

	if (locale != NULL)
		SetAccountLocale(id, *locale);

	BString command;
	command << "CREATE TABLE account_" << id
			<< " (timestamp INT PRIMARY KEY, transid INT, date INT, type VARCHAR(24), "
			   "payee VARCHAR(96), amount INT, category VARCHAR(96), memo VARCHAR(63), "
			   "status CHAR(21));";

	DBCommand(command.String(), "Database::AddAccount:create account");

	Account* account = new Account(name);
	account->SetID(id);
	account->UseDefaultLocale(locale == NULL);
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
	command.SetToFormat("SELECT accountid FROM accountlist WHERE accountid = %i;", accountid);
	CppSQLite3Query query = DBQuery(command.String(), "Database::RemoveAccount:accountid check");

	if (!query.eof()) {
		query.finalize();

		Account* item = AccountByID(accountid);
		if (item) {
			BMessage msg;
			msg.AddPointer("item", (void*)item);
			Notify(WATCH_DELETE | WATCH_ACCOUNT, &msg);
		}

		command.SetToFormat("DELETE FROM accountlist WHERE accountid = %i;", accountid);
		DBCommand(command.String(), "Database::RemoveAccount:delete accountlist item");

		command.SetToFormat("DELETE FROM accountlocale WHERE accountid = %i;", accountid);
		DBCommand(command.String(), "Database::RemoveAccount:delete accountlocale item");

		command.SetToFormat("DELETE FROM scheduledlist WHERE accountid = %i OR destination = %i;",
			accountid, accountid);
		DBCommand(command.String(), "Database::RemoveAccount:delete scheduled items");

		command.SetToFormat("DELETE FROM transactionlist WHERE accountid = %i;", accountid);
		DBCommand(command.String(), "Database::RemoveAccount:delete transactionlist items");

		command.SetToFormat("DROP TABLE account_%i;", accountid);
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
	BString category = entry.name.String();

	// See if the budget list already has the category and if it does, update the entry
	// to the new value. Otherwise, add the entry to the list
	CppSQLite3Buffer bufSQL;
	bufSQL.format("SELECT entryid FROM budgetlist WHERE category = %Q;", category.String());
	CppSQLite3Query query = DBQuery(bufSQL, "Database::AddBudgetEntry:check existing");

	int value = -1;
	if (!query.eof())
		value = query.getIntField(0);
	query.finalize();

	if (value >= 0) {
		bufSQL.format("UPDATE budgetlist SET amount = %li WHERE entryid = %i;",
			entry.amount.AsFixed(), value);
		DBCommand(bufSQL, "Database::AddBudgetEntry:update budgetlist amount");

		bufSQL.format(
			"UPDATE budgetlist SET period = %i WHERE entryid = %i;", (int)entry.period, value);
		DBCommand(bufSQL, "Database::AddBudgetEntry:update budgetlist period");
		return;
	}

	// We got this far, so we just add the entry to the list
	value = _GetLastKey("budgetlist", "entryid");
	value++;
	bufSQL.format("INSERT INTO budgetlist VALUES(%i, %Q, %li, %i, %i);", value, category.String(),
		entry.amount.AsFixed(), (int)entry.period, (entry.isexpense ? 1 : 0));
	DBCommand(bufSQL, "Database::AddBudgetEntry:insert into budgetlist");
	UNLOCK;
}


bool
Database::RemoveBudgetEntry(const char* category)
{
	if (!category || !HasBudgetEntry(category))
		return false;

	LOCK;
	CppSQLite3Buffer bufSQL;
	bufSQL.format("DELETE FROM budgetlist WHERE category = %Q;", category);
	DBCommand(bufSQL, "Database::RemoveBudgetEntry");
	UNLOCK;
	return true;
}


bool
Database::HasBudgetEntry(const char* category)
{
	if (!category)
		return false;

	LOCK;
	CppSQLite3Buffer bufSQL;
	bufSQL.format("SELECT entryid FROM budgetlist WHERE category = %Q ORDER BY 1;", category);
	CppSQLite3Query query = DBQuery(bufSQL, "Database::HasBudgetEntry");

	bool value = query.eof();
	UNLOCK;
	return value;
}


bool
Database::GetBudgetEntry(const char* name, BudgetEntry& entry)
{
	if (!name)
		return false;

	CppSQLite3Buffer bufSQL;
	bufSQL.format("SELECT amount,period,isexpense FROM budgetlist WHERE category = %Q;", name);
	CppSQLite3Query query = gDatabase.DBQuery(bufSQL, "Database::GetBudgetEntry");

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
Database::CountBudgetEntries()
{
	CppSQLite3Query query
		= gDatabase.DBQuery("SELECT COUNT(*) FROM budgetlist", "Database::CountBudgetEntries");

	if (query.eof())
		return 0;

	return query.getIntField(0);
}


void
Database::SetAccountLocale(const uint32& accountid, const Locale& data)
{
	// Check if there are any changes, return early if not
	if (gCurrentLocale == data)
		return;

	gCurrentLocale = data;

	LOCK;
	BString command;
	command << "SELECT accountid FROM accountlocale WHERE accountid = " << accountid << ";";
	CppSQLite3Query query = DBQuery(command.String(), "Database::SetAccountLocale:find accountid");

	// Todo: remove table columns for date format
	if (query.eof()) {
		command << "INSERT INTO accountlocale VALUES(" << accountid << ",'" << data.CurrencySymbol()
				<< "'," << data.CurrencyDecimalPlace() << ",'"
				<< (data.IsCurrencySymbolPrefix() ? "true" : "false") << "');";
		DBCommand(command.String(), "Database::SetAccountLocale:insert into accountlocale");
		UNLOCK;
		return;
	}

	query.finalize();

	// This already has the locale data in the table, so we'll just update it and return
	command = "";
	command << "UPDATE accountlocale SET "
			<< "currencysymbol = '" << data.CurrencySymbol() << "', "
			<< "currencydecimal = '" << data.CurrencyDecimalPlace() << "', "
			<< "currencyprefix = '" << (data.IsCurrencySymbolPrefix() ? "true" : "false") << "' "
			<< "WHERE accountid = " << accountid << ";";
	DBCommand(command.String(), "Database::SetAccountLocale:update all fields");
	UNLOCK;
}


Locale
Database::LocaleForAccount(const uint32& id)
{
	LOCK;
	BString command;
	command << "SELECT * FROM accountlocale WHERE accountid = " << id << ";";
	CppSQLite3Query query = DBQuery(command.String(), "Database::LocaleForAccount:find account");

	Locale locale;
	if (query.eof()) {
		UNLOCK;
		return locale;
	}

	locale.SetCurrencySymbol(query.getStringField(1));
	locale.SetCurrencyDecimalPlace(query.getIntField(2));
	BString isCurrencyPrefix = query.getStringField(3);
	locale.SetCurrencySymbolPrefix(isCurrencyPrefix.Compare("true") == 0 ? true : false);
	UNLOCK;
	return locale;
}


bool
Database::UsesDefaultLocale(const uint32& id)
{
	LOCK;
	BString command;
	command << "SELECT accountid FROM accountlocale WHERE accountid = " << id << ";";

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

	CppSQLite3Buffer bufSQL;
	bufSQL.format(
		"INSERT INTO transactionlist VALUES(%" B_PRIdBIGTIME ", %i, %Q, %i);", timestamp, id, category, accountid);
	DBCommand(bufSQL, "Database::AddTransaction:insert into transactionlist");

	BString _status, command;
	BString _memo = memo;

	if (status == TRANS_CLEARED)
		_status << "cleared";
	else if (status == TRANS_RECONCILED)
		_status << "reconciled";
	else
		_status << "open";

	bufSQL.format("INSERT INTO account_%i VALUES(%" B_PRIdBIGTIME ", %i, %" B_PRIdTIME ", %Q, %Q, %li, %Q, %Q, %Q);",
		accountid, timestamp, id, date, type.Type(), payee, amount.AsFixed(), category, memo,
		_status.String());

	DBCommand(bufSQL, "Database::AddTransaction:insert into account");

	Account* account = AccountByID(accountid);
	if (account) {
		if (account->CurrentTransaction() == 0) {
			// It appears that the account is empty. Make sure and, if so, set the just-added
			// transaction to the current one
			command = "SELECT * FROM account_";
			command << accountid << ";";
			CppSQLite3Query query
				= DBQuery(command.String(), "Database::AddTransaction:get current transaction");
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
	if (newid)
		id = NextTransactionID();
	else
		id = data.GetID();

	if (data.CountCategories() == 1) {
		AddTransaction(data.GetAccount()->GetID(), id, data.Date(), data.Type(), data.Payee(),
			data.Amount(), data.NameAt(0), data.Memo(), data.Status());
	} else {
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

	command << "SELECT accountid FROM transactionlist WHERE transid = " << transid << ";";
	CppSQLite3Query query
		= DBQuery(command.String(), "Database::RemoveTransaction:find transaction");

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

		command = "DELETE FROM account_";
		command << accountid << " WHERE transid = " << transid << ";";
		DBCommand(command.String(), "Database::RemoveTransaction:delete from account");

		// determine if the account is empty and set the current to -1 if it is.
		Account* account = AccountByID(accountid);
		if (account) {
			command = "SELECT * FROM account_";
			command << accountid << ";";
			query = DBQuery(command.String(), "Database::RemoveTransaction:check account empty");
			if (query.eof())
				account->SetCurrentTransaction(-1);
			query.finalize();
		}
	}

	command = "DELETE FROM transactionlist WHERE transid = ";
	command << transid << ";";
	DBCommand(command.String(), "Database::RemoveTransaction:delete from transactionlist");

	UNLOCK;
	return true;
}


uint32
Database::NextTransactionID()
{
	LOCK;
	uint32 key = _GetLastKey("transactionlist", "transid");
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
	command << "SELECT transid FROM transactionlist WHERE transid = " << transid << ";";
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

	command = "SELECT date,payee,amount,category,memo,type,timestamp FROM account_";
	command << accountid << " WHERE transid = " << transid << ";";
	query = DBQuery(command.String(), "Database::GetTransaction:get transaction data");

	if (query.eof()) {
		UNLOCK;
		return false;
	}

	Locale loc = LocaleForAccount(accountid);

	data.MakeEmpty();
	data.SetAccount(AccountByID(accountid));
	data.SetDate(atol(query.getStringField(0)));
	data.SetPayee(query.getStringField(1));
	data.SetType(query.getStringField(5));
	data.SetID(transid);
	while (!query.eof()) {
		Fixed f;
		f.SetPremultiplied(atol(query.getStringField(2)));

		data.AddCategory(query.getStringField(3), f, true);

		if (!query.fieldIsNull(4))
			data.SetMemoAt(data.CountCategories() - 1, query.getStringField(4));

		query.nextRow();
	}

	if ((data.CountCategories() == 1) && strlen(data.MemoAt(0)) > 0)
		data.SetMemo(data.MemoAt(0));

	UNLOCK;
	return true;
}


bool
Database::GetTransaction(const uint32& transid, TransactionData& data)
{
	LOCK;

	BString command;
	CppSQLite3Query query;

	command << "SELECT accountid FROM transactionlist WHERE transid = " << transid << ";";
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
	command << "SELECT accountid FROM transactionlist WHERE transid = " << transid << ";";
	CppSQLite3Query query
		= DBQuery(command.String(), "Database::SetTransactionStatus:get accountid");

	uint32 accountid = query.getIntField(0);
	query.finalize();

	command = "UPDATE account_";
	command << accountid << " SET status = ";

	if (status == TRANS_OPEN)
		command << "'Open' ";
	else if (status == TRANS_RECONCILED)
		command << "'Reconciled' ";
	else
		command << "'Cleared'";

	command << "WHERE transid = " << transid << ";";
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
	command << "SELECT accountid FROM transactionlist WHERE transid = " << transid << " AND "
			<< "accountid != " << data.GetAccount()->GetID() << ";";
	CppSQLite3Query query
		= DBQuery(command.String(), "Database::SetTransferCounterpart:get accountid");

	uint32 accountid = query.getIntField(0);
	query.finalize();

	UNLOCK;

	data.SetAccount(AccountByID(accountid));

	return GetTransaction(transid, data);
}


int32
Database::GetTransferDestination(const uint32& transid, const uint32& accountid)
{
	LOCK;

	BString command;
	command.SetToFormat(
		"SELECT accountid FROM transactionlist WHERE transid = %i AND "
		"accountid != %i;",
		transid, accountid);
	CppSQLite3Query query
		= DBQuery(command.String(), "Database::GetTransferDestination:get accountid");

	if (query.eof())
		return -1;

	uint32 destination = query.getIntField(0);
	query.finalize();

	UNLOCK;

	return destination;
}


void
Database::AddScheduledTransaction(const ScheduledTransData& data, const bool& newid)
{
	if (!data.IsValid())
		return;

	LOCK;

	uint32 id;
	if (newid) {
		id = _GetLastKey("scheduledlist", "transid");
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
		_InsertSchedTransaction(id, data.GetAccount()->GetID(), data.Date(), data.Type(),
			data.Payee(), data.Amount(), data.NameAt(0), data.Memo(), data.GetInterval(), nextdate,
			data.GetCount(), data.GetDestination());
	} else {
		// We are disabling notifications for the moment so that we don't end up with
		// multiple single-category transaction entries in the transaction view.
		SetNotify(false);
		for (int32 i = 0; i < data.CountCategories() - 1; i++) {
			// We have to be careful here because we have the potential to be adding
			// multiple categories.
			_InsertSchedTransaction(id, data.GetAccount()->GetID(), data.Date(), data.Type(),
				data.Payee(), data.AmountAt(i), data.NameAt(i), data.MemoAt(i), data.GetInterval(),
				nextdate, data.GetCount(), data.GetDestination());
		}

		// We now re-enable notifications and add the final transaction entry. This will
		// cause everyone to be notified of what really is just 1 transaction that has
		// multiple categories
		SetNotify(true);
		int32 index = data.CountCategories() - 1;
		_InsertSchedTransaction(id, data.GetAccount()->GetID(), data.Date(), data.Type(),
			data.Payee(), data.AmountAt(index), data.NameAt(index), data.MemoAt(index),
			data.GetInterval(), nextdate, data.GetCount(), data.GetDestination());
	}
	UNLOCK;
}


void
Database::RemoveScheduledTransaction(const uint32& id)
{
	LOCK;
	BString command = "DELETE FROM scheduledlist WHERE transid = ";
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

	command
		= "SELECT accountid,date,payee,amount,category,memo,type,nextdate,"
		  "count,interval,destination FROM scheduledlist WHERE transid = ";
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
	data.SetPayee(query.getStringField(2));
	data.SetType(query.getStringField(6));
	data.SetNextDueDate(query.getInt64Field(7));
	data.SetCount(query.getIntField(8));
	data.SetInterval((TransactionInterval)query.getIntField(9));
	data.SetDestination(query.getInt64Field(10));
	while (!query.eof()) {
		Fixed f;
		f.SetPremultiplied(atol(query.getStringField(3)));

		data.AddCategory(query.getStringField(4), f, true);

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
Database::CountScheduledTransactions()
{
	CppSQLite3Query query = gDatabase.DBQuery("SELECT COUNT(*) FROM scheduledlist",
		"ScheduleListView::RefreshScheduleList: count transactions");
	if (query.eof())
		return 0;

	return query.getInt64Field(0);
}


uint32
Database::CountScheduledTransactions(int accountid)
{
	BString command;
	command.SetToFormat(
		"SELECT COUNT(*) FROM scheduledlist WHERE accountid = %i OR destination = %i;", accountid,
		accountid);
	CppSQLite3Query query
		= gDatabase.DBQuery(command.String(), "Database::CountScheduledTransactions");

	if (query.eof())
		return 0;

	return query.getInt64Field(0);
}


bool
Database::_InsertSchedTransaction(const uint32& id, const uint32& accountid,
	const time_t& startdate, const TransactionType& type, const char* payee, const Fixed& amount,
	const char* category, const char* memo, const TransactionInterval& interval,
	const time_t& nextdate, const int32& count, const int32& destination)
{
	// Internal method. No locking required
	if (!payee || !category)
		return false;

	LOCK;
	bigtime_t timestamp = real_time_clock_usecs();

	CppSQLite3Buffer bufSQL;
	BString _memo = memo;
	bufSQL.format(
		"INSERT INTO scheduledlist VALUES(%" B_PRIdBIGTIME ", %u, %u, %" B_PRIdTIME ", %Q, %Q, %ld, %Q, %Q, %u, %d, %ld, %i);",
		timestamp, accountid, id, startdate, type.Type(), payee, amount.AsFixed(), category,
		_memo.String(), interval, count, nextdate, destination);
	CppSQLite3Query query
		= gDatabase.DBQuery(bufSQL, "Database::InsertSchedTransaction:insert into table");
	// TODO: Implement destination for transfers between accounts (WIP)

	return true;
}


int32
Database::_GetLastKey(const char* table, const char* column)
{
	// Internal method. No locking required
	if (!table || !column)
		return B_ERROR;

	BString command;
	command << "SELECT " << column << " FROM " << table << " ORDER BY 1;";
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

	if (strcasecmp(name,
			B_TRANSLATE_ALL("Split", "CommonTerms", "The noun 'split', as in 'a split-category'"))
		== 0) {
		return;
	}

	CppSQLite3Buffer bufSQL;
	bufSQL.format("INSERT INTO categorylist VALUES(%Q, %i)", name, (isexpense ? 0 : 1));
	CppSQLite3Query query = gDatabase.DBQuery(bufSQL, "Database::AddCategory");
}


void
Database::RemoveCategory(const char* name)
{
	if (!name || !HasCategory(name))
		return;

	CppSQLite3Buffer bufSQL;
	bufSQL.format("DELETE FROM categorylist WHERE name = %Q;", name);
	CppSQLite3Query query = gDatabase.DBQuery(bufSQL, "Database::RemoveCategory");
}


bool
Database::RenameCategory(const char* oldname, const char* newname)
{
	if (oldname == NULL || newname == NULL)
		return false;

	if (strcmp(oldname, newname) == 0)
		return false;

	if (!HasCategory(oldname) || HasCategory(newname))
		return false;

	CppSQLite3Buffer bufSQL;
	bufSQL.format("UPDATE categorylist SET name = %Q WHERE name = %Q", newname, oldname);
	CppSQLite3Query query = gDatabase.DBQuery(bufSQL, "Database::RenameCategory");

	return true;
}


bool
Database::HasCategory(const char* name)
{
	BString command;
	CppSQLite3Query query;

	if (!name || strlen(name) == 0) {
		command = "SELECT * FROM categorylist;";
		query = DBQuery(command.String(), "Database::HasCategory:find category");

		while (!query.eof()) {
			BString cat = query.getStringField(0);
			if (cat.CountChars() == 0)
				return true;

			query.nextRow();
		}

		return false;
	}

	CppSQLite3Buffer bufSQL;
	bufSQL.format("SELECT name FROM categorylist WHERE name = %Q", name);
	query = gDatabase.DBQuery(bufSQL, "Database::HasCategory:find category");

	return !query.eof();
}


bool
Database::IsCategoryExpense(const char* name)
{
	if (!name)
		return false;

	if (!HasCategory(name))
		ShowBug("Called IsCategoryExpense on a nonexistent category");

	CppSQLite3Buffer bufSQL;
	bufSQL.format("SELECT type FROM categorylist WHERE name = %Q", name);
	CppSQLite3Query query = gDatabase.DBQuery(bufSQL, "Database::IsCategoryExpense");

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

	CppSQLite3Buffer bufSQL;
	bufSQL.format("UPDATE categorylist SET type = %i WHERE name = %Q;", (isexpense ? 0 : 1), name);
	gDatabase.DBCommand(bufSQL, "Database::SetCategoryExpense");
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

		CppSQLite3Buffer bufSQL;
		BString account;
		account << "account_" << acc->GetID();
		bufSQL.format(
			"UPDATE %s SET category = %Q WHERE category = %Q;", account.String(), to, from);
		gDatabase.DBCommand(bufSQL, "Database::RecategorizeTransactions");
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

static const char* sIllegalCharacters[]
	= {"!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "-", "+", "=", "{", "}", "[", "]", "\\",
		"|", ";", ":", "'", "\"", "<", ">", ",", ".", "/", "?", "`", "~", " ", NULL};
static const char* sReplacementCharacters[]
	= {"£21£", "£40£", "£23£", "£24£", "£25£", "£5e£", "£26£", "£2a£", "£28£", "£29£", "£2d£",
		"£2b£", "£3d£", "£7b£", "£7d£", "£5b£", "£5d£", "£5c£", "£7c£", "£3b£", "£3a£", "£27£",
		"£22£", "£3c£", "£3e£", "£2c£", "£2e£", "£2f£", "£3f£", "£60£", "£7e£", "£20£", NULL};

static const char* sIllegalWords[] = {" select ", " drop ", " create ", " delete ", " where ",
	" update ", " order ", " by ", " and ", " or ", " in ", " between ", " aliases ", " join ",
	" union ", " alter ", " functions ", " group ", " into ", " view ", NULL};
static const char* sReplacementWords[]
	= {" ¥select ", " ¥drop ", " ¥create ", " ¥delete ", " ¥where ", " ¥update ", " ¥order ",
		" ¥by ", " ¥and ", " ¥or ", " ¥in ", " ¥between ", " ¥aliases ", " ¥join ", " ¥union ",
		" ¥alter ", " ¥functions ", " ¥group ", " ¥into ", " ¥view ", NULL};


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


status_t
Database::_ApplyMigrations()
{
	int currentVersion = 0;

	// Look for db_version, create if it doesn't exist. Set version to 0,
	// all migrations will be applied.
	if (!fDB.tableExists("db_version")) {
		if (_CreateDBBackup(currentVersion) != B_OK)
			return B_ERROR;

		DBCommand("CREATE TABLE db_version (version INTEGER PRIMARY KEY);",
			"Database::ApplyMigrations:create db_version");
		DBCommand("INSERT INTO db_version (version) VALUES (0);",
			"Database::ApplyMigrations:insert db_version");
	} else {
		// Get current db_version
		CppSQLite3Query query = DBQuery(
			"SELECT version FROM db_version", "Database::ApplyMigrations: get currentVersion");

		if (query.eof())
			return B_ERROR;

		currentVersion = query.getIntField(0);
	}

	// Apply all missing migrations
	if (currentVersion < 1) {
		// Remove unused tables from accountlocale
		DBCommand("ALTER TABLE accountlocale DROP COLUMN dateformat;",
			"Database::ApplyMigrations: accountlocale:remove dateformat");
		DBCommand("ALTER TABLE accountlocale DROP COLUMN dateseparator;",
			"Database::ApplyMigrations: accountlocale:remove dateseparator");
		DBCommand("ALTER TABLE accountlocale DROP COLUMN currencyseparator;",
			"Database::ApplyMigrations: accountlocale:remove currencyseparator");
		// Set decimal count to 2 by default
		DBCommand("UPDATE accountlocale SET currencydecimal = 2;",
			"Database::ApplyMigrations: accountlocale:set default currency decimal");

		// Add column for transfer destination in scheduled transactions
		DBCommand("ALTER TABLE scheduledlist ADD COLUMN destination;",
			"Database::ApplyMigrations: scheduledlist:add destination column");

		// TODO: Drop table for default date settings - we're always using system settings
		// Fails with error "SQLITE_LOCKED[6]: database table is locked"
		// DBCommand("DROP TABLE defaultlocale;",
		// 		"Database::ApplyMigrations: defaultlocale:remove dateformat");

		// Deescape database
		if (!(_DeescapeDatabase() == B_OK))
			return B_ERROR;
		// Update version number
		DBCommand("UPDATE db_version SET version = 1;", "Database::ApplyMigrations:update version");
	}

	return B_OK;
}


status_t
Database::_CreateDBBackup(int32 version)
{
	BString sourcePath = gSettingsPath.Path();
	sourcePath << "/MyAccountData";
	BString destinationPath;
	destinationPath << sourcePath << "_backup_" << version;

	BFile sourceFile;
	BFile destinationFile;
	ssize_t bytesRead, bytesWritten;

	// Open the database file for reading
	status_t result = sourceFile.SetTo(sourcePath, B_READ_ONLY);
	if (result != B_OK)
		return B_ERROR;

	// Get the size of the database file and set buffer size
	off_t fileSize;
	if (sourceFile.GetSize(&fileSize) != B_OK)
		return B_ERROR;

	size_t bufferSize = static_cast<size_t>(fileSize);
	std::vector<char> buffer(bufferSize);

	// Create and open the destination file for writing
	result = destinationFile.SetTo(destinationPath, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (result != B_OK)
		return B_ERROR;

	// Read from source and write to destination
	while ((bytesRead = sourceFile.Read(&buffer[0], buffer.size())) > 0) {
		bytesWritten = destinationFile.Write(&buffer[0], bytesRead);
		if (bytesWritten < 0)
			return B_ERROR;
	}

	return B_OK;
}


status_t
Database::_DeescapeDatabase()
{
	CppSQLite3Buffer bufSQL;
	CppSQLite3Query query, transactionQuery;
	BString name, payee, category, memo;
	// Deescape account names
	query = DBQuery(
		"SELECT name, accountid FROM accountlist", "Database::DeescapeDatabase:account names");
	while (!query.eof()) {
		name = DeescapeIllegalCharacters(query.getStringField(0));
		uint32 id = query.getIntField(1);
		bufSQL.format("UPDATE accountlist SET name = %Q WHERE accountid = %i;", name.String(), id);
		DBCommand(bufSQL, "Database::DeescapeDatabase:account names");

		// Deescape transactions for current account
		bufSQL.format("SELECT timestamp, payee, category, memo FROM account_%" B_PRIu32, id);
		transactionQuery = DBQuery(bufSQL, "Database::DeescapeDatabase:account data");
		while (!transactionQuery.eof()) {
			payee = DeescapeIllegalCharacters(transactionQuery.getStringField(1));
			category = DeescapeIllegalCharacters(transactionQuery.getStringField(2));
			memo = DeescapeIllegalCharacters(transactionQuery.getStringField(3));

			bufSQL.format(
				"UPDATE account_%" B_PRIu32 " SET payee = %Q, category = %Q, memo = %Q"
				"WHERE timestamp = %" B_PRId64 ";",
				id, payee.String(), category.String(), memo.String(),
				transactionQuery.getInt64Field(0));
			DBCommand(bufSQL, "Database::DeescapeDatabase:account details");
			transactionQuery.nextRow();
		}

		// Deescape transactionlist
		bufSQL.format("SELECT timestamp, category FROM account_%" B_PRIu32, id);
		transactionQuery = DBQuery(bufSQL, "Database::DeescapeDatabase:transactionlist");
		while (!transactionQuery.eof()) {
			category = DeescapeIllegalCharacters(transactionQuery.getStringField(1));

			bufSQL.format("UPDATE transactionlist SET category = %Q WHERE timestamp = %" B_PRId64 ";",
				category.String(), transactionQuery.getInt64Field(0));
			DBCommand(bufSQL, "Database::DeescapeDatabase:transactionlist");
			transactionQuery.nextRow();
		}

		query.nextRow();
	}

	// Deescape categories
	query = DBQuery("SELECT name FROM categorylist;", "Database::DeescapeDatabase");
	BString originalName;
	while (!query.eof()) {
		originalName = query.getStringField(0);
		name = DeescapeIllegalCharacters(originalName.String());
		bufSQL.format("UPDATE categorylist SET name = %Q WHERE name = %Q;", name.String(),
			originalName.String());
		DBCommand(bufSQL, "Database::DeescapeDatabase:update category");
		query.nextRow();
	}

	// Deescape scheduled transactions
	query = DBQuery("SELECT transid, payee, category, memo FROM scheduledlist;",
		"Database::DeescapeDatabase:scheduledlist");
	while (!query.eof()) {
		payee = DeescapeIllegalCharacters(query.getStringField(1));
		category = DeescapeIllegalCharacters(query.getStringField(2));
		memo = DeescapeIllegalCharacters(query.getStringField(3));

		bufSQL.format(
			"UPDATE scheduledlist SET payee = %Q, category = %Q, memo = %Q"
			"WHERE transid = %i;",
			payee.String(), category.String(), memo.String(), query.getIntField(0));
		DBCommand(bufSQL, "Database::DeescapeDatabase:scheduledlist");
		query.nextRow();
	}

	// Deescape budget entries
	query = DBQuery(
		"SELECT entryid, category FROM budgetlist;", "Database::DeescapeDatabase:budgetlist");
	while (!query.eof()) {
		category = DeescapeIllegalCharacters(query.getStringField(1));

		bufSQL.format(
			"UPDATE budgetlist SET category = %Q"
			"WHERE entryid = %i;",
			category.String(), query.getIntField(0));
		DBCommand(bufSQL, "Database::DeescapeDatabase:budgetlist");
		query.nextRow();
	}

	return B_OK;
}


bool
IsInternalCategory(const char* category)
{
	const char* internal_categories[] = {B_TRANSLATE_CONTEXT("Income", "CommonTerms"),
		B_TRANSLATE_CONTEXT("Spending", "CommonTerms"),
		B_TRANSLATE_ALL("Split", "CommonTerms", "The noun 'split', as in 'a split-category'"),
		B_TRANSLATE_CONTEXT("Transfer", "CommonTerms"),
		B_TRANSLATE_CONTEXT("Uncategorized", "CommonTerms"), NULL};

	for (int32 i = 0; internal_categories[i] != NULL; i++) {
		if (strcasecmp(internal_categories[i], category) == 0)
			return true;
	}
	return false;
}
