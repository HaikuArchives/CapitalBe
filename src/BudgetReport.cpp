#include "Account.h"
#include "Budget.h"
#include "CBLocale.h"
#include "ColumnListView.h"
#include "ColumnTypes.h"
#include "Database.h"
#include "ReportGrid.h"
#include "ReportWindow.h"
#include "TimeSupport.h"

/*
	Status:
	Different tack on the same task - go one row at a time by querying once for
	all transactions in the category within the time range and iterating over the
	row. Right now, it only acts on the first category in the list (easy to mod to
	work on all of 'em) and the logic doesn't work right. No subtotaling results
	in proper display, but going to monthly displays absolutely nothing

	There also seems to be some bugs in the BudgetWindow editor code -- are the items
	in the budget defaulting to a Weekly period?
*/

typedef enum { MAP_UNCHANGED = 0, MAP_CHANGED = 1, MAP_BAD_VALUE = -1 } map_status;

map_status
MapBudgetToSubtotal(
	Fixed &fixed, const int &budgetperiod, const int &subtotal, const time_t &start,
	const time_t &end
);

void
ReportWindow::ComputeBudget(void) {
	BObjectList<time_t> timelist(20, true);

	// Calculate the number of columns and the starting date for each one
	if (fSubtotalMode == SUBTOTAL_NONE) {
		timelist.AddItem(new time_t(fStartDate));
		timelist.AddItem(new time_t(fEndDate));
	} else {
		for (time_t t = fStartDate; t < fEndDate;) {
			time_t *item = new time_t(t);
			timelist.AddItem(item);

			switch (fSubtotalMode) {
			case SUBTOTAL_MONTH: {
				t = IncrementDateByMonth(t);
				break;
			}
			case SUBTOTAL_QUARTER: {
				t = IncrementDateByQuarter(t);
				break;
			}
			case SUBTOTAL_YEAR: {
				t = IncrementDateByYear(t);
				break;
			}
			default: {
				t = fEndDate;
				break;
			}
			}
		}
		timelist.AddItem(new time_t(fEndDate));
	}

	// Here's where all the *real* work begins. For each category in the selected
	// accounts, we need to get the budget amount, the actual amount, and the difference
	// for each time period.

	// The real kicker is that the subtotal period may not necessarily fit the
	// budget period. A category budgeted in quarterly periods should not be arbitrarily
	// divided up by 3 for a monthly display -- it should be displayed only at the
	// beginning of each quarter. Yuck.
	int32 count = timelist.CountItems() - 1;

	// Just in case I missed it somewhere else. Otherwise, this will crash rather
	// spectacularly.
	// "Where's the kaboom? There's supposed to be an earth-shattering kaboom!"
	if (count < 1)
		return;

	// Add all the columns that we'll need for the report

	// The rest of the array will be initialized in the column-adding loop
	float maxwidths[count + 1];
	maxwidths[0] = be_plain_font->StringWidth("Category") + 20;
	fGridView->AddColumn(new BStringColumn("Category", maxwidths[0], 10, 300, B_TRUNCATE_END), 0);

	int32 i;
	for (i = 0; i < timelist.CountItems() - 1; i++) {
		char columntitle[128];
		struct tm *timestruct = localtime((time_t *)timelist.ItemAt(i));

		BString formatstring;
		switch (fSubtotalMode) {
		case SUBTOTAL_MONTH: {
			strftime(columntitle, 128, "%b %Y", timestruct);
			break;
		}
		case SUBTOTAL_QUARTER: {
			char year[10];
			int quarternumber = ((timestruct->tm_mon + 2) / 3) + 1;
			strftime(year, 10, "%Y", timestruct);
			sprintf(columntitle, "Q%d %s", quarternumber, year);
			break;
		}
		case SUBTOTAL_YEAR: {
			strftime(columntitle, 128, "%Y", timestruct);
			break;
		}
		default: {
			sprintf(columntitle, TRANSLATE("Amount"));
			break;
		}
		}
		maxwidths[i + 1] = be_plain_font->StringWidth(columntitle) + 20;
		fGridView->AddColumn(
			new BStringColumn(columntitle, maxwidths[i + 1], 10, 300, B_TRUNCATE_END), i + 1
		);
	}

	// Later this will iterate over all items in the category list
	BStringItem *stringitem = (BStringItem *)fCategoryList->ItemAt(0);
	if (!stringitem) {
		ShowBug("NULL category BStringItem in ReportWindow::ComputeBudget");
		return;
	}

	BudgetEntry budgetentry;

	// Remove this line when moving to code which loops over all categories
	gDatabase.GetBudgetEntry(stringitem->Text(), budgetentry);

	// TODO: Operate only on selected categories which are in the budget. For now,
	// only fight with the first entry
	//	if(	!stringitem->IsSelected() ||
	//			!gDatabase.GetBudgetEntry(stringitem->Text(),budgetentry) )
	//		continue;


	// Spacer row under title headings
	fGridView->AddRow(new BRow());

	// Construct and execute the query which finds all expense in the selected
	// category
	BString command;
	BString escaped = EscapeIllegalCharacters(stringitem->Text());
	int32 accountcount = 0;
	for (i = 0; i < fAccountList->CountItems(); i++) {
		AccountItem *item = (AccountItem *)fAccountList->ItemAt(i);
		if (!item || !item->IsSelected())
			continue;

		if (accountcount > 0)
			command << " union all ";

		command << "select date,amount from account_";
		command << item->account->GetID() << " where category = '" << escaped
				<< "' and date >= " << fStartDate << " and date < " << fEndDate;

		accountcount++;
	}
	command << " order by date;";
	CppSQLite3Query query = gDatabase.DBQuery(command.String(), "ComputeBudget::Get amounts");
	// printf("%s\n",command.String());
	if (query.eof())
		return;

	BRow *budrow = new BRow();
	fGridView->AddRow(budrow);
	budrow->SetField(new BStringField(stringitem->Text()), 0);

	BRow *amtrow = new BRow();
	fGridView->AddRow(amtrow);
	amtrow->SetField(new BStringField(""), 0);

	BRow *diffrow = new BRow();
	fGridView->AddRow(diffrow);
	diffrow->SetField(new BStringField(""), 0);

	float stringwidth = be_plain_font->StringWidth(stringitem->Text()) + 20;
	if (maxwidths[0] < stringwidth)
		maxwidths[0] = stringwidth;

	// This flag is for getting the next value from the row only when we don't
	// already have a value
	bool have_row = false;
	time_t transdate = 0;
	Fixed transamount;
	Fixed transtotal;

	// This loop places values for a row into corresponding places in each grid
	for (int32 subtotal_index = 0; subtotal_index < count; subtotal_index++) {
		time_t subtotal_start = *((time_t *)timelist.ItemAt(subtotal_index));
		time_t subtotal_end = *((time_t *)timelist.ItemAt(subtotal_index + 1));

		Fixed budamount = budgetentry.amount;
		map_status mapstatus = MapBudgetToSubtotal(
			budamount, budgetentry.period, fSubtotalMode, subtotal_start, subtotal_end
		);

		if (mapstatus == MAP_UNCHANGED)
			continue;
		else if (mapstatus == MAP_BAD_VALUE) {
			ShowBug("Passed bad value to ReportWindow::BudgetReport:MapBudgetToSubtotal");
			return;
		}

		if (!have_row) {
			transdate = query.getInt64Field(0);
			transamount.SetPremultiplied(query.getInt64Field(1));
			have_row = true;
		}

		if (transdate < subtotal_start)
			continue;

		while (transdate < subtotal_end) {
			transtotal += transamount;

			query.nextRow();

			if (query.eof())
				break;

			transdate = query.getInt64Field(0);
			transamount.SetPremultiplied(query.getInt64Field(1));
		}

		printf(
			"Actual vs Budget for %s: (%ld,%ld)\n", stringitem->Text(), transtotal.AsLong(),
			budamount.AsLong()
		);
		BString string;
		BStringField *field;

		string << budamount.AbsoluteValue().AsFloat();
		field = new BStringField(string.String());
		budrow->SetField(field, i);

		string = "";
		string << transtotal.AbsoluteValue().AsFloat();
		field = new BStringField(string.String());
		amtrow->SetField(field, i);

		transamount = budamount.AbsoluteValue() - transtotal.AbsoluteValue();

		string = "";
		string << transamount.AsFloat();
		field = new BStringField(string.String());
		diffrow->SetField(field, i);

	} // end for-each-subtotal-period row loop
	query.finalize();
	fGridView->AddRow(new BRow());
}

