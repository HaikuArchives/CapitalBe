/*
 * Copyright 2009. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 */
#ifndef BUDGET_H
#define BUDGET_H

#include "Fixed.h"
#include <String.h>

// This is ordered from smallest interval to largest for a reason. :^)
// Note that if this is changed, you need to also change the SUBTOTAL_ enum in
// ReportWindow.h. The BudgetReport code depends on them having the same values
// clang-format off
typedef enum {
	BUDGET_WEEKLY = 0,
	BUDGET_MONTHLY,
	BUDGET_QUARTERLY,
	BUDGET_ANNUALLY,
	BUDGET_UNKNOWN
} BudgetPeriod;
// clang-format on

class BudgetEntry {
public:
	BudgetEntry();
	BudgetEntry(const char* name, const Fixed& amount, const BudgetPeriod& period,
		const bool& isexpense);
	BudgetEntry(const BudgetEntry& from);
	BudgetEntry& operator=(const BudgetEntry& from);
	void Set(const char* name, const Fixed& amount, const BudgetPeriod& period,
		const bool& isexpense);

	BString name;
	Fixed amount;
	BudgetPeriod period;
	bool isexpense;
};

BString BudgetPeriodToString(const BudgetPeriod& period);
BudgetPeriod StringToBudgetPeriod(const char* string);

#endif // BUDGET_H
