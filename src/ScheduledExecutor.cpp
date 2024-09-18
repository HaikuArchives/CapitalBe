#include "Database.h"
#include "ScheduledTransData.h"
#include "TimeSupport.h"
#include <Catalog.h>


void
HandleScheduledTransactions()
{
	// This function does one thing: enter any transactions which are
	// scheduled for entry

	CppSQLite3Query query = gDatabase.DBQuery("SELECT COUNT(*) FROM scheduledlist",
		"ScheduleListView::RefreshScheduleList: count transactions");

	if (query.eof())
		return;

	int transcount = query.getIntField(0);
	query.finalize();

	if (transcount < 1)
		return;

	uint32 idlist[transcount];

	query = gDatabase.DBQuery(
		"SELECT a.transid FROM scheduledlist as a LEFT JOIN accountlist AS b ON a.accountid = "
		"b.accountid WHERE b.status = \"open\" OR b.status = \"Open\";",
		"ScheduleListView::RefreshScheduleList: get transids");
	uint32 count = 0;
	idlist[count] = query.getInt64Field(0);
	query.nextRow();

	while (!query.eof()) {
		uint32 value = query.getInt64Field(0);
		if (value != idlist[count]) {
			count++;
			idlist[count] = value;
		}
		query.nextRow();
	}
	query.finalize();


	// Now that we have obtained the transaction IDs of all the scheduled transactions,
	// we need to actually get them from the database and add them to the list.
	time_t current = GetCurrentDate();
	for (uint32 i = 0; i <= count; i++) {
		ScheduledTransData sdata;

		if (!gDatabase.GetScheduledTransaction(idlist[i], sdata))
			continue;
		int32 destAccount = -1;
		destAccount = sdata.GetDestination();

		time_t saved_date = sdata.Date();
		bool updated = false;
		while (sdata.GetNextDueDate() < current) {
			updated = true;
			sdata.SetDate(sdata.GetNextDueDate());
			gDatabase.AddTransaction(sdata);

			// If this is a transfer between accounts, duplicate the transaction object
			// and modify account ID, payee and amount
			if (destAccount >= 0) {
				ScheduledTransData ddata = sdata;
				ddata.SetAccount(gDatabase.AccountByID(destAccount));
				BString payee = B_TRANSLATE_CONTEXT("Transfer from '%%PAYEE%%'", "MainWindow");
				payee.ReplaceFirst("%%PAYEE%%", sdata.GetAccount()->Name());
				ddata.SetPayee(payee);
				ddata.SetAmount(sdata.Amount().InvertAsCopy());
				gDatabase.AddTransaction(ddata);
			}

			sdata.CalculateNextDueDate();
		}

		if (updated) {
			gDatabase.RemoveScheduledTransaction(sdata.GetID());
			sdata.SetDate(saved_date);
			gDatabase.AddScheduledTransaction(sdata, false);
		}
	}
}
