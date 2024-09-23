/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	humdinger (Joachim Seemer)
 *	dospuntos (Johan Wagenheim)
 */
#include "Account.h"
#include "Database.h"

#include <Catalog.h>

#include <stdlib.h>


Account::Account(const char* name, const bool& isclosed)
	: fName(name),
	  fID(0),
	  fClosed(isclosed),
	  fCurrentTransaction(0),
	  fUseDefaultLocale(true)
{
}


Account::~Account() {}


void
Account::SetName(const char* name)
{
	fName = name;
}


bool
Account::SetCurrentTransaction(const uint32& id)
{
	if (gDatabase.HasTransaction(id)) {
		if (id != fCurrentTransaction) {
			BMessage msg;
			msg.AddInt32("id", id);
			Notify(WATCH_TRANSACTION | WATCH_SELECT, &msg);
		}
		fCurrentTransaction = id;
		return true;
	}
	return false;
}


Fixed
Account::Balance()
{
	BString command;
	command.SetToFormat("SELECT SUM(amount) FROM account_%i;", fID);
	CppSQLite3Query query = gDatabase.DBQuery(command.String(), "Account::Balance");

	int64 amount = 0;
	if (!query.eof())
		amount = query.getInt64Field(0);

	Fixed f;
	f.SetPremultiplied(amount);
	return f;
}


Fixed
Account::BalanceAt(const time_t& date)
{
	BString command;
	command.SetToFormat(
		"SELECT SUM(amount) FROM account_%i WHERE date <= %li ORDER BY payee;", fID, date);
	CppSQLite3Query query = gDatabase.DBQuery(command.String(), "Account::BalanceAt");

	int64 amount = 0;
	if (query.eof())
		return Fixed();

	amount = query.getInt64Field(0);

	Fixed f;
	f.SetPremultiplied(amount);
	return f;
}


Fixed
Account::BalanceAtTransaction(const time_t& time, const char* payee)
{
	if (!payee)
		return Fixed();

	BString command;
	command.SetToFormat(
		"SELECT date,payee,amount FROM account_%i WHERE date <= %li ORDER BY date,payee;", fID,
		time);
	CppSQLite3Query query = gDatabase.DBQuery(command.String(), "Account::BalanceAt");

	int64 amount = 0;
	time_t date = 0;

	while (!query.eof()) {
		date = query.getInt64Field(0);
		if (date < time) {
			amount += query.getInt64Field(2);
		} else {
			if (strcmp(query.getStringField(1), payee) < 1)
				amount += query.getInt64Field(2);
		}
		query.nextRow();
	}


	Fixed f;
	f.SetPremultiplied(amount);
	return f;
}


BString
Account::AutocompleteCategory(const char* input)
{
	if (!input)
		return BString();

	CppSQLite3Buffer bufSQL;
	BString searchString;
	searchString << input << "%";
	bufSQL.format("SELECT name FROM categorylist WHERE name LIKE %Q", searchString.String());
	CppSQLite3Query query = gDatabase.DBQuery(bufSQL, "Account::AutocompleteCategory");

	if (query.eof())
		return NULL;

	return query.getStringField(0);
}


BString
Account::AutocompletePayee(const char* input)
{
	if (!input)
		return BString();

	CppSQLite3Buffer bufSQL;
	BString searchString;
	searchString << input << "%";
	bufSQL.format("SELECT payee FROM account_%i WHERE payee LIKE %Q", fID, searchString.String());
	CppSQLite3Query query = gDatabase.DBQuery(bufSQL, "Account::AutocompletePayee");

	if (query.eof())
		return NULL;

	return query.getStringField(0);
}


Locale
Account::GetLocale() const
{
	return fUseDefaultLocale ? gDefaultLocale : fLocale;
}


