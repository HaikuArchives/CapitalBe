#include "SplitItem.h"
#include "Database.h"

SplitItem::SplitItem(void)
 :	BStringItem(""),
 	fName(""),
 	fAmount(0L),
 	fMemo("")
{
}

SplitItem::~SplitItem(void)
{
}

void SplitItem::SetCategory(const char *value)
{
	fName=value;
	UpdateLabel();
}

const char *SplitItem::GetCategory(void) const
{
	return fName.String();
}

void SplitItem::SetAmount(const Fixed &fixed)
{
	fAmount=fixed;
	UpdateLabel();
}

Fixed SplitItem::GetAmount(void) const
{
	return fAmount;
}

void SplitItem::SetMemo(const char *value)
{
	fMemo=value;
	UpdateLabel();
}

const char *SplitItem::GetMemo(void) const
{	
	return fMemo.String();
}

void SplitItem::UpdateLabel(void)
{
	BString text = fName;
	BString amount;
	gCurrentLocale.CurrencyToString(fAmount.AbsoluteValue(),amount);
	text << "    " << amount << "    " << fMemo;
	SetText(text.String());
}
