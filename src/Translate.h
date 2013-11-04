#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "Language.h"

extern Language *gCurrentLanguage;

#define TRANSLATE(x) gCurrentLanguage->Translate(x).String()

#endif
