#include "ScheduleListWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <ListItem.h>
#include <ListView.h>
#include <Message.h>
#include <Messenger.h>
#include <Region.h>
#include <ScrollView.h>
#include <TextView.h>
#include <View.h>

#include "ColumnListView.h"
#include "ColumnTypes.h"
#include "Database.h"
#include "HelpButton.h"
#include "Preferences.h"
#include "ScheduledTransData.h"
#include "ScheduledTransItem.h"
#include "TransactionLayout.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ScheduleListWindow"


enum {
	M_REMOVE_ITEM = 'rmit'
};

class ScheduleListView : public BView {
public:
	ScheduleListView(const char* name, const int32& flags);
	void AttachedToWindow(void);
	void MessageReceived(BMessage* msg);

private:
	// This is a float so we can get the maximum string width for payees.
	float RefreshScheduleList(void);

	BColumnListView* fListView;

	BButton* fRemoveButton;
	BList fTransList;
	HelpButton* fHelpButton;

	float fBestWidth;
};

ScheduleListView::ScheduleListView(const char* name, const int32& flags)
	: BView(name, flags)
{
	// the buttons
	fRemoveButton = new BButton(
		"removebutton", B_TRANSLATE("Remove"), new BMessage(M_REMOVE_ITEM));

	// the transaction list
	fListView = new BColumnListView("listview", B_FANCY_BORDER);

	fListView->SetSortingEnabled(false);
	fListView->SetEditMode(false);

	fListView->AddColumn(new BStringColumn(B_TRANSLATE("Payee"), 100, 25, 300, B_ALIGN_LEFT), 0);

	float amountwidth = StringWidth("$000,000.00");
	float amountlabelwidth = StringWidth(B_TRANSLATE("Amount"));
	fListView->AddColumn(new BStringColumn(B_TRANSLATE("Amount"),
							 MAX(amountwidth, amountlabelwidth), 25, 300, B_ALIGN_LEFT),
		1);
	fListView->AddColumn(new BStringColumn(B_TRANSLATE("Payments"),
							 StringWidth(B_TRANSLATE("Payments")) + 20, 25, 300, B_ALIGN_LEFT),
		2);
	fListView->AddColumn(new BStringColumn(B_TRANSLATE("Frequency"),
							 StringWidth(B_TRANSLATE("Frequency")) + 20, 25, 300, B_ALIGN_LEFT),
		3);
	fListView->AddColumn(new BStringColumn(B_TRANSLATE("Next Payment"),
							 StringWidth(B_TRANSLATE("Next Payment")) + 20, 25, 300, B_ALIGN_LEFT),
		4);

	float maxwidth = RefreshScheduleList();
	fBestWidth = (fRemoveButton->Frame().Width() * 2) + 45;
	fBestWidth = MAX(fBestWidth, maxwidth + 35);

	prefsLock.Lock();
	BString schedhelp = gAppPath;
	prefsLock.Unlock();
	schedhelp << "helpfiles/"
			  << B_TRANSLATE_COMMENT("English",
					 "Path to localized helpfiles. Only translate if available in your language.")
			  << "/Scheduled Transaction Window Help";
	fHelpButton = new HelpButton("schedhelp", schedhelp.String());

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(15, 15)
		.AddGrid(1.0f, 1.0f)
		.Add(fListView, 0, 0, 3)
		.AddGrid(1.0f, 1.0f, 0, 1, 3)
		.AddGlue(0, 0)
		.Add(fHelpButton, 1, 0)
		.Add(fRemoveButton, 2, 0)
		.End()
		.End()
		.End();
}


void
ScheduleListView::AttachedToWindow(void)
{
	fListView->SetTarget(this);
	fRemoveButton->SetTarget(this);

	Window()->ResizeTo(fBestWidth, Window()->Frame().Height());
	fListView->MakeFocus(true);
}