map_status
MapBudgetToSubtotal(
	Fixed &fixed, const int &budgetperiod, const int &subtotal, const time_t &start,
	const time_t &end
) {
	// Converts a Fixed value into the proper period. When the value is actually
	// converted (Subtotal-Monthly + Budget-Weekly, etc.), MAP_CHANGED is returned and
	// the actual value is found in the Fixed parameter originally passed to it.
	// When the value is untouched because the budget period is greater than the
	// subtotal period, MAP_UNCHANGED is returned. MAP_BAD_VALUE is kicked back only on
	// developer stupidity errors -- unsupported subtotal and budget periods.

	// This is currently unsupported
	if (subtotal == SUBTOTAL_WEEK) {
		ShowBug("Weekly Subtotal period in MapBudgetToSubtotal");
		return MAP_BAD_VALUE;
	}

	struct tm timestruct = *localtime(&start);

	switch (budgetperiod) {
	case BUDGET_WEEKLY: {
		switch (subtotal) {
		case SUBTOTAL_MONTH: {
			// Budget = $/Week, Report = Monthly
			fixed *= 4;
			break;
		}
		case SUBTOTAL_QUARTER: {
			// Budget = $/Week, Report = Quarterly
			// We use the average # of weeks in a quarter for this: 13.
			fixed *= 13;
			break;
		}
		case SUBTOTAL_YEAR: {
			// Budget = $/Week, Report = Yearly
			fixed *= 52;
			break;
		}
		case SUBTOTAL_NONE: {
			// Budget = $/Week, Report = Whatever
			time_t span = end - start;
			span /= SECONDS_PER_WEEK;
			fixed *= span;
			return MAP_CHANGED;
		}
		default: {
			return MAP_BAD_VALUE;
		}
		}
		break;
	}
	case BUDGET_MONTHLY: {
		switch (subtotal) {
		case SUBTOTAL_MONTH: {
			// Budget = $/Month, Report = Monthly
			// We do nothing because we want the monthly amount (which is
			// what we were given) in the amount box.
			break;
		}
		case SUBTOTAL_QUARTER: {
			// Budget = $/Month, Report = Quarterly
			fixed *= 4;
			break;
		}
		case SUBTOTAL_YEAR: {
			// Budget = $/Month, Report = Quarterly
			fixed *= 12;
			break;
		}
		case SUBTOTAL_NONE: {
			// Budget = $/Month, Report = Whatever
			// basing calculations on a 30-day should work well enough
			// for our purposes
			time_t span = end - start;
			span /= SECONDS_PER_DAY * 30;
			fixed *= (span < 1) ? 1 : span;
			break;
		}
		default: {
			return MAP_BAD_VALUE;
		}
		}
		break;
	}
	case BUDGET_QUARTERLY: {
		switch (subtotal) {
		case SUBTOTAL_MONTH: {
			// Budget = $/Quarter, Report = Monthly
			// If it's not the beginning of the quarter, return MAP_UNCHANGED
			// so that the caller knows to skip this box in the grid
			if (GetQuarterMonthForDate(start) != timestruct.tm_mon + 1)
				return MAP_UNCHANGED;

			// Note that we don't do anything if it's the beginning of the
			// quarter -- we *want* to return the budget amount in the box
			// for the beginning of the quarter
			break;
		}
		case SUBTOTAL_QUARTER: {
			// Budget = $/Quarter, Report = Quarterly
			break;
		}
		case SUBTOTAL_YEAR: {
			// Budget = $/Quarter, Report = Yearly
			fixed *= 4;
			break;
		}
		case SUBTOTAL_NONE: {
			// Budget = $/Quarter, Report = Whatever
			// Q1 is 90 days, Q2 is 91, Q3 is 92, Q4 is 91, so we will
			// go with the average: 91
			time_t span = end - start;
			span /= SECONDS_PER_DAY * 91;
			fixed *= (span < 1) ? 1 : span;
			break;
		}
		default: {
			return MAP_BAD_VALUE;
		}
		}
		break;
	}
	case BUDGET_ANNUALLY: {
		switch (subtotal) {
		case SUBTOTAL_MONTH:
		case SUBTOTAL_QUARTER: {
			// Budget = $/Year, Report = Monthly, Quarterly
			// On yearly basis for a budget item, we only show the
			// amount in January
			if (timestruct.tm_mon != 0)
				return MAP_UNCHANGED;
			break;
		}
		case SUBTOTAL_YEAR: {
			break;
		}
		case SUBTOTAL_NONE: {
			time_t span = end - start;
			span /= SECONDS_PER_YEAR;
			fixed *= (span < 1) ? 1 : span;
			break;
		}
		default: {
			return MAP_BAD_VALUE;
		}
		}
		break;
	}
	default:
		return MAP_BAD_VALUE;
	}
	return MAP_CHANGED;
}
