/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	humdinger (Joachim Seemer)
 */
#ifndef TRANSFERWINDOW_H
#define TRANSFERWINDOW_H

#include "Account.h"
#include "AccountListView.h"
#include "Fixed.h"

#include <Button.h>
#include <ListView.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>

#include <time.h>

#define M_CREATE_TRANSFER 'crtn'
#define M_EDIT_TRANSFER 'edtn'

class DateBox;
class CurrencyBox;


class TransferWindow : public BWindow {
public:
	TransferWindow(BHandler* target);
	TransferWindow(BHandler* target, Account* src, Account* dest, const Fixed& amount);
	void MessageReceived(BMessage* msg);

	void SetMessage(BMessage msg);

private:
	friend class TransferFilter;

	void _InitObject(Account* src, Account* dest, const Fixed& amount);
	void _HandleOKButton();

	BTextControl* fMemo;
	DateBox* fDate;
	CurrencyBox* fAmount;

	BButton *fCancel, *fOK;

	BStringView *fFromLabel, *fToLabel;
	BListView *fSourceList, *fDestList;

	BMessenger fMessenger;
	BMessage fMessage;
};

#endif	// TRANSFERWINDOW_H
