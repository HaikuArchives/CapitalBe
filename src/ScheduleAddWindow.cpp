#include "ScheduleAddWindow.h"

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <SeparatorView.h>
#include <String.h>
#include <StringView.h>
#include <stdlib.h>

#include "CBLocale.h"
#include "CalendarButton.h"
#include "Database.h"
#include "DateBox.h"
#include "NumBox.h"
#include "ScheduledExecutor.h"
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
	: BWindow(frame, B_TRANSLATE("Schedule transaction"), B_TITLED_WINDOW_LOOK,
		B_MODAL_APP_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	  fTransData(data)
{
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	// Type
	BStringView* typeLabel
		= new BStringView("typelabel", B_TRANSLATE_CONTEXT("Type", "CommonTerms"));
	typeLabel->SetExplicitSize(BSize(be_plain_font->StringWidth("ShortType"), B_SIZE_UNSET));
	BTextControl* type = new BTextControl("type", NULL, data.Type().Type(), NULL);
	type->SetEnabled(false);

	// Payee
	BStringView* payeeLabel
		= new BStringView("payeelabel", B_TRANSLATE_CONTEXT("Payee", "CommonTerms"));
	payeeLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	payeeLabel->SetExplicitMinSize(
		BSize(be_plain_font->StringWidth("anAveragelyLongPayee"), B_SIZE_UNSET));
	BTextControl* payee = new BTextControl("payee", NULL, data.Payee(), NULL);
	payee->SetEnabled(false);

	// Amount
	BStringView* amountLabel = new BStringView("amountlabel", B_TRANSLATE("Amount"));
	amountLabel->SetExplicitSize(
		BSize(be_plain_font->StringWidth("$10,000,000,000.00"), B_SIZE_UNSET));
	BString temp;
	gCurrentLocale.CurrencyToString(data.Amount().AbsoluteValue(), temp);
	BTextControl* amount = new BTextControl("amount", NULL, temp.String(), NULL);
	amount->SetEnabled(false);

	// Category
	BStringView* categoryLabel
		= new BStringView("categorylabel", B_TRANSLATE_CONTEXT("Category", "CommonTerms"));
	categoryLabel->SetExplicitSize(
		BSize(be_plain_font->StringWidth("aLongCategoryName"), B_SIZE_UNSET));
	BString label;
	if (data.CountCategories() > 1)
		label << B_TRANSLATE_CONTEXT("Split", "CommonTerms");
	else
		label << data.NameAt(0);
	BTextControl* category = new BTextControl("category", NULL, label.String(), NULL);
	category->SetEnabled(false);

	// Memo
	BStringView* memoLabel
		= new BStringView("memolabel", B_TRANSLATE_CONTEXT("Memo", "CommonTerms"));
	memoLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	memoLabel->SetExplicitMinSize(
		BSize(be_plain_font->StringWidth("$10,000,000,000.00"), B_SIZE_UNSET));
	BTextControl* memo = new BTextControl("memo", NULL, data.Memo(), NULL);
	memo->SetEnabled(false);

	fIntervalMenu = new BMenu("Frequency");
	fIntervalMenu->AddItem(
		new BMenuItem(B_TRANSLATE("Monthly"), new BMessage(M_SCHEDULED_MONTHLY)));
	fIntervalMenu->AddItem(
		new BMenuItem(B_TRANSLATE("Quarterly"), new BMessage(M_SCHEDULED_QUARTERLY)));
	fIntervalMenu->AddItem(
		new BMenuItem(B_TRANSLATE("Annually"), new BMessage(M_SCHEDULED_ANNUALLY)));
	fIntervalMenu->ItemAt(0)->SetMarked(true);
	fIntervalMenu->SetLabelFromMarked(true);

	BMenuField* intervalfield
		= new BMenuField("intervalfield", B_TRANSLATE("Frequency:"), fIntervalMenu);

	BStringView* startLabel = new BStringView("startlabel", B_TRANSLATE("Starting date:"));
	startLabel->SetExplicitMinSize(BSize(be_plain_font->StringWidth(B_TRANSLATE("Starting date:"))
		+ 10, B_SIZE_UNSET));
	fStartDate
		= new DateBox("startdate", NULL, "", new BMessage(M_DATE_CHANGED));
	fStartDate->UseTabFiltering(false);
	gDefaultLocale.DateToString(data.Date(), temp);
	fStartDate->SetText(temp.String());

	CalendarButton* calendarButton = new CalendarButton(fStartDate);

	fRepeatAlways
		= new BRadioButton("inftimes", B_TRANSLATE("Indefinitely"), new BMessage(M_REPEAT_ALWAYS));
	fRepeatAlways->SetValue(B_CONTROL_ON);

	fRepeatLimited = new BRadioButton("limitedtimes", " ", new BMessage(M_REPEAT_LIMITED));

	fRepeatCount = new NumBox("repeatcount", NULL, "999", new BMessage(M_COUNT_CHANGED));
	fRepeatCount->UseTabFiltering(false);
	fRepeatCount->SetEnabled(false);
	temp = " ";
	temp << B_TRANSLATE("times");
	BStringView* timesLabel = new BStringView("timeslabel", temp);
	timesLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	BStringView* repeatLabel = new BStringView("repeatlabel", B_TRANSLATE("Repeat:"));
	repeatLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	BStringView* dummy = new BStringView("dummy", "");	// used to align "Repeat" label
	dummy->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BButton* okButton
		= new BButton("okbutton", B_TRANSLATE("OK"), new BMessage(M_SCHEDULE_TRANSACTION));
	okButton->MakeDefault(true);

	BButton* cancelButton
		= new BButton("cancelbutton", B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));

	// clang-format off
	BView* calendarStartWidget = new BView("calendarstartwidget", B_WILL_DRAW);
	BLayoutBuilder::Group<>(calendarStartWidget, B_HORIZONTAL, -2)
		.Add(fStartDate->CreateTextViewLayoutItem())
		.Add(calendarButton)
		.End();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_WINDOW_SPACING)
		.AddGrid(1.0f, 0.0f)
			.Add(typeLabel, 0, 0)
			.Add(type->CreateTextViewLayoutItem(), 0, 1)
			.Add(payeeLabel, 1, 0)
			.Add(payee->CreateTextViewLayoutItem(), 1, 1)
			.Add(amountLabel, 2, 0)
			.Add(amount->CreateTextViewLayoutItem(), 2, 1)
			.End()
		.AddGrid(1.0f, 0.0f)
			.Add(categoryLabel, 0, 0)
			.Add(category->CreateTextViewLayoutItem(), 0, 1)
			.Add(memoLabel, 1, 0)
			.Add(memo->CreateTextViewLayoutItem(), 1, 1)
			.End()
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
		.AddStrut(B_USE_DEFAULT_SPACING)
		.AddGrid(1.0f, B_USE_DEFAULT_SPACING)
			.SetColumnWeight(1, 2.0f)
			.Add(intervalfield->CreateLabelLayoutItem(), 0, 0)
			.Add(intervalfield->CreateMenuBarLayoutItem(), 1, 0)
			.Add(startLabel, 0, 1)
			.Add(calendarStartWidget, 1, 1)
			.Add(repeatLabel, 0, 2)
			.Add(dummy, 0, 3)
			.AddGroup(B_VERTICAL, 1.0f, 1, 2, 1, 2)
				.Add(fRepeatAlways)
				.AddGroup(B_HORIZONTAL, 0)
					.Add(fRepeatLimited)
					.Add(fRepeatCount)
					.Add(timesLabel)
					.End()
				.End()
			.AddGlue(4, 0, 4)
			.End()
		.AddStrut(B_USE_BIG_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.AddGlue()
			.Add(cancelButton)
			.Add(okButton)
			.End()
		.End();
	// clang-format on

	CenterIn(Frame());
}