void
Account::SetLocale(const Locale& locale)
{
	if (fUseDefaultLocale) {
		ShowBug("Calling SetLocale on an account with default locale settings");
		return;
	}

	if (locale != fLocale) {
		fLocale = locale;
		gDatabase.SetAccountLocale(fID, fLocale);

		BMessage msg;
		msg.AddPointer("item", this);
		Notify(WATCH_ACCOUNT | WATCH_LOCALE | WATCH_CHANGE, &msg);
	}
}


uint32
Account::CountTransactions()
{
	BString command;
	command << ("SELECT COUNT(*) FROM account_") << GetID();
	CppSQLite3Query query = gDatabase.DBQuery(command.String(), "Account::CountTransactions");

	if (query.eof())
		return 0;

	return query.getIntField(0);
}


void
Account::DoForEachTransaction(void (*func)(const TransactionData&, void*), void* ptr)
{
	BString command;
	command << ("SELECT * FROM account_") << GetID() << " ORDER BY date,transid";
	CppSQLite3Query query = gDatabase.DBQuery(command.String(), "Account::DoForEachTransaction");

	uint32 currentid = 0, newid = 0;
	if (!query.eof()) {
		TransactionData data;

		currentid = query.getIntField(1);
		newid = query.getIntField(1);
		data.SetID(currentid);
		data.SetDate(atol(query.getStringField(2)));
		data.SetType(query.getStringField(3));
		data.SetPayee(query.getStringField(4));
		data.SetAccount(this);

		Fixed f;
		f.SetPremultiplied(atol(query.getStringField(5)));
		data.AddCategory(query.getStringField(6), f, true);

		if (!query.fieldIsNull(7))
			data.SetMemoAt(data.CountCategories() - 1, query.getStringField(7));

		BString status = query.getStringField(8);
		if (status.ICompare("Reconciled") == 0)
			data.SetStatus(TRANS_RECONCILED);
		else if (status.ICompare("Cleared") == 0)
			data.SetStatus(TRANS_CLEARED);
		else
			data.SetStatus(TRANS_OPEN);
		query.nextRow();

		while (!query.eof()) {
			newid = query.getIntField(1);

			if (currentid != newid) {
				if (data.CountCategories() == 1)
					data.SetMemo(data.MemoAt(0));

				func(data, ptr);
				data.MakeEmpty();

				currentid = newid;
				newid = query.getIntField(1);
				data.SetID(currentid);
				data.SetDate(atol(query.getStringField(2)));
				data.SetType(query.getStringField(3));
				data.SetPayee(query.getStringField(4));
				data.SetAccount(this);
			}

			f.SetPremultiplied(atol(query.getStringField(5)));
			data.AddCategory(query.getStringField(6), f, true);

			if (!query.fieldIsNull(7))
				data.SetMemoAt(data.CountCategories() - 1, query.getStringField(7));

			status = query.getStringField(8);
			if (status.ICompare("Reconciled") == 0)
				data.SetStatus(TRANS_RECONCILED);
			else if (status.ICompare("Cleared") == 0)
				data.SetStatus(TRANS_CLEARED);
			else
				data.SetStatus(TRANS_OPEN);
			query.nextRow();
		}

		func(data, ptr);
	}
}


void
Account::UseDefaultLocale(const bool& usedefault)
{
	if (usedefault == fUseDefaultLocale)
		return;

	fUseDefaultLocale = usedefault;

	BString command;
	if (fUseDefaultLocale) {
		command << "DELETE FROM accountlocale WHERE accountid = " << fID << ";";
		gDatabase.DBCommand(command.String(), "Account::UseDefaultLocale");
		gCurrentLocale = gDefaultLocale;
	} else {
		// update the local copy in case it changed since the program was opened
		fLocale = gCurrentLocale;
		gDatabase.SetAccountLocale(fID, fLocale);
	}

	BMessage msg;
	msg.AddPointer("item", this);
	Notify(WATCH_ACCOUNT | WATCH_LOCALE | WATCH_CHANGE, &msg);
}
