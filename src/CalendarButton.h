#ifndef CALENDARBUTTON_H
#define CALENDARBUTTON_H


#include <Button.h>
#include <SupportDefs.h>

#include "DateBox.h"


class CalendarButton : public BButton {
public:
	CalendarButton(DateBox* box);

	void AttachedToWindow(void);
	virtual void MessageReceived(BMessage* message);

private:
	BBitmap* DrawIcon();
	void ShowPopUpCalendar();
	void UpdateIcon();

	DateBox* fDateBox;
	BMessenger fCalendarWindow;
	BString fDay;
};

#endif	// CALENDARBUTTON_H
