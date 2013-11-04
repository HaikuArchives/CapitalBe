#include "ScheduleAddWindow.h"
#include <Box.h>
#include <StringView.h>
#include <String.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Button.h>
#include <stdlib.h>

#include "CBLocale.h"
#include "Layout.h"
#include "NumBox.h"
#include "Database.h"
#include "DateBox.h"
#include "ScheduledTransData.h"
#include "Translate.h"

enum
{
	M_SCHEDULED_MONTHLY='schm',
	M_SCHEDULED_WEEKLY,
	M_SCHEDULED_QUARTERLY,
	M_SCHEDULED_ANNUALLY,
	M_DATE_CHANGED,
	M_COUNT_CHANGED,
	M_REPEAT_ALWAYS,
	M_REPEAT_LIMITED,
	M_SCHEDULE_TRANSACTION
};

ScheduleAddWindow::ScheduleAddWindow(const BRect &frame, const TransactionData &data)
 :	BWindow(frame,TRANSLATE("Schedule Transaction"),B_TITLED_WINDOW_LOOK,
 			B_MODAL_APP_WINDOW_FEEL,B_NOT_ZOOMABLE | B_NOT_RESIZABLE | 
			B_NOT_MINIMIZABLE),
	fTransData(data)
{
	AddShortcut('W',B_COMMAND_KEY,new BMessage(B_QUIT_REQUESTED));
	
	BView *back = new BView(Bounds(),"backview",B_FOLLOW_ALL,B_WILL_DRAW);
	AddChild(back);
	back->SetViewColor(240,240,240);
	
	BString label;
	label << TRANSLATE("Type") << ": " << data.Type().Type();
	BStringView *typelabel = new BStringView(BRect(10,10,11,11),"typelabel",
											label.String());
	back->AddChild(typelabel);
	typelabel->ResizeToPreferred();
	
	BRect r(typelabel->Frame());
	r.OffsetBy(r.Width() + 15,0);
	
	label = TRANSLATE("Payee");
	label << ": " << data.Payee();
	BStringView *payeelabel = new BStringView(r,"payeelabel",label.String());
	back->AddChild(payeelabel);
	payeelabel->ResizeToPreferred();
	
	BString temp;
	gCurrentLocale.CurrencyToString(data.Amount().AbsoluteValue(),temp);
	label = TRANSLATE("Amount");
	label << ": " << temp;
	
	r=payeelabel->Frame();
	r.OffsetBy(r.Width() + 15,0);
	BStringView *amountlabel = new BStringView(r,"amountlabel",label.String());
	back->AddChild(amountlabel);
	amountlabel->ResizeToPreferred();
	
	label = TRANSLATE("Category"); label+=": ";
	if(data.CountCategories()>1)
		label << TRANSLATE("Split");
	else
		label << data.NameAt(0);
	
	r.OffsetTo(10,r.bottom + 15);
	BStringView *categorylabel = new BStringView(r,"categorylabel",label.String());
	back->AddChild(categorylabel);
	categorylabel->ResizeToPreferred();
	r=categorylabel->Frame();
	
	label = TRANSLATE("Memo");
	label << ": " << data.Memo();
	r.OffsetBy(r.Width()+15,0);
	BStringView *memolabel = new BStringView(r,"memolabel",label.String());
	back->AddChild(memolabel);
	memolabel->ResizeToPreferred();
	
	r.OffsetTo(10,r.bottom + 15);
	r.bottom = r.top + 2;
	r.right = Bounds().right - 10;
	
	BBox *divider = new BBox(r);
	AddChild(divider);
	
	fIntervalMenu = new BMenu(TRANSLATE("Frequency"));
	fIntervalMenu->AddItem(new BMenuItem(TRANSLATE("Monthly"),
										new BMessage(M_SCHEDULED_MONTHLY)));
	fIntervalMenu->AddItem(new BMenuItem(TRANSLATE("Quarterly"),
										new BMessage(M_SCHEDULED_QUARTERLY)));
	fIntervalMenu->AddItem(new BMenuItem(TRANSLATE("Annually"),
										new BMessage(M_SCHEDULED_ANNUALLY)));
	fIntervalMenu->ItemAt(0)->SetMarked(true);
	fIntervalMenu->SetLabelFromMarked(true);
	
	// The resizing code in R5's BMenuField sucks. ResizeToPreferred only
	// tweaks the height, not the width, so we have to guesstimate the width
	// in the constructor.
	r.OffsetTo(10,r.bottom + 15);
	temp = TRANSLATE("Frequency"); temp += ": ";
	float strwidth = fIntervalMenu->StringWidth(temp.String())+5;
	r.right = r.left + strwidth + fIntervalMenu->StringWidth(TRANSLATE("Quarterly"))+25;
	BMenuField *intervalfield = new BMenuField(r,"intervalfield",
											temp.String(),fIntervalMenu);
	intervalfield->SetDivider(strwidth);
	
	r = intervalfield->Frame();
	float w,h;
	intervalfield->GetPreferredSize(&w,&h);
	intervalfield->ResizeTo(r.Width(),h);
	back->AddChild(intervalfield);
	
	r = intervalfield->Frame();
	r.OffsetBy(0,r.Height() + 15);
	temp = TRANSLATE("Starting Date"); temp += ": ";
	strwidth = back->StringWidth(temp.String());
	r.right = r.left + strwidth + 
				back->StringWidth("00-00-0000") + 15;
	
	fStartDate = new DateBox(r,"startdate",temp.String(),"",
							new BMessage(M_DATE_CHANGED));
	fStartDate->SetDivider(strwidth + 5);
	fStartDate->UseTabFiltering(false);
	back->AddChild(fStartDate);
	gDefaultLocale.DateToString(data.Date(),temp);
	fStartDate->SetText(temp.String());
	
	r.OffsetTo(Bounds().Width()/2,intervalfield->Frame().top);
	fRepeatAlways = new BRadioButton(r,"inftimes",TRANSLATE("Indefinitely"),
										new BMessage(M_REPEAT_ALWAYS));
	back->AddChild(fRepeatAlways);
	fRepeatAlways->ResizeToPreferred();
	
	r = fRepeatAlways->Frame();
	r.OffsetBy(0,r.Height());
	fRepeatLimited = new BRadioButton(r,"limitedtimes","",new BMessage(M_REPEAT_LIMITED));
	back->AddChild(fRepeatLimited);
	fRepeatLimited->ResizeToPreferred();
	
	r.OffsetTo(fRepeatLimited->Frame().right-5,r.top);
	r = fStartDate->Frame().OffsetToCopy(r.left,r.top);
	r.right = r.left + back->StringWidth("0000") + 5;
	fRepeatCount = new NumBox(r,"repeatcount",NULL,"999",
							new BMessage(M_COUNT_CHANGED));
	fRepeatCount->SetDivider(0);
	fRepeatCount->UseTabFiltering(false);
	back->AddChild(fRepeatCount);
	fRepeatCount->SetEnabled(false);
	
	r.OffsetBy(r.Width() + 5, 0);
	BStringView *timeslabel = new BStringView(r,"timeslabel",TRANSLATE("times"));
	back->AddChild(timeslabel);
	
	fRepeatAlways->SetValue(B_CONTROL_ON);
	
	intervalfield->MakeFocus(true);
	
	BButton *okbutton = new BButton(BRect(0,0,1,1),"okbutton",TRANSLATE("Cancel"),
									new BMessage(M_SCHEDULE_TRANSACTION));
	okbutton->MakeDefault(true);
	okbutton->ResizeToPreferred();
	okbutton->SetLabel(TRANSLATE("OK"));
	okbutton->MoveTo( MAX(memolabel->Frame().right,amountlabel->Frame().right) -
					okbutton->Frame().Width(), fStartDate->Frame().bottom + 10);
	
	BButton *cancelbutton = new BButton(BRect(0,0,1,1),"cancelbutton",
									TRANSLATE("Cancel"),
									new BMessage(B_QUIT_REQUESTED));
	cancelbutton->MakeDefault(true);
	cancelbutton->ResizeToPreferred();
	
	// Another Zeta-based hack. :/
	cancelbutton->MoveTo(okbutton->Frame().left - cancelbutton->Frame().Width() - 15,
						okbutton->Frame().top + 
						(B_BEOS_VERSION > 0x510 ? 0 : 2) );
	
	back->AddChild(cancelbutton);
	back->AddChild(okbutton);
	
	ResizeTo( MAX(amountlabel->Frame().right, timeslabel->Frame().right) + 15,
			fStartDate->Frame().bottom + 20 + okbutton->Frame().Height());
}

void ScheduleAddWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_REPEAT_ALWAYS:
		{
			fRepeatCount->SetEnabled(false);
			break;
		}
		case M_REPEAT_LIMITED:
		{
			fRepeatCount->SetEnabled(true);
			break;
		}
		case M_COUNT_CHANGED:
		{
			if(fRepeatLimited->Value() == B_CONTROL_ON)
			{
				if(strlen(fRepeatCount->Text())<1)
					fRepeatCount->SetText("1");
				
				if(atoi(fRepeatCount->Text()) < 1)
					fRepeatCount->SetText("1");
			}
			break;
		}
		case M_SCHEDULE_TRANSACTION:
		{
			ScheduledTransData stdata(fTransData);
			
			BMenuItem *intervalitem = fIntervalMenu->FindMarked();
			if(!intervalitem)
			{
				ShowBug("NULL menu item in ScheduleAddWindow");
				break;
			}
			
			switch(fIntervalMenu->IndexOf(intervalitem))
			{
				case 0:
				{
					stdata.SetInterval(SCHEDULED_MONTHLY);
					break;
				}
				case 1:
				{
					stdata.SetInterval(SCHEDULED_QUARTERLY);
					break;
				}
				case 2:
				{
					stdata.SetInterval(SCHEDULED_ANNUALLY);
					break;
				}
				default:
				{
					ShowBug("Bad Interval index in ScheduleAddWindow");
					break;
				}
			}
			
			if(fRepeatCount->IsEnabled())
				stdata.SetCount(atoi(fRepeatCount->Text()));
			
			time_t tempdate;
			BString datestr = fStartDate->Text();
			if(datestr.CountChars()<3 ||
					gDefaultLocale.StringToDate(datestr.String(),tempdate)!=B_OK)
			{
				ShowAlert(TRANSLATE("Capital Be didn't understand the date you entered."),
						TRANSLATE(
						"Capital Be understands lots of different ways of entering dates. "
						"Apparently, this wasn't one of them. You'll need to change how you "
						"entered this date. Sorry."));
				break;
			}
			
			stdata.SetDate(tempdate);
			
			gDatabase.AddScheduledTransaction(stdata);
			
			if(fTransData.Type().TypeCode()==TRANS_XFER)
			{
				// Get the counterpart and add it to the scheduled list
				gDatabase.GetTransferCounterpart(stdata.GetID(),stdata);
				gDatabase.AddScheduledTransaction(stdata);
			}
			
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

