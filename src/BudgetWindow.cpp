#include "BudgetWindow.h"
#include <MenuBar.h>
#include <Message.h>
#include <StringView.h>
#include "ColumnTypes.h"

#include "Account.h"
#include "CurrencyBox.h"
#include "DStringList.h"
#include "Budget.h"
#include "Database.h"
#include "HelpButton.h"
#include "MsgDefs.h"
#include "Preferences.h"
#include "TimeSupport.h"
#include "Translate.h"

enum
{
	M_CATEGORIES_CHANGED='mccc',

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

extern int compare_stringitem(const void *item1, const void *item2);

BudgetWindow::BudgetWindow(const BRect &frame)
 :	BWindow(frame,TRANSLATE("Budget"), B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS),
 	fIncomeGrid(13,0),
 	fSpendingGrid(13,0)
{
	fBackView = new BView(Bounds(),"background",B_FOLLOW_ALL,B_WILL_DRAW);
	AddChild(fBackView);
	fBackView->SetViewColor(240,240,240);

	BRect r(Bounds());
	r.bottom = 16;
	fBar = new BMenuBar(Bounds(),"menubar");
	fBackView->AddChild(fBar);

	fBar->AddItem(new BMenuItem(TRANSLATE("Recalculate All"),new BMessage(M_BUDGET_RECALCULATE)));
	fBar->AddItem(new BMenuItem(TRANSLATE("Set All to Zero"),new BMessage(M_BUDGET_ZERO)));

	BuildBudgetSummary();

	if(frame.Height() < ((fBudgetSummary->Frame().Height()+15) * 3)+45)
	{
		ResizeTo(Frame().Width(),((fBudgetSummary->Frame().Height()+15) * 3)+45);
		fBudgetSummary->MoveTo(15,Bounds().bottom - 15 - fBudgetSummary->Frame().Height());
	}

	BuildStatsAndEditor();
	BuildCategoryList();

	// We add the children here in order to make Tab navigation work properly
	fBackView->AddChild(fCategoryList);

	fCatBox->AddChild(fAmountBox);
	fCatBox->AddChild(fMonthly);
	fCatBox->AddChild(fWeekly);
	fCatBox->AddChild(fQuarterly);
	fCatBox->AddChild(fAnnually);
	fBackView->AddChild(fCatBox);

	// Have to do this idiocy because BTextControl::Resize* don't work unless
	// attached to a window. Grr...
	fAmountBox->ResizeToPreferred();
	fAmountBox->ResizeTo( ((fCatBox->Bounds().Width() - 10)/2) - 5,
						fAmountBox->Bounds().Height());
	fAmountBox->SetText("");

	fAmountLabel->MoveTo(5,( (fCatBox->Bounds().Height() - 10) -
								fAmountLabel->Bounds().Height() -
								fAmountBox->Bounds().Height()) / 2 );
	fAmountBox->MoveTo(5,fAmountLabel->Frame().bottom + 1);

	fBackView->AddChild(fCatStat);

	fBackView->AddChild(fBudgetSummary);

	fAmountBox->GetFilter()->SetMessenger(new BMessenger(this));

	// This jazz ensures that if Bounds()/3 isn't adequate for the current
	// font size and locale that it will be at least wide enough to display
	// the column headings
	if (fCategoryList->ScrollBar(B_HORIZONTAL)) {
		float hmin,hmax;
		fCategoryList->ScrollBar(B_HORIZONTAL)->GetRange(&hmin, &hmax);
		if(hmax > 0)
			ResizeBy(hmax + B_V_SCROLL_BAR_WIDTH + 20,0);
	}

	// These are just rough edjumacated guesses which should do for most purposes
	SetSizeLimits( ((fBackView->StringWidth("0000")+20)*10)+30,30000,
					((fBudgetSummary->Frame().Height()+15) * 3)+45,30000);

	if(gDatabase.CountBudgetEntries()==0)
		GenerateBudget(false);

	RefreshBudgetGrid();
	RefreshCategories();
	RefreshBudgetSummary();
	fCategoryList->MakeFocus(true);
}

BudgetWindow::~BudgetWindow(void)
{
}

void BudgetWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_SELECT_CATEGORY:
		{
			HandleCategorySelection();
			fAmountBox->MakeFocus(true);
			break;
		}
		case M_AMOUNT_CHANGED:
		{
			BString str(fAmountBox->Text());
			if(str.CountChars()<1)
				str = "0";

			Fixed f;
			if(gDefaultLocale.StringToCurrency(str.String(),f)!=B_OK)
				break;
			f.Round();
			gDefaultLocale.CurrencyToString(f,str);
			str.Truncate(str.FindFirst(gDefaultLocale.CurrencyDecimal()));
			str.RemoveFirst(gDefaultLocale.CurrencySymbol());

			BRow *row = fCategoryList->CurrentSelection();
			if(!row)
				break;

			row->SetField(new BStringField(str.String()),1);
			fCategoryList->UpdateRow(row);

			BudgetEntry entry;
			gDatabase.GetBudgetEntry( ((BStringField*)row->GetField(0))->String(),entry );
			entry.amount = f;
			if(entry.isexpense)
				entry.amount.Invert();
			gDatabase.AddBudgetEntry(entry);

			RefreshBudgetGrid();
			RefreshBudgetSummary();

			fBudgetSummary->SetFocusRow( entry.isexpense ? 1 : 0);
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
			if(fAmountBox->ChildAt(0)->IsFocus())
				fMonthly->MakeFocus(true);
			break;
		}
		case M_PREVIOUS_FIELD:
		{
			if(fAmountBox->ChildAt(0)->IsFocus())
				fCategoryList->MakeFocus(true);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

void BudgetWindow::HandleCategorySelection(void)
{
	BRow *row = fCategoryList->CurrentSelection();
	if(!row)
	{
		fAmountBox->SetText("");
		fMonthly->SetValue(B_CONTROL_ON);
		fStatAverageRow->SetField(new BStringField(""),1);
		fStatHighestRow->SetField(new BStringField(""),1);
		fStatLowestRow->SetField(new BStringField(""),1);
	}

	BudgetEntry entry;
	BStringField *strfield = (BStringField*)row->GetField(0);
	if(!gDatabase.GetBudgetEntry(strfield->String(),entry))
		return;

	switch(entry.period)
	{
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
	gDefaultLocale.CurrencyToString(entry.amount.AbsoluteValue(),str);
	str.Truncate(str.FindFirst(gDefaultLocale.CurrencyDecimal()));
	str.RemoveFirst(gDefaultLocale.CurrencySymbol());
	fAmountBox->SetText(str.String());

	Fixed high,low,avg;
	CalcStats(entry.name.String(),high,low,avg);

	gDefaultLocale.CurrencyToString(high.AbsoluteValue(),str);
	str.RemoveFirst(gDefaultLocale.CurrencySymbol());
	fStatHighestRow->SetField(new BStringField(str.String()),1);

	gDefaultLocale.CurrencyToString(low.AbsoluteValue(),str);
	str.RemoveFirst(gDefaultLocale.CurrencySymbol());
	fStatLowestRow->SetField(new BStringField(str.String()),1);

	gDefaultLocale.CurrencyToString(avg.AbsoluteValue(),str);
	str.RemoveFirst(gDefaultLocale.CurrencySymbol());
	fStatAverageRow->SetField(new BStringField(str.String()),1);

	fCatStat->Invalidate();
}

void BudgetWindow::RefreshCategories(void)
{
	fCategoryList->Clear();
	fIncomeRow = new BRow();
	fCategoryList->AddRow(fIncomeRow);
	fSpendingRow = new BRow();
	fCategoryList->AddRow(fSpendingRow);
	fIncomeRow->SetField(new BStringField(TRANSLATE("Income")),0);
	fSpendingRow->SetField(new BStringField(TRANSLATE("Spending")),0);

	CppSQLite3Query query = gDatabase.DBQuery("select category,amount,period,isexpense from "
											"budgetlist order by category",
											"BudgetWindow::RefreshCategories");
	float maxwidth=fCategoryList->StringWidth("Category");
	while(!query.eof())
	{
		BString cat = DeescapeIllegalCharacters(query.getStringField(0));
		Fixed amount;
		amount.SetPremultiplied(query.getInt64Field(1));
		BudgetPeriod period = (BudgetPeriod)query.getIntField(2);

		BRow *row = new BRow();

		if(query.getIntField(3)==0)
			fCategoryList->AddRow(row,fIncomeRow);
		else
			fCategoryList->AddRow(row,fSpendingRow);

		row->SetField(new BStringField(cat.String()),0);

		BString amountstr;
		gDefaultLocale.CurrencyToString(amount.AbsoluteValue(),amountstr);
		amountstr.Truncate(amountstr.FindFirst(gDefaultLocale.CurrencyDecimal()));
		amountstr.RemoveFirst(gDefaultLocale.CurrencySymbol());

		row->SetField(new BStringField(amountstr.String()),1);

		float tempwidth = fCategoryList->StringWidth(cat.String());
		maxwidth = MAX(tempwidth,maxwidth);

		row->SetField(new BStringField(BudgetPeriodToString(period).String()),2);

		query.nextRow();
	}
	fCategoryList->ColumnAt(0)->SetWidth(maxwidth+30);
	fCategoryList->ExpandOrCollapse(fIncomeRow,true);
	fCategoryList->ExpandOrCollapse(fSpendingRow,true);
}

void BudgetWindow::RefreshBudgetSummary(void)
{
	Fixed itotal,stotal,mtotal,f;
	Fixed irowtotal,srowtotal, ttotal;
	for(int32 i=0; i<12; i++)
	{
		itotal = stotal = mtotal = 0;

		for(int32 j=0; j<fIncomeGrid.CountItems(); j++)
		{
			fIncomeGrid.ValueAt(i,j,f);
			itotal += f;
			irowtotal += f;
		}

		for(int32 j=0; j<fSpendingGrid.CountItems(); j++)
		{
			fSpendingGrid.ValueAt(i,j,f);
			stotal += f.AbsoluteValue();
			srowtotal += f;
		}

		mtotal = itotal - stotal;
		ttotal += mtotal;

		itotal.Round();
		stotal.Round();
		mtotal.Round();

		BString itemp,stemp,mtemp;
		gDefaultLocale.CurrencyToString(itotal,itemp);
		gDefaultLocale.CurrencyToString(stotal,stemp);
		gDefaultLocale.CurrencyToString(mtotal,mtemp);

		itemp.Truncate(itemp.FindFirst(gDefaultLocale.CurrencyDecimal()));
		stemp.Truncate(stemp.FindFirst(gDefaultLocale.CurrencyDecimal()));
		mtemp.Truncate(mtemp.FindFirst(gDefaultLocale.CurrencyDecimal()));

		itemp.RemoveFirst(gDefaultLocale.CurrencySymbol());
		stemp.RemoveFirst(gDefaultLocale.CurrencySymbol());
		mtemp.RemoveFirst(gDefaultLocale.CurrencySymbol());

		BRow *irow = fBudgetSummary->RowAt(0);
		BRow *srow = fBudgetSummary->RowAt(1);
		BRow *mrow = fBudgetSummary->RowAt(2);

		irow->SetField(new BStringField(itemp.String()),i+1);
		srow->SetField(new BStringField(stemp.String()),i+1);
		mrow->SetField(new BStringField(mtemp.String()),i+1);

		float colwidth = fBudgetSummary->StringWidth(itemp.String()) + 20;
		if(fBudgetSummary->ColumnAt(i+1)->Width() < colwidth)
			fBudgetSummary->ColumnAt(i+1)->SetWidth(colwidth);

		colwidth = fBudgetSummary->StringWidth(stemp.String()) + 20;
		if(fBudgetSummary->ColumnAt(i+1)->Width() < colwidth)
			fBudgetSummary->ColumnAt(i+1)->SetWidth(colwidth);

		colwidth = fBudgetSummary->StringWidth(mtemp.String()) + 20;
		if(fBudgetSummary->ColumnAt(i+1)->Width() < colwidth)
			fBudgetSummary->ColumnAt(i+1)->SetWidth(colwidth);
	}

	BString ttemp;

	gDefaultLocale.CurrencyToString(irowtotal,ttemp);
	ttemp.Truncate(ttemp.FindFirst(gDefaultLocale.CurrencyDecimal()));
	ttemp.RemoveFirst(gDefaultLocale.CurrencySymbol());
	fBudgetSummary->RowAt(0)->SetField(new BStringField(ttemp.String()),13);

	gDefaultLocale.CurrencyToString(srowtotal,ttemp);
	ttemp.Truncate(ttemp.FindFirst(gDefaultLocale.CurrencyDecimal()));
	ttemp.RemoveFirst(gDefaultLocale.CurrencySymbol());
	fBudgetSummary->RowAt(1)->SetField(new BStringField(ttemp.String()),13);

	gDefaultLocale.CurrencyToString(ttotal,ttemp);
	ttemp.Truncate(ttemp.FindFirst(gDefaultLocale.CurrencyDecimal()));
	ttemp.RemoveFirst(gDefaultLocale.CurrencySymbol());
	fBudgetSummary->RowAt(2)->SetField(new BStringField(ttemp.String()),13);

	fBudgetSummary->Invalidate();
}

void BudgetWindow::RefreshBudgetGrid(void)
{
	fIncomeGrid.MakeEmpty();
	fSpendingGrid.MakeEmpty();

	CppSQLite3Query query = gDatabase.DBQuery("select category,amount,period from "
											"budgetlist order by category",
											"BudgetWindow::RefreshCategories");
	while(!query.eof())
	{
		BString cat = DeescapeIllegalCharacters(query.getStringField(0));
		Fixed amount;
		amount.SetPremultiplied(query.getInt64Field(1));
		BudgetPeriod period = (BudgetPeriod)query.getIntField(2);

		ReportGrid *grid = (amount.IsPositive()) ? &fIncomeGrid : &fSpendingGrid;

		int32 index = grid->CountItems();
		grid->AddItem();
		grid->SetRowTitle(index, cat.String());

		Fixed f(amount.AbsoluteValue());
		switch(period)
		{
			case BUDGET_QUARTERLY:
			{
				f /= 3;
				Fixed qamt(amount);
				qamt *= 4;
				grid->SetValue(12,index,qamt);
				break;
			}
			case BUDGET_ANNUALLY:
			{
				f /=12;
				grid->SetValue(12,index,amount);
				break;
			}
			default:
			{
				Fixed mamt(f);
				mamt *= 12;
				grid->SetValue(12,index,mamt);
				break;
			}
		}

		for(int32 i=0; i<12; i++)
			grid->SetValue(i,index,f);

		query.nextRow();
	}
}

void BudgetWindow::GenerateBudget(const bool &zero)
{
	// Generate a budget based on the last year's transactions
	ReportGrid income(1,0), spending(1,0);

	gDatabase.DBCommand("delete from budgetlist",
						"BudgetWindow::GenerateBudget:empty budget");

	CppSQLite3Query query;
	query = gDatabase.DBQuery("select * from categorylist order by name",
							"BudgetWindow::GenerateBudget:get categories");

	if(query.eof())
		return;

	float maxwidth=fCategoryList->StringWidth(TRANSLATE("Category"));
	while(!query.eof())
	{
		BString catname = DeescapeIllegalCharacters(query.getStringField(0));

		if(catname.ICompare(TRANSLATE("Transfer"))==0)
		{
			query.nextRow();
			continue;
		}

		bool isexpense = !query.getIntField(1);

		if(isexpense)
		{
			spending.AddItem();
			spending.SetRowTitle(spending.CountItems()-1,catname.String());
		}
		else
		{
			income.AddItem();
			income.SetRowTitle(income.CountItems()-1,catname.String());
		}
		float tempwidth = fCategoryList->StringWidth(catname.String());
		maxwidth = MAX(maxwidth,tempwidth);
		query.nextRow();
	}

	query.finalize();

	// Now that we have the list of categories, query for transactions for each
	// account from each category

	BString querystring;
	Fixed cattotal;
	for(int32 i=0; i<income.CountItems(); i++)
	{
		querystring = "";
		cattotal = 0;

		if(!zero)
		{
			for(int32 j=0; j<gDatabase.CountAccounts(); j++)
			{
				Account *acc = gDatabase.AccountAt(j);
				querystring = "select sum(amount) from account_" ;
				querystring << acc->GetID()	<< " where category = '"
							<< EscapeIllegalCharacters(income.RowTitle(i))
							<< "' and date > " << DecrementDateByYear(GetCurrentDate()) << ";";
				query = gDatabase.DBQuery(querystring.String(),
										"BudgetWindow::GenerateBudget:get category");
				cattotal.AddPremultiplied(query.getInt64Field(0));
				query.finalize();
			}
			cattotal /= 12;
			cattotal.Round();
		}
		income.SetValue(0,i,cattotal);
		gDatabase.AddBudgetEntry(BudgetEntry(income.RowTitle(i),cattotal,BUDGET_MONTHLY,false));
	}

	for(int32 i=0; i<spending.CountItems(); i++)
	{
		querystring = "";
		cattotal = 0;

		if(!zero)
		{
			for(int32 j=0; j<gDatabase.CountAccounts(); j++)
			{
				Account *acc = gDatabase.AccountAt(j);
				querystring = "select sum(amount) from account_" ;
				querystring << acc->GetID()	<< " where category = '"
							<< EscapeIllegalCharacters(spending.RowTitle(i))
							<< "';";
				query = gDatabase.DBQuery(querystring.String(),
										"BudgetWindow::GenerateBudget:get category");
				cattotal.AddPremultiplied(query.getInt64Field(0));
				query.finalize();
			}
			cattotal /= 12;
			cattotal.Round();
		}
		spending.SetValue(0,i,cattotal);
		gDatabase.AddBudgetEntry(BudgetEntry(spending.RowTitle(i),cattotal,BUDGET_MONTHLY,true));
	}
}

void BudgetWindow::CalcStats(const char *cat, Fixed &high, Fixed &low, Fixed &avg)
{
	if(!cat)
		return;

	BString querystring;
	Fixed cattotal;
	CppSQLite3Query query;

	// find the average amount
	for(int32 j=0; j<gDatabase.CountAccounts(); j++)
	{
		Account *acc = gDatabase.AccountAt(j);
		querystring = "select sum(amount) from account_" ;
		querystring << acc->GetID()	<< " where category = '"
					<< EscapeIllegalCharacters(cat)
					<< "';";
		query = gDatabase.DBQuery(querystring.String(),
								"BudgetWindow::CalcStats:get average");
		cattotal.AddPremultiplied(query.getInt64Field(0));
		query.finalize();
	}
	avg = cattotal;
	avg /= 12;

	// find the highest amount
	cattotal=0;
	for(int32 j=0; j<gDatabase.CountAccounts(); j++)
	{
		Account *acc = gDatabase.AccountAt(j);
		querystring = "select max(amount) from account_" ;
		querystring << acc->GetID()	<< " where category = '"
					<< EscapeIllegalCharacters(cat)
					<< "';";
		query = gDatabase.DBQuery(querystring.String(),
								"BudgetWindow::CalcStats:get highest");
		Fixed value;
		value.SetPremultiplied(query.getInt64Field(0));
		cattotal = MAX(cattotal,value);
		query.finalize();
	}
	high = cattotal;

	// find the lowest amount
	cattotal=0;
	for(int32 j=0; j<gDatabase.CountAccounts(); j++)
	{
		Account *acc = gDatabase.AccountAt(j);
		querystring = "select min(amount) from account_" ;
		querystring << acc->GetID()	<< " where category = '"
					<< EscapeIllegalCharacters(cat)
					<< "';";
		query = gDatabase.DBQuery(querystring.String(),
								"BudgetWindow::CalcStats:get highest");
		Fixed value;
		value.SetPremultiplied(query.getInt64Field(0));
		cattotal = MIN(cattotal,value);
		query.finalize();
	}
	low = cattotal;
}

void BudgetWindow::SetPeriod(const BudgetPeriod &period)
{
	BRow *row = fCategoryList->CurrentSelection();
	if(!row)
		return;

	BudgetEntry entry;
	BStringField *strfield = (BStringField*)row->GetField(0);
	if(!gDatabase.GetBudgetEntry(strfield->String(),entry))
		return;

	// Convert the amount to reflect the change in period
	switch(entry.period)
	{
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

	switch(entry.period)
	{
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
	gDefaultLocale.CurrencyToString(entry.amount,str);
	str.Truncate(str.FindFirst(gDefaultLocale.CurrencyDecimal()));
	str.RemoveFirst(gDefaultLocale.CurrencySymbol());

	row->SetField(new BStringField(str.String()),1);
	fAmountBox->SetText(str.String());

	row->SetField(new BStringField(BudgetPeriodToString(entry.period).String()),2);
	fCategoryList->UpdateRow(row);
}

void BudgetWindow::BuildStatsAndEditor(void)
{
	// Add the category statistics
	BRect temprect;
	BString temp;
	rgb_color white = {255,255,255,255};
	float statwidth = fBackView->StringWidth(TRANSLATE("12 Month Statistics")) + 20;
	float amountwidth = fBackView->StringWidth("000,000.00") + 20;

	temprect.right = Bounds().right - 15;
	temprect.left = temprect.right - statwidth - amountwidth -
					B_V_SCROLL_BAR_WIDTH - 20;
	temprect.bottom = fBudgetSummary->Frame().top - 10;
	temprect.top = fBar->Frame().bottom + 15;

	BRect r = temprect;
	r.top = r.bottom - (r.Height() / 2) + 5;

	fCatStat = new BColumnListView(r,"categorystats",B_FOLLOW_BOTTOM |
														B_FOLLOW_RIGHT,
										B_WILL_DRAW | B_NAVIGABLE,B_FANCY_BORDER);
	fCatStat->AddColumn(new BStringColumn(TRANSLATE("12 Month Statistics"),statwidth,
											10,300,B_TRUNCATE_END),0);
	fCatStat->AddColumn(new BStringColumn(TRANSLATE("Amount"),amountwidth,
											10,300,B_TRUNCATE_END),1);
	fCatStat->SetSortingEnabled(false);
	fCatStat->SetColumnFlags(B_ALLOW_COLUMN_RESIZE);
	fStatAverageRow = new BRow();
	fStatHighestRow = new BRow();
	fStatLowestRow = new BRow();
	fCatStat->AddRow(fStatAverageRow);
	fCatStat->AddRow(fStatHighestRow);
	fCatStat->AddRow(fStatLowestRow);

	fStatAverageRow->SetField(new BStringField(TRANSLATE("Average")),0);
	fStatHighestRow->SetField(new BStringField(TRANSLATE("Highest")),0);
	fStatLowestRow->SetField(new BStringField(TRANSLATE("Lowest")),0);

	fCatStat->SetColor(B_COLOR_BACKGROUND,white);
	fCatStat->SetColor(B_COLOR_SELECTION,white);
	fCatStat->SetColor(B_COLOR_NON_FOCUS_SELECTION,white);
	fCatStat->SetColor(B_COLOR_SEPARATOR_BORDER,white);
	fCatStat->SetColor(B_COLOR_SEPARATOR_LINE,white);
	fCatStat->SetColor(B_COLOR_HEADER_BACKGROUND,GetColor(BC_GRID_HEADER));

	// Add the category editor
	r = temprect;
	r.bottom = r.top + (r.Height() / 2) - 5;
	fCatBox = new BBox(r,"catbox",B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM);
	fCatBox->SetLabel(TRANSLATE("Edit Category"));

	r = fCatBox->Bounds().InsetByCopy(10,10);
	r.left += (r.Width()/2) + 5;
	fMonthly = new BRadioButton(r,"monthoption",TRANSLATE("Monthly"),new BMessage(M_SET_PERIOD_MONTH),
								B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	fMonthly->ResizeToPreferred();
	r = fMonthly->Frame();
	r.right = fCatBox->Bounds().right - 10;

	fWeekly = new BRadioButton(r,"weekoption",TRANSLATE("Weekly"),new BMessage(M_SET_PERIOD_WEEK),
								B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	fWeekly->ResizeToPreferred();
	r = fWeekly->Frame();
	r.right = fCatBox->Bounds().right - 10;

	fQuarterly = new BRadioButton(r,"quarteroption",TRANSLATE("Quarterly"),new BMessage(M_SET_PERIOD_QUARTER),
								B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	fQuarterly->ResizeToPreferred();
	r = fQuarterly->Frame();
	r.right = fCatBox->Bounds().right - 10;

	fAnnually = new BRadioButton(r,"yearoption",TRANSLATE("Annually"),new BMessage(M_SET_PERIOD_YEAR),
								B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	fAnnually->ResizeToPreferred();
	r = fAnnually->Frame();
	r.right = fCatBox->Bounds().right - 10;

	fWeekly->MoveBy(0,fQuarterly->Bounds().Height());
	fQuarterly->MoveBy(0,(fQuarterly->Bounds().Height()*2));
	fAnnually->MoveBy(0,(fQuarterly->Bounds().Height()*3));

	fMonthly->SetValue(B_CONTROL_ON);

	temp = TRANSLATE("Amount"); temp += ":";
	fAmountLabel = new BStringView(BRect(5,5,20,25),"amountlabel",temp.String());
	fAmountLabel->ResizeToPreferred();
	fCatBox->AddChild(fAmountLabel);

	fAmountBox = new CurrencyBox(BRect(5,5,20,20),"amountbox",NULL,"$00,000.00",
									new BMessage(M_AMOUNT_CHANGED));

	fAmountBox->SetDivider(0);
}

void BudgetWindow::BuildBudgetSummary(void)
{
	rgb_color white = {255,255,255,255};

	fSummaryIncomeRow = new BRow();
	fSummarySpendingRow = new BRow();
	fSummaryTotalRow = new BRow();

	BRect r;
	r.bottom = Bounds().bottom - 15;
	r.top = r.bottom - (fSummaryIncomeRow->Height() * 4.4) - B_H_SCROLL_BAR_HEIGHT;
	r.left = 15;
	r.right = Bounds().right - 15;
	fBudgetSummary = new BColumnListView(r,"budgetsummary",B_FOLLOW_LEFT_RIGHT |
															B_FOLLOW_BOTTOM,
										B_WILL_DRAW | B_NAVIGABLE,B_FANCY_BORDER);
	fBudgetSummary->SetSortingEnabled(false);
	fBudgetSummary->AddColumn(new BStringColumn(TRANSLATE("Summary"),
											fBudgetSummary->StringWidth(TRANSLATE("Spending"))+20,
											10,300,B_TRUNCATE_END),0);
	fBudgetSummary->AddRow(fSummaryIncomeRow);
	fBudgetSummary->AddRow(fSummarySpendingRow);
	fBudgetSummary->AddRow(fSummaryTotalRow);
	fBudgetSummary->SetColumnFlags(B_ALLOW_COLUMN_RESIZE);

	fSummaryIncomeRow->SetField(new BStringField(TRANSLATE("Income")),0);
	fSummarySpendingRow->SetField(new BStringField(TRANSLATE("Spending")),0);
	fSummaryTotalRow->SetField(new BStringField(TRANSLATE("Total")),0);

	fBudgetSummary->SetColor(B_COLOR_BACKGROUND,white);
	fBudgetSummary->SetColor(B_COLOR_SELECTION,white);
	fBudgetSummary->SetColor(B_COLOR_NON_FOCUS_SELECTION,white);
	fBudgetSummary->SetColor(B_COLOR_SEPARATOR_BORDER,white);
	fBudgetSummary->SetColor(B_COLOR_SEPARATOR_LINE,white);
	fBudgetSummary->SetColor(B_COLOR_HEADER_BACKGROUND,GetColor(BC_GRID_HEADER));

	// Add all the calendar stuff
	time_t rawtime;
	time(&rawtime);
	struct tm timestruct = *localtime(&rawtime);

	for(int8 i=0; i<12; i++)
	{
		char month[32];
		timestruct.tm_mon = i;
		strftime(month,32,"%b",&timestruct);
		fBudgetSummary->AddColumn(new BStringColumn(month,
										fBudgetSummary->StringWidth(month)+25,
										10,300,B_TRUNCATE_END,B_ALIGN_RIGHT),i+1);
		fSummaryIncomeRow->SetField(new BStringField(""),i+1);
		fSummarySpendingRow->SetField(new BStringField(""),i+1);
		fSummaryTotalRow->SetField(new BStringField(""),i+1);
	}
	fBudgetSummary->AddColumn(new BStringColumn(TRANSLATE("Total"),
									fBudgetSummary->StringWidth(TRANSLATE("Total"))+20,
									10,300,B_TRUNCATE_END,B_ALIGN_RIGHT),13);
	fSummaryIncomeRow->SetField(new BStringField(""),13);
	fSummarySpendingRow->SetField(new BStringField(""),13);
	fSummaryTotalRow->SetField(new BStringField(""),13);

	fBudgetSummary->UpdateRow(fSummaryIncomeRow);
	fBudgetSummary->UpdateRow(fSummarySpendingRow);
	fBudgetSummary->UpdateRow(fSummaryTotalRow);
}

void BudgetWindow::BuildCategoryList(void)
{
	rgb_color white = {255,255,255,255};

	// Set up category list
	BRect r;
	r.top = fBar->Frame().bottom + 15;
	r.left = 15;
	r.right = fCatStat->Frame().left - 10;
	r.bottom = fBudgetSummary->Frame().top - 10;

	fCategoryList = new BColumnListView(r,"categorylist",B_FOLLOW_ALL,
										B_WILL_DRAW | B_NAVIGABLE,B_FANCY_BORDER, true);
	fCategoryList->SetSortingEnabled(false);
	fCategoryList->SetSelectionMessage(new BMessage(M_SELECT_CATEGORY));
	fCategoryList->AddColumn(new BStringColumn(TRANSLATE("Category"),
											fCategoryList->StringWidth(TRANSLATE("Category"))+20,
											10,300,B_TRUNCATE_END),0);
	fCategoryList->AddColumn(new BStringColumn(TRANSLATE("Amount"),
											fCategoryList->StringWidth(TRANSLATE("Amount"))+20,
											10,300,B_TRUNCATE_END,B_ALIGN_RIGHT),1);
	fCategoryList->AddColumn(new BStringColumn(TRANSLATE("Frequency"),
											fCategoryList->StringWidth(TRANSLATE("Frequency"))+20,
											10,300,B_TRUNCATE_END,B_ALIGN_RIGHT),2);
	fCategoryList->SetColumnFlags(B_ALLOW_COLUMN_RESIZE);

	fIncomeRow = new BRow();
	fCategoryList->AddRow(fIncomeRow);
	fSpendingRow = new BRow();
	fCategoryList->AddRow(fSpendingRow);
	fIncomeRow->SetField(new BStringField(TRANSLATE("Income")),0);
	fSpendingRow->SetField(new BStringField(TRANSLATE("Spending")),0);

	fCategoryList->SetColor(B_COLOR_BACKGROUND,white);
	fCategoryList->SetColor(B_COLOR_SELECTION,GetColor(BC_SELECTION_FOCUS));
	fCategoryList->SetColor(B_COLOR_NON_FOCUS_SELECTION,GetColor(BC_SELECTION_NOFOCUS));
	fCategoryList->SetColor(B_COLOR_SEPARATOR_BORDER,white);
	fCategoryList->SetColor(B_COLOR_SEPARATOR_LINE,white);
	fCategoryList->SetColor(B_COLOR_HEADER_BACKGROUND,GetColor(BC_GRID_HEADER));
}
