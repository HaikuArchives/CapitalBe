#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <Entry.h>
#include <File.h>
#include <String.h>
#include <map>
#include <string>
#include "BuildOptions.h"

typedef std::map<BString, BString> LanguageEntryList;

class Language {
public:
	Language(const entry_ref& ref);
	Language(void);

	virtual ~Language(void);

	virtual void Initialize(void);
	bool IsInitialized(void) const { return fInit; }

	const char* Name(void) { return fName.String(); }

	virtual BString Translate(const char* instring);

protected:
	void SetInitialized(bool value) { fInit = value; }

private:
	LanguageEntryList fEntryList;
	bool fInit;
	BString fName;
	entry_ref fFileRef;
};

// Debugging class used to generate language files quickly
class LanguageLogger : public Language {
public:
	LanguageLogger(const entry_ref& ref);
	~LanguageLogger(void);
	void Initialize(void);
	BString Translate(const char* instring);

	LanguageEntryList fLogList;
	BFile fFile;
};

#endif
