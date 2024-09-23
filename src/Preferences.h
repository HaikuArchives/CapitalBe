/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	dospuntos (Johan Wagenheim)
 *	humdinger (Joachim Seemer)
 */
#ifndef PREFERENCES_H_
/*
 * Copyright 2009-2024. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 *	dospuntos (Johan Wagenheim)
 *	humdinger (Joachim Seemer)
 */
#define PREFERENCES_H_

#include <File.h>
#include <InterfaceDefs.h>
#include <Locker.h>
#include <Message.h>
#include <String.h>

extern BLocker prefsLock;
extern BMessage gPreferences;
extern BPath gSettingsPath;
extern rgb_color gNegativeColor;

status_t SavePreferences();
status_t LoadPreferences();

// clang-format off
typedef enum {
	CB_MUTED_TEXT = 0,
	CB_MUTED_BG,
	CB_ALT_ROW
} CapitalBeMuted;
// clang-format on

const float GetMutedTint(const rgb_color color, const CapitalBeMuted& type);

#endif	// PREFERENCES_H_
