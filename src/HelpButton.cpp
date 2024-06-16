#include "HelpButton.h"
#include <Catalog.h>
#include <File.h>
#include <LayoutBuilder.h>
#include <LocaleRoster.h>
#include <PathFinder.h>
#include <ScrollView.h>
#include <StringList.h>
#include <TextView.h>
#include <TranslationUtils.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


class HelpButtonWindow : public BWindow {
public:
	HelpButtonWindow(const BRect& frame, const char* title, const entry_ref& helpfileref);

private:
	BTextView* fTextView;
};

HelpButton::HelpButton(const char* title, const char* helpfilename)
	: BButton(helpfilename, "", new BMessage(M_HELPBUTTON_PRESSED))
{
	fWindowTitle = title;
	fRef = GetHelpFile(helpfilename);
	SetIcon(BTranslationUtils::GetBitmap('PNG ', "HelpButtonUp.png"));
}

HelpButton::~HelpButton(void) {}

void
HelpButton::AttachedToWindow()
{
	SetTarget(this);
	BButton::AttachedToWindow();
}

void
HelpButton::MessageReceived(BMessage* msg)
{
	if (msg->what == M_HELPBUTTON_PRESSED) {
		BRect frame = Window()->Frame();
		HelpButtonWindow* help = new HelpButtonWindow(frame, fWindowTitle, fRef);
		help->Show();
	} else {
		BButton::MessageReceived(msg);
	}
}


static bool
getLocalHelpFile(entry_ref& ref, const char* helpfilename, const char* language = "en")
{
	BStringList paths;
	BString helpFile("packages/capitalbe/");
	helpFile += language;
	helpFile += "/";
	helpFile += helpfilename;

	status_t status = BPathFinder::FindPaths(
		B_FIND_PATH_DOCUMENTATION_DIRECTORY, helpFile, B_FIND_PATH_EXISTING_ONLY, paths);

	if (!paths.IsEmpty() && status == B_OK) {
		BEntry data_entry(paths.StringAt(0).String());
		data_entry.GetRef(&ref);

		return true;
	}

	// nothing in DOCUMENTATION_DIRECTORY, try a local file
	helpFile = "helpfiles/";
	helpFile += language;
	helpFile += "/";
	helpFile += helpfilename;

	BEntry entry(helpFile);
	entry.GetRef(&ref);

	return entry.Exists();
}


entry_ref
HelpButton::GetHelpFile(const char* helpfilename)
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
			else if (getLocalHelpFile(
						 ref, helpfilename, BString(language, BString(language).FindFirst("_"))))
				return ref;
		}
	}
	getLocalHelpFile(ref, helpfilename);

	return ref;
}


HelpButtonWindow::HelpButtonWindow(
	const BRect& frame, const char* title, const entry_ref& helpfileref)
	: BWindow(BRect(0, 0, 600, 400), title, B_DOCUMENT_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE)
{
	BView* view = new BView("back", B_WILL_DRAW | B_FRAME_EVENTS);
	view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	BLayoutBuilder::Group<>(this, B_VERTICAL).Add(view).End();

	fTextView = new BTextView("textview", B_WILL_DRAW);
	fTextView->MakeEditable(false);
	fTextView->SetStylable(true);
	fTextView->SetWordWrap(true);

	BScrollView* sv = new BScrollView("scrollview", fTextView, B_FRAME_EVENTS, false, true);

	BFile file(&helpfileref, B_READ_ONLY);
	BTranslationUtils::GetStyledText(&file, fTextView);
	fTextView->MakeFocus(true);

	BLayoutBuilder::Group<>(view, B_VERTICAL).SetInsets(B_USE_WINDOW_INSETS).Add(sv).End();

	CenterIn(frame);
}
