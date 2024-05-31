#include "PrefWindow.h"

#include <Box.h>
#include <Catalog.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuItem.h>

#include "Database.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PrefWindow"


// PrefWindow
#define M_EDIT_OPTIONS 'edop'

// PrefView
enum {
	M_NEW_CURRENCY_SYMBOL,
	M_NEW_CURRENCY_SEPARATOR,
	M_NEW_CURRENCY_DECIMAL,
	M_TOGGLE_PREFIX
};


PrefWindow::PrefWindow(const BRect& frame)
	: BWindow(frame, B_TRANSLATE("Settings"), B_FLOATING_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		  B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS |
			  B_CLOSE_ON_ESCAPE)
{
	BString temp;
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	fLabel = new BStringView("windowlabel", B_TRANSLATE("Default account settings"));
	BFont font(be_bold_font);
	font.SetSize(font.Size() * 1.2f);
	fLabel->SetFont(&font);

	//fCurrencyPrefView = new CurrencyPrefView("dateview", &gDefaultLocale);
	fLocaleView = new LocaleView("localeview");

	fOK = new BButton("okbutton", B_TRANSLATE("OK"), new BMessage(M_EDIT_OPTIONS));

	BButton* cancel =
		new BButton("cancelbutton", B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));

	SetDefaultButton(fOK);

	// clang-format off
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.AddGlue()
			.Add(fLabel)
			.AddGlue()
			.End()
		.Add(fLocaleView)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.AddGlue()
			.Add(cancel)
			.Add(fOK)
			.End()
		.End();
	// clang-format on
	ResizeToPreferred();
	CenterIn(frame);
}

void
PrefWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_EDIT_OPTIONS:
		{
			Locale temp = gDefaultLocale;
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

CurrencyPrefView::CurrencyPrefView(const char* name, Locale* locale, const int32& flags)
	: BView(name, flags),
	  fSampleAmount((long)12345678, true)
{
	BString temp;

	if (locale)
		fLocale = *locale;
	else
		fLocale = gDefaultLocale;

	fCurrencyBox = new BBox("CurrencyBox");

	BString curstr;
	fLocale.CurrencyToString(fSampleAmount, curstr);
	temp.SetToFormat(B_TRANSLATE("Currency format: %s"), curstr.String());
	fCurrencyBox->SetLabel(temp.String());

	fCurrencySymbolBox = new AutoTextControl("moneysym", B_TRANSLATE("Symbol:"),
		fLocale.CurrencySymbol(), new BMessage(M_NEW_CURRENCY_SYMBOL));
	fCurrencySymbolBox->SetCharacterLimit(2);

	fCurrencySymbolPrefix = new BCheckBox(
		"prefixcheck", B_TRANSLATE("Appears before amount"), new BMessage(M_TOGGLE_PREFIX));
	fCurrencySymbolPrefix->SetValue(
		(fLocale.IsCurrencySymbolPrefix()) ? B_CONTROL_ON : B_CONTROL_OFF);

	fCurrencySeparatorBox = new AutoTextControl("moneysep", B_TRANSLATE("Separator:"),
		fLocale.CurrencySeparator(), new BMessage(M_NEW_CURRENCY_SEPARATOR));
	fCurrencySeparatorBox->SetCharacterLimit(2);

	fCurrencyDecimalBox = new AutoTextControl("moneydecimal", B_TRANSLATE("Decimal:"),
		fLocale.CurrencyDecimal(), new BMessage(M_NEW_CURRENCY_DECIMAL));
	fCurrencyDecimalBox->SetCharacterLimit(2);

	// clang-format off
	BLayoutBuilder::Group<>(fCurrencyBox, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(
			B_USE_DEFAULT_SPACING, B_USE_BIG_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
			.Add(fCurrencySymbolBox->CreateLabelLayoutItem(), 0, 0)
			.Add(fCurrencySymbolBox->CreateTextViewLayoutItem(), 1, 0)
			.Add(fCurrencySymbolPrefix, 2, 0, 3, 1)
			.Add(fCurrencySeparatorBox->CreateLabelLayoutItem(), 0, 1)
			.Add(fCurrencySeparatorBox->CreateTextViewLayoutItem(), 1, 1)
			.Add(fCurrencyDecimalBox->CreateLabelLayoutItem(), 2, 1)
			.Add(fCurrencyDecimalBox->CreateTextViewLayoutItem(), 3, 1)
			.AddGlue(4, 1)
			.End()
		.End();

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fCurrencyBox)
		.End();
	// clang-format on
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
	fCurrencyBox->SetLabel(label.String());
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
