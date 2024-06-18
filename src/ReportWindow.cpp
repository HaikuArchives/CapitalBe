#include "ReportWindow.h"
#include <Bitmap.h>
#include <Catalog.h>
#include <GridLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <StringView.h>
#include <TranslationUtils.h>
#include <View.h>

#include "CalendarButton.h"
#include "ColumnTypes.h"
#include "Database.h"
#include "DateBox.h"
#include "HelpButton.h"
#include "MsgDefs.h"
#include "ObjectList.h"
#include "Preferences.h"
#include "ReportGrid.h"
#include "StickyDrawButton.h"
#include "TimeSupport.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ReportWindow"


int compare_stringitem(const void* item1, const void* item2);

enum {
	M_REPORT_CASH_FLOW = 'csfl',
	M_REPORT_NET_WORTH,
	M_REPORT_TRANSACTIONS,
	M_REPORT_BUDGET,

	M_START_DATE_CHANGED,
	M_END_DATE_CHANGED,
	M_CATEGORIES_CHANGED,

	M_SUBTOTAL_NONE,
	M_SUBTOTAL_MONTH,
	M_SUBTOTAL_QUARTER,
	M_SUBTOTAL_YEAR,

	M_TOGGLE_ACCOUNT,
	M_TOGGLE_GRAPH
};

