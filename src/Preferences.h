#ifndef PREFERENCES_H_
#define PREFERENCES_H_

#include "Fixed.h"
#include <File.h>
#include <InterfaceDefs.h>
#include <Locker.h>
#include <Message.h>
#include <String.h>

extern BLocker prefsLock;
extern BMessage gPreferences;
extern BString gAppPath;

#define PREFERENCES_PATH "/boot/home/config/settings/CapitalBe"

status_t
SavePreferences(const char *path);
status_t
LoadPreferences(const char *path);

void
ConstrainWindowFrameToScreen(BRect *rect);

typedef enum { BC_SELECTION_FOCUS = 0, BC_SELECTION_NOFOCUS, BC_GRID_HEADER } CapitalBeColor;

rgb_color
GetColor(const CapitalBeColor &color);

#endif
