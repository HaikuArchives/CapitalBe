#include "Database.h"
#include "ScheduledTransData.h"
#include "TimeSupport.h"

void
HandleScheduledTransactions(void)
{
	// This function does one thing: enter any transactions which are
	// scheduled for entry

	CppSQLite3Query query = gDatabase.DBQuery("select count(*) from scheduledlist",
		"ScheduleListView::RefreshScheduleList: count transactions");

	if (query.eof())
		return;

	int transcount = query.getIntField(0);
	query.finalize();

	if (transcount < 1)
		return;

	uint32 idlist[transcount];

	query = gDatabase.DBQuery("select transid from scheduledlist order by transid",
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

		time_t saved_date = sdata.Date();
		bool updated = false;
		while (sdata.GetNextDueDate() < current) {
			updated = true;
			sdata.SetDate(sdata.GetNextDueDate());
			gDatabase.AddTransaction(sdata);

			sdata.CalculateNextDueDate();
		}

		if (updated) {
			gDatabase.RemoveScheduledTransaction(sdata.GetID());
			sdata.SetDate(saved_date);
			gDatabase.AddScheduledTransaction(sdata, false);
		}
	}
}
