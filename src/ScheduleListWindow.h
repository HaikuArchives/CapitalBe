/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	humdinger (Joachim Seemer)
 */
#ifndef SCHEDULELIST_WINDOW
#define SCHEDULELIST_WINDOW

#include "ColumnListView.h"

#include <PopUpMenu.h>
#include <Window.h>

// clang-format off
enum {
	M_REMOVE_ITEM = 'rmit',
	M_SELECTION,
	M_CLOSE_CONTEXT
};
// clang-format on

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

	void AttachedToWindow();
	void MessageReceived(BMessage* msg);

private:
	// This is a float so we can get the maximum string width for payees.
	float _RefreshScheduleList();
	void _ShowPopUpMenu(BPoint screen);

	BColumnListView* fListView;
	bool fShowingPopUpMenu;

	BButton* fRemoveButton;
	BList fTransList;

	float fBestWidth;
};

class ScheduleListWindow : public BWindow {
public:
	ScheduleListWindow(const BRect& frame);
};

#endif // SCHEDULELIST_WINDOW
