#include "PrefWindow.h"
#include "Preferences.h"

#include <Box.h>
#include <Catalog.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuItem.h>

#include "Database.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PrefWindow"


PrefWindow::PrefWindow(const BRect& frame, BMessenger target)
	: BWindow(frame, B_TRANSLATE("Settings"), B_FLOATING_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		  B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS |
			  B_CLOSE_ON_ESCAPE),
	  fTarget(target)
{
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	fCurrencyPrefView = new CurrencyPrefView("currencyview", &gDefaultLocale);

	fNegNumberView = new NegativeNumberView("negcolor", gNegativeColor);

	BButton* cancel =
		new BButton("cancelbutton", B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));
	BButton* ok = new BButton("okbutton", B_TRANSLATE("OK"), new BMessage(M_EDIT_OPTIONS));

	SetDefaultButton(ok);

	// clang-format off
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fCurrencyPrefView)
		.Add(fNegNumberView)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.AddGlue()
			.Add(cancel)
			.Add(ok)
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
		case M_CURRENCY_UPADTED:
		{
			BMessenger(fNegNumberView).SendMessage(msg);
			break;
		}
		case M_EDIT_OPTIONS:
		{
			Locale temp = gDefaultLocale;
			fCurrencyPrefView->GetSettings(temp);

			if (temp != gDefaultLocale) {
				gDefaultLocale = temp;
				gDatabase.SetDefaultLocale(gDefaultLocale);
			}

			fNegNumberView->GetColor(gNegativeColor);
			prefsLock.Lock();
			gPreferences.RemoveData("negativecolor");
			gPreferences.AddColor("negativecolor", gNegativeColor);
			prefsLock.Unlock();

			fTarget.SendMessage(M_NEG_COLOR_CHANGED);
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
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
	fCurrencyBox->SetLabel(B_TRANSLATE("Default account settings"));

	BStringView* previewLabel = new BStringView("currencylabel", B_TRANSLATE("Currency format:"));
	previewLabel->SetAlignment(B_ALIGN_RIGHT);

	BString curstr;
	fLocale.CurrencyToString(fSampleAmount, curstr);
	fCurrencyPreview = new BStringView("currencypreview", curstr.String());

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
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGlue()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.AddGrid(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
				.Add(previewLabel, 0, 0, 2, 1)
				.Add(fCurrencyPreview, 2, 0, 3, 1)
				.Add(fCurrencySymbolBox->CreateLabelLayoutItem(), 0, 2)
				.Add(fCurrencySymbolBox->CreateTextViewLayoutItem(), 1, 2)
				.Add(fCurrencySymbolPrefix, 2, 2, 3, 1)
				.Add(fCurrencySeparatorBox->CreateLabelLayoutItem(), 0, 3)
				.Add(fCurrencySeparatorBox->CreateTextViewLayoutItem(), 1, 3)
				.Add(fCurrencyDecimalBox->CreateLabelLayoutItem(), 2, 3)
				.Add(fCurrencyDecimalBox->CreateTextViewLayoutItem(), 3, 3)
				.End()
			.AddGlue()
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

	UpdateCurrencyLabel();
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
	fCurrencyPreview->SetText(temp.String());

	// For the neg. number previews we need the fSampleAmount * -1
	fLocale.CurrencyToString(fSampleAmount.InvertAsCopy(), temp);
	BMessage msg(M_CURRENCY_UPADTED);
	msg.AddString("currency", temp);
	Window()->PostMessage(&msg);
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


NegativeNumberView::NegativeNumberView(const char* name, rgb_color negColor)
	: BView(name, B_WILL_DRAW)
{
	BBox* negColorBox = new BBox("negativecolor");
	negColorBox->SetLabel(B_TRANSLATE("Color for negative amounts"));

	// Color picker
	fColorPicker =
		new BColorControl(B_ORIGIN, B_CELLS_32x8, 8.0, "colorpicker", new BMessage(M_UPDATE_COLOR));
	fColorPicker->SetValue(negColor);

	// Preview of the colored text on different background colors
	BStringView* label = new BStringView("label", B_TRANSLATE("Preview:"));
	label->SetFont(be_bold_font);

	rgb_color initColor = negColor;
	fUnselectedPreview = new BTextView("unselected");
	fUnselectedPreview->SetAlignment(B_ALIGN_CENTER);
	fUnselectedPreview->SetViewColor(ui_color(B_LIST_BACKGROUND_COLOR));

	fSelectedPreview = new BTextView("selected");
	fSelectedPreview->SetAlignment(B_ALIGN_CENTER);
	fSelectedPreview->SetViewUIColor(B_LIST_SELECTED_BACKGROUND_COLOR);

	fClosedPreview = new BTextView("unselected-closed");
	fClosedPreview->SetAlignment(B_ALIGN_CENTER);
	fClosedPreview->SetViewColor(ui_color(B_LIST_BACKGROUND_COLOR));

	UpdateColor(negColor);

	// clang-format off
	BLayoutBuilder::Group<>(negColorBox, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(
			B_USE_DEFAULT_SPACING, B_USE_BIG_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fColorPicker)
		.Add(label)
		.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
			.Add(new BStringView("labelunselected", B_TRANSLATE("Unselected:")), 0, 0)
			.Add(fUnselectedPreview, 2, 0)
			.Add(new BStringView("labelselected", B_TRANSLATE("Selected:")), 0, 1)
			.Add(fSelectedPreview, 2, 1)
			.Add(new BStringView("labelclosed", B_TRANSLATE("Closed account:")), 0, 2)
			.Add(fClosedPreview, 2, 2)
			.End()
		.End();

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(negColorBox)
		.End();
	// clang-format on
}

void
NegativeNumberView::AttachedToWindow(void)
{
	fColorPicker->SetTarget(this);

	BView::AttachedToWindow();
}

void
NegativeNumberView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_UPDATE_COLOR:
			UpdateColor(fColorPicker->ValueAsColor());
			break;

		case M_CURRENCY_UPADTED:
		{
			BString text;
			if (msg->FindString("currency", &text) == B_OK)
				UpdateText(text);
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}

void
NegativeNumberView::GetColor(rgb_color& color)
{
	color = fColorPicker->ValueAsColor();
}

void
NegativeNumberView::UpdateText(BString text)
{
	fUnselectedPreview->SetText(text);
	fSelectedPreview->SetText(text);
	fClosedPreview->SetText(text);
}

void
NegativeNumberView::UpdateColor(rgb_color color)
{
	fUnselectedPreview->SetFontAndColor(be_plain_font, B_FONT_ALL, &color);
	fSelectedPreview->SetFontAndColor(be_plain_font, B_FONT_ALL, &color);

	float tint = GetMutedTint(ui_color(B_LIST_BACKGROUND_COLOR), CB_MUTED_TEXT);
	rgb_color closed = tint_color(color, tint);
	fClosedPreview->SetFontAndColor(be_plain_font, B_FONT_ALL, &closed);
}