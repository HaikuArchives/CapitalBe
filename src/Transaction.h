#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "Category.h"
#include "Fixed.h"
#include <String.h>

class Account;

#define TRANS_OPEN 0
#define TRANS_CLEARED 1
#define TRANS_RECONCILED 2

enum {
	TRANS_ATM = 0,
	TRANS_XFER,
	TRANS_DEP,
	TRANS_OTHER,
	TRANS_INIT
};

typedef enum {
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

	const char* Type() const { return fTypeData.String(); }

	uint8 TypeCode() const { return fTypeCode; }

private:
	uint8 fTypeCode;
	BString fTypeData;
};

class Transaction {
public:
	Transaction();

	void SetID(const time_t& id) { fID = id; }

	time_t GetID() const { return fID; }

private:
	bigtime_t fID;
};

#endif
