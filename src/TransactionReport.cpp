#include <Catalog.h>
#include <stdlib.h>
#include "Account.h"
#include "CBLocale.h"
#include "ColumnListView.h"
#include "ColumnTypes.h"
#include "Database.h"
#include "ReportGrid.h"
#include "ReportWindow.h"
#include "TimeSupport.h"
#include "Transaction.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TransactionReport"


void
ReportWindow::ComputeTransactions(void)
{
	// Total of all accounts
	// Calculate the number of columns and the starting date for each one
	BObjectList<time_t> timelist(20, true);

	if (fSubtotalMode == SUBTOTAL_NONE) {
		timelist.AddItem(new time_t(fStartDate));
		timelist.AddItem(new time_t(fEndDate));
	} else {
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
	}

	BString longestname(B_TRANSLATE("Transactions"));
	//	int longestnamelength = 12;

	BColumn* col = new BStringColumn("", fGridView->StringWidth(B_TRANSLATE("Transactions")) + 20,
		10, 300, B_TRUNCATE_END, B_ALIGN_RIGHT);
	fGridView->AddColumn(col, 0);
	col = new BStringColumn(B_TRANSLATE_CONTEXT("Date", "CommonTerms"),
		fGridView->StringWidth("00-00-0000") + 15, 10, 300, B_TRUNCATE_END);
	fGridView->AddColumn(col, 1);
	col = new BStringColumn(B_TRANSLATE_CONTEXT("Type", "CommonTerms"),
		fGridView->StringWidth(B_TRANSLATE_CONTEXT("Type", "CommonTerms")) + 20, 10, 300,
		B_TRUNCATE_END);
	fGridView->AddColumn(col, 2);
	col = new BStringColumn(
		B_TRANSLATE_CONTEXT("Payee", "CommonTerms"), 75, 10, 300, B_TRUNCATE_END);
	fGridView->AddColumn(col, 3);

	// The string we use for calculating width here should work well enough for general purposes
	col = new BStringColumn(B_TRANSLATE_CONTEXT("Amount", "CommonTerms"),
		fGridView->StringWidth("$00,000.00") + 15, 10, 300, B_TRUNCATE_END);
	fGridView->AddColumn(col, 4);
	col = new BStringColumn(B_TRANSLATE_CONTEXT("Category", "CommonTerms"),
		fGridView->StringWidth("0000000000") + 20, 10, 300, B_TRUNCATE_END);
	fGridView->AddColumn(col, 5);
	col = new BStringColumn(
		B_TRANSLATE_CONTEXT("Memo", "CommonTerms"), 75, 10, 300, B_TRUNCATE_END);
	fGridView->AddColumn(col, 6);

	fGridView->AddRow(new BRow());
	BRow* titlerow = new BRow();
	fGridView->AddRow(titlerow);
	titlerow->SetField(new BStringField(B_TRANSLATE("Transactions")), 0);
	fGridView->AddRow(new BRow());


	int32 payeechars = strlen(B_TRANSLATE_CONTEXT("Payee", "CommonTerms")),
		  memochars = strlen(B_TRANSLATE_CONTEXT("Memo", "CommonTerms")),
		  categorychars = strlen(B_TRANSLATE_CONTEXT("Category", "CommonTerms"));
	float maxpayee = fGridView->StringWidth(B_TRANSLATE_CONTEXT("Payee", "CommonTerms")),
		  maxmemo = fGridView->StringWidth(B_TRANSLATE_CONTEXT("Memo", "CommonTerms")),
		  maxcategory = fGridView->StringWidth(B_TRANSLATE_CONTEXT("Category", "CommonTerms"));

	int32 count = timelist.CountItems() - 1;
	for (int32 subtotal_index = 0; subtotal_index < count; subtotal_index++) {
		time_t subtotal_start = *((time_t*)timelist.ItemAt(subtotal_index));
		time_t subtotal_end = *((time_t*)timelist.ItemAt(subtotal_index + 1));

		BRow* row = new BRow();
		fGridView->AddRow(row);

		BString datestring, tempstr;
		gDefaultLocale.DateToString(subtotal_start, datestring);
		row->SetField(new BStringField(datestring.String()), 0);
		row->SetField(new BStringField("-"), 1);
		row = new BRow();
		fGridView->AddRow(row);
		gDefaultLocale.DateToString(subtotal_end, datestring);
		row->SetField(new BStringField(datestring.String()), 0);

		fGridView->AddRow(new BRow());

		BString command;

		int32 accountcount = 0;
		for (int32 i = 0; i < fAccountList->CountItems(); i++) {
			AccountItem* item = (AccountItem*)fAccountList->ItemAt(i);
			if (!item || !item->IsSelected())
				continue;

			if (accountcount > 0)
				command << " union all ";

			command << "SELECT date,type,payee,amount,category,memo,transid FROM account_"
					<< item->account->GetID() << " WHERE category IN (" << fCategoryString
					<< ") AND date >= " << subtotal_start << " AND date < " << subtotal_end;

			accountcount++;
		}
		command << " ORDER BY date,transid;";
		CppSQLite3Query query
			= gDatabase.DBQuery(command.String(), "ReportWindow::ComputeTransactions()");

		if (query.eof()) {
			row = new BRow();
			fGridView->AddRow(row);
			row->SetField(new BStringField(B_TRANSLATE("No transactions")), 0);
			fGridView->ColumnAt(0)->SetWidth(
				fGridView->StringWidth(B_TRANSLATE("No transactions")) + 15);
		} else
			while (!query.eof()) {
				row = new BRow();
				fGridView->AddRow(row);

				row->SetField(new BStringField(""), 0);

				// date
				gDefaultLocale.DateToString(query.getInt64Field(0), tempstr);
				row->SetField(new BStringField(tempstr.String()), 1);

				// type
				row->SetField(
					new BStringField(DeescapeIllegalCharacters(query.getStringField(1)).String()),
					2);

				// payee
				tempstr = DeescapeIllegalCharacters(query.getStringField(2));
				if (tempstr.CountChars() > payeechars) {
					payeechars = tempstr.CountChars();
					maxpayee = fGridView->StringWidth(tempstr.String());
				}
				row->SetField(new BStringField(tempstr.String()), 3);

				// amount
				Fixed f(query.getInt64Field(3), true);
				gCurrentLocale.CurrencyToString(f.AbsoluteValue(), tempstr);
				row->SetField(new BStringField(tempstr.String()), 4);

				// category
				tempstr = DeescapeIllegalCharacters(query.getStringField(4));
				if (tempstr.CountChars() > categorychars) {
					categorychars = tempstr.CountChars();
					maxcategory = fGridView->StringWidth(tempstr.String());
				}
				row->SetField(new BStringField(tempstr.String()), 5);

				// memo
				if (!query.fieldIsNull(5)) {
					tempstr = DeescapeIllegalCharacters(query.getStringField(5));
					if (tempstr.CountChars() > memochars) {
						memochars = tempstr.CountChars();
						maxmemo = fGridView->StringWidth(tempstr.String());
					}
					row->SetField(new BStringField(tempstr.String()), 6);
				}

				query.nextRow();
			}
		query.finalize();

		fGridView->AddRow(new BRow());
	}
	fGridView->ColumnAt(3)->SetWidth(maxpayee + 15);
	fGridView->ColumnAt(5)->SetWidth(maxcategory + 15);
	fGridView->ColumnAt(6)->SetWidth(maxmemo + 15);
}
