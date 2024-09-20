/*
 * Copyright 2009. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	dospuntos (Johan Wagenheim)
 *	Thomas Schmidt
 */
#ifndef TRANSACTIONDATA_H
#define TRANSACTIONDATA_H

#include "Category.h"
#include "Fixed.h"
#include "TextFile.h"
#include "Transaction.h"

#include <String.h>

#include <cstdio>

class TransactionData {
public:
	TransactionData();
	TransactionData(Account* account, const char* date, const char* type, const char* payee,
		const char* amount, const char* category, const char* memo, uint8 status = TRANS_OPEN);
	TransactionData(const TransactionData& trans);
	virtual ~TransactionData();
	TransactionData& operator=(const TransactionData& from);

	status_t Set(Account* account, const char* date, const char* type, const char* payee,
		const char* amount, const char* category, const char* memo, uint8 status = TRANS_OPEN);

	uint8 Month();
	uint8 Year();

	time_t Date() const { return fDate; }
	void SetDate(const time_t& t) { fDate = t; }

	TransactionType Type() const { return fType; }

	void SetType(const TransactionType& type);
	void SetType(const char* type);

	const char* Payee() const { return fPayee.String(); }
	void SetPayee(const char* payee) { fPayee = payee; }

	Fixed Amount() const { return fAmount; }
	void SetAmount(const Fixed& fixed) { fAmount = fixed; }

	void PrintToStream();

	void SetCategory(const char* cat);
	void SetCategory(const Category& cat);
	void AddCategory(const char* name, const Fixed& amount = 0, const bool& recalculate = true);
	void RemoveCategory(const int32& index) { fCategory.RemoveItem(index); }

	void SetNameAt(const int32& index, const char* name) { fCategory.SetNameAt(index, name); }
	const char* NameAt(const int32& index) const { return fCategory.NameAt(index); }

	void SetAmountAt(const int32& index, const Fixed& amount)
	{
		fCategory.SetAmountAt(index, amount);
	}

	Fixed AmountAt(const int32& index) const { return fCategory.AmountAt(index); }

	void SetMemoAt(const int32& index, const char* memo) { fCategory.SetMemoAt(index, memo); }
	const char* MemoAt(const int32& index) const { return fCategory.MemoAt(index); }

	int32 CountCategories() const { return fCategory.CountItems(); }
	Category GetCategory() { return fCategory; }

	const char* Memo() const { return fMemo.String(); }
	void SetMemo(const char* memo) { fMemo = memo; }

	Account* GetAccount() const { return fAccount; }
	void SetAccount(Account* acc) { fAccount = acc; }

	uint32 GetID() const { return fID; }
	void SetID(const uint32 value) { fID = value; }

	// Cleared, Reconciled, etc.
	uint8 Status() const { return fStatus; }
	void SetStatus(uint8 value) { fStatus = value; }

	void SetTimeStamp(const bigtime_t& time) { fTimeStamp = time; }
	bigtime_t GetTimeStamp() const { return fTimeStamp; }

	virtual void MakeEmpty();
	// If everything which needs to be in a transaction is there and valid, it
	// returns true
	virtual bool IsValid() const;

private:
	time_t fDate;
	TransactionType fType;

	Account* fAccount;
	BString fPayee;
	Fixed fAmount;
	Category fCategory;
	BString fMemo;
	uint8 fStatus;
	uint32 fID;
	bigtime_t fTimeStamp;
};

#endif // TRANSACTIONDATA_H
