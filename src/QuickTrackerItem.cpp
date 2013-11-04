#include "QuickTrackerItem.h"
#include <String.h>
#include "Fixed.h"
#include "CBLocale.h"
#include "Database.h"
#include "Account.h"
#include "Translate.h"

// Calculates the user's net worth by adding up the balances of all accounts
QTNetWorthItem::QTNetWorthItem(const BRect &rect, const char *name, uint32 resize, uint32 flags)
 :	QuickTrackerItem(rect,name,resize,flags),
 	fIgnore(false)
{
	for(int32 i=0; i<gDatabase.CountAccounts(); i++)
	{
		Account *account = gDatabase.AccountAt(i);
		account->AddObserver(this);
	}
}

QTNetWorthItem::~QTNetWorthItem(void)
{
	for(int32 i=0; i<gDatabase.CountAccounts(); i++)
	{
		Account *account = gDatabase.AccountAt(i);
		account->RemoveObserver(this);
	}
}

void QTNetWorthItem::AttachedToWindow(void)
{
	QuickTrackerItem::AttachedToWindow();
	Calculate();
	ResizeTo(Bounds().Width(),(LineHeight() * CountLines()) + 5);
}

void QTNetWorthItem::SetObserving(const bool &value)
{
	if(IsObserving()!=value)
	{
		Observer::SetObserving(value);
		Calculate();
	}
}

void QTNetWorthItem::HandleNotify(const uint64 &value, const BMessage *msg)
{
/*
	TODO: Make QTNetWorthItem ignore mass edits
	if(value & WATCH_MASS_EDIT)
	{
		// Something which will massively change the database will happen. We will
		// receive two of them, so we will start ignoring when we receive the first
		// one and stop the second time. We do this because each call to Calculate
		// resultes in a hit on the database for each account
		fIgnore = !fIgnore;
	}
	
	if(fIgnore)
		return;
*/	
	if(value & WATCH_ACCOUNT)
	{
		Account *acc = NULL;
		if(msg->FindPointer("item",(void**)&acc)!=B_OK)
			return;
		
		if(value & WATCH_CREATE)
		{
			acc->AddObserver(this);
		}
		else
		if(value & WATCH_DELETE)
		{
			acc->RemoveObserver(this);
			if(gDatabase.CountAccounts()==1)
			{
				if(Window())
					Window()->Lock();
				
				BString label,temp;
				temp << TRANSLATE("Account Total") << ": ";
				if(gCurrentLocale.CurrencyToString(Fixed(),label)==B_OK)
					temp << label;
				SetText(temp.String());
				
				if(Window())
					Window()->Unlock();
				return;
			}
		}
	}
	if(!(value & WATCH_SELECT))
		Calculate();
}

void QTNetWorthItem::Calculate(void)
{
	BString label,temp;
	Fixed balance;
		
	temp << TRANSLATE("Account Total") << ": ";
	
	if(gDatabase.CountAccounts() == 0)
	{
		if(Window())
			Window()->Lock();
		
		if(gCurrentLocale.CurrencyToString(balance,label)==B_OK)
			temp << " \n" << label;
		
		SetText(temp.String());
		
		if(Window())
			Window()->Unlock();
		return;
	}
	
	for(int32 i=0; i<gDatabase.CountAccounts(); i++)
	{
		Account *account = gDatabase.AccountAt(i);
		if(!account->IsClosed())
			balance += account->Balance();
	}
		
	if(gDefaultLocale.CurrencyToString(balance,label)==B_OK)
	{
		temp << " \n" << label;
		
		if(Window())
		{
			Window()->Lock();
			SetText(label.String());
			Invalidate();
			Window()->Unlock();
		}
	}
}

// Calculates the budget variance for one category
QTBudgetCategoryItem::QTBudgetCategoryItem(const char *category, const BRect &rect,
		const char *name, uint32 resize, uint32 flags)
 :	QuickTrackerItem(rect,name,resize,flags),
 	fIgnore(false)
{
	for(int32 i=0; i<gDatabase.CountAccounts(); i++)
	{
		Account *account = gDatabase.AccountAt(i);
		account->AddObserver(this);
	}
	
	gDatabase.GetBudgetEntry(category,fEntry);
}

QTBudgetCategoryItem::~QTBudgetCategoryItem(void)
{
	for(int32 i=0; i<gDatabase.CountAccounts(); i++)
	{
		Account *account = gDatabase.AccountAt(i);
		account->RemoveObserver(this);
	}
}

void QTBudgetCategoryItem::AttachedToWindow(void)
{
	QuickTrackerItem::AttachedToWindow();
	Calculate();
//	ResizeTo(Bounds().Width(),(LineHeight() * CountLines()) + 5);
}

void QTBudgetCategoryItem::SetObserving(const bool &value)
{
	if(IsObserving()!=value)
	{
		Observer::SetObserving(value);
		Calculate();
	}
}

void QTBudgetCategoryItem::HandleNotify(const uint64 &value, const BMessage *msg)
{
/*
	TODO: Make QTBudgetCategoryItem ignore mass edits
	if(value & WATCH_MASS_EDIT)
	{
		// Something which will massively change the database will happen. We will
		// receive two of them, so we will start ignoring when we receive the first
		// one and stop the second time. We do this because each call to Calculate
		// resultes in a hit on the database for each account
		fIgnore = !fIgnore;
	}
	
	if(fIgnore)
		return;
*/	
	if(value & WATCH_ACCOUNT)
	{
		Account *acc = NULL;
		if(msg->FindPointer("item",(void**)&acc)!=B_OK)
			return;
		
		if(value & WATCH_CREATE)
		{
			acc->AddObserver(this);
		}
		else
		if(value & WATCH_DELETE)
		{
			acc->RemoveObserver(this);
			if(gDatabase.CountAccounts()==1)
			{
				if(Window())
					Window()->Lock();
				
				BString label,temp;
				temp << TRANSLATE("Account Total") << ": ";
				if(gCurrentLocale.CurrencyToString(Fixed(),label)==B_OK)
					temp << label;
				SetText(temp.String());
				
				if(Window())
					Window()->Unlock();
				return;
			}
		}
	}
	if(!(value & WATCH_SELECT))
		Calculate();
}

void QTBudgetCategoryItem::Calculate(void)
{
	BString label,temp;
	Fixed variance;
	
	temp << TRANSLATE("Budget") << ": " << fEntry.name;
	
	if(gDefaultLocale.CurrencyToString(variance,label)==B_OK)
	{
		temp << " \n" << label;
		
		if(Window())
		{
			Window()->Lock();
			SetText(label.String());
			Invalidate();
			Window()->Unlock();
		}
	}
}

QuickTrackerItem::QuickTrackerItem(const BRect &rect, const char *name, uint32 resize, uint32 flags)
 : BTextView(rect,name,BRect(rect).OffsetToCopy(0,0).InsetByCopy(2,2),resize,flags)
{
	MakeEditable(false);
	MakeSelectable(false);
	gDatabase.AddObserver(this);
}

QuickTrackerItem::~QuickTrackerItem(void)
{
	gDatabase.RemoveObserver(this);
}

void QuickTrackerItem::AttachedToWindow(void)
{
	SetViewColor(Parent()->ViewColor());
}

void QuickTrackerItem::HandleNotify(const uint64 &value, const BMessage *msg)
{
	// Does nothing by default - hook function for child classes
}

void QuickTrackerItem::Configure(void)
{
	// Does nothing by default - hook function for child classes
}
