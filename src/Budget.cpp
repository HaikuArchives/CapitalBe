/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	humdinger (Joachim Seemer)
 *	dospuntos (Johan Wagenheim)
 */
#include "Budget.h"

#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "BudgetWindow"


BudgetEntry::BudgetEntry()
{
	Set("", 0, BUDGET_MONTHLY, true);
}


BudgetEntry::BudgetEntry(const char* nm, const Fixed& amt, const BudgetPeriod& per,
	const bool& isexp)
{
	Set(nm, amt, per, isexp);
}


BudgetEntry::BudgetEntry(const BudgetEntry& from)
{
	Set(from.name.String(), from.amount, from.period, from.isexpense);
}


BudgetEntry&
BudgetEntry::operator=(const BudgetEntry& from)
{
	Set(from.name.String(), from.amount, from.period, from.isexpense);
	return *this;
}


void
BudgetEntry::Set(const char* nm, const Fixed& amt, const BudgetPeriod& per, const bool& isexp)
{
	name = nm;
	amount = amt;
	period = per;
	isexpense = isexp;
}


BString
BudgetPeriodToString(const BudgetPeriod& period)
{
	switch (period) {
		case BUDGET_MONTHLY:
		{
			return BString(B_TRANSLATE("Monthly"));
			break;
		}
		case BUDGET_WEEKLY:
		{
			return BString(B_TRANSLATE("Weekly"));
			break;
		}
		case BUDGET_QUARTERLY:
		{
			return BString(B_TRANSLATE("Quarterly"));
			break;
		}
		case BUDGET_ANNUALLY:
		{
			return BString(B_TRANSLATE("Annually"));
			break;
		}
		default:
		{
			return BString(B_TRANSLATE("Unknown"));
			break;
		}
	}
}


BudgetPeriod
StringToBudgetPeriod(const char* string)
{
	// TODO: Does this need translated?
	BString str(string);
	if (str.CountChars() < 1)
		return BUDGET_UNKNOWN;

	if (str.ICompare("monthly") == 0)
		return BUDGET_MONTHLY;
	else if (str.ICompare("weekly") == 0)
		return BUDGET_WEEKLY;
	else if (str.ICompare("quarterly") == 0)
		return BUDGET_QUARTERLY;
	else if (str.ICompare("annually") == 0)
		return BUDGET_ANNUALLY;
	else if (str.ICompare("yearly") == 0)
		return BUDGET_ANNUALLY;

	return BUDGET_UNKNOWN;
}
