#include "ScheduleAddWindow.h"

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <String.h>
#include <StringView.h>
#include <stdlib.h>

#include "CBLocale.h"
#include "Database.h"
#include "DateBox.h"
#include "Layout.h"
#include "NumBox.h"
#include "ScheduledTransData.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ScheduleAddWindow"

enum {
	M_SCHEDULED_MONTHLY = 'schm',
	M_SCHEDULED_WEEKLY,
	M_SCHEDULED_QUARTERLY,
	M_SCHEDULED_ANNUALLY,
	M_DATE_CHANGED,
	M_COUNT_CHANGED,
	M_REPEAT_ALWAYS,
	M_REPEAT_LIMITED,
	M_SCHEDULE_TRANSACTION
};

ScheduleAddWindow::ScheduleAddWindow(const BRect& frame, const TransactionData& data)
	: BWindow(
		  frame, B_TRANSLATE("Schedule transaction"), B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		  B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_AUTO_UPDATE_SIZE_LIMITS
	  ),
	  fTransData(data)
{
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	BView* back = new BView("backview", B_WILL_DRAW);
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0).SetInsets(0).Add(back).End();
	back->SetViewColor(240, 240, 240);

	BString label;
	label.SetToFormat(B_TRANSLATE("Type: %s"), data.Type().Type());
	BStringView* typelabel = new BStringView("typelabel", label.String());

	label.SetToFormat(B_TRANSLATE("Payee: %s"), data.Payee());
	BStringView* payeelabel = new BStringView("payeelabel", label.String());

	BString temp;
	gCurrentLocale.CurrencyToString(data.Amount().AbsoluteValue(), temp);
	label.SetToFormat(B_TRANSLATE("Amount: %s"), temp);

	BStringView* amountlabel = new BStringView("amountlabel", label.String());

	label = B_TRANSLATE("Category:");
	label += " ";
	if (data.CountCategories() > 1)
		label << B_TRANSLATE("Split");
	else
		label << data.NameAt(0);

	BStringView* categorylabel = new BStringView("categorylabel", label.String());

	label = B_TRANSLATE("Memo:");
	label << " " << data.Memo();
	BStringView* memolabel = new BStringView("memolabel", label.String());

	//	Since layout-api, we need other way to make divider
	//	BBox *divider = new BBox(r);
	//	AddChild(divider);

	fIntervalMenu = new BMenu(B_TRANSLATE("Frequency"));
	fIntervalMenu->AddItem(new BMenuItem(B_TRANSLATE("Monthly"), new BMessage(M_SCHEDULED_MONTHLY))
	);
	fIntervalMenu->AddItem(
		new BMenuItem(B_TRANSLATE("Quarterly"), new BMessage(M_SCHEDULED_QUARTERLY))
	);
	fIntervalMenu->AddItem(
		new BMenuItem(B_TRANSLATE("Annually"), new BMessage(M_SCHEDULED_ANNUALLY))
	);
	fIntervalMenu->ItemAt(0)->SetMarked(true);
	fIntervalMenu->SetLabelFromMarked(true);

	temp = B_TRANSLATE("Frequency:");
	temp += " ";
	BMenuField* intervalfield = new BMenuField("intervalfield", temp.String(), fIntervalMenu);

	temp = B_TRANSLATE("Starting date:");
	temp += " ";
	fStartDate = new DateBox("startdate", temp.String(), "", new BMessage(M_DATE_CHANGED));
	fStartDate->UseTabFiltering(false);
	gDefaultLocale.DateToString(data.Date(), temp);
	fStartDate->SetText(temp.String());

	fRepeatAlways =
		new BRadioButton("inftimes", B_TRANSLATE("Indefinitely"), new BMessage(M_REPEAT_ALWAYS));

	fRepeatLimited = new BRadioButton("limitedtimes", "", new BMessage(M_REPEAT_LIMITED));

	fRepeatCount = new NumBox("repeatcount", NULL, "999", new BMessage(M_COUNT_CHANGED));
	fRepeatCount->UseTabFiltering(false);
	fRepeatCount->SetEnabled(false);

	BStringView* timeslabel = new BStringView("timeslabel", B_TRANSLATE("times"));

	fRepeatAlways->SetValue(B_CONTROL_ON);

	intervalfield->MakeFocus(true);

	BButton* okbutton =
		new BButton("okbutton", B_TRANSLATE("Cancel"), new BMessage(M_SCHEDULE_TRANSACTION));
	okbutton->MakeDefault(true);
	okbutton->SetLabel(B_TRANSLATE("OK"));

	BButton* cancelbutton =
		new BButton("cancelbutton", B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));
	cancelbutton->MakeDefault(true);

	BLayoutBuilder::Group<>(back, B_VERTICAL)
		.SetInsets(10)
		.AddGrid(8.0f, 1.0f)
		.Add(typelabel, 0, 0)
		.Add(payeelabel, 1, 0)
		.Add(amountlabel, 2, 0)
		.Add(categorylabel, 0, 1)
		.Add(memolabel, 1, 1)
		.End()
		.AddGrid(1.0f, 1.0f)
		.Add(intervalfield, 0, 0)
		.Add(fRepeatAlways, 1, 0)
		.Add(fStartDate, 0, 1)
		.AddGrid(1.0f, 1.0f, 1, 1)
		.Add(fRepeatLimited, 0, 0)
		.Add(fRepeatCount, 1, 0)
		.End()
		.End()
		.AddGrid(1.0f, 1.0f)
		.AddGlue(0, 0)
		.Add(cancelbutton, 1, 0)
		.Add(okbutton, 2, 0)
		.End()
		.End();
}

