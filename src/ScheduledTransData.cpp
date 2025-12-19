/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	dospuntos (Johan Wagenheim)
 */
#include "ScheduledTransData.h"
#include "CBLocale.h"
#include "TimeSupport.h"


ScheduledTransData::ScheduledTransData()
	: fInterval(SCHEDULED_UNKNOWN),
	  fCount(0),
	  fNextDate(0)
{
}


ScheduledTransData::ScheduledTransData(Account* account, const char* date, const char* type,
	const char* payee, const char* amount, const char* category, const char* memo,
	const TransactionInterval& interval, const int32& count)
	: TransactionData(account, date, type, payee, amount, category, memo, TRANS_OPEN),
	  fInterval(interval),
	  fCount(count),
	  fNextDate(0),
	  fDestination(-1)
{
}


ScheduledTransData::ScheduledTransData(const ScheduledTransData& trans)
{
	*this = trans;
}


ScheduledTransData::ScheduledTransData(
	const TransactionData& data, const TransactionInterval& interval, const int32& count)
	: TransactionData(data),
	  fInterval(interval),
	  fCount(count),
	  fNextDate(0),
	  fDestination(-1)
{
}


ScheduledTransData::~ScheduledTransData() {}


ScheduledTransData&
ScheduledTransData::operator=(const ScheduledTransData& from)
{
	TransactionData::operator=(from);
	fInterval = from.GetInterval();
	fCount = from.GetCount();
	fDestination = from.GetDestination();
	return *this;
}


void
ScheduledTransData::CalculateNextDueDate()
{
	switch (GetInterval()) {
		case SCHEDULED_MONTHLY:
		{
			fNextDate = IncrementDateByMonth(Date());
			break;
		}
		case SCHEDULED_WEEKLY:
		{
			fNextDate = IncrementDateByWeek(Date());
			break;
		}
		case SCHEDULED_QUARTERLY:
		{
			fNextDate = IncrementDateByQuarter(Date());
			break;
		}
		case SCHEDULED_ANNUALLY:
		{
			fNextDate = IncrementDateByYear(Date());
			break;
		}
		default:
		{
			ShowBug("Unknown scheduled value in Database::AddScheduledTransaction()");
			break;
		}
	}
}


bool
ScheduledTransData::IsValid() const
{
	if (!TransactionData::IsValid())
		return false;

	if (fInterval == SCHEDULED_UNKNOWN)
		return false;

	return true;
}


void
ScheduledTransData::MakeEmpty()
{
	TransactionData::MakeEmpty();
	fInterval = SCHEDULED_UNKNOWN;
	fCount = 0;
	fDestination = -1;
}