void
ScheduleListView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_REMOVE_ITEM:
		{
			int32 selection = fListView->IndexOf(fListView->CurrentSelection());
			if (selection < 0)
				break;

			ScheduledTransData* data = (ScheduledTransData*)fTransList.ItemAt(selection);
			gDatabase.RemoveScheduledTransaction(data->GetID());

			fTransList.RemoveItem(data);
			delete data;

			// TODO: Do we need to delete this row ourselves?
			fListView->RemoveRow(fListView->CurrentSelection());
			break;
		}
		default:
		{
			BView::MessageReceived(msg);
			break;
		}
	}
}

float
ScheduleListView::RefreshScheduleList(void)
{
	for (int32 i = 0; i < fTransList.CountItems(); i++) {
		ScheduledTransData* data = (ScheduledTransData*)fTransList.ItemAt(i);
		delete data;
	}
	fTransList.MakeEmpty();
	fListView->Clear();

	CppSQLite3Query query = gDatabase.DBQuery("select count(*) from scheduledlist",
		"ScheduleListView::RefreshScheduleList: count transactions");

	if (query.eof())
		return 0;

	int transcount = query.getIntField(0);
	query.finalize();

	if (transcount < 1)
		return 0;

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
	float maxwidth = 0;

	for (uint32 i = 0; i <= count; i++) {
		ScheduledTransData* sdata = new ScheduledTransData();
		if (!gDatabase.GetScheduledTransaction(idlist[i], *sdata)) {
			delete sdata;
			continue;
		}

		float width = StringWidth(sdata->Payee());
		maxwidth = MAX(maxwidth, width);

		fTransList.AddItem(sdata);
	}

	for (int32 i = 0; i < fTransList.CountItems(); i++) {
		BRow* row = new BRow();
		fListView->AddRow(row);

		ScheduledTransData* sdata = (ScheduledTransData*)fTransList.ItemAt(i);

		row->SetField(new BStringField(sdata->Payee()), 0);

		BString string;
		Locale locale = sdata->GetAccount()->GetLocale();
		locale.CurrencyToString(sdata->Amount().AbsoluteValue(), string);

		row->SetField(new BStringField(sdata->Payee()), 0);
		row->SetField(new BStringField(string.String()), 1);

		if (sdata->GetCount() > 0) {
			string = "";
			string << sdata->GetCount();
		} else
			string = B_TRANSLATE("Unlimited");

		row->SetField(new BStringField(string.String()), 2);

		switch (sdata->GetInterval()) {
			case SCHEDULED_MONTHLY:
			{
				string = B_TRANSLATE("Monthly");
				break;
			}
			case SCHEDULED_WEEKLY:
			{
				string = B_TRANSLATE("Weekly");
				break;
			}
			case SCHEDULED_QUARTERLY:
			{
				string = B_TRANSLATE("Quarterly");
				break;
			}
			case SCHEDULED_ANNUALLY:
			{
				string = B_TRANSLATE("Annually");
				break;
			}
			default:
			{
				string = B_TRANSLATE("Unknown");
				break;
			}
		}

		// frequency
		row->SetField(new BStringField(string.String()), 3);

		// next pay date
		gDefaultLocale.DateToString(sdata->GetNextDueDate(), string);
		row->SetField(new BStringField(string.String()), 4);
	}

	return fListView->PreferredSize().Width();
}


ScheduleListWindow::ScheduleListWindow(const BRect& frame)
	: BWindow(frame, B_TRANSLATE("Scheduled transactions"), B_DOCUMENT_WINDOW_LOOK,
		  B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS | B_CLOSE_ON_ESCAPE)
{
	ScheduleListView* view = new ScheduleListView("schedview", B_WILL_DRAW);
	BLayoutBuilder::Group<>(this, B_VERTICAL).SetInsets(0).Add(view).End();
	CenterIn(Frame());

	//	AddShortcut('A',B_COMMAND_KEY, new BMessage(M_SHOW_ADD_WINDOW),view);
	//	AddShortcut('R',B_COMMAND_KEY, new BMessage(M_REMOVE_CATEGORY),view);
}
