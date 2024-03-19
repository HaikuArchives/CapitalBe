#include "ScheduledTransData.h"
#include "CBLocale.h"
#include "TimeSupport.h"

ScheduledTransData::ScheduledTransData(void) : fInterval(SCHEDULED_UNKNOWN), fCount(0), fNextDate(0)
{
}

ScheduledTransData::ScheduledTransData(
	Account* account, const char* date, const char* type, const char* payee, const char* amount,
	const char* category, const char* memo, const TransactionInterval& interval, const int32& count
)
	: TransactionData(account, date, type, payee, amount, category, memo, TRANS_OPEN),
	  fInterval(interval), fCount(count), fNextDate(0)
{
}

ScheduledTransData::ScheduledTransData(const ScheduledTransData& trans) { *this = trans; }

ScheduledTransData::ScheduledTransData(
	const TransactionData& data, const TransactionInterval& interval, const int32& count
)
	: TransactionData(data), fInterval(interval), fCount(count), fNextDate(0)
{
}

ScheduledTransData::~ScheduledTransData(void) {}

ScheduledTransData&
ScheduledTransData::operator=(const ScheduledTransData& from)
{
	TransactionData::operator=(from);
	fInterval = from.GetInterval();
	fCount = from.GetCount();
	return *this;
}

void
ScheduledTransData::CalculateNextDueDate(void)
{
	switch (GetInterval()) {
	case SCHEDULED_MONTHLY: {
		fNextDate = IncrementDateByMonth(Date());
		break;
	}
	case SCHEDULED_WEEKLY: {
		// TODO: Add weekly scheduling support
		//				data.SetNextDueDate(IncrementDateByMonth(data.Date()));
		ShowBug(
			"Unimplemented Weekly scheduling support in ScheduledTransData::CalculateNextDueDate()"
		);
		break;
	}
	case SCHEDULED_QUARTERLY: {
		fNextDate = IncrementDateByQuarter(Date());
		break;
	}
	case SCHEDULED_ANNUALLY: {
		fNextDate = IncrementDateByYear(Date());
		break;
	}
	default: {
		ShowBug("Unknown scheduled value in Database::AddScheduledTransaction()");
		break;
	}
	}
}

bool
ScheduledTransData::IsValid(void) const
{
	if (!TransactionData::IsValid())
		return false;

	if (fInterval == SCHEDULED_UNKNOWN)
		return false;

	return true;
}

void
ScheduledTransData::MakeEmpty(void)
{
	TransactionData::MakeEmpty();
	fInterval = SCHEDULED_UNKNOWN;
	fCount = 0;
}
