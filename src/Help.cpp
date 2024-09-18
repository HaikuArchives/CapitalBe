#include "Help.h"

#include <Application.h>
#include <Bitmap.h>
#include <ControlLook.h>
#include <File.h>
#include <IconUtils.h>
#include <LocaleRoster.h>
#include <Path.h>
#include <PathFinder.h>
#include <Resources.h>
#include <Roster.h>
#include <TranslationUtils.h>


static bool
getLocalHelpFile(entry_ref& ref, const char* helpfilename, const char* language = "en")
{
	BStringList paths;
	BString helpFile("packages/capitalbe/");
	helpFile += language;
	helpFile += "/";
	helpFile += helpfilename;

	status_t status = BPathFinder::FindPaths(B_FIND_PATH_DOCUMENTATION_DIRECTORY, helpFile,
		B_FIND_PATH_EXISTING_ONLY, paths);

	if (!paths.IsEmpty() && status == B_OK) {
		BEntry data_entry(paths.StringAt(0).String());
		data_entry.GetRef(&ref);

		return true;
	}

	// nothing in DOCUMENTATION_DIRECTORY, try a local file
	helpFile = "documentation/";
	helpFile += language;
	helpFile += "/";
	helpFile += helpfilename;

	BEntry entry(helpFile);
	entry.GetRef(&ref);

	return entry.Exists();
}


entry_ref
getHelpFile(const char* helpfilename)
{
	entry_ref ref;
	BMessage message;
	BLocaleRoster* roster = BLocaleRoster::Default();

	if (roster->GetPreferredLanguages(&message) == B_OK) {
		const char* language;

		for (int32 i = 0; (language = message.GetString("language", i, NULL)) != NULL; i++) {
			if (getLocalHelpFile(ref, helpfilename, language))
				return ref;
			// in case of the current locale is a subset like "es_419", remove the "_419"
			else if (getLocalHelpFile(ref, helpfilename,
						 BString(language, BString(language).FindFirst("_"))))
				return ref;
		}
	}
	getLocalHelpFile(ref, helpfilename);

	return ref;
}


void
openDocumentation(const char* helpfilename, const char* anchor = NULL)
{
	entry_ref ref = getHelpFile(helpfilename);
	BPath path(&ref);
	BString url;
	url << "file://" << path.Path();
	if (anchor != NULL)
		url << anchor;

	BMessage message(B_REFS_RECEIVED);
	message.AddString("url", url.String());
	be_roster->Launch("text/html", &message);
}


BBitmap*
getHelpIcon()
{
	BBitmap* icon = NULL;
	BResources* resources = BApplication::AppResources();
	if (resources != NULL) {
		size_t size;
		const uint8* data
			= (const uint8*)resources->LoadResource(B_VECTOR_ICON_TYPE, "help-icon", &size);
		icon = new BBitmap(BRect(BPoint(0, 0), be_control_look->ComposeIconSize(B_MINI_ICON)),
			B_RGBA32);
		if (icon != NULL) {
			if (data == NULL || BIconUtils::GetVectorIcon(data, size, icon) != B_OK) {
				delete icon;
				return NULL;
			}
		}
	}
	return icon;
}


HelpButton::HelpButton(const char* helpfilename, const char* anchor = NULL)
	:
	BButton(helpfilename, "", new BMessage(M_HELP)),
	fHelpfile(helpfilename),
	fAnchor(anchor)
{
	BBitmap* icon = getHelpIcon();
	SetIcon(icon);
	float width = icon->Bounds().Width() + be_control_look->ComposeSpacing(8);
	SetExplicitSize(BSize(width, width));
}


HelpButton::~HelpButton()
{
}


void
HelpButton::AttachedToWindow()
{
	SetTarget(this);
	BButton::AttachedToWindow();
}


void
HelpButton::MessageReceived(BMessage* msg)
{
	if (msg->what == M_HELP)
		openDocumentation(fHelpfile, fAnchor);
	else
		BButton::MessageReceived(msg);
}
