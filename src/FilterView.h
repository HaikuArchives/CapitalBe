#ifndef FILTERVIEW_H
#define FILTERVIEW_H

#include "NumBox.h"
#include <Button.h>
#include <Menu.h>
#include <TextControl.h>
#include <View.h>


enum {
	M_FILTER_CHANGED = 'flch',
	M_CLEAR_FILTER = 'clar',
	M_START_FILTER = 'strt',
};


class FilterView : public BView {
public:
	FilterView(const char* name, int32 flags);
	~FilterView();

	void AttachedToWindow();
	void MessageReceived(BMessage* msg);

	void SetMessenger(BMessenger* msgr) { fMessenger = msgr; }
	bool IsEmpty();

private:
	void MakeEmpty();
	void SendFilterMessage();

	BMenu* fPeriodMenu;
	BMenu* fCompareMenu;

	BTextControl* fPayee;
	BTextControl* fCategory;
	BTextControl* fMemo;
	NumBox* fAmount;

	BButton* fClear;
	BButton* fFilter;

	BMessenger* fMessenger;
};


#endif