void
ScheduleAddWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_REPEAT_ALWAYS:
		{
			fRepeatCount->SetEnabled(false);
			break;
		}
		case M_REPEAT_LIMITED:
		{
			fRepeatCount->SetEnabled(true);
			break;
		}
		case M_COUNT_CHANGED:
		{
			if (fRepeatLimited->Value() == B_CONTROL_ON) {
				if (strlen(fRepeatCount->Text()) < 1)
					fRepeatCount->SetText("1");

				if (atoi(fRepeatCount->Text()) < 1)
					fRepeatCount->SetText("1");
			}
			break;
		}
		case M_SCHEDULE_TRANSACTION:
		{
			ScheduledTransData stdata(fTransData);

			BMenuItem* intervalitem = fIntervalMenu->FindMarked();
			if (!intervalitem) {
				ShowBug("NULL menu item in ScheduleAddWindow");
				break;
			}

			switch (fIntervalMenu->IndexOf(intervalitem)) {
				case 0:
				{
					stdata.SetInterval(SCHEDULED_MONTHLY);
					break;
				}
				case 1:
				{
					stdata.SetInterval(SCHEDULED_QUARTERLY);
					break;
				}
				case 2:
				{
					stdata.SetInterval(SCHEDULED_ANNUALLY);
					break;
				}
				default:
				{
					ShowBug("Bad Interval index in ScheduleAddWindow");
					break;
				}
			}

			if (fRepeatCount->IsEnabled())
				stdata.SetCount(atoi(fRepeatCount->Text()));

			time_t tempdate;
			BString datestr = fStartDate->Text();
			if (datestr.CountChars() < 3
				|| gDefaultLocale.StringToDate(datestr.String(), tempdate) != B_OK) {
				ShowAlert(B_TRANSLATE_CONTEXT(
							  "CapitalBe didn't understand the date you entered", "TextInput"),
					B_TRANSLATE_CONTEXT(
						"CapitalBe understands lots of different ways of entering dates. "
						"Apparently, this wasn't one of them. You'll need to change how you "
						"entered this date. Sorry.",
						"TextInput"));
				break;
			}

			stdata.SetDate(tempdate);

			gDatabase.AddScheduledTransaction(stdata);

			if (fTransData.Type().TypeCode() == TRANS_XFER) {
				// Get the counterpart and add it to the scheduled list
				gDatabase.GetTransferCounterpart(stdata.GetID(), stdata);
				gDatabase.AddScheduledTransaction(stdata);
			}

			HandleScheduledTransactions();

			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}
