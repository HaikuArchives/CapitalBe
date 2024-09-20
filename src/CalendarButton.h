/*
 * Copyright 2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	humdinger (Joachim Seemer)
 */
#ifndef CALENDARBUTTON_H
#define CALENDARBUTTON_H


#include <Button.h>
#include <SupportDefs.h>

#include "DateBox.h"


class CalendarButton : public BButton {
public:
	CalendarButton(DateBox* box);

	void AttachedToWindow();
	virtual void MessageReceived(BMessage* message);

private:
	void _ShowPopUpCalendar();
	void _UpdateIcon();
	BBitmap* _DrawIcon();

	DateBox* fDateBox;
	BMessenger fCalendarWindow;
	BString fDay;
};

#endif // CALENDARBUTTON_H