void
ScheduleAddWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case M_REPEAT_ALWAYS: {
		fRepeatCount->SetEnabled(false);
		break;
	}
	case M_REPEAT_LIMITED: {
		fRepeatCount->SetEnabled(true);
		break;
	}
	case M_COUNT_CHANGED: {
		if (fRepeatLimited->Value() == B_CONTROL_ON) {
			if (strlen(fRepeatCount->Text()) < 1)
				fRepeatCount->SetText("1");

			if (atoi(fRepeatCount->Text()) < 1)
				fRepeatCount->SetText("1");
		}
		break;
	}
	case M_SCHEDULE_TRANSACTION: {
		ScheduledTransData stdata(fTransData);

		BMenuItem* intervalitem = fIntervalMenu->FindMarked();
		if (!intervalitem) {
			ShowBug("NULL menu item in ScheduleAddWindow");
			break;
		}

		switch (fIntervalMenu->IndexOf(intervalitem)) {
		case 0: {
			stdata.SetInterval(SCHEDULED_MONTHLY);
			break;
		}
		case 1: {
			stdata.SetInterval(SCHEDULED_QUARTERLY);
			break;
		}
		case 2: {
			stdata.SetInterval(SCHEDULED_ANNUALLY);
			break;
		}
		default: {
			ShowBug("Bad Interval index in ScheduleAddWindow");
			break;
		}
		}

		if (fRepeatCount->IsEnabled())
			stdata.SetCount(atoi(fRepeatCount->Text()));

		time_t tempdate;
		BString datestr = fStartDate->Text();
		if (datestr.CountChars() < 3 ||
			gDefaultLocale.StringToDate(datestr.String(), tempdate) != B_OK) {
			ShowAlert(
				B_TRANSLATE("CapitalBe didn't understand the date you entered."),
				B_TRANSLATE("CapitalBe understands lots of different ways of entering dates. "
							"Apparently, this wasn't one of them. You'll need to change how you "
							"entered this date. Sorry.")
			);
			break;
		}

		stdata.SetDate(tempdate);

		gDatabase.AddScheduledTransaction(stdata);

		if (fTransData.Type().TypeCode() == TRANS_XFER) {
			// Get the counterpart and add it to the scheduled list
			gDatabase.GetTransferCounterpart(stdata.GetID(), stdata);
			gDatabase.AddScheduledTransaction(stdata);
		}

		PostMessage(B_QUIT_REQUESTED);
		break;
	}
	default:
		BWindow::MessageReceived(msg);
	}
}
