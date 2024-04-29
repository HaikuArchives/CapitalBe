#include "HelpButton.h"
#include <Catalog.h>
#include <File.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <ScrollView.h>
#include <TextView.h>
#include <TranslationUtils.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "HelpButton"


class HelpButtonWindow : public BWindow {
public:
	HelpButtonWindow(const BRect& frame, const entry_ref& helpfileref);

private:
	BTextView* fTextView;
};

HelpButton::HelpButton(const char* name, const char* path)
	: BButton(name, "", new BMessage(M_HELPBUTTON_PRESSED))
{
	get_ref_for_path(path, &fRef);
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
		HelpButtonWindow* help = new HelpButtonWindow(frame, fRef);
		help->Show();
	} else {
		BButton::MessageReceived(msg);
	}
}

HelpButtonWindow::HelpButtonWindow(const BRect& frame, const entry_ref& helpfileref)
	: BWindow(frame, B_TRANSLATE("Help"), B_DOCUMENT_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL,
		  B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
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
	SetTitle(helpfileref.name);
	BTranslationUtils::GetStyledText(&file, fTextView);
	fTextView->MakeFocus(true);

	BLayoutBuilder::Group<>(view, B_VERTICAL).SetInsets(10).Add(sv).End();
}
