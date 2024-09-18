#include "BudgetWindow.h"

#include <Catalog.h>
#include <ColumnTypes.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <Message.h>
#include <NumberFormat.h>
#include <StringView.h>

#include "Account.h"
#include "Budget.h"
#include "CurrencyBox.h"
#include "Database.h"
#include "Help.h"
#include "IconMenuItem.h"
#include "MsgDefs.h"
#include "TimeSupport.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "BudgetWindow"


enum {
	M_CATEGORIES_CHANGED = 'mccc',

	M_BUDGET_CATEGORIES,
	M_BUDGET_RECALCULATE,
	M_BUDGET_ZERO,

	M_SET_PERIOD_MONTH,
	M_SET_PERIOD_WEEK,
	M_SET_PERIOD_QUARTER,
	M_SET_PERIOD_YEAR,

	M_AMOUNT_CHANGED,
	M_SELECT_CATEGORY
};

extern int compare_stringitem(const void* item1, const void* item2);


BudgetWindow::BudgetWindow(const BRect& frame)
	:
	BWindow(BRect(), B_TRANSLATE("Budget"), B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fIncomeGrid(13, 0),
	fSpendingGrid(13, 0)
{
	BNumberFormat numberFormatter;
	fDecimalSymbol = numberFormatter.GetSeparator(B_DECIMAL_SEPARATOR);
	fBar = new BMenuBar("menubar");
	fBar->AddItem(
		new BMenuItem(B_TRANSLATE("Recalculate all"), new BMessage(M_BUDGET_RECALCULATE)));
	fBar->AddItem(new BMenuItem(B_TRANSLATE("Set all to zero"), new BMessage(M_BUDGET_ZERO)));

	// Help icon menu
	BMenuBar* helpBar = new BMenuBar("helpbar");
	helpBar->AddItem(new IconMenuItem("", new BMessage(M_HELP), getHelpIcon(), B_MINI_ICON));

	BuildBudgetSummary();
	BuildStatsAndEditor();
	BuildCategoryList();

	// clang-format off
	BLayoutBuilder::Group<>(fCatBox, B_VERTICAL, 0.0f)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_BIG_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
		.Add(fAmountLabel)
		.Add(fAmountBox)
		.AddGrid(B_USE_DEFAULT_SPACING, 1.0f)
			.Add(fMonthly, 0, 0)
			.Add(fWeekly, 1, 0)
			.Add(fQuarterly, 0, 1)
			.Add(fAnnually, 1, 1)
			.End()
		.End();
	// clang-format on
	fAmountBox->SetText("");
	fAmountBox->GetFilter()->SetMessenger(new BMessenger(this));

	if (gDatabase.CountBudgetEntries() == 0)
		GenerateBudget(false);

	RefreshBudgetGrid();
	RefreshCategories();
	RefreshBudgetSummary();
	fCategoryList->MakeFocus(true);

	// clang-format off
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.SetInsets(0)
		.AddGroup(B_HORIZONTAL, 0.0f)
			.Add(fBar, 1.0f)
			.Add(helpBar, 0.0f)
		.End()
		.AddGroup(B_VERTICAL)
			.SetInsets(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING)
			.AddGroup(B_HORIZONTAL, 0, 1.0f)
				.Add(fCategoryList)
				.AddGroup(B_VERTICAL, -2)
					.Add(fCatBox)
					.Add(fCatStat)
					.End()
				.End()
			.Add(fBudgetSummary, 0.0f)
			.End()
		.End();
	// clang-format on

	font_height fh;
	be_plain_font->GetHeight(&fh);
	float rowheight = fh.ascent + fh.descent + fh.leading;
	rowheight = MAX(14, rowheight);
	fBudgetSummary->SetExplicitMinSize(BSize(B_SIZE_UNSET, rowheight * 6));

	ResizeToPreferred();
	ResizeBy(0, rowheight * 3);

	CenterIn(frame);
}


BudgetWindow::~BudgetWindow()
{
}


void
BudgetWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_HELP:
		{
			openDocumentation("budget.html", NULL);
			break;
		}
		case M_SELECT_CATEGORY:
		{
			HandleCategorySelection();
			fAmountBox->MakeFocus(true);
			break;
		}
		case M_AMOUNT_CHANGED:
		{
			BString str(fAmountBox->Text());
			if (str.CountChars() < 1)
				str = "0";

			Fixed f;
			if (gDefaultLocale.StringToCurrency(str.String(), f) != B_OK)
				break;
			f.Round();
			gDefaultLocale.CurrencyToString(f, str);
			str.Truncate(str.FindFirst(fDecimalSymbol));
			str.RemoveFirst(gDefaultLocale.CurrencySymbol());

			BRow* row = fCategoryList->CurrentSelection();
			if (!row)
				break;

			row->SetField(new BStringField(str.String()), 1);
			fCategoryList->UpdateRow(row);

			BudgetEntry entry;
			gDatabase.GetBudgetEntry(((BStringField*)row->GetField(0))->String(), entry);
			entry.amount = f;
			if (entry.isexpense)
				entry.amount.Invert();
			gDatabase.AddBudgetEntry(entry);

			RefreshBudgetGrid();
			RefreshBudgetSummary();

			fBudgetSummary->SetFocusRow(entry.isexpense ? 1 : 0);
			fBudgetSummary->SetFocusRow(2);
			break;
		}
		case M_BUDGET_RECALCULATE:
		{
			GenerateBudget(false);
			RefreshBudgetGrid();
			RefreshBudgetSummary();
			RefreshCategories();
			break;
		}
		case M_BUDGET_ZERO:
		{
			GenerateBudget(true);
			RefreshBudgetGrid();
			RefreshBudgetSummary();
			RefreshCategories();
			break;
		}
		case M_SET_PERIOD_MONTH:
		{
			SetPeriod(BUDGET_MONTHLY);
			break;
		}
		case M_SET_PERIOD_WEEK:
		{
			SetPeriod(BUDGET_WEEKLY);
			break;
		}
		case M_SET_PERIOD_QUARTER:
		{
			SetPeriod(BUDGET_QUARTERLY);
			break;
		}
		case M_SET_PERIOD_YEAR:
		{
			SetPeriod(BUDGET_ANNUALLY);
			break;
		}
		case M_NEXT_FIELD:
		{
			if (fAmountBox->ChildAt(0)->IsFocus())
				fMonthly->MakeFocus(true);
			break;
		}
		case M_PREVIOUS_FIELD:
		{
			if (fAmountBox->ChildAt(0)->IsFocus())
				fCategoryList->MakeFocus(true);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}


void
BudgetWindow::HandleCategorySelection()
{
	BRow* row = fCategoryList->CurrentSelection();
	if (!row) {
		fAmountBox->SetText("");
		fMonthly->SetValue(B_CONTROL_ON);
		fStatAverageRow->SetField(new BStringField(""), 1);
		fStatHighestRow->SetField(new BStringField(""), 1);
		fStatLowestRow->SetField(new BStringField(""), 1);
	}

	BudgetEntry entry;
	BStringField* strfield = (BStringField*)row->GetField(0);
	if (!gDatabase.GetBudgetEntry(strfield->String(), entry))
		return;

	BString label(B_TRANSLATE("Budget category: %category%"));
	label.ReplaceFirst("%category%", strfield->String());
	fCatBox->SetLabel(label);

	switch (entry.period) {
		case BUDGET_WEEKLY:
		{
			fWeekly->SetValue(B_CONTROL_ON);
			break;
		}
		case BUDGET_QUARTERLY:
		{
			fQuarterly->SetValue(B_CONTROL_ON);
			break;
		}
		case BUDGET_ANNUALLY:
		{
			fAnnually->SetValue(B_CONTROL_ON);
			break;
		}
		default:
		{
			fMonthly->SetValue(B_CONTROL_ON);
			break;
		}
	}

	BString str;
	gDefaultLocale.CurrencyToString(entry.amount.AbsoluteValue(), str);
	str.Truncate(str.FindFirst(fDecimalSymbol));
	str.RemoveFirst(gDefaultLocale.CurrencySymbol());
	fAmountBox->SetText(str.String());

	Fixed high, low, avg;
	CalcStats(entry.name.String(), high, low, avg);

	gDefaultLocale.CurrencyToString(avg.AbsoluteValue(), str);
	str.RemoveFirst(gDefaultLocale.CurrencySymbol());
	fStatAverageRow->SetField(new BStringField(str.String()), 1);

	gDefaultLocale.CurrencyToString(high.AbsoluteValue(), str);
	str.RemoveFirst(gDefaultLocale.CurrencySymbol());
	fStatHighestRow->SetField(new BStringField(str.String()), 1);

	gDefaultLocale.CurrencyToString(low.AbsoluteValue(), str);
	str.RemoveFirst(gDefaultLocale.CurrencySymbol());
	fStatLowestRow->SetField(new BStringField(str.String()), 1);

	fCatStat->Invalidate();
}


void
BudgetWindow::RefreshCategories()
{
	fCategoryList->Clear();
	fIncomeRow = new BRow();
	fCategoryList->AddRow(fIncomeRow);
	fSpendingRow = new BRow();
	fCategoryList->AddRow(fSpendingRow);
	fIncomeRow->SetField(new BStringField(B_TRANSLATE_CONTEXT("Income", "CommonTerms")), 0);
	fSpendingRow->SetField(new BStringField(B_TRANSLATE_CONTEXT("Spending", "CommonTerms")), 0);

	CppSQLite3Query query = gDatabase.DBQuery(
		"SELECT category,amount,period,isexpense FROM budgetlist ORDER BY category",
		"BudgetWindow::RefreshCategories");
	float maxwidth = fCategoryList->StringWidth("Category");
	while (!query.eof()) {
		BString cat = query.getStringField(0);
		Fixed amount;
		amount.SetPremultiplied(query.getInt64Field(1));
		BudgetPeriod period = (BudgetPeriod)query.getIntField(2);

		BRow* row = new BRow();

		if (query.getIntField(3) == 0)
			fCategoryList->AddRow(row, fIncomeRow);
		else
			fCategoryList->AddRow(row, fSpendingRow);

		row->SetField(new BStringField(cat.String()), 0);

		BString amountstr;
		gDefaultLocale.CurrencyToString(amount.AbsoluteValue(), amountstr);
		amountstr.Truncate(amountstr.FindFirst(fDecimalSymbol));
		amountstr.RemoveFirst(gDefaultLocale.CurrencySymbol());

		row->SetField(new BStringField(amountstr.String()), 1);

		float tempwidth = fCategoryList->StringWidth(cat.String());
		maxwidth = MAX(tempwidth, maxwidth);

		row->SetField(new BStringField(BudgetPeriodToString(period).String()), 2);

		query.nextRow();
	}
	fCategoryList->ColumnAt(0)->SetWidth(maxwidth + 30);
	fCategoryList->ExpandOrCollapse(fIncomeRow, true);
	fCategoryList->ExpandOrCollapse(fSpendingRow, true);
}


void
BudgetWindow::RefreshBudgetSummary()
{
	Fixed itotal, stotal, mtotal, f;
	Fixed irowtotal, srowtotal, ttotal;
	for (int32 i = 0; i < 12; i++) {
		itotal = stotal = mtotal = 0;

		for (int32 j = 0; j < fIncomeGrid.CountItems(); j++) {
			fIncomeGrid.ValueAt(i, j, f);
			itotal += f;
			irowtotal += f;
		}

		for (int32 j = 0; j < fSpendingGrid.CountItems(); j++) {
			fSpendingGrid.ValueAt(i, j, f);
			stotal += f.AbsoluteValue();
			srowtotal += f;
		}

		mtotal = itotal - stotal;
		ttotal += mtotal;

		itotal.Round();
		stotal.Round();
		mtotal.Round();

		BString itemp, stemp, mtemp;
		gDefaultLocale.CurrencyToString(itotal, itemp);
		gDefaultLocale.CurrencyToString(stotal, stemp);
		gDefaultLocale.CurrencyToString(mtotal, mtemp);

		itemp.Truncate(itemp.FindFirst(fDecimalSymbol));
		stemp.Truncate(stemp.FindFirst(fDecimalSymbol));
		mtemp.Truncate(mtemp.FindFirst(fDecimalSymbol));

		itemp.RemoveFirst(gDefaultLocale.CurrencySymbol());
		stemp.RemoveFirst(gDefaultLocale.CurrencySymbol());
		mtemp.RemoveFirst(gDefaultLocale.CurrencySymbol());

		BRow* irow = fBudgetSummary->RowAt(0);
		BRow* srow = fBudgetSummary->RowAt(1);
		BRow* mrow = fBudgetSummary->RowAt(2);

		irow->SetField(new BStringField(itemp.String()), i + 1);
		srow->SetField(new BStringField(stemp.String()), i + 1);
		mrow->SetField(new BStringField(mtemp.String()), i + 1);
	}

	BString ttemp;

	gDefaultLocale.CurrencyToString(irowtotal, ttemp);
	ttemp.Truncate(ttemp.FindFirst(fDecimalSymbol));
	ttemp.RemoveFirst(gDefaultLocale.CurrencySymbol());
	fBudgetSummary->RowAt(0)->SetField(new BStringField(ttemp.String()), 13);

	gDefaultLocale.CurrencyToString(srowtotal, ttemp);
	ttemp.Truncate(ttemp.FindFirst(fDecimalSymbol));
	ttemp.RemoveFirst(gDefaultLocale.CurrencySymbol());
	fBudgetSummary->RowAt(1)->SetField(new BStringField(ttemp.String()), 13);

	gDefaultLocale.CurrencyToString(ttotal, ttemp);
	ttemp.Truncate(ttemp.FindFirst(fDecimalSymbol));
	ttemp.RemoveFirst(gDefaultLocale.CurrencySymbol());
	fBudgetSummary->RowAt(2)->SetField(new BStringField(ttemp.String()), 13);

	fBudgetSummary->ResizeAllColumnsToPreferred();
	fBudgetSummary->Invalidate();
}


void
BudgetWindow::RefreshBudgetGrid()
{
	fIncomeGrid.MakeEmpty();
	fSpendingGrid.MakeEmpty();

	CppSQLite3Query query
		= gDatabase.DBQuery("SELECT category,amount,period FROM budgetlist ORDER BY category",
			"BudgetWindow::RefreshCategories");
	while (!query.eof()) {
		BString cat = query.getStringField(0);
		Fixed amount;
		amount.SetPremultiplied(query.getInt64Field(1));
		BudgetPeriod period = (BudgetPeriod)query.getIntField(2);

		ReportGrid* grid = (amount.IsPositive()) ? &fIncomeGrid : &fSpendingGrid;

		int32 index = grid->CountItems();
		grid->AddItem();
		grid->SetRowTitle(index, cat.String());

		Fixed f(amount.AbsoluteValue());
		switch (period) {
			case BUDGET_QUARTERLY:
			{
				f /= 3;
				Fixed qamt(amount);
				qamt *= 4;
				grid->SetValue(12, index, qamt);
				break;
			}
			case BUDGET_ANNUALLY:
			{
				f /= 12;
				grid->SetValue(12, index, amount);
				break;
			}
			default:
			{
				Fixed mamt(f);
				mamt *= 12;
				grid->SetValue(12, index, mamt);
				break;
			}
		}

		for (int32 i = 0; i < 12; i++)
			grid->SetValue(i, index, f);

		query.nextRow();
	}
}


void
BudgetWindow::GenerateBudget(const bool& zero)
{
	// Generate a budget based on the last year's transactions
	ReportGrid income(1, 0), spending(1, 0);

	gDatabase.DBCommand("DELETE FROM budgetlist", "BudgetWindow::GenerateBudget:empty budget");

	CppSQLite3Query query;
	query = gDatabase.DBQuery("SELECT * FROM categorylist ORDER BY name",
		"BudgetWindow::GenerateBudget:get categories");

	if (query.eof())
		return;

	float maxwidth = fCategoryList->StringWidth(B_TRANSLATE_CONTEXT("Category", "CommonTerms"));
	while (!query.eof()) {
		BString catname = query.getStringField(0);

		if (catname.ICompare(B_TRANSLATE("Transfer")) == 0) {
			query.nextRow();
			continue;
		}

		bool isexpense = !query.getIntField(1);

		if (isexpense) {
			spending.AddItem();
			spending.SetRowTitle(spending.CountItems() - 1, catname.String());
		} else {
			income.AddItem();
			income.SetRowTitle(income.CountItems() - 1, catname.String());
		}
		float tempwidth = fCategoryList->StringWidth(catname.String());
		maxwidth = MAX(maxwidth, tempwidth);
		query.nextRow();
	}

	query.finalize();

	// Now that we have the list of categories, query for transactions for each
	// account from each category

	BString accountName;
	CppSQLite3Buffer bufSQL;
	Fixed cattotal;
	for (int32 i = 0; i < income.CountItems(); i++) {
		cattotal = 0;

		if (!zero) {
			for (int32 j = 0; j < gDatabase.CountAccounts(); j++) {
				Account* acc = gDatabase.AccountAt(j);
				accountName = "account_";
				accountName << acc->GetID();
				bufSQL.format("SELECT SUM(amount) FROM %s WHERE category = %Q AND date > %ld;",
					accountName.String(), income.RowTitle(i),
					DecrementDateByYear(GetCurrentDate()));

				query = gDatabase.DBQuery(bufSQL, "BudgetWindow::GenerateBudget:get category");
				cattotal.AddPremultiplied(query.getInt64Field(0));
				query.finalize();
			}
			cattotal /= 12;
			cattotal.Round();
		}
		income.SetValue(0, i, cattotal);
		gDatabase.AddBudgetEntry(BudgetEntry(income.RowTitle(i), cattotal, BUDGET_MONTHLY, false));
	}

	for (int32 i = 0; i < spending.CountItems(); i++) {
		cattotal = 0;

		if (!zero) {
			for (int32 j = 0; j < gDatabase.CountAccounts(); j++) {
				Account* acc = gDatabase.AccountAt(j);
				accountName = "account_";
				accountName << acc->GetID();
				bufSQL.format("SELECT SUM(amount) FROM %s WHERE category = %Q;",
					accountName.String(), spending.RowTitle(i));

				query = gDatabase.DBQuery(bufSQL, "BudgetWindow::GenerateBudget:get category");
				cattotal.AddPremultiplied(query.getInt64Field(0));
				query.finalize();
			}
			cattotal /= 12;
			cattotal.Round();
		}
		spending.SetValue(0, i, cattotal);
		gDatabase.AddBudgetEntry(BudgetEntry(spending.RowTitle(i), cattotal, BUDGET_MONTHLY, true));
	}
}


void
BudgetWindow::CalcStats(const char* cat, Fixed& high, Fixed& low, Fixed& avg)
{
	if (!cat)
		return;

	BString accountName;
	Fixed cattotal;
	CppSQLite3Buffer bufSQL;
	CppSQLite3Query query;

	// find the average amount
	for (int32 j = 0; j < gDatabase.CountAccounts(); j++) {
		Account* acc = gDatabase.AccountAt(j);
		accountName = "account_";
		accountName << acc->GetID();
		bufSQL.format("SELECT SUM(amount) FROM %s WHERE category = %Q;", accountName.String(), cat);

		query = gDatabase.DBQuery(bufSQL, "BudgetWindow::CalcStats:get average");
		cattotal.AddPremultiplied(query.getInt64Field(0));
		query.finalize();
	}
	avg = cattotal;
	avg /= 12;

	// find the highest amount
	cattotal = 0;
	for (int32 j = 0; j < gDatabase.CountAccounts(); j++) {
		Account* acc = gDatabase.AccountAt(j);
		accountName = "account_";
		accountName << acc->GetID();
		bufSQL.format("SELECT MAX(amount) FROM %s WHERE category = %Q;", accountName.String(), cat);

		query = gDatabase.DBQuery(bufSQL, "BudgetWindow::CalcStats:get highest");
		Fixed value;
		value.SetPremultiplied(query.getInt64Field(0));
		cattotal = MAX(cattotal, value);
		query.finalize();
	}
	high = cattotal;

	// find the lowest amount
	cattotal = 0;
	for (int32 j = 0; j < gDatabase.CountAccounts(); j++) {
		Account* acc = gDatabase.AccountAt(j);
		accountName = "account_";
		accountName << acc->GetID();
		bufSQL.format("SELECT MIN(amount) FROM %s WHERE category = %Q;", accountName.String(), cat);

		query = gDatabase.DBQuery(bufSQL, "BudgetWindow::CalcStats:get highest");
		Fixed value;
		value.SetPremultiplied(query.getInt64Field(0));
		cattotal = MIN(cattotal, value);
		query.finalize();
	}
	low = cattotal;
}


void
BudgetWindow::SetPeriod(const BudgetPeriod& period)
{
	BRow* row = fCategoryList->CurrentSelection();
	if (!row)
		return;

	BudgetEntry entry;
	BStringField* strfield = (BStringField*)row->GetField(0);
	if (!gDatabase.GetBudgetEntry(strfield->String(), entry))
		return;

	// Convert the amount to reflect the change in period
	switch (entry.period) {
		case BUDGET_WEEKLY:
		{
			entry.amount *= 52;
			break;
		}
		case BUDGET_QUARTERLY:
		{
			entry.amount *= 4;
			break;
		}
		case BUDGET_ANNUALLY:
		{
			break;
		}
		default:
		{
			entry.amount *= 12;
			break;
		}
	}
	entry.period = period;

	switch (entry.period) {
		case BUDGET_WEEKLY:
		{
			entry.amount /= 52;
			break;
		}
		case BUDGET_QUARTERLY:
		{
			entry.amount /= 4;
			break;
		}
		case BUDGET_ANNUALLY:
		{
			break;
		}
		default:
		{
			entry.amount /= 12;
			break;
		}
	}
	// yeah, yeah, I know about rounding errors. It's not that big of a deal,
	// so deal with it. *famous last words*
	entry.amount.Round();

	gDatabase.AddBudgetEntry(entry);
	RefreshBudgetGrid();
	RefreshBudgetSummary();

	BString str;
	gDefaultLocale.CurrencyToString(entry.amount, str);
	str.Truncate(str.FindFirst(fDecimalSymbol));
	str.RemoveFirst(gDefaultLocale.CurrencySymbol());

	row->SetField(new BStringField(str.String()), 1);
	fAmountBox->SetText(str.String());

	row->SetField(new BStringField(BudgetPeriodToString(entry.period).String()), 2);
	fCategoryList->UpdateRow(row);
}


void
BudgetWindow::BuildStatsAndEditor()
{
	// Add the category statistics
	BString temp;
	float statwidth = be_plain_font->StringWidth(B_TRANSLATE("12 month statistics")) + 20;
	float amountwidth = be_plain_font->StringWidth("000,000.00") + 20;

	fCatStat
		= new BColumnListView("categorystats", B_WILL_DRAW | B_NAVIGABLE, B_FANCY_BORDER, false);

	font_height fh;
	fCatStat->GetFontHeight(&fh);
	float rowheight = fh.ascent + fh.descent + fh.leading;
	rowheight = MAX(14, rowheight);
	fCatStat->SetExplicitMinSize(BSize(statwidth + amountwidth + 50, rowheight * 5));

	fCatStat->AddColumn(
		new BStringColumn(B_TRANSLATE("12 month statistics"), statwidth, 10, 300, B_TRUNCATE_END),
		0);
	fCatStat->AddColumn(new BStringColumn(B_TRANSLATE_CONTEXT("Amount", "CommonTerms"), amountwidth,
							10, 300, B_TRUNCATE_END),
		1);
	fCatStat->SetSortingEnabled(false);
	fCatStat->SetColumnFlags(B_ALLOW_COLUMN_RESIZE);
	fStatAverageRow = new BRow();
	fStatHighestRow = new BRow();
	fStatLowestRow = new BRow();
	fCatStat->AddRow(fStatAverageRow);
	fCatStat->AddRow(fStatHighestRow);
	fCatStat->AddRow(fStatLowestRow);

	fStatAverageRow->SetField(new BStringField(B_TRANSLATE("Average")), 0);
	fStatHighestRow->SetField(new BStringField(B_TRANSLATE("Highest")), 0);
	fStatLowestRow->SetField(new BStringField(B_TRANSLATE("Lowest")), 0);

	// Add the category editor
	fCatBox = new BBox("catbox");
	fCatBox->SetLabel(B_TRANSLATE("Budget category:"));

	fMonthly
		= new BRadioButton("monthoption", B_TRANSLATE("Monthly"), new BMessage(M_SET_PERIOD_MONTH));
	fWeekly
		= new BRadioButton("weekoption", B_TRANSLATE("Weekly"), new BMessage(M_SET_PERIOD_WEEK));
	fQuarterly = new BRadioButton("quarteroption", B_TRANSLATE("Quarterly"),
		new BMessage(M_SET_PERIOD_QUARTER));
	fAnnually
		= new BRadioButton("yearoption", B_TRANSLATE("Annually"), new BMessage(M_SET_PERIOD_YEAR));

	fMonthly->SetValue(B_CONTROL_ON);

	temp = B_TRANSLATE_CONTEXT("Amount", "CommonTerms");
	temp << ":";
	fAmountLabel = new BStringView("amountlabel", temp.String());

	fAmountBox = new CurrencyBox("amountbox", NULL, "$00,000.00", new BMessage(M_AMOUNT_CHANGED));
}


void
BudgetWindow::BuildBudgetSummary()
{
	fSummaryIncomeRow = new BRow();
	fSummarySpendingRow = new BRow();
	fSummaryTotalRow = new BRow();

	fBudgetSummary
		= new BColumnListView("budgetsummary", B_WILL_DRAW | B_NAVIGABLE, B_FANCY_BORDER);
	fBudgetSummary->SetSortingEnabled(false);
	fBudgetSummary->AddColumn(
		new BStringColumn(B_TRANSLATE("Summary"),
			fBudgetSummary->StringWidth(B_TRANSLATE_CONTEXT("Spending", "CommonTerms")) + 20, 10,
			300, B_TRUNCATE_END),
			0);
	fBudgetSummary->AddRow(fSummaryIncomeRow);
	fBudgetSummary->AddRow(fSummarySpendingRow);
	fBudgetSummary->AddRow(fSummaryTotalRow);
	fBudgetSummary->SetColumnFlags(B_ALLOW_COLUMN_RESIZE);

	fSummaryIncomeRow->SetField(new BStringField(B_TRANSLATE_CONTEXT("Income", "CommonTerms")), 0);
	fSummarySpendingRow->SetField(new BStringField(B_TRANSLATE_CONTEXT("Spending", "CommonTerms")),
		0);
	fSummaryTotalRow->SetField(new BStringField(B_TRANSLATE("Total")), 0);

	// Add all the calendar stuff
	time_t rawtime;
	time(&rawtime);
	struct tm timestruct = *localtime(&rawtime);

	for (int8 i = 0; i < 12; i++) {
		char month[32];
		timestruct.tm_mon = i;
		strftime(month, 32, "%b", &timestruct);
		fBudgetSummary->AddColumn(new BStringColumn(month, fBudgetSummary->StringWidth(month) + 25,
			10, 300, B_TRUNCATE_END, B_ALIGN_RIGHT),
			i + 1);
		fSummaryIncomeRow->SetField(new BStringField(""), i + 1);
		fSummarySpendingRow->SetField(new BStringField(""), i + 1);
		fSummaryTotalRow->SetField(new BStringField(""), i + 1);
	}
	fBudgetSummary->AddColumn(new BStringColumn(B_TRANSLATE("Total"),
		fBudgetSummary->StringWidth(B_TRANSLATE("Total")) + 20, 10, 300, B_TRUNCATE_END,
			B_ALIGN_RIGHT),
			13);
	fSummaryIncomeRow->SetField(new BStringField(""), 13);
	fSummarySpendingRow->SetField(new BStringField(""), 13);
	fSummaryTotalRow->SetField(new BStringField(""), 13);

	fBudgetSummary->UpdateRow(fSummaryIncomeRow);
	fBudgetSummary->UpdateRow(fSummarySpendingRow);
	fBudgetSummary->UpdateRow(fSummaryTotalRow);
}


void
BudgetWindow::BuildCategoryList()
{
	fCategoryList
		= new BColumnListView("categorylist", B_WILL_DRAW | B_NAVIGABLE, B_FANCY_BORDER, true);
	fCategoryList->SetSortingEnabled(false);
	fCategoryList->SetSelectionMessage(new BMessage(M_SELECT_CATEGORY));
	fCategoryList->AddColumn(
		new BStringColumn(B_TRANSLATE_CONTEXT("Category", "CommonTerms"),
			fCategoryList->StringWidth(B_TRANSLATE_CONTEXT("Category", "CommonTerms")) + 20, 40,
			300, B_TRUNCATE_END),
		0);
	fCategoryList->AddColumn(
		new BStringColumn(B_TRANSLATE_CONTEXT("Amount", "CommonTerms"),
			fCategoryList->StringWidth(B_TRANSLATE_CONTEXT("Amount", "CommonTerms")) + 20, 40, 300,
			B_TRUNCATE_END, B_ALIGN_RIGHT),
		1);
	fCategoryList->AddColumn(
		new BStringColumn(B_TRANSLATE("Frequency"),
			fCategoryList->StringWidth(B_TRANSLATE("Frequency")) + 20, 40, 300,
			B_TRUNCATE_END, B_ALIGN_RIGHT),
		2);
	fCategoryList->SetColumnFlags(B_ALLOW_COLUMN_RESIZE);

	fIncomeRow = new BRow();
	fCategoryList->AddRow(fIncomeRow);
	fSpendingRow = new BRow();
	fCategoryList->AddRow(fSpendingRow);
	fIncomeRow->SetField(new BStringField(B_TRANSLATE_CONTEXT("Income", "CommonTerms")), 0);
	fSpendingRow->SetField(new BStringField(B_TRANSLATE("Spending")), 0);
}
