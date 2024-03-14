#include "Language.h"
#include "BuildOptions.h"
#include "TextFile.h"
#include <OS.h>
#include <stdio.h>
#include <string.h>
#include <string>

/*
	This class is probably the complete wrong way to go about adding
	translations, but it shouldn't be too hard to rip out if it needs
	to be rewritten or replaced later.
*/

Language::Language(const entry_ref &ref) : fInit(false), fName(ref.name), fFileRef(ref) {}

Language::Language(void) : fInit(false) {}

Language::~Language(void) {}

void
Language::Initialize(void) {
	// Here we load the dictionary file for the app

	fInit = true;

	if (fName.CountChars() < 1) {
		fName = "English";
		return;
	}

	// Read in the dictionary file into a BString
	TextFile file(fFileRef, B_READ_ONLY);

	// At this point, we'll read in all the data about the translation

	// Now read in the translation strings
	bool nextline = false;

	while (1) {
		BString line = file.ReadLine(), key = "", value = "";

		if (line.CountChars() == 0)
			break;

		int32 i = 0;

		for (i = 0; i < line.CountChars(); i++) {
			if (line[i] == '\t')
				break;

			// lines starting with ; or # are comments
			if (i == 0) {
				if (line[i] == '#' || line[i] == ';') {
					nextline = true;
					break;
				} else {
					key += line[i];
				}
			} else {
				key += line[i];
			}
		}

		if (nextline) {
			nextline = false;
			continue;
		}

		i++;
		while (line[i]) {
			value += line[i];
			i++;
		}

		fEntryList[key.String()] = value.String();
	}
}

BString
Language::Translate(const char *instring) {
	if (!instring)
		return NULL;

	if (!fInit) {
		Initialize();
		if (!fInit) {
			printf("Language %s has bad Initialize() function\n", Name());
			return instring;
		}
	}

	BString str = fEntryList[instring];
	if (str.CountChars() > 0)
		return str;

	return BString(instring);
}

LanguageLogger::LanguageLogger(const entry_ref &ref)
	: Language(ref), fFile(&ref, B_CREATE_FILE | B_ERASE_FILE | B_READ_WRITE)

{
	SetInitialized(true);
}

LanguageLogger::~LanguageLogger(void) { fFile.Unset(); }

void
LanguageLogger::Initialize(void) {}

BString
LanguageLogger::Translate(const char *instring) {
	if (!instring)
		return BString();

	if (fLogList[instring].CountChars() == 0) {
		BString outstring(instring);
		outstring << "\t" << BString(instring).ToUpper() << "\n";

		fFile.Write(outstring.String(), outstring.Length());

		fLogList[instring] = instring;
	}

	return BString(instring);
}
