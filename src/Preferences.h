#ifndef PREFERENCES_H_
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

typedef enum {
	CB_MUTED_TEXT = 0,
	CB_MUTED_BG,
	CB_ALT_ROW
} CapitalBeMuted;

const float GetMutedTint(const rgb_color color, const CapitalBeMuted& type);

#endif
