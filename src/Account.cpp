#include "Account.h"
#include "Database.h"
#include <ctype.h>
#include <stdlib.h>

Account::Account(const char *name, const bool &isclosed)
 :	fName(name),
 	fID(0),
 	fClosed(isclosed),
 	fCurrentTransaction(0),
 	fLastCheckNumber(0),
 	fUseDefaultLocale(true)
{
}

Account::~Account(void)
{
}

void Account::SetName(const char *name)
{
	fName = name;
}

bool Account::SetCurrentTransaction(const uint32 &id)
{
	if(gDatabase.HasTransaction(id))
	{
		if(id != fCurrentTransaction)
		{
			BMessage msg;
			msg.AddInt32("id",id);
			Notify(WATCH_TRANSACTION | WATCH_SELECT,&msg);
		}
		fCurrentTransaction = id;
		return true;
	}
	return false;
}

uint16 Account::LookupLastCheckNumber(void)
{
	BString command("select max(type) from account_");
	command << fID << " where type between 0 and 65536;";
	
	CppSQLite3Query query = gDatabase.DBQuery(command.String(),"Account::LookupLastCheckNumber");
	if(query.eof())
		return 0;
	
	return query.getIntField(0);
}

Fixed Account::Balance(void)
{
	BString command("select sum(amount) from account_");
	command << fID << ";";
	
	CppSQLite3Query query = gDatabase.DBQuery(command.String(),
												"Account::Balance");
	
	int64 amount = 0;
	if(query.eof())
		return Fixed();
	
	amount = query.getInt64Field(0);
	
	Fixed f;
	f.SetPremultiplied(amount);
	return f;
}

Fixed Account::BalanceAt(const time_t &date)
{
	BString command("select sum(amount) from account_");
	command << fID << " where date <= " << date << " order by payee;";
	
	CppSQLite3Query query = gDatabase.DBQuery(command.String(),"Account::BalanceAt");
	
	int64 amount = 0;
	if(query.eof())
		return Fixed();
	
	amount = query.getInt64Field(0);
	
	Fixed f;
	f.SetPremultiplied(amount);
	return f;
}

Fixed Account::BalanceAtTransaction(const time_t &time, const char *payee)
{
	if(!payee)
		return Fixed();
	
	BString command("select date,payee,amount from account_");
	command << fID << " where date <= " << time << " order by date,payee;";
	
	CppSQLite3Query query = gDatabase.DBQuery(command.String(),"Account::BalanceAt");
	
	int64 amount = 0;
	time_t date = 0;
	
	while(!query.eof())
	{
		date = query.getInt64Field(0);
		if(date < time)
		{
			amount += query.getInt64Field(2);
		}
		else
		{
			BString temp(DeescapeIllegalCharacters(query.getStringField(1)));
			if(strcmp(temp.String(),payee)<1)
				amount += query.getInt64Field(2);
		}
		query.nextRow();
	}
	
	
	Fixed f;
	f.SetPremultiplied(amount);
	return f;
	
}

BString Account::AutocompleteCategory(const char *input)
{
	if(!input)
		return BString();
	
	// TODO: Add language support here
	if(toupper(input[0]) == (int)'S')
	{
		int32 inputlength = strlen(input);
		if(strncasecmp(input,"Split",inputlength)==0)
			return "Split";
	}
	
	BString command("select name from categorylist where name like '");
	command << EscapeIllegalCharacters(input) << "%' ;";
	
	CppSQLite3Query query = gDatabase.DBQuery(command.String(),
												"Account::AutocompleteCategory");
	
	if(query.eof())
		return NULL;
	
	return DeescapeIllegalCharacters(query.getStringField(0));
}

BString Account::AutocompletePayee(const char *input)
{
	if(!input)
		return BString();
	
	BString command("select payee from account_");
	command << fID << " where payee like '" 
			<< EscapeIllegalCharacters(input) << "%' ;";
	
	CppSQLite3Query query = gDatabase.DBQuery(command.String(),
												"Account::AutocompletePayee");
	
	if(query.eof())
		return NULL;
	
	return DeescapeIllegalCharacters(query.getStringField(0));
}

BString Account::AutocompleteType(const char *input)
{
	if(!input)
		return BString();
	
	if(toupper(input[0]) == (int)'A')
	{
		int32 inputlength = strlen(input);
		if(strncasecmp(input,"ATM",inputlength)==0)
			return BString("ATM");
	}
	else
	if(toupper(input[0]) == (int)'D')
	{
		int32 inputlength = strlen(input);
		if(strncasecmp(input,"DEP",inputlength)==0)
			return BString("DEP");
	}
	else
	{
		// Numeric autocompletion
		BString str;
		str << fLastCheckNumber;
		if(input[0] == str.ByteAt(0))
			return str;
	}
		
	BString command("select type from account_");
	command << fID << " where type like '" 
			<< EscapeIllegalCharacters(input) << "%' ;";
	
	CppSQLite3Query query = gDatabase.DBQuery(command.String(),
												"Account::AutocompleteType");
	
	if(query.eof())
		return BString();
	
	return DeescapeIllegalCharacters(query.getStringField(0));
}

