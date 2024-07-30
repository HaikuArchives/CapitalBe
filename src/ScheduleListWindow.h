#ifndef SCHEDULELIST_WINDOW
#define SCHEDULELIST_WINDOW

#include <PopUpMenu.h>
#include <Window.h>
#include "ColumnListView.h"

enum {
	M_REMOVE_ITEM = 'rmit',
	M_SELECTION,
	M_CLOSE_CONTEXT
};


class ScheduleContext : public BPopUpMenu {
public:
	ScheduleContext(const char* name, BMessenger target);
	virtual ~ScheduleContext();

private:
	BMessenger fTarget;
};


class ScheduleListView : public BView {
public:
	ScheduleListView(const char* name, const int32& flags);
	void AttachedToWindow(void);
	void MessageReceived(BMessage* msg);

private:
	// This is a float so we can get the maximum string width for payees.
	float RefreshScheduleList(void);

	BColumnListView* fListView;
	void ShowPopUpMenu(BPoint screen);
	bool fShowingPopUpMenu;

	BButton* fRemoveButton;
	BList fTransList;

	float fBestWidth;
};

class ScheduleListWindow : public BWindow {
public:
	ScheduleListWindow(const BRect& frame);
};

#endif
