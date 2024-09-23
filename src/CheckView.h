/*
 * Copyright 2009. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 */
#ifndef CHECKVIEW_H
#define CHECKVIEW_H

#include <Button.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>

#include <time.h>

#include "Account.h"
#include "Help.h"
#include "Notifier.h"

// clang-format off
enum {
	M_DATE_CHANGED = 'dtch',
	M_PAYEE_CHANGED = 'pych',
	M_AMOUNT_CHANGED = 'amch',
	M_CATEGORY_CHANGED = 'ctch',
	M_MEMO_CHANGED = 'mmch'
};
// clang-format on

class CheckViewFilter;
class CategoryBox;
class CurrencyBox;
class DateBox;
class NavTextBox;
class PayeeBox;

class CheckView : public BView, public Observer {
public:
	CheckView(const char* name, int32 flags);
	~CheckView();
	void AttachedToWindow();
	void MessageReceived(BMessage* msg);
	void ClearAllFields();
	void SetFields(const char* date, const char* type, const char* payee, const char* amount,
		const char* category, const char* memo);

	void HandleNotify(const uint64& value, const BMessage* msg);
	void MakeEmpty();
	void MakeFocus(bool value = true);
	void FrameResized(float width, float height);
	void SetFieldsEnabled(bool enabled);

private:
	void _DoNextField();

	DateBox* fDate;
	PayeeBox* fPayee;
	CurrencyBox* fAmount;
	CategoryBox* fCategory;
	NavTextBox* fMemo;

	BButton *fEnter, *fTransfer;
};

#endif	// CHECKVIEW_H
