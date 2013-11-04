#include <Box.h>
#include "PrefWindow.h"
#include "EscapeCancelFilter.h"
#include "Database.h"
#include <Menu.h>
#include <MenuItem.h>
#include "Translate.h"

// PrefWindow
#define M_EDIT_OPTIONS 'edop'

// PrefView
enum
{
	M_DMY_FORMAT='dmyf',
	M_MDY_FORMAT,
	M_NEW_DATE_SEPARATOR,
	
	M_NEW_CURRENCY_SYMBOL,
	M_NEW_CURRENCY_SEPARATOR,
	M_NEW_CURRENCY_DECIMAL,
	M_TOGGLE_PREFIX
};

PrefWindow::PrefWindow(const BRect &frame)
 : BWindow(frame,TRANSLATE("Options"),B_FLOATING_WINDOW_LOOK,
 			B_MODAL_APP_WINDOW_FEEL,
 			B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BString temp;
	AddShortcut('W',B_COMMAND_KEY,new BMessage(B_QUIT_REQUESTED));
	AddShortcut('Q',B_COMMAND_KEY,new BMessage(B_QUIT_REQUESTED));
	AddCommonFilter(new EscapeCancelFilter());
	
	BView *back = new BView(Bounds(),"back",B_FOLLOW_ALL,B_WILL_DRAW);
	back->SetViewColor(240,240,240);
	AddChild(back);
	
	BRect r(Bounds());
	r.InsetBy(10,10);
	
	temp = TRANSLATE("Default Account Settings"); temp += ":";
	fLabel = new BStringView(BRect(10,10,11,11),"windowlabel", temp.String());
	fLabel->SetFont(be_bold_font);
	fLabel->ResizeToPreferred();
	back->AddChild(fLabel);
	
	fDatePrefView = new DatePrefView(Bounds(),"dateview",&gDefaultLocale);
	fDatePrefView->MoveTo(0,fLabel->Frame().bottom + 5);

	fCurrencyPrefView = new CurrencyPrefView(Bounds(),"dateview",&gDefaultLocale);
	fCurrencyPrefView->MoveTo(0,fDatePrefView->Frame().bottom + 1);
	
	fOK = new BButton(BRect(0,0,1,1),"okbutton",TRANSLATE("Cancel"),
						new BMessage(M_EDIT_OPTIONS),
						B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
	fOK->ResizeToPreferred();
	fOK->SetLabel(TRANSLATE("OK"));
	fOK->MoveTo(fCurrencyPrefView->Frame().right - fOK->Frame().Width() - 10,
				fCurrencyPrefView->Frame().bottom + 10);
	
	fCancel = new BButton(BRect(0,0,1,1),"cancelbutton",TRANSLATE("Cancel"),
				new BMessage(B_QUIT_REQUESTED),B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
	fCancel->ResizeToPreferred();
	fCancel->MoveTo(fOK->Frame().left - 10 - fCancel->Frame().Width(), fOK->Frame().top);
	
	ResizeTo(fOK->Frame().right + 10,fOK->Frame().bottom + 10);
	
	back->AddChild(fDatePrefView);
	back->AddChild(fCurrencyPrefView);
	back->AddChild(fCancel);
	back->AddChild(fOK);
	
	SetDefaultButton(fOK);
}

void PrefWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_EDIT_OPTIONS:
		{
			Locale temp = gDefaultLocale;
			fDatePrefView->GetSettings(temp);
			fCurrencyPrefView->GetSettings(temp);
			
			if(temp != gDefaultLocale)
			{
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

DatePrefView::DatePrefView(const BRect &frame, const char *name,  Locale *locale,
		const int32 &resize, const int32 &flags)
 :	BView(frame,name,resize,flags)
{
	BString temp;
	SetViewColor(240,240,240);
	
	if(locale)
		fLocale = *locale;
	else
		fLocale = gDefaultLocale;
	
	BRect r(Bounds());
	r.InsetBy(10,10);
	
	BBox *fDateBox = new BBox(r);
	fDateBox->SetLabel(TRANSLATE("Date"));
	AddChild(fDateBox);
	
	r.Set(10,15,11,16);
	BString datestr;
	fLocale.DateToString(0,datestr);
	temp = "";
	temp << TRANSLATE("Date Format") << ":   " << datestr;
	fDateLabel = new BStringView(r,"datelabel",temp.String());
	fDateLabel->ResizeToPreferred();
	fDateBox->AddChild(fDateLabel);
	
	r = fDateLabel->Frame();
	r.OffsetBy(0,r.Height() + 10);
	
	temp = "";
	temp << TRANSLATE("Month") << ", " << TRANSLATE("Day") << ", "
		<< TRANSLATE("Year");
	fDateMDY = new BRadioButton(r,"mdybutton",temp.String(),
								new BMessage(M_MDY_FORMAT));
	fDateMDY->ResizeToPreferred();
	r = fDateMDY->Frame();
	fDateBox->AddChild(fDateMDY);
	
	r.OffsetBy(0,r.Height());
	temp = "";
	temp << TRANSLATE("Day") << ", " << TRANSLATE("Month") << ", "
		<< TRANSLATE("Year");
	fDateDMY = new BRadioButton(r,"dmybutton",temp.String(),
								new BMessage(M_DMY_FORMAT));
	fDateBox->AddChild(fDateDMY);
	if(fLocale.DateFormat()==DATE_MDY)
		fDateMDY->SetValue(B_CONTROL_ON);
	else
		fDateDMY->SetValue(B_CONTROL_ON);
	
	r = fDateMDY->Frame();
	r.OffsetBy(r.Width() + 10,0);
	temp = TRANSLATE("Separator"); temp += ":";
	r.right = r.left + StringWidth(temp.String()) + 35;
	fDateSeparatorBox = new AutoTextControl(r,"datesep",temp.String(),
		fLocale.DateSeparator(),new BMessage(M_NEW_DATE_SEPARATOR));
	fDateSeparatorBox->SetDivider(StringWidth(temp.String()) + 5);
	fDateSeparatorBox->SetCharacterLimit(2);
	fDateBox->AddChild(fDateSeparatorBox);
	
	fDateBox->ResizeTo(fDateSeparatorBox->Frame().right + 10,
						fDateDMY->Frame().bottom + 10);
		
	ResizeTo(fDateBox->Frame().right + 1,fDateBox->Frame().bottom + 1);
}

void DatePrefView::AttachedToWindow(void)
{
	fDateMDY->SetTarget(this);
	fDateDMY->SetTarget(this);
	fDateSeparatorBox->SetTarget(this);
}

void DatePrefView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
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
			if(strlen(fDateSeparatorBox->Text())<1)
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

void DatePrefView::UpdateDateLabel(void)
{
	BString temp,label;
	fLocale.DateToString(0,temp);
	label << TRANSLATE("Date Format") << ":   " << temp;
	fDateLabel->SetText(label.String());
}

void DatePrefView::GetSettings(Locale &locale)
{
	if(strlen(fDateSeparatorBox->Text())>0)
		locale.SetDateSeparator(fDateSeparatorBox->Text());
	locale.SetDateFormat( (fDateDMY->Value()==B_CONTROL_ON) ? DATE_DMY : DATE_MDY );
}


CurrencyPrefView::CurrencyPrefView(const BRect &frame, const char *name,  Locale *locale,
		const int32 &resize, const int32 &flags)
 :	BView(frame,name,resize,flags),
 	fSampleAmount((long)12345678,true)
{
	BString temp;
	SetViewColor(240,240,240);
	
	if(locale)
		fLocale = *locale;
	else
		fLocale = gDefaultLocale;
	
	BRect r(Bounds());
	r.InsetBy(10,10);
	
	BBox *fCurrencyBox = new BBox(Bounds().InsetByCopy(10,10));
	fCurrencyBox->SetLabel(TRANSLATE("Currency"));
	AddChild(fCurrencyBox);
	
	r.Set(10,15,11,16);
	
	BString curstr;
	fLocale.CurrencyToString(fSampleAmount,curstr);
	temp = TRANSLATE("Currency Format");
	temp << ":   " << curstr;
	fCurrencyLabel = new BStringView(r,"datelabel",temp.String());
	fCurrencyLabel->ResizeToPreferred();
	fCurrencyBox->AddChild(fCurrencyLabel);
	
	r = fCurrencyLabel->Frame();
	r.OffsetBy(0,r.Height() + 10);
	temp = TRANSLATE("Symbol"); temp += ":";
	r.right = r.left + StringWidth(temp.String()) + 35;
	fCurrencySymbolBox = new AutoTextControl(r,"moneysym",temp.String(),
		fLocale.CurrencySymbol(),new BMessage(M_NEW_CURRENCY_SYMBOL));
	fCurrencySymbolBox->SetDivider(StringWidth(temp.String()) + 5);
	fCurrencySymbolBox->SetCharacterLimit(2);
	fCurrencyBox->AddChild(fCurrencySymbolBox);
	
	r.OffsetBy(r.Width() + 10,0);
	fCurrencySymbolPrefix = new BCheckBox(r,"prefixcheck",
										TRANSLATE("Appears Before Amount"),
										new BMessage(M_TOGGLE_PREFIX));
	fCurrencyBox->AddChild(fCurrencySymbolPrefix);
	fCurrencySymbolPrefix->ResizeToPreferred();
	fCurrencySymbolPrefix->SetValue( (fLocale.IsCurrencySymbolPrefix()) ?
										B_CONTROL_ON : B_CONTROL_OFF );

	r = fCurrencySymbolBox->Frame();
	r.OffsetBy(0,r.Height() + 10);
	temp = TRANSLATE("Separator"); temp += ":";
	r.right = r.left + StringWidth(temp.String()) + 35;
	fCurrencySeparatorBox = new AutoTextControl(r,"moneysep",temp.String(),
		fLocale.CurrencySeparator(),new BMessage(M_NEW_CURRENCY_SEPARATOR));
	fCurrencySeparatorBox->SetDivider(StringWidth(temp.String()) + 5);
	fCurrencySeparatorBox->SetCharacterLimit(2);
	fCurrencyBox->AddChild(fCurrencySeparatorBox);
	
	r.OffsetBy(r.Width() + 10,0);
	temp = TRANSLATE("Decimal"); temp += ":";
	r.right = r.left + StringWidth(temp.String()) + 35;
	fCurrencyDecimalBox = new AutoTextControl(r,"moneydecimal",temp.String(),
		fLocale.CurrencyDecimal(),new BMessage(M_NEW_CURRENCY_DECIMAL));
	fCurrencyDecimalBox->SetDivider(StringWidth(temp.String()) + 5);
	fCurrencyDecimalBox->SetCharacterLimit(2);
	fCurrencyBox->AddChild(fCurrencyDecimalBox);
	
	fCurrencyBox->ResizeTo(fCurrencySymbolPrefix->Frame().right + 10,
			fCurrencySeparatorBox->Frame().bottom + 10);
	
	ResizeTo(fCurrencyBox->Frame().right + 10,fCurrencyBox->Frame().bottom + 10);
}

void CurrencyPrefView::AttachedToWindow(void)
{
	fCurrencySymbolBox->SetTarget(this);
	fCurrencyDecimalBox->SetTarget(this);
	fCurrencySeparatorBox->SetTarget(this);
	fCurrencySymbolPrefix->SetTarget(this);
}

void CurrencyPrefView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_NEW_CURRENCY_SYMBOL:
		{
			if(strlen(fCurrencySymbolBox->Text())<1)
				break;
			
			fLocale.SetCurrencySymbol(fCurrencySymbolBox->Text());
			UpdateCurrencyLabel();
			break;
		}
		case M_NEW_CURRENCY_SEPARATOR:
		{
			if(strlen(fCurrencySeparatorBox->Text())<1)
				break;
			
			fLocale.SetCurrencySeparator(fCurrencySeparatorBox->Text());
			UpdateCurrencyLabel();
			break;
		}
		case M_NEW_CURRENCY_DECIMAL:
		{
			if(strlen(fCurrencyDecimalBox->Text())<1)
				break;
			
			fLocale.SetCurrencyDecimal(fCurrencyDecimalBox->Text());
			UpdateCurrencyLabel();
			break;
		}
		case M_TOGGLE_PREFIX:
		{
			fLocale.SetCurrencySymbolPrefix(fCurrencySymbolPrefix->Value()==B_CONTROL_ON);
			UpdateCurrencyLabel();
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

void CurrencyPrefView::UpdateCurrencyLabel(void)
{
	BString temp,label;
	fLocale.CurrencyToString(fSampleAmount,temp);
	label << TRANSLATE("Currency Format") << ":   " << temp;
	fCurrencyLabel->SetText(label.String());
}

void CurrencyPrefView::GetSettings(Locale &locale)
{
	if(strlen(fCurrencySeparatorBox->Text())>0)
		locale.SetCurrencySeparator(fCurrencySeparatorBox->Text());
	if(strlen(fCurrencyDecimalBox->Text())>0)
		locale.SetCurrencyDecimal(fCurrencyDecimalBox->Text());
	if(strlen(fCurrencySymbolBox->Text())>0)
		locale.SetCurrencySymbol(fCurrencySymbolBox->Text());
	
	locale.SetCurrencySymbolPrefix(fCurrencySymbolPrefix->Value()==B_CONTROL_ON);
}
