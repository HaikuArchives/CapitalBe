#include "LanguageRoster.h"
#include <Directory.h>
#include <Entry.h>
//#include "Preferences.h"
#include "BuildOptions.h"

//#define BUILD_LOGFILE


LanguageRoster *language_roster=NULL;

LanguageRoster::LanguageRoster(const char *path)
 :	fLanguagePath(path),
 	fCurrentLanguage(NULL),
 	fDefaultLanguage(NULL)
{
	BDirectory dir(fLanguagePath.String());
	
	if(dir.InitCheck()!=B_OK)
		create_directory(fLanguagePath.String(),0777);
	
	entry_ref ref;
	#ifdef BUILD_LOGFILE
	BEntry entry("LoggedTranslationFile");
	entry.GetRef(&ref);
	fCurrentLanguage = fDefaultLanguage = new LanguageLogger(ref);
	#else
	fCurrentLanguage = fDefaultLanguage = new Language();
	#endif
	
	fCurrentLanguage->Initialize();
	fLanguageList.AddItem(fDefaultLanguage);
	while(dir.GetNextRef(&ref)==B_OK)
	{
		Language *loc=new Language(ref);
		fLanguageList.AddItem(loc);
	}
}

LanguageRoster::~LanguageRoster(void)
{
	for(int32 i=0; i<fLanguageList.CountItems(); i++)
	{
		Language *loc=(Language*)fLanguageList.ItemAt(i);
		delete loc;
	}
	fLanguageList.MakeEmpty();
}
	
int32 LanguageRoster::CountLanguages(void) const
{
	return fLanguageList.CountItems();
}

Language *LanguageRoster::LanguageAt(const int32 &index)
{
	return (Language*)fLanguageList.ItemAt(index);
}
	
void LanguageRoster::SetLanguage(const int32 &index)
{
	Language *language = (Language *) fLanguageList.ItemAt(index);
	
	SetLanguage(language);
}

void LanguageRoster::SetLanguage(Language *language)
{
	if(language)
	{
		if(!language->IsInitialized())
			language->Initialize();
		
		fCurrentLanguage = language;
		
		return;
	}
	
	fCurrentLanguage = fDefaultLanguage;
}

bool LanguageRoster::SetLanguage(const char *name)
{
	if(!name)
		return false;
	
	for(int32 i=0; i<fLanguageList.CountItems(); i++)
	{
		Language *language = (Language*)fLanguageList.ItemAt(i);
		if(language && strcmp(language->Name(),name)==0)
		{
			SetLanguage(language);
			return true;
		}
	}

	return false;
}
