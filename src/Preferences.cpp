/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	dospuntos (Johan Wagenheim)
 *	humdinger (Joachim Seemer)
 */
#include "Preferences.h"
#include <Application.h>

#include <Entry.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>

#include <math.h>

BLocker prefsLock;
BMessage gPreferences;
BPath gSettingsPath;
rgb_color gNegativeColor;


status_t
SavePreferences()
{
	prefsLock.Lock();

	BPath path = gSettingsPath;
	path.Append("CapitalBeSettings");
	BFile file(path.Path(), B_READ_WRITE | B_ERASE_FILE | B_CREATE_FILE);

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
LoadPreferences()
{
	prefsLock.Lock();

	BPath path = gSettingsPath;
	path.Append("CapitalBeSettings");
	BFile file(path.Path(), B_READ_ONLY);

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


const float
GetMutedTint(const rgb_color color, const CapitalBeMuted& type)
{
	switch (type) {
		case CB_MUTED_TEXT:
		{
			return color.IsDark() ? B_DARKEN_2_TINT : B_LIGHTEN_1_TINT;
		}
		case CB_MUTED_BG:
		{
			return color.IsDark() ? B_LIGHTEN_2_TINT : B_DARKEN_1_TINT;
		}
		case CB_ALT_ROW:
		{
			return color.IsDark() ? 0.94 : 1.05;
		}
		default:
			return B_NO_TINT;
	}
}
