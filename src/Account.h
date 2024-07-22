#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <String.h>
#include "CBLocale.h"
#include "Notifier.h"
#include "ObjectList.h"
#include "Transaction.h"
#include "TransactionData.h"

typedef enum {
	ACCOUNT_BANK = 0,
	ACCOUNT_CASH,
	ACCOUNT_CREDIT
} AccountType;

class Account : public Notifier {
public:
	Account(const char* name = NULL, const bool& isclosed = false);
	~Account(void);

	void SetName(const char* name);
	const char* Name(void) const { return fName.String(); }

	void SetID(const time_t& id) { fID = id; }
	uint32 GetID(void) const { return fID; }

	void SetClosed(const bool& value) { fClosed = value; }
	bool IsClosed(void) const { return fClosed; }

	uint32 CurrentTransaction(void) const { return fCurrentTransaction; }
	bool SetCurrentTransaction(const uint32& id);

	BString AutocompleteCategory(const char* input);
	BString AutocompletePayee(const char* input);
	// BString AutocompleteType(const char* input);

	Fixed Balance(void);
	Fixed BalanceAt(const time_t& time);
	Fixed BalanceAtTransaction(const time_t& time, const char* payee);

	Locale GetLocale(void) const;
	void SetLocale(const Locale& locale);

	uint32 CountTransactions(void);

	void DoForEachTransaction(
		void (*TransactionIteratorFunc)(const TransactionData&, void*), void* ptr);

	void UseDefaultLocale(const bool& usedefault);
	bool IsUsingDefaultLocale(void) const { return fUseDefaultLocale; }

private:
	BString fName;
	uint32 fID;
	bool fClosed;
	uint32 fCurrentTransaction;
	Locale fLocale;
	bool fUseDefaultLocale;
};

#endif
