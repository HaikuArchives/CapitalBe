/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	humdinger (Joachim Seemer)
 */
#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "CBLocale.h"
#include "Notifier.h"
#include "ObjectList.h"
#include "Transaction.h"
#include "TransactionData.h"

#include <String.h>

// clang-format off
typedef enum {
	ACCOUNT_BANK = 0,
	ACCOUNT_CASH,
	ACCOUNT_CREDIT
} AccountType;
// clang-format on

class Account : public Notifier {
public:
	Account(const char* name = NULL, const bool& isclosed = false);
	~Account();

	void SetName(const char* name);
	const char* Name() const { return fName.String(); }

	void SetID(const time_t& id) { fID = id; }
	uint32 GetID() const { return fID; }

	void SetClosed(const bool& value) { fClosed = value; }
	bool IsClosed() const { return fClosed; }

	uint32 CurrentTransaction() const { return fCurrentTransaction; }
	bool SetCurrentTransaction(const uint32& id);

	BString AutocompleteCategory(const char* input);
	BString AutocompletePayee(const char* input);
	// BString AutocompleteType(const char* input);

	Fixed Balance();
	Fixed BalanceAt(const time_t& time);
	Fixed BalanceAtTransaction(const time_t& time, const char* payee);

	Locale GetLocale() const;
	void SetLocale(const Locale& locale);

	uint32 CountTransactions();

	void DoForEachTransaction(
		void (*TransactionIteratorFunc)(const TransactionData&, void*), void* ptr);

	void UseDefaultLocale(const bool& usedefault);
	bool IsUsingDefaultLocale() const { return fUseDefaultLocale; }

private:
	BString fName;
	uint32 fID;
	bool fClosed;
	uint32 fCurrentTransaction;
	Locale fLocale;
	bool fUseDefaultLocale;
};

#endif	// ACCOUNT_H
