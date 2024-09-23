/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	dospuntos (Johan Wagenheim)
 */
#ifndef SCHEDULED_TRANS_DATA_H
#define SCHEDULED_TRANS_DATA_H

#include "TransactionData.h"

class ScheduledTransData : public TransactionData {
public:
	ScheduledTransData();
	ScheduledTransData(Account* account, const char* date, const char* type, const char* payee,
		const char* amount, const char* category, const char* memo,
		const TransactionInterval& interval, const int32& count = 0);
	ScheduledTransData(const ScheduledTransData& trans);
	ScheduledTransData(const TransactionData& data,
		const TransactionInterval& interval = SCHEDULED_MONTHLY, const int32& count = 0);
	virtual ~ScheduledTransData();

	ScheduledTransData& operator=(const ScheduledTransData& from);

	TransactionInterval GetInterval() const { return fInterval; }
	void SetInterval(const TransactionInterval& interval) { fInterval = interval; }

	int32 GetCount() const { return fCount; }
	// count < 0 means repeat indefinitely
	void SetCount(const int32 count) { fCount = count; }

	void SetNextDueDate(const time_t& date) { fNextDate = date; }
	void CalculateNextDueDate();
	time_t GetNextDueDate() const { return fNextDate; }

	void SetDestination(const int32 destinationAccount) { fDestination = destinationAccount; }
	int32 GetDestination() const { return fDestination; }

	// If everything which needs to be in a transaction is there and valid, it
	// returns true
	virtual bool IsValid() const;
	virtual void MakeEmpty();

private:
	TransactionInterval fInterval;
	uint32 fCount;
	time_t fNextDate;
	int32 fDestination;
};

#endif	// SCHEDULED_TRANS_DATA_H
