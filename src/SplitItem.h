/*
 * Copyright 2009. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 */
#ifndef SPLIT_ITEM_H
#define SPLIT_ITEM_H

#include "Fixed.h"

#include <ListItem.h>
#include <String.h>

class SplitItem : public BStringItem {
public:
	SplitItem();
	~SplitItem();
	//	void DrawItem(BView *owner, BRect frame, bool complete = false);

	void SetCategory(const char* value);
	const char* GetCategory() const;

	void SetAmount(const Fixed& fixed);
	Fixed GetAmount() const;

	void SetMemo(const char* value);
	const char* GetMemo() const;

private:
	void _UpdateLabel();
	BString fName;
	Fixed fAmount;
	BString fMemo;
};

#endif	// SPLIT_ITEM_H