Locale Account::GetLocale(void) const
{
	return fUseDefaultLocale ? gDefaultLocale : fLocale;
}

void Account::SetLocale(const Locale &locale)
{
	if(fUseDefaultLocale)
	{
		ShowBug("Calling SetLocale on an account with default locale settings");
		return;
	}
	
	if(locale != fLocale)
	{
		fLocale = locale;
		gDatabase.SetAccountLocale(fID,fLocale);
		
		BMessage msg;
		msg.AddPointer("item",this);
		Notify(WATCH_ACCOUNT | WATCH_LOCALE | WATCH_CHANGE,&msg);
	}
}

uint32 Account::CountTransactions(void)
{
	BString command("select * from account_");
	command << GetID() << " order by transid";
	CppSQLite3Query query = gDatabase.DBQuery(command.String(),
												"Account::CountTransactions");
	
	if(query.eof())
		return 0;
	
	uint32 currentid=0,newid=0,count=0;
	currentid=query.getIntField(1);
	newid=query.getIntField(1);
	
	while(!query.eof())
	{
		count++;
		query.nextRow();
	}
	return count;
}

void Account::DoForEachTransaction(void (*func)(const TransactionData &,void*),void *ptr)
{
	BString command("select * from account_");
	command << GetID() << " order by date,transid";
	CppSQLite3Query query = gDatabase.DBQuery(command.String(),"Account::DoForEachTransaction");
	
	uint32 currentid=0,newid=0;
	if(!query.eof())
	{
		TransactionData data;
		
		currentid=query.getIntField(1);
		newid=query.getIntField(1);
		data.SetID(currentid);
		data.SetDate(atol(query.getStringField(2)));
		data.SetType(DeescapeIllegalCharacters(query.getStringField(3)).String());
		data.SetPayee(DeescapeIllegalCharacters(query.getStringField(4)).String());
		data.SetAccount(this);
		
		Fixed f;
		f.SetPremultiplied(atol(query.getStringField(5)));
		data.AddCategory(DeescapeIllegalCharacters(query.getStringField(6)).String(),f,true);
			
		if(!query.fieldIsNull(7))
			data.SetMemoAt(data.CountCategories()-1,
							DeescapeIllegalCharacters(query.getStringField(7)).String());
		
		BString status = query.getStringField(8);
		if(status.ICompare("Reconciled")==0)
			data.SetStatus(TRANS_RECONCILED);
		else
		if(status.ICompare("Cleared")==0)
			data.SetStatus(TRANS_CLEARED);
		else
			data.SetStatus(TRANS_OPEN);
		query.nextRow();
		
		while(!query.eof())
		{
			newid=query.getIntField(1);
			
			if(currentid!=newid)
			{
				if(data.CountCategories() == 1)
					data.SetMemo(data.MemoAt(0));
				
				func(data,ptr);
				data.MakeEmpty();
				
				currentid=newid;
				newid=query.getIntField(1);
				data.SetID(currentid);
				data.SetDate(atol(query.getStringField(2)));
				data.SetType(DeescapeIllegalCharacters(query.getStringField(3)).String());
				data.SetPayee(DeescapeIllegalCharacters(query.getStringField(4)).String());
				data.SetAccount(this);
			}
			
			f.SetPremultiplied(atol(query.getStringField(5)));
			data.AddCategory(DeescapeIllegalCharacters(query.getStringField(6)).String(),f,true);
			
			if(!query.fieldIsNull(7))
				data.SetMemoAt(data.CountCategories()-1,
								DeescapeIllegalCharacters(query.getStringField(7)).String());
			
			status = query.getStringField(8);
			if(status.ICompare("Reconciled")==0)
				data.SetStatus(TRANS_RECONCILED);
			else
			if(status.ICompare("Cleared")==0)
				data.SetStatus(TRANS_CLEARED);
			else
				data.SetStatus(TRANS_OPEN);
			query.nextRow();
		}
		
		func(data,ptr);
	}
}

void Account::UseDefaultLocale(const bool &usedefault)
{
	if(usedefault == fUseDefaultLocale)
		return;
	
	fUseDefaultLocale = usedefault;
	
	BString command;
	if(fUseDefaultLocale)
	{
		command = "delete from accountlocale where accountid = ";
		command << fID << ";";
	}
	else
	{
		// update the local copy in case it changed since the program was opened
		fLocale = gDefaultLocale;
		
		gDatabase.SetAccountLocale(fID,fLocale);
	}

}
