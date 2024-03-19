#ifndef SCHEDULE_ADD_WINDOW_H
#define SCHEDULE_ADD_WINDOW_H

#include <Menu.h>
#include <Message.h>
#include <RadioButton.h>
#include <View.h>
#include <Window.h>

#include "TransactionData.h"

class DateBox;
class NumBox;

class ScheduleAddWindow : public BWindow {
  public:
	ScheduleAddWindow(const BRect& frame, const TransactionData& data);
	void MessageReceived(BMessage* msg);

  private:
	DateBox* fStartDate;
	BRadioButton *fRepeatAlways, *fRepeatLimited;
	BMenu* fIntervalMenu;
	NumBox* fRepeatCount;

	TransactionData fTransData;
};

#endif
