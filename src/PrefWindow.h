#ifndef PREFWIN_H
#define PREFWIN_H

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <ColorControl.h>
#include <MenuField.h>
#include <Message.h>
#include <RadioButton.h>
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


// PrefWindow
enum {
	M_EDIT_OPTIONS = 'edop',
	M_NEG_COLOR_CHANGED
};

// CurrencyPrefView
enum {
	M_NEW_CURRENCY_SYMBOL,
	M_NEW_CURRENCY_SEPARATOR,
	M_NEW_CURRENCY_DECIMAL,
	M_TOGGLE_PREFIX,
	M_CURRENCY_UPADTED
};

// NegativeNumberView
enum {
	M_UPDATE_COLOR
};


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

	void AttachedToWindow(void);
	void MessageReceived(BMessage* msg);

	void GetSettings(Locale& locale);

private:
	void UpdateCurrencyLabel(void);

	BBox* fCurrencyBox;
	AutoTextControl *fCurrencySymbolBox, *fCurrencyDecimalBox, *fCurrencySeparatorBox;
	BCheckBox* fCurrencySymbolPrefix;
	BStringView* fCurrencyPreview;
	Locale fLocale;
	Fixed fSampleAmount;
};


class NegativeNumberView : public BView {
public:
	NegativeNumberView(const char* name, rgb_color negColor);

	void AttachedToWindow(void);
	void MessageReceived(BMessage* msg);

	void GetColor(rgb_color& color);

private:
	void UpdateText(BString text);
	void UpdateColor(rgb_color color);

	BColorControl* fColorPicker;
	BTextView *fSelectedPreview, *fUnselectedPreview, *fClosedPreview;
};

#endif
