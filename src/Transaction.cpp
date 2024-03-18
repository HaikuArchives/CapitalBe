#include "Transaction.h"
#include "Account.h"
#include "CBLocale.h"
#include "Database.h"
#include <File.h>
#include <stdio.h>
#include <stdlib.h>

// #define DEBUG_TRANS
#ifdef DEBUG_TRANS
#define STRACE(x) printf x
#else
#define STRACE(x) /* nothing */
#endif

Transaction::Transaction(void) : fID(0) {}

void
TransactionType::SetType(const char* data)
{
	if (!data) {
		fTypeCode = TRANS_INIT;
		fTypeData = "";
		return;
	}

	long number = atol(data);
	if (number != 0) {
		fTypeCode = TRANS_NUMERIC;
		fTypeData = "";
		fTypeData << number;
		return;
	}

	fTypeData = data;
	if (fTypeData.ICompare("ATM") == 0)
		fTypeCode = TRANS_ATM;
	else if (fTypeData.ICompare("DEP") == 0)
		fTypeCode = TRANS_DEP;
	else if (fTypeData.ICompare("XFER") == 0)
		fTypeCode = TRANS_XFER;
	else
		fTypeCode = TRANS_OTHER;
}
