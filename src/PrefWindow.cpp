#include "PrefWindow.h"

#include <Box.h>
#include <Catalog.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuItem.h>

#include "Database.h"
#include "EscapeCancelFilter.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PrefWindow"


// PrefWindow
#define M_EDIT_OPTIONS 'edop'

// PrefView
enum {
	M_DMY_FORMAT = 'dmyf',
	M_MDY_FORMAT,
	M_NEW_DATE_SEPARATOR,

	M_NEW_CURRENCY_SYMBOL,
	M_NEW_CURRENCY_SEPARATOR,
	M_NEW_CURRENCY_DECIMAL,
	M_TOGGLE_PREFIX
};

PrefWindow::PrefWindow(const BRect& frame)
	: BWindow(frame, B_TRANSLATE("Options"), B_FLOATING_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		  B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	BString temp;
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddCommonFilter(new EscapeCancelFilter());

	BView* back = new BView(NULL, B_WILL_DRAW);
	back->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	fLabel = new BStringView("windowlabel", B_TRANSLATE("Default account settings:"));
	fLabel->SetFont(be_bold_font);

	fDatePrefView = new DatePrefView("dateview", &gDefaultLocale);

	fCurrencyPrefView = new CurrencyPrefView("dateview", &gDefaultLocale);

	fOK = new BButton("okbutton", B_TRANSLATE("Cancel"), new BMessage(M_EDIT_OPTIONS));
	fOK->SetLabel(B_TRANSLATE("OK"));

	fCancel = new BButton("cancelbutton", B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));

	SetDefaultButton(fOK);

	BLayoutBuilder::Group<>(back, B_VERTICAL, 0.0f)
		.SetInsets(10, 10, 10, 10)
		.Add(fLabel)
		.Add(fDatePrefView)
		.Add(fCurrencyPrefView)
		.AddGrid(0.0f, 0.0f)
		.AddGlue(0, 0)
		.Add(fCancel, 1, 0)
		.Add(fOK, 2, 0)
		.End()
		.End();
	BLayoutBuilder::Group<>(this, B_VERTICAL).SetInsets(0).Add(back).End();
}

void
PrefWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_EDIT_OPTIONS:
		{
			Locale temp = gDefaultLocale;
			fDatePrefView->GetSettings(temp);
			fCurrencyPrefView->GetSettings(temp);

			if (temp != gDefaultLocale) {
				gDefaultLocale = temp;
				gDatabase.SetDefaultLocale(gDefaultLocale);
			}
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}

DatePrefView::DatePrefView(const char* name, Locale* locale, const int32& flags)
	: BView(name, flags)
{
	BString temp;
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	if (locale)
		fLocale = *locale;
	else
		fLocale = gDefaultLocale;

	BBox* fDateBox = new BBox("DateBox");
	fDateBox->SetLabel(B_TRANSLATE("Date"));

	BString datestr;
	fLocale.DateToString(0, datestr);
	temp = "";
	temp.SetToFormat(B_TRANSLATE("Date format: %s"), datestr.String());
	fDateLabel = new BStringView("datelabel", temp.String());

	temp = "";
	temp.SetToFormat(B_TRANSLATE("Month, Day, Year"));
	fDateMDY = new BRadioButton("mdybutton", temp.String(), new BMessage(M_MDY_FORMAT));

	temp = "";
	temp.SetToFormat(B_TRANSLATE("Day, Month, Year"));
	fDateDMY = new BRadioButton("dmybutton", temp.String(), new BMessage(M_DMY_FORMAT));
	if (fLocale.DateFormat() == DATE_MDY)
		fDateMDY->SetValue(B_CONTROL_ON);
	else
		fDateDMY->SetValue(B_CONTROL_ON);

	temp = B_TRANSLATE("Separator:");
	fDateSeparatorBox = new AutoTextControl(
		"datesep", temp.String(), fLocale.DateSeparator(), new BMessage(M_NEW_DATE_SEPARATOR));
	fDateSeparatorBox->SetDivider(StringWidth(temp.String()) + 5);
	fDateSeparatorBox->SetCharacterLimit(2);

	BFont font;
	BLayoutBuilder::Group<>(fDateBox, B_VERTICAL, 0.0f)
		.SetInsets(10, font.Size() * 1.3, 10, 10)
		.Add(fDateLabel)
		.Add(fDateMDY)
		.Add(fDateDMY)
		.Add(fDateSeparatorBox)
		.End();
	BLayoutBuilder::Group<>(this, B_VERTICAL).Add(fDateBox).End();
}

void
DatePrefView::AttachedToWindow(void)
{
	fDateMDY->SetTarget(this);
	fDateDMY->SetTarget(this);
	fDateSeparatorBox->SetTarget(this);
}

