#include "DateBox.h"

#include "CalendarMenuWindow.h"
#include "CBLocale.h"
#include "Database.h"
#include <DateFormat.h>
#include "MsgDefs.h"
#include "TimeSupport.h"

#include <Catalog.h>
#include <LayoutBuilder.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TextInput"

enum {
	M_SHOW_CALENDER,
	M_SET_DATE
};

DateBoxFilter::DateBoxFilter(DateBox* box)
	: AutoTextControlFilter(box)
{
}

filter_result
DateBoxFilter::KeyFilter(const int32& key, const int32& mod)
{
	// Here is where all the *real* work for a date box is done.
	if (key == B_TAB) {
		if (!((DateBox*)TextControl())->IsTabFiltering())
			return B_DISPATCH_MESSAGE;

		if (mod & B_SHIFT_KEY)
			SendMessage(new BMessage(M_PREVIOUS_FIELD));
		else
			SendMessage(new BMessage(M_NEXT_FIELD));
		return B_SKIP_MESSAGE;
	}

#ifdef ENTER_NAVIGATION
	if (key == B_ENTER) {
		SendMessage(new BMessage(M_ENTER_NAVIGATION));
		return B_SKIP_MESSAGE;
	}
#endif

	//	if(key == B_ESCAPE && !IsEscapeCancel())
	//		return B_SKIP_MESSAGE;

	// Weed out navigation keys
	if ((key < 32 && key != B_PAGE_UP && key != B_PAGE_DOWN))
		return B_DISPATCH_MESSAGE;

	int32 start, end;
	DateBox* box = (DateBox*)TextControl();
	box->TextView()->GetSelection(&start, &end);

	BString keystring;
	GetCurrentMessage()->FindString("bytes", &keystring);

	BString string;
	if (keystring == "+") {
		if (strlen(box->Text()) > 0)
			box->fCurrentDate = IncrementDateByDay(box->fCurrentDate);

		gDefaultLocale.DateToString(box->fCurrentDate, string);
		box->SetText(string.String());
		box->TextView()->SelectAll();
		TextControl()->Invoke();
		return B_SKIP_MESSAGE;
	} else if (key == B_PAGE_UP) {
		if (strlen(box->Text()) > 0) {
			if (mod & B_SHIFT_KEY)
				box->fCurrentDate = IncrementDateByYear(box->fCurrentDate);
			else
				box->fCurrentDate = IncrementDateByMonth(box->fCurrentDate);
		}
		gDefaultLocale.DateToString(box->fCurrentDate, string);
		box->SetText(string.String());
		box->TextView()->SelectAll();
		TextControl()->Invoke();
		return B_SKIP_MESSAGE;
	} else if (key == B_PAGE_DOWN) {
		if (strlen(box->Text()) > 0) {
			if (mod & B_SHIFT_KEY)
				box->fCurrentDate = DecrementDateByYear(box->fCurrentDate);
			else
				box->fCurrentDate = DecrementDateByMonth(box->fCurrentDate);
		}
		gDefaultLocale.DateToString(box->fCurrentDate, string);
		box->SetText(string.String());
		box->TextView()->SelectAll();
		TextControl()->Invoke();
		return B_SKIP_MESSAGE;
	}

	return B_DISPATCH_MESSAGE;
}

DateBox::DateBox(const char* name, const char* label, const char* text, BMessage* msg, uint32 flags)
	: AutoTextControl(name, label, text, msg, flags),
	  fFilterTab(true)
{
	SetFilter(new DateBoxFilter(this));
	fCurrentDate = GetCurrentDate();

	const char date_disallowed[] =
		"`~!@#$%^&*()_=QWERTYUIOP{[}]|\\ASDFGHJKL;:'\""
		"ZXCVBNM,<>?qwertyuiopasdfghjklzxcvbnm";
	int32 i = 0;
	while (date_disallowed[i]) {
		TextView()->DisallowChar(date_disallowed[i]);
		i++;
	}

	// Even though dates should have a limit of 10 characters, we want to give
	// the user a little elbow room. ;^)
	SetCharacterLimit(15);

	fCalenderButton = new BButton("calenderbutton", "📅", new BMessage(M_SHOW_CALENDER));
	float height;
	GetPreferredSize(NULL, &height);
	BSize size(height + 2, height);
	fCalenderButton->SetExplicitSize(size);

	BGroupLayout* textcontrolGroup;
	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0)
		.GetLayout(&textcontrolGroup)
		.Add(fCalenderButton)
		.End();
}


void
DateBox::AttachedToWindow(void)
{
	fCalenderButton->SetTarget(this);
}


void
DateBox::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_SHOW_CALENDER:
		{
			ShowPopUpCalendar();
			break;
		}
		case M_SET_DATE:
		{
			int32 day, month, year;
			msg->FindInt32("day", &day);
			msg->FindInt32("month", &month);
			msg->FindInt32("year", &year);
			BString date;
			date << day << "." << month << "." << year;
			SetText(date);
			break;
		}
		default:
		{
			BTextControl::MessageReceived(msg);
		}
	}
}

bool
DateBox::Validate(const bool& alert)
{
	BString date;
	time_t tdate;
	if (strlen(Text()) < 1) {
		BString date;
		gDefaultLocale.DateToString(fCurrentDate, date);
		SetText(date.String());
	} else if (gDefaultLocale.StringToDate(Text(), tdate) != B_OK) {
		if (alert) {
			ShowAlert(B_TRANSLATE("CapitalBe didn't understand the date you entered"),
				B_TRANSLATE("CapitalBe understands lots of different ways of entering dates. "
							"Apparently, this wasn't one of them. You'll need to change how you "
							"entered this date. Sorry."));
			MakeFocus(true);
		}
		return false;
	} else {
		fCurrentDate = tdate;
		gDefaultLocale.DateToString(fCurrentDate, date);
		SetText(date.String());
	}
	return true;
}


void
DateBox::ShowPopUpCalendar()
{
	if (fCalendarWindow.IsValid()) {
		BMessage activate(B_SET_PROPERTY);
		activate.AddSpecifier("Active");
		activate.AddBool("data", true);

		if (fCalendarWindow.SendMessage(&activate) == B_OK)
			return;
	}

	BDate date = GetDate();
	BMessage* invocationMessage = new BMessage(M_SET_DATE);
	BPoint where = fCalenderButton->Frame().LeftTop();
	ConvertToScreen(&where);

	CalendarMenuWindow* window = new CalendarMenuWindow(this, where);
	window->SetDate(date);
	window->SetInvocationMessage(invocationMessage);
	window->SetDate(date);
	fCalendarWindow = BMessenger(window);
	window->Show();

	window->MoveBy(fCalenderButton->Bounds().Width() / 2, window->Bounds().Height() * -1.5);
	window->MoveOnScreen(B_MOVE_IF_PARTIALLY_OFFSCREEN);
}
