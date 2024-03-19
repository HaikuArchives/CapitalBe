#ifndef LANGUAGE_ROSTER_H
#define LANGUAGE_ROSTER_H

#include "Language.h"
#include <List.h>

class LanguageRoster {
  public:
	LanguageRoster(const char* path);
	~LanguageRoster(void);

	int32 CountLanguages(void) const;
	Language* LanguageAt(const int32& index);

	void SetLanguage(const int32& index);
	void SetLanguage(Language* language);
	bool SetLanguage(const char* name);

	Language* GetLanguage(void) { return fCurrentLanguage; }

  private:
	BString fLanguagePath;
	BList fLanguageList;
	Language* fCurrentLanguage;
	Language* fDefaultLanguage;
};

extern Language* gCurrentLanguage;
extern LanguageRoster* language_roster;

#endif
