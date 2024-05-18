#ifndef PREFERENCES_H_
#define PREFERENCES_H_

#include <File.h>
#include <InterfaceDefs.h>
#include <Locker.h>
#include <Message.h>
#include <String.h>
#include "Fixed.h"

extern BLocker prefsLock;
extern BMessage gPreferences;

#define PREFERENCES_PATH "/boot/home/config/settings/CapitalBe"

status_t SavePreferences(const char* path);
status_t LoadPreferences(const char* path);

typedef enum {
	BC_SELECTION_FOCUS = 0,
	BC_SELECTION_NOFOCUS,
	BC_GRID_HEADER

} CapitalBeColor;

typedef enum {
	CB_MUTED_TEXT = 0,
	CB_MUTED_BG
} CapitalBeMuted;

rgb_color GetColor(const CapitalBeColor& color);
const float GetMutedTint(const rgb_color color, const CapitalBeMuted& type);

#endif
