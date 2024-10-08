/*
 * Copyright 2009. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 */
#include "SplitItem.h"
#include "Database.h"


SplitItem::SplitItem()
	: BStringItem(""),
	  fName(""),
	  fAmount(0L),
	  fMemo("")
{
}


SplitItem::~SplitItem() {}


void
SplitItem::SetCategory(const char* value)
{
	fName = value;
	_UpdateLabel();
}


const char*
SplitItem::GetCategory() const
{
	return fName.String();
}


void
SplitItem::SetAmount(const Fixed& fixed)
{
	fAmount = fixed;
	_UpdateLabel();
}


Fixed
SplitItem::GetAmount() const
{
	return fAmount;
}


void
SplitItem::SetMemo(const char* value)
{
	fMemo = value;
	_UpdateLabel();
}


const char*
SplitItem::GetMemo() const
{
	return fMemo.String();
}


void
SplitItem::_UpdateLabel()
{
	BString text = fName;
	BString amount;
	gCurrentLocale.CurrencyToString(fAmount.AbsoluteValue(), amount);
	text << "    " << amount << "    " << fMemo;
	SetText(text.String());
}
