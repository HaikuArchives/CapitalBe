#include "Preferences.h"
#include <Application.h>
#include <Entry.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <math.h>

BLocker prefsLock;
BMessage gPreferences;

static rgb_color sSelectionFocusColor = {205, 205, 255, 255};
static rgb_color sSelectionNoFocusColor = {205, 205, 230, 255};
static rgb_color sGridHeaderColor = {225, 225, 255, 255};

status_t
SavePreferences(const char* path)
{
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
LoadPreferences(const char* path)
{
	if (!path)
		return B_ERROR;

	prefsLock.Lock();

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


bool
IsDark()
{
	rgb_color color = ui_color(B_PANEL_BACKGROUND_COLOR);
	// From http://alienryderflex.com/hsp.html
	// Useful in particular to decide if the color is "light" or "dark"
	// by checking if the perceptual brightness is > 127.

	int32 brightness =
		(uint8)roundf(sqrtf(0.299f * color.red * color.red + 0.587f * color.green * color.green +
							0.114 * color.blue * color.blue));

	return brightness <= 127;
}


const float
GetMutedTint(const CapitalBeMuted& type)
{
	switch (type) {
		case CB_MUTED_TEXT:
		{
			return IsDark() ? B_DARKEN_2_TINT : B_LIGHTEN_1_TINT;
		}
		case CB_MUTED_BG:
		{
			return IsDark() ? B_LIGHTEN_2_TINT : B_DARKEN_1_TINT;
		}
		default:
			return B_NO_TINT;
	}
}


rgb_color
GetColor(const CapitalBeColor& color)
{
	rgb_color returncolor = {0, 0, 0, 0};

	switch (color) {
		case BC_SELECTION_NOFOCUS:
		{
			returncolor = sSelectionNoFocusColor;
			break;
		}
		case BC_SELECTION_FOCUS:
		{
			returncolor = sSelectionFocusColor;
			break;
		}
		case BC_GRID_HEADER:
		{
			returncolor = sGridHeaderColor;
		}
		default:
			break;
	}
	return returncolor;
}
