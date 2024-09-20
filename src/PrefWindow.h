/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	dospuntos (Johan Wagenheim)
 *	humdinger (Joachim Seemer)
 *	raefaldhia (Raefaldhi Amartya Junior)
 *	Thomas Schmidt
 */
#ifndef PREFWIN_H
#define PREFWIN_H

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <ColorControl.h>
#include <MenuField.h>
#include <Message.h>
#include <RadioButton.h>
#include <Spinner.h>
#include <String.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>

#include "AutoTextControl.h"
#include "CBLocale.h"
#include "Fixed.h"

class DatePrefView;
class CurrencyPrefView;
class NegativeNumberView;

// clang-format off
// PrefWindow
enum {
	M_EDIT_OPTIONS = 'edop',
	M_NEG_COLOR_CHANGED
};

// CurrencyPrefView
enum {
	M_NEW_CURRENCY_SYMBOL,
	M_TOGGLE_PREFIX,
	M_CURRENCY_UPDATED,
	M_CURRENCY_DECIMAL_PLACE
};

// NegativeNumberView
enum {
	M_UPDATE_COLOR
};
// clang-format on

class PrefWindow : public BWindow {
public:
	PrefWindow(const BRect& frame, BMessenger target);

	void AllAttached();
	void MessageReceived(BMessage* msg);

private:
	BMessenger fTarget;
	CurrencyPrefView* fCurrencyPrefView;
	NegativeNumberView* fNegNumberView;
};


class CurrencyPrefView : public BView {
public:
	CurrencyPrefView(const char* name, Locale* locale = NULL, const int32& flags = B_WILL_DRAW);

	void AttachedToWindow();
	void MessageReceived(BMessage* msg);

	void GetSettings(Locale& locale);

private:
	void _UpdateCurrencyLabel();

	BBox* fCurrencyBox;
	AutoTextControl* fCurrencySymbolBox;
	BCheckBox* fCurrencySymbolPrefix;
	BSpinner* fDecimalSpinner;
	BStringView* fCurrencyPreview;
	Locale fLocale;
	Fixed fSampleAmount;
};


class NegativeNumberView : public BView {
public:
	NegativeNumberView(const char* name, rgb_color negColor);

	void AttachedToWindow();
	void MessageReceived(BMessage* msg);

	void GetColor(rgb_color& color);

private:
	void _UpdateColor(rgb_color color);

	BColorControl* fColorPicker;
	BTextView *fSelectedPreview, *fUnselectedPreview, *fClosedPreview;
};

#endif // PREFWIN_H
