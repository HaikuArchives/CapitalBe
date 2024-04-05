#ifndef SCHEDULED_TRANS_DATA_H
#define SCHEDULED_TRANS_DATA_H

#include "TransactionData.h"

class ScheduledTransData : public TransactionData {
public:
	ScheduledTransData(void);
	ScheduledTransData(Account* account, const char* date, const char* type, const char* payee,
		const char* amount, const char* category, const char* memo,
		const TransactionInterval& interval, const int32& count = 0);
	ScheduledTransData(const ScheduledTransData& trans);
	ScheduledTransData(const TransactionData& data,
		const TransactionInterval& interval = SCHEDULED_MONTHLY, const int32& count = 0);
	virtual ~ScheduledTransData(void);
	ScheduledTransData& operator=(const ScheduledTransData& from);

	TransactionInterval GetInterval(void) const { return fInterval; }

	void SetInterval(const TransactionInterval& interval) { fInterval = interval; }

	int32 GetCount(void) const { return fCount; }

	// count < 0 means repeat indefinitely
	void SetCount(const int32 count) { fCount = count; }

	void SetNextDueDate(const time_t& date) { fNextDate = date; }

	void CalculateNextDueDate(void);

	time_t GetNextDueDate(void) const { return fNextDate; }

	// If everything which needs to be in a transaction is there and valid, it
	// returns true
	virtual bool IsValid(void) const;
	virtual void MakeEmpty(void);

private:
	TransactionInterval fInterval;
	uint32 fCount;
	time_t fNextDate;
};

#endif