void
DatePrefView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_DMY_FORMAT:
		{
			fLocale.SetDateFormat(DATE_DMY);
			UpdateDateLabel();
			break;
		}
		case M_MDY_FORMAT:
		{
			fLocale.SetDateFormat(DATE_MDY);
			UpdateDateLabel();
			break;
		}
		case M_NEW_DATE_SEPARATOR:
		{
			if (strlen(fDateSeparatorBox->Text()) < 1)
				break;

			fLocale.SetDateSeparator(fDateSeparatorBox->Text());
			UpdateDateLabel();
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

void
DatePrefView::UpdateDateLabel(void)
{
	BString temp, label;
	fLocale.DateToString(0, temp);
	label.SetToFormat(B_TRANSLATE("Date format: %s"), temp.String());
	fDateLabel->SetText(label.String());
}

void
DatePrefView::GetSettings(Locale& locale)
{
	if (strlen(fDateSeparatorBox->Text()) > 0)
		locale.SetDateSeparator(fDateSeparatorBox->Text());
	locale.SetDateFormat((fDateDMY->Value() == B_CONTROL_ON) ? DATE_DMY : DATE_MDY);
}


CurrencyPrefView::CurrencyPrefView(const char* name, Locale* locale, const int32& flags)
	: BView(name, flags), fSampleAmount((long)12345678, true)
{
	BString temp;
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	if (locale)
		fLocale = *locale;
	else
		fLocale = gDefaultLocale;

	BBox* fCurrencyBox = new BBox("CurrencyBox");
	fCurrencyBox->SetLabel(B_TRANSLATE("Currency"));

	BString curstr;
	fLocale.CurrencyToString(fSampleAmount, curstr);
	temp.SetToFormat(B_TRANSLATE("Currency format: %s"), curstr.String());
	fCurrencyLabel = new BStringView("datelabel", temp.String());

	temp = B_TRANSLATE("Symbol:");
	fCurrencySymbolBox = new AutoTextControl(
		"moneysym", temp.String(), fLocale.CurrencySymbol(), new BMessage(M_NEW_CURRENCY_SYMBOL));
	fCurrencySymbolBox->SetDivider(StringWidth(temp.String()) + 5);
	fCurrencySymbolBox->SetCharacterLimit(2);

	fCurrencySymbolPrefix = new BCheckBox(
		"prefixcheck", B_TRANSLATE("Appears before amount"), new BMessage(M_TOGGLE_PREFIX));
	fCurrencySymbolPrefix->SetValue(
		(fLocale.IsCurrencySymbolPrefix()) ? B_CONTROL_ON : B_CONTROL_OFF);

	temp = B_TRANSLATE("Separator:");
	fCurrencySeparatorBox = new AutoTextControl("moneysep", temp.String(),
		fLocale.CurrencySeparator(), new BMessage(M_NEW_CURRENCY_SEPARATOR));
	fCurrencySeparatorBox->SetDivider(StringWidth(temp.String()) + 5);
	fCurrencySeparatorBox->SetCharacterLimit(2);

	temp = B_TRANSLATE("Decimal:");
	fCurrencyDecimalBox = new AutoTextControl("moneydecimal", temp.String(),
		fLocale.CurrencyDecimal(), new BMessage(M_NEW_CURRENCY_DECIMAL));
	fCurrencyDecimalBox->SetDivider(StringWidth(temp.String()) + 5);
	fCurrencyDecimalBox->SetCharacterLimit(2);

	BFont font;
	BLayoutBuilder::Group<>(fCurrencyBox, B_VERTICAL, 0.0f)
		.SetInsets(10, font.Size() * 1.3, 10, 10)
		.Add(fCurrencyLabel)
		.AddGrid(1.0f, 1.0f)
		.Add(fCurrencySymbolBox, 0, 0)
		.Add(fCurrencySymbolPrefix, 1, 0)
		.Add(fCurrencySeparatorBox, 0, 1)
		.Add(fCurrencyDecimalBox, 1, 1)
		.End()
		.End();
	BLayoutBuilder::Group<>(this, B_VERTICAL).Add(fCurrencyBox).End();
}

void
CurrencyPrefView::AttachedToWindow(void)
{
	fCurrencySymbolBox->SetTarget(this);
	fCurrencyDecimalBox->SetTarget(this);
	fCurrencySeparatorBox->SetTarget(this);
	fCurrencySymbolPrefix->SetTarget(this);
}

void
CurrencyPrefView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_NEW_CURRENCY_SYMBOL:
		{
			if (strlen(fCurrencySymbolBox->Text()) < 1)
				break;

			fLocale.SetCurrencySymbol(fCurrencySymbolBox->Text());
			UpdateCurrencyLabel();
			break;
		}
		case M_NEW_CURRENCY_SEPARATOR:
		{
			if (strlen(fCurrencySeparatorBox->Text()) < 1)
				break;

			fLocale.SetCurrencySeparator(fCurrencySeparatorBox->Text());
			UpdateCurrencyLabel();
			break;
		}
		case M_NEW_CURRENCY_DECIMAL:
		{
			if (strlen(fCurrencyDecimalBox->Text()) < 1)
				break;

			fLocale.SetCurrencyDecimal(fCurrencyDecimalBox->Text());
			UpdateCurrencyLabel();
			break;
		}
		case M_TOGGLE_PREFIX:
		{
			fLocale.SetCurrencySymbolPrefix(fCurrencySymbolPrefix->Value() == B_CONTROL_ON);
			UpdateCurrencyLabel();
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

void
CurrencyPrefView::UpdateCurrencyLabel(void)
{
	BString temp, label;
	fLocale.CurrencyToString(fSampleAmount, temp);
	label.SetToFormat(B_TRANSLATE("Currency format: %s"), temp.String());
	fCurrencyLabel->SetText(label.String());
}

void
CurrencyPrefView::GetSettings(Locale& locale)
{
	if (strlen(fCurrencySeparatorBox->Text()) > 0)
		locale.SetCurrencySeparator(fCurrencySeparatorBox->Text());
	if (strlen(fCurrencyDecimalBox->Text()) > 0)
		locale.SetCurrencyDecimal(fCurrencyDecimalBox->Text());
	if (strlen(fCurrencySymbolBox->Text()) > 0)
		locale.SetCurrencySymbol(fCurrencySymbolBox->Text());

	locale.SetCurrencySymbolPrefix(fCurrencySymbolPrefix->Value() == B_CONTROL_ON);
}