ReportWindow::ReportWindow(BRect frame)
	: BWindow(frame, B_TRANSLATE("Reports"), B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	  fSubtotalMode(SUBTOTAL_NONE),
	  fReportMode(REPORT_CASH_FLOW),
	  fStartDate(GetCurrentYear()),
	  fEndDate(GetCurrentDate()),
	  fTitleFont(be_bold_font),
	  fHeaderFont(be_plain_font)
{
	BString temp;

	fHeaderFont.SetFace(B_ITALIC_FACE);

	BGroupLayout* accountsLayout = new BGroupLayout(B_VERTICAL, 0);
	BGroupLayout* subtotalLayout = new BGroupLayout(B_VERTICAL, 0);
	BGroupLayout* categoriesLayout = new BGroupLayout(B_VERTICAL, 0);
	BGroupLayout* reportsLayout = new BGroupLayout(B_VERTICAL, 0);
	BGridLayout* datesLayout = new BGridLayout(B_USE_DEFAULT_SPACING, 1.0f);

	BGroupLayout* listLayout = new BGroupLayout(B_VERTICAL, 1.0f);

	HelpButton* help = new HelpButton(B_TRANSLATE("Help: Report"), "Report.txt");

	// clang-format off
	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
			.Add(accountsLayout, 5)
			.Add(subtotalLayout, 1)
			.Add(categoriesLayout, 5)
			.End()
		.AddGroup(B_VERTICAL)
			.AddGroup(B_HORIZONTAL, B_USE_BIG_SPACING)
				.Add(reportsLayout)
				.Add(new BSeparatorView(B_VERTICAL, B_PLAIN_BORDER))
				.Add(datesLayout)
				.AddGlue()
				.Add(help)
				.End()
			.Add(listLayout)
		.End();
	// clang-format on

	// Reports
	BMenu* reportmenu = new BMenu("Reports");
	reportmenu->SetLabelFromMarked(true);

	// 	TODO: Re-enable the Budget report
	//	reportmenu->AddItem(new BMenuItem(B_TRANSLATE("Budget"), new BMessage(M_REPORT_BUDGET)));
	temp << B_TRANSLATE_CONTEXT("Income", "CommonTerms") << " / "
		 << B_TRANSLATE_CONTEXT("Spending", "CommonTerms");
	reportmenu->AddItem(new BMenuItem(temp.String(), new BMessage(M_REPORT_CASH_FLOW)));
	reportmenu->AddItem(
		new BMenuItem(B_TRANSLATE("Total worth"), new BMessage(M_REPORT_NET_WORTH)));
	reportmenu->AddItem(
		new BMenuItem(B_TRANSLATE("Transactions"), new BMessage(M_REPORT_TRANSACTIONS)));
	reportmenu->SetRadioMode(true);
	reportmenu->ItemAt(0L)->SetMarked(true);

	fReportMode = REPORT_BUDGET;

	reportsLayout->AddView(new BStringView("reportsv", B_TRANSLATE("Reports:")));
	reportsLayout->SetExplicitSize(
		BSize(be_plain_font->StringWidth(temp.String()) + 64, B_SIZE_UNSET));

	fReportField = new BMenuField("reportfield", "", reportmenu);
	reportsLayout->AddView(fReportField);

	// Accounts
	accountsLayout->AddView(new BStringView("accountsv", B_TRANSLATE("Accounts:")));

	fAccountList = new BListView("reportaccountlist", B_MULTIPLE_SELECTION_LIST);
	BScrollView* scrollview = new BScrollView("accountscroller", fAccountList, 0, false, true);
	scrollview->SetExplicitSize(
		BSize(be_plain_font->StringWidth("aQuiteLongishAccountName"), B_SIZE_UNSET));
	accountsLayout->AddView(scrollview);

	//	This is disabled because otherwise the report is rendered once for each
	//	account added when the report window is shown initially
	//	fAccountList->SetSelectionMessage(new BMessage(M_TOGGLE_ACCOUNT));

	// Subtotal
	subtotalLayout->AddView(new BStringView("subtotalsv", B_TRANSLATE("Subtotal:")));

	BMenu* subtotalmenu = new BMenu("Subtotal");
	subtotalmenu->AddItem(new BMenuItem(B_TRANSLATE("None"), new BMessage(M_SUBTOTAL_NONE)));
	subtotalmenu->AddItem(new BMenuItem(B_TRANSLATE("Month"), new BMessage(M_SUBTOTAL_MONTH)));
	subtotalmenu->AddItem(new BMenuItem(B_TRANSLATE("Quarter"), new BMessage(M_SUBTOTAL_QUARTER)));
	subtotalmenu->AddItem(new BMenuItem(B_TRANSLATE("Year"), new BMessage(M_SUBTOTAL_YEAR)));
	subtotalmenu->SetLabelFromMarked(true);
	subtotalmenu->SetRadioMode(true);
	subtotalmenu->ItemAt(0)->SetMarked(true);

	fSubtotalField = new BMenuField("subtotalfield", "", subtotalmenu);
	subtotalLayout->AddView(fSubtotalField);

	// Categories
	categoriesLayout->AddView(new BStringView("catsv", B_TRANSLATE("Categories:")));

	fCategoryList = new BListView("reportcattlist", B_MULTIPLE_SELECTION_LIST,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE);
	fCategoryScroller = new BScrollView("catscroller", fCategoryList, 0, false, true);
	categoriesLayout->AddView(fCategoryScroller);
	fCategoryList->SetSelectionMessage(new BMessage(M_CATEGORIES_CHANGED));
	fCategoryList->SetInvocationMessage(new BMessage(M_CATEGORIES_CHANGED));

	// Dates
	BString datestring;
	gDefaultLocale.DateToString(GetCurrentYear(), datestring);

	temp = B_TRANSLATE("Starting date:");

	fStartDateBox = new DateBox(
		"startdate", temp.String(), datestring.String(), new BMessage(M_START_DATE_CHANGED));
	fStartDateBox->SetDate(GetCurrentYear());
	CalendarButton* calendarStartButton = new CalendarButton(fStartDateBox);

	// clang-format off
	BView* calendarStartWidget = new BView("calendarstartwidget", B_WILL_DRAW);
	BLayoutBuilder::Group<>(calendarStartWidget, B_HORIZONTAL, -2)
		.Add(fStartDateBox->CreateTextViewLayoutItem())
		.Add(calendarStartButton)
		.End();
	// clang-format on

	datesLayout->AddView(new BStringView("startdate", temp.String()), 0, 0);
	datesLayout->AddView(calendarStartWidget, 1, 0);
	fStartDateBox->GetFilter()->SetMessenger(new BMessenger(this));

	gDefaultLocale.DateToString(GetCurrentDate(), datestring);
	temp = B_TRANSLATE("Ending date:");

	fEndDateBox = new DateBox(
		"enddate", temp.String(), datestring.String(), new BMessage(M_END_DATE_CHANGED));
	fEndDateBox->SetDate(GetCurrentDate());
	CalendarButton* calendarEndButton = new CalendarButton(fEndDateBox);

	// clang-format off
	BView* calendarEndWidget = new BView("calendarendwidget", B_WILL_DRAW);
	BLayoutBuilder::Group<>(calendarEndWidget, B_HORIZONTAL, -2)
		.Add(fEndDateBox->CreateTextViewLayoutItem())
		.Add(calendarEndButton)
		.End();
	// clang-format on

	datesLayout->AddView(new BStringView("enddate", temp.String()), 0, 1);
	datesLayout->AddView(calendarEndWidget, 1, 1);
	fEndDateBox->GetFilter()->SetMessenger(new BMessenger(this));

	// TODO: Implement graph support
	// BBitmap *up, *down;
	// BRect brect(0, 0, 16, 16);
	// up = BTranslationUtils::GetBitmap('PNG ', "BarGraphUp.png");
	// if (!up)
	// up = new BBitmap(brect, B_RGB32);
	// down = BTranslationUtils::GetBitmap('PNG ', "BarGraphDown.png");
	// if (!down)
	// down = new BBitmap(brect, B_RGB32);
	//
	// brect.OffsetTo(
	// Bounds().right - 10 - brect.Width(), 10 + ((fEndDateBox->Frame().Height() - 16) / 2));
	// fGraphButton = new StickyDrawButton(brect, "graphbutton", up, down,
	// new BMessage(M_TOGGLE_GRAPH), B_FOLLOW_TOP | B_FOLLOW_RIGHT, B_WILL_DRAW);
	//	view->AddChild(fGraphButton);

	fGridView = new BColumnListView("gridview", B_WILL_DRAW, B_FANCY_BORDER);
	listLayout->AddView(fGridView);

	// Configuring to make it look good and not like a grid
	fGridView->SetColumnFlags(B_ALLOW_COLUMN_RESIZE);
	fGridView->SetSortingEnabled(false);
	fGridView->SetEditMode(false);

	gDatabase.AddObserver(this);
	for (int32 i = 0; i < gDatabase.CountAccounts(); i++) {
		Account* acc = gDatabase.AccountAt(i);
		if (!acc)
			continue;

		AddAccount(acc);
	}

	fAccountList->SortItems(compare_stringitem);
	fCategoryList->SortItems(compare_stringitem);

	for (int32 i = 0; i < fAccountList->CountItems(); i++) {
		BStringItem* item = (BStringItem*)fAccountList->ItemAt(i);
		if (!item)
			continue;

		Account* itemaccount = gDatabase.AccountByName(item->Text());
		if (itemaccount && (!itemaccount->IsClosed()))
			fAccountList->Select(i, true);
	}
	//	fAccountList->Select(0,fAccountList->CountItems()-1,true);
	fCategoryList->Select(0, fCategoryList->CountItems() - 1, true);

	// Set up the scrollbars
	CalcCategoryString();

	fReportField->MakeFocus(true);

	fAccountList->SetSelectionMessage(new BMessage(M_TOGGLE_ACCOUNT));
}

void
ReportWindow::HandleNotify(const uint64& value, const BMessage* msg)
{
	Lock();

	if (value & WATCH_ACCOUNT) {
		Account* acc;
		if (msg->FindPointer("item", (void**)&acc) != B_OK) {
			Unlock();
			return;
		}

		if (value & WATCH_CREATE) {
			AddAccount(acc);
		} else if (value & WATCH_DELETE) {
		} else if (value & WATCH_RENAME) {
		} else if (value & WATCH_CHANGE) {
		}
	} else if (value & WATCH_TRANSACTION) {
	}
	RenderReport();
	Unlock();
}

void
ReportWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_PREVIOUS_FIELD:
		{
			if (fStartDateBox->ChildAt(0)->IsFocus())
				fCategoryList->MakeFocus();
			else if (fEndDateBox->ChildAt(0)->IsFocus())
				fStartDateBox->MakeFocus();
			break;
		}
		case M_NEXT_FIELD:
		{
			if (fStartDateBox->ChildAt(0)->IsFocus())
				fEndDateBox->MakeFocus();
			else if (fEndDateBox->ChildAt(0)->IsFocus())
				fGridView->MakeFocus();
			break;
		}
		case M_REPORT_CASH_FLOW:
		{
			fReportMode = REPORT_CASH_FLOW;
			RenderReport();
			break;
		}
		case M_REPORT_NET_WORTH:
		{
			fReportMode = REPORT_NET_WORTH;
			RenderReport();
			break;
		}
		case M_REPORT_TRANSACTIONS:
		{
			fReportMode = REPORT_TRANSACTIONS;
			RenderReport();
			break;
		}
		case M_REPORT_BUDGET:
		{
			fReportMode = REPORT_BUDGET;
			RenderReport();
			break;
		}


		case M_SUBTOTAL_NONE:
		{
			fSubtotalMode = SUBTOTAL_NONE;
			RenderReport();
			break;
		}
		case M_SUBTOTAL_MONTH:
		{
			fSubtotalMode = SUBTOTAL_MONTH;
			RenderReport();
			break;
		}
		case M_SUBTOTAL_QUARTER:
		{
			fSubtotalMode = SUBTOTAL_QUARTER;
			RenderReport();
			break;
		}
		case M_SUBTOTAL_YEAR:
		{
			fSubtotalMode = SUBTOTAL_YEAR;
			RenderReport();
			break;
		}

		case M_START_DATE_CHANGED:
		{
			time_t temp;
			if (gDefaultLocale.StringToDate(fStartDateBox->Text(), temp) == B_OK) {
				fStartDate = temp;
				if (fStartDate > fEndDate)
					fStartDate = fEndDate;
				RenderReport();
			} else {
				ShowAlert(B_TRANSLATE_CONTEXT(
							  "CapitalBe didn't understand the date you entered", "TextInput"),
					B_TRANSLATE_CONTEXT(
						"CapitalBe understands lots of different ways of entering dates. "
						"Apparently, this wasn't one of them. You'll need to change how you "
						"entered this date. Sorry.",
						"TextInput"));
				fStartDateBox->MakeFocus(true);
				break;
			}

			BString formatted;
			gDefaultLocale.DateToString(fStartDate, formatted);
			fStartDateBox->SetText(formatted.String());

			break;
		}
		case M_END_DATE_CHANGED:
		{
			time_t temp;
			if (gDefaultLocale.StringToDate(fEndDateBox->Text(), temp) == B_OK) {
				fEndDate = temp;
				if (fStartDate > fEndDate)
					fStartDate = fEndDate;
				RenderReport();
			} else {
				ShowAlert(B_TRANSLATE_CONTEXT(
							  "CapitalBe didn't understand the date you entered", "TextInput"),
					B_TRANSLATE_CONTEXT(
						"CapitalBe understands lots of different ways of entering dates. "
						"Apparently, this wasn't one of them. You'll need to change how you "
						"entered this date. Sorry.",
						"TextInput"));
				fEndDateBox->MakeFocus(true);
				break;
			}

			BString formatted;
			gDefaultLocale.DateToString(fEndDate, formatted);
			fEndDateBox->SetText(formatted.String());
			break;
		}
		case M_CATEGORIES_CHANGED:
		{
			CalcCategoryString();
			RenderReport();
			break;
		}
		case M_TOGGLE_ACCOUNT:
		{
			RenderReport();
			break;
		}
		case M_TOGGLE_GRAPH:
		{
			if (fGridView->IsHidden()) {
				fGridView->Show();
				fGraphView->Hide();
			} else {
				fGridView->Hide();
				fGraphView->Show();
			}
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

void
ReportWindow::FrameResized(float w, float h)
{
	BScrollBar* bar = fCategoryScroller->ScrollBar(B_VERTICAL);

	if (fCategoryList->CountItems()) {
		float itemheight = fCategoryList->ItemAt(0)->Height();
		float itemrange = (fCategoryList->CountItems() + 1) * itemheight;
		if (itemrange < fCategoryScroller->Frame().Height())
			bar->SetRange(0, 0);
		else
			bar->SetRange(0, itemrange - fCategoryScroller->Frame().Height());

		float big, small;
		bar->GetSteps(&small, &big);
		big = (int32)(fCategoryScroller->Frame().Height() * .9);
		bar->SetSteps(small, big);
	} else
		bar->SetRange(0, 0);

	FixGridScrollbar();
}

void
ReportWindow::AddAccount(Account* acc)
{
	if (!acc)
		return;

	acc->AddObserver(this);

	AccountItem* accountitem = new AccountItem(acc);
	fAccountList->AddItem(accountitem);

	BString command;
	command << "SELECT category FROM account_" << acc->GetID() << ";";
	CppSQLite3Query query = gDatabase.DBQuery(command.String(), "ReportWindow::AddAccount");

	while (!query.eof()) {
		BString catstr = DeescapeIllegalCharacters(query.getStringField(0));
		if (catstr.CountChars() < 1) {
			query.nextRow();
			continue;
		}

		// Make sure that the category is not already in the list.
		BStringItem* existing = NULL;
		for (int32 k = 0; k < fCategoryList->CountItems(); k++) {
			BStringItem* item = (BStringItem*)fCategoryList->ItemAt(k);
			if (!item)
				continue;

			if (catstr.ICompare(item->Text()) == 0) {
				existing = item;
				break;
			}
		}

		if (existing) {
			query.nextRow();
			continue;
		}

		fCategoryList->AddItem(new BStringItem(catstr.String()));

		query.nextRow();
	}
}

void
ReportWindow::FixGridScrollbar(void)
{
	BScrollBar* bar = fGridView->ScrollBar(B_VERTICAL);
	if (!bar)
		return;
	BRow* row = fGridView->RowAt(0);
	if (!row)
		return;
	float itemrange = (fGridView->CountRows() + 3) * row->Height();
	;
	if (itemrange < fGridView->Frame().Height() - B_H_SCROLL_BAR_HEIGHT)
		bar->SetRange(0, 0);
	else
		bar->SetRange(0, itemrange - fGridView->Frame().Height());

	float big, small;
	bar->GetSteps(&small, &big);
	big = (int32)(fGridView->Frame().Height() * .9);
	bar->SetSteps(small, big);
}

/*
void ReportWindow::CalcAccountString(void)
{
	// Compile list of selected accounts
	fAccountString = "";
	for(int32 i=0; i<gDatabase.CountAccounts(); i++)
	{
		AccountItem *accitem = (AccountItem*)fAccountList->ItemAt(i);
		if(accitem && accitem->IsSelected())
		{
			if(fAccountString.CountChars()>0)
				fAccountString << ",account_" << accitem->account->GetID();
			else
				fAccountString << "account_" << accitem->account->GetID();
		}
	}
}
*/
void
ReportWindow::CalcCategoryString(void)
{
	// Compile list of selected categories
	fCategoryString = "";
	CppSQLite3Buffer bufSQL;
	for (int32 i = 0; i < fCategoryList->CountItems(); i++) {
		BStringItem* catitem = (BStringItem*)fCategoryList->ItemAt(i);
		if (catitem && catitem->IsSelected()) {
			if (fCategoryString.CountChars() > 0)
				fCategoryString << bufSQL.format(",%Q", catitem->Text());
			else
				fCategoryString << bufSQL.format("%Q", catitem->Text());
		}
	}
}

void
ReportWindow::RenderReport(void)
{
	fGridView->Clear();
	BColumn* column = fGridView->ColumnAt(0);
	while (column) {
		fGridView->RemoveColumn(column);
		delete column;
		column = fGridView->ColumnAt(0);
	}

	if (fAccountList->CountItems() < 1 || fAccountList->CurrentSelection() < 0)
		return;

	switch (fReportMode) {
		case REPORT_CASH_FLOW:
		{
			ComputeCashFlow();
			break;
		}
		case REPORT_NET_WORTH:
		{
			ComputeNetWorth();
			break;
		}
		case REPORT_TRANSACTIONS:
		{
			ComputeTransactions();
			break;
		}
		case REPORT_BUDGET:
		{
			ComputeBudget();
			break;
		}
		default:
			break;
	}
	FixGridScrollbar();
}

bool
ReportWindow::QuitRequested(void)
{
	// We need to remove this window's observer object from each account's notifier, or else
	// we will crash the app after closing the window and selecting an account

	for (int32 i = 0; i < gDatabase.CountAccounts(); i++) {
		Account* acc = gDatabase.AccountAt(i);
		if (!acc)
			continue;

		acc->RemoveObserver(this);
	}
	gDatabase.RemoveObserver(this);
	return true;
}


int
compare_stringitem(const void* item1, const void* item2)
{
	BListItem* listitem1 = *((BListItem**)item1);
	BListItem* listitem2 = *((BListItem**)item2);

	BStringItem* stritem1 = (BStringItem*)listitem1;
	BStringItem* stritem2 = (BStringItem*)listitem2;

	int len1 = (stritem1 && stritem1->Text()) ? strlen(stritem1->Text()) : 0;
	int len2 = (stritem2 && stritem2->Text()) ? strlen(stritem2->Text()) : 0;

	if (len1 < 1) {
		return (len2 < 1) ? 0 : 1;
	} else if (len2 < 1) {
		return (len1 < 1) ? 0 : -1;
	}

	return strcmp(stritem1->Text(), stritem2->Text());
}
