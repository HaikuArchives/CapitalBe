#include "Preferences.h"
#include <Application.h>
#include <Entry.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <math.h>
#include <cstdio>

BLocker prefsLock;
BMessage gPreferences;
rgb_color gNegativeColor;


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
IsDark(rgb_color color)
{
	// From http://alienryderflex.com/hsp.html
	// Useful in particular to decide if the color is "light" or "dark"
	// by checking if the perceptual brightness is > 127.
	// TODO: Once Haiku Beta5 is out, we can use its IsDark()

	int32 brightness
		= (uint8)roundf(sqrtf(0.299f * color.red * color.red + 0.587f * color.green * color.green
							  + 0.114 * color.blue * color.blue));

	return brightness <= 127;
}


const float
GetMutedTint(const rgb_color color, const CapitalBeMuted& type)
{
	switch (type) {
		case CB_MUTED_TEXT:
		{
			return IsDark(color) ? B_DARKEN_2_TINT : B_LIGHTEN_1_TINT;
		}
		case CB_MUTED_BG:
		{
			return IsDark(color) ? B_LIGHTEN_2_TINT : B_DARKEN_1_TINT;
		}
		case CB_ALT_ROW:
		{
			return IsDark(color) ? 0.94 : 1.05;
		}
		default:
			return B_NO_TINT;
	}
}
