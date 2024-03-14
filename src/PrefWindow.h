#ifndef PREFWIN_H
#define PREFWIN_H

#include <Button.h>
#include <CheckBox.h>
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

class PrefWindow : public BWindow {
  public:
	PrefWindow(const BRect &frame);
	void MessageReceived(BMessage *msg);

  private:
	DatePrefView *fDatePrefView;
	CurrencyPrefView *fCurrencyPrefView;
	BButton *fOK, *fCancel;
	BStringView *fLabel;
};

class DatePrefView : public BView {
  public:
	DatePrefView(const char *name, Locale *locale = NULL, const int32 &flags = B_WILL_DRAW);
	void AttachedToWindow(void);
	void MessageReceived(BMessage *msg);

	void GetSettings(Locale &locale);

  private:
	void UpdateDateLabel(void);

	BStringView *fDateLabel;
	BRadioButton *fDateMDY, *fDateDMY;
	AutoTextControl *fDateSeparatorBox;

	Locale fLocale;
};

class CurrencyPrefView : public BView {
  public:
	CurrencyPrefView(const char *name, Locale *locale = NULL, const int32 &flags = B_WILL_DRAW);
	void AttachedToWindow(void);
	void MessageReceived(BMessage *msg);

	void GetSettings(Locale &locale);

  private:
	void UpdateCurrencyLabel(void);

	BStringView *fCurrencyLabel;
	AutoTextControl *fCurrencySymbolBox, *fCurrencyDecimalBox, *fCurrencySeparatorBox;
	BCheckBox *fCurrencySymbolPrefix;

	Locale fLocale;
	Fixed fSampleAmount;
};

#endif
