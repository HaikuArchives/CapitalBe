#include "Preferences.h"
#include <Application.h>
#include <Entry.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <time.h>

BLocker prefsLock;
BMessage gPreferences;
BString gAppPath;

static rgb_color sSelectionFocusColor = {205, 205, 255, 255};
static rgb_color sSelectionNoFocusColor = {205, 205, 230, 255};
static rgb_color sGridHeaderColor = {225, 225, 255, 255};

void
ConstrainWindowFrameToScreen(BRect *rect) {
	if (!rect)
		return;

	// Even though we have the proper settings, we're going to make sure that
	// the whole window can be seen on screen.
	BRect screenframe = BScreen().Frame();

	// make sure that the top left corner is actually on the screen
	int32 xoff = 0, yoff = 0;
	if (rect->left < 0)
		xoff = (int32)(0 - rect->left) + 5;
	if (rect->top < 0)
		yoff = (int32)(0 - rect->top) + 15;
	rect->OffsetBy(xoff, yoff);

	// If either dimension is too large for the screen, we move and resize the rect so
	// that it occupies the screen in that dimension
	if (rect->Width() > screenframe.Width()) {
		rect->left = 5;
		rect->right = screenframe.right - 5;
	}

	if (rect->Height() > screenframe.Height()) {
		// We set top to 15 to account for the window tab
		rect->top = 15;
		rect->bottom = screenframe.bottom - 5;
	}

	if (rect->right > screenframe.right)
		rect->right = screenframe.right - 5;

	if (rect->bottom > screenframe.bottom)
		rect->bottom = screenframe.bottom - 5;
}

status_t
SavePreferences(const char *path) {
	if (!path)
		return B_ERROR;

	prefsLock.Lock();

	BFile file(path, B_READ_WRITE | B_ERASE_FILE | B_CREATE_FILE);

	status_t status = file.InitCheck();
	if (status != B_OK) {
		prefsLock.Unlock();
		return status;
	}

	status = gPreferences.Flatten(&file);
	prefsLock.Unlock();
	return status;
}

status_t
LoadPreferences(const char *path) {
	if (!path)
		return B_ERROR;

	prefsLock.Lock();

	app_info ai;
	be_app->GetAppInfo(&ai);

	BEntry entry(&ai.ref);
	entry_ref ref;
	BEntry parent;

	entry.GetParent(&parent);
	parent.GetRef(&ref);
	BPath pathobj(&ref);
	gAppPath = pathobj.Path();
	gAppPath += "/";

	BFile file(path, B_READ_ONLY);

	BMessage msg;

	status_t status = file.InitCheck();
	if (status != B_OK) {
		prefsLock.Unlock();
		return status;
	}

	status = msg.Unflatten(&file);
	if (status == B_OK)
		gPreferences = msg;

	prefsLock.Unlock();
	return status;
}

rgb_color
GetColor(const CapitalBeColor &color) {
	rgb_color returncolor = {0, 0, 0, 0};

	switch (color) {
	case BC_SELECTION_NOFOCUS: {
		returncolor = sSelectionNoFocusColor;
		break;
	}
	case BC_SELECTION_FOCUS: {
		returncolor = sSelectionFocusColor;
		break;
	}
	case BC_GRID_HEADER: {
		returncolor = sGridHeaderColor;
	}
	default:
		break;
	}
	return returncolor;
}
