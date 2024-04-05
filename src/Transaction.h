#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <String.h>
#include "Category.h"
#include "Fixed.h"

class Account;

#define TRANS_OPEN 0
#define TRANS_CLEARED 1
#define TRANS_RECONCILED 2

enum
{
	TRANS_NUMERIC = 0,
	TRANS_ATM,
	TRANS_XFER,
	TRANS_DEP,
	TRANS_OTHER,
	TRANS_INIT
};

typedef enum
{
	SCHEDULED_MONTHLY = 0,
	SCHEDULED_WEEKLY,
	SCHEDULED_QUARTERLY,
	SCHEDULED_ANNUALLY,
	SCHEDULED_UNKNOWN
} TransactionInterval;

class TransactionType {
public:
	TransactionType(const char* data) { SetType(data); }
	void SetType(const char* data);
	const char* Type(void) const { return fTypeData.String(); }
	uint8 TypeCode(void) const { return fTypeCode; }

private:
	uint8 fTypeCode;
	BString fTypeData;
};

class Transaction {
public:
	Transaction(void);

	void SetID(const time_t& id) { fID = id; };
	time_t GetID(void) const { return fID; }

private:
	bigtime_t fID;
};

#endif
