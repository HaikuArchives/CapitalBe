/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	bitigchi (Emir Sari)
 *	humdinger (Joachim Seemer)
 *	dospuntos (Johan Wagenheim)
 */
#include "Account.h"
#include "CBLocale.h"
#include "ColumnListView.h"
#include "ColumnTypes.h"
#include "Database.h"
#include "ReportGrid.h"
#include "ReportWindow.h"
#include "TimeSupport.h"

#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CashFlowReport"


void
ReportWindow::_ComputeCashFlow()
{
	// Calculate the number of columns and the starting date for each one
	BObjectList<time_t> timelist(20, true);

	for (time_t t = fStartDate; t < fEndDate;) {
		time_t* item = new time_t(t);
		timelist.AddItem(item);

		switch (fSubtotalMode) {
			case SUBTOTAL_MONTH:
			{
				t = IncrementDateByMonth(t);
				break;
			}
			case SUBTOTAL_QUARTER:
			{
				t = IncrementDateByQuarter(t);
				break;
			}
			case SUBTOTAL_YEAR:
			{
				t = IncrementDateByYear(t);
				break;
			}
			default:
			{
				t = fEndDate;
				break;
			}
		}
	}
	timelist.AddItem(new time_t(fEndDate));

	ReportGrid incomegrid(timelist.CountItems() - 1, 0), expensegrid(timelist.CountItems() - 1, 0);
	BString longestname("");

	for (int32 i = 0; i < fCategoryList->CountItems(); i++) {
		BStringItem* catitem = (BStringItem*)fCategoryList->ItemAt(i);
		if (!catitem || !catitem->IsSelected() || strlen(catitem->Text()) < 1)
			continue;

		if ((uint32)longestname.CountChars() < strlen(catitem->Text()))
			longestname = catitem->Text();

		BString escaped = catitem->Text();

		CppSQLite3Query query;

		BString command, expcommand;
		int32 accountcount = 0;
		/*		if(!query.eof())
				{
					doincome = true;
					incomegrid.AddItem();
					incomegrid.SetRowTitle(incomegrid.CountItems()-1,catitem->Text());
				}
		*/

		/*		if(!query.eof())
				{
					query.finalize();
					doexpense = true;
					expensegrid.AddItem();
					expensegrid.SetRowTitle(expensegrid.CountItems()-1,catitem->Text());
				}
		*/

		int32 count = timelist.CountItems() - 1;
		for (int32 subtotal_index = 0; subtotal_index < count; subtotal_index++) {
			time_t subtotal_start = *((time_t*)timelist.ItemAt(subtotal_index));
			time_t subtotal_end = *((time_t*)timelist.ItemAt(subtotal_index + 1));

			accountcount = 0;
			command = expcommand = "";
			CppSQLite3Buffer commandBuffer, expcommandBuffer;

			for (int32 i = 0; i < fAccountList->CountItems(); i++) {
				AccountItem* item = (AccountItem*)fAccountList->ItemAt(i);
				if (!item || !item->IsSelected())
					continue;

				if (accountcount > 0) {
					command << " UNION ALL ";
					expcommand << " UNION ALL ";
				}

				BString account;
				account << "account_" << item->account->GetID();
				commandBuffer.format(
					"SELECT SUM(amount) from %s WHERE category = %Q AND amount > "
					"0 AND date >= %" B_PRIdTIME " AND date < %" B_PRIdTIME,
					account.String(), escaped.String(), subtotal_start, subtotal_end);
				command << commandBuffer;

				expcommandBuffer.format(
					"SELECT SUM(amount) from %s WHERE category = %Q AND amount "
					"< 0 AND date >= %" B_PRIdTIME " AND date < %" B_PRIdTIME,
					account.String(), escaped.String(), subtotal_start, subtotal_end);
				expcommand << expcommandBuffer;

				accountcount++;
			}
			command << ";";
			expcommand << ";";

			// query for income in this category
			query = gDatabase.DBQuery(command.String(), "ReportWindow::ComputeCashFlow:income");

			if (!query.eof()) {
				Fixed inctotal, f;

				while (!query.eof()) {
					f.SetPremultiplied(query.getInt64Field(0));
					inctotal += f;
					query.nextRow();
				}
				if (inctotal > 0) {
					incomegrid.AddItem();
					incomegrid.SetRowTitle(incomegrid.CountItems() - 1, catitem->Text());
					incomegrid.SetValue(subtotal_index, incomegrid.CountItems() - 1, inctotal);
				}
			} else
				incomegrid.SetValue(subtotal_index, incomegrid.CountItems() - 1, Fixed());

			// query for expenses in this category
			query = gDatabase.DBQuery(expcommand.String(), "ReportWindow::ComputeCashFlow:expense");

			if (!query.eof()) {
				Fixed exptotal, f;

				while (!query.eof()) {
					f.SetPremultiplied(query.getInt64Field(0));
					exptotal += f.AbsoluteValue();
					query.nextRow();
				}
				if (exptotal > 0) {
					expensegrid.AddItem();
					expensegrid.SetRowTitle(expensegrid.CountItems() - 1, catitem->Text());
					expensegrid.SetValue(subtotal_index, expensegrid.CountItems() - 1, exptotal);
				}
			} else
				expensegrid.SetValue(subtotal_index, expensegrid.CountItems() - 1, Fixed());

			query.finalize();
		}
	}
	incomegrid.Sort();
	expensegrid.Sort();

	// Now that we have all the data, we need to set up the rows and columns for the report grid
	BColumn* col = new BStringColumn(B_TRANSLATE_CONTEXT("Category", "CommonTerms"),
		fGridView->StringWidth(longestname.String()) + 20, 10, 300, B_TRUNCATE_END);
	fGridView->AddColumn(col, 0);

	int32 i;
	for (i = 0; i < timelist.CountItems() - 1; i++) {
		char columntitle[128];
		struct tm* timestruct = localtime((time_t*)timelist.ItemAt(i));

		BString formatstring;
		switch (fSubtotalMode) {
			case SUBTOTAL_MONTH:
			{
				strftime(columntitle, 128, "%b %Y", timestruct);
				break;
			}
			case SUBTOTAL_QUARTER:
			{
				char year[10];
				int quarternumber = (ceil(timestruct->tm_mon / 3) + 1);
				strftime(year, 10, "%Y", timestruct);
				sprintf(columntitle, "Q%d %s", quarternumber, year);
				break;
			}
			case SUBTOTAL_YEAR:
			{
				strftime(columntitle, 128, "%Y", timestruct);
				break;
			}
			default:
			{
				sprintf(columntitle, B_TRANSLATE_CONTEXT("Amount", "CommonTerms"));
				break;
			}
		}

		col = new BStringColumn(
			columntitle, fGridView->StringWidth("$1,000,000.00"), 10, 300, B_TRUNCATE_END);
		fGridView->AddColumn(col, i + 1);
	}

	fGridView->AddRow(new BRow());
	BRow* titlerow = new BRow();
	fGridView->AddRow(titlerow);
	titlerow->SetField(new BStringField(B_TRANSLATE_CONTEXT("Income", "CommonTerms")), 0);
	fGridView->AddRow(new BRow());

	if (!fGridView->IsHidden()) {
		// Now that the grid is set up, start adding data to the grid
		for (i = 0; i < incomegrid.CountItems(); i++) {
			BRow* row = new BRow();
			fGridView->AddRow(row);

			BStringField* catname = new BStringField(incomegrid.RowTitle(i));
			row->SetField(catname, 0);

			for (int32 columnindex = 0; columnindex < timelist.CountItems(); columnindex++) {
				BString temp;
				Fixed f;

				incomegrid.ValueAt(columnindex, i, f);
				gCurrentLocale.CurrencyToString(f, temp);

				BStringField* amountfield = new BStringField(temp.String());
				row->SetField(amountfield, columnindex + 1);
			}
		}

		fGridView->AddRow(new BRow());
		titlerow = new BRow();
		fGridView->AddRow(titlerow);
		titlerow->SetField(new BStringField(B_TRANSLATE_CONTEXT("Expenses", "CommonTerms")), 0);
		fGridView->AddRow(new BRow());

		for (i = 0; i < expensegrid.CountItems(); i++) {
			BRow* row = new BRow();
			fGridView->AddRow(row);

			BStringField* catname = new BStringField(expensegrid.RowTitle(i));
			row->SetField(catname, 0);

			for (int32 columnindex = 0; columnindex < timelist.CountItems(); columnindex++) {
				BString temp;
				Fixed f;

				expensegrid.ValueAt(columnindex, i, f);
				gCurrentLocale.CurrencyToString(f, temp);

				BStringField* amountfield = new BStringField(temp.String());
				row->SetField(amountfield, columnindex + 1);
			}
		}
	} else {
		// Graph view is showing. Render the graph
		// TODO: Implement as line graph

		/*
				The main point behind the graphs is to give the user a different way of looking
				at the data in order to calculate a trend and also be able to look at quite a bit of
		   data for an extended period of time, which the report view doesn't do a very good job of
		   showing.

				As such, what we need is a line graph view.
		*/
	}
}
