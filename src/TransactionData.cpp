#include <stdio.h>
#include <File.h>
#include <stdlib.h>
#include <stdio.h>
#include "TransactionData.h"
#include "CBLocale.h"
#include "Database.h"
#include "Account.h"

//#define DEBUG_TRANS
#ifdef DEBUG_TRANS
#define STRACE(x) printf x
#else
#define STRACE(x) /* nothing */
#endif

TransactionData::TransactionData(void)
 :	fDate(0),
	fType(""),
	fStatus(TRANS_OPEN),
	fID(0),
	fTimeStamp(0)
{
}

TransactionData::TransactionData(Account *account, const char *date, const char *type, 
								const char *payee, const char *amount, const char *category, 
								const char *memo, uint8 status)
 :	fDate(0),
	fType(type),
 	fAccount(NULL),
	fStatus(TRANS_OPEN),
	fID(0),
	fTimeStamp(0)
{
	fCategory.MakeEmpty();
	Set(account,date,type,payee,amount,category,memo,status);
}

TransactionData::TransactionData(const TransactionData &trans)
 :	fDate(0),
	fType(""),
	fStatus(TRANS_OPEN),
	fTimeStamp(0)
{
	fCategory.MakeEmpty();
	*this = trans;
}


TransactionData::~TransactionData()
{
}

TransactionData &TransactionData::operator=(const TransactionData &from)
{
	fDate = from.fDate;
	fType = from.fType;
	
	fAccount = from.fAccount;
	fPayee = from.fPayee;
	fAmount = from.fAmount;
	fCategory = from.fCategory;
	
	fMemo = from.fMemo;
	fStatus = from.fStatus;
	fID = from.fID;
	fTimeStamp = from.fTimeStamp;
	
	return *this;
}

status_t TransactionData::Set(Account *account, const char *date, const char *type, 
				const char *payee,const char *amount, const char *category, const char *memo,
				uint8 status)
{
	STRACE(("TransactionData(%s,%s,%s,%s,%s,%s,%s\n",date,type,payee,amount,
			category,memo,status ? "reconciled" : "not reconciled"));
	
	fAccount = account;
	
	fType.SetType(type);
	gDefaultLocale.StringToDate(date,fDate);
	
	fPayee = payee;
	if(gCurrentLocale.StringToCurrency(amount,fAmount)!=B_OK)
		fAmount = 0;
	
	if(fType.TypeCode() == TRANS_DEP)
	{
		if(fAmount.IsNegative())
			fAmount.Invert();
	}
	else
	{
		if(fAmount.IsPositive())
			fAmount.Invert();
	}
	
	SetCategory(category);
	fMemo = memo;
	fStatus = status;
	
	return B_OK;
}

uint8 TransactionData::Month()
{
	tm* t = localtime(&fDate);
	return(t->tm_mon);
}

uint8 TransactionData::Year()
{
	tm* t = localtime(&fDate);
	return(t->tm_year);
}

void TransactionData::PrintToStream(void)
{
	BString str,temp;
	gDefaultLocale.DateToString(fDate,str);
	
	str.Prepend("<TRANS>");
	
	str << "  " << fType.Type() << "  " << fPayee << "  " << (float)(fAmount.AsFloat()) << "  ";
	printf(str.String());
	
	fCategory.PrintToStream();
	
	str = "";
	str << "  " << fMemo << "  ";
	if(fStatus == TRANS_CLEARED)
		str << "cleared";
	else
	if(fStatus == TRANS_RECONCILED)
		str << "reconciled";
	else
		str << "cpen";
	
	str << "  " << "</TRANS>\n";
	printf(str.String());
}

void TransactionData::SetType(const TransactionType &type)
{
	fType=type;
}

void TransactionData::SetType(const char *type)
{
	fType.SetType(type);
}

void TransactionData::SetCategory(const char *cat)
{
	if(!cat)
	{
		fCategory.MakeEmpty();
		return;
	}
	
	if(fCategory.CountItems()>0)
		fCategory.MakeEmpty();
	
	fCategory.AddItem(cat,fAmount);
}

void TransactionData::SetCategory(const Category &cat)
{
	fCategory = cat;
}

void TransactionData::AddCategory(const char *name, const Fixed &amount, const bool &recalculate)
{
	if(!name)
		return;
	
	fCategory.AddItem(name,amount);
	if(recalculate)
	{
		Fixed total;
		for(int32 i=0; i<fCategory.CountItems(); i++)
			total += AmountAt(i);
		fAmount = total;
	}
}

void TransactionData::MakeEmpty(void)
{
	SetAccount(NULL);
	SetDate(0);
	SetType(TransactionType(NULL));
	SetPayee("");
	SetAmount(Fixed());
	SetCategory("");
	SetMemo("");
	SetStatus(TRANS_OPEN);
}

bool TransactionData::IsValid(void) const
{
	if(fType.TypeCode()==TRANS_INIT)
		return false;
	
	if(!Payee())
		return false;
	
	if(CountCategories()==0 && fType.TypeCode()!=TRANS_XFER)
		return false;
	
	return true;
}
