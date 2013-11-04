#include "HelpButton.h"
#include "Preferences.h"
#include <TranslationUtils.h>
#include <TextView.h>
#include <ScrollView.h>
#include <File.h>
#include <Path.h>
#include "Translate.h"

class HelpButtonWindow : public BWindow
{
public:
	HelpButtonWindow(const BRect &frame,const entry_ref &helpfileref);
	HelpButtonWindow(const BRect &frame,const char *helppath);
	void FrameResized(float height, float width);

private:
	BTextView *fTextView;
};

HelpButton::HelpButton(const BPoint &pt, const char *name,
						const entry_ref &ref, const int32 &resize,
						const int32 &flags)
 : DrawButton(BRect(pt.x,pt.y,pt.x+15,pt.y+15),name,
 				BTranslationUtils::GetBitmap('PNG ',"HelpButtonUp20.png"),
 				BTranslationUtils::GetBitmap('PNG ',"HelpButtonDown20.png"),
 				new BMessage(M_HELPBUTTON_PRESSED),resize,flags),
 	fRef(ref)
{
	ResizeToPreferred();
}

HelpButton::HelpButton(const BPoint &pt, const char *name,
						const char *path, const int32 &resize,
						const int32 &flags)
 : DrawButton(BRect(pt.x,pt.y,pt.x+15,pt.y+15),name,
 				BTranslationUtils::GetBitmap('PNG ',"HelpButtonUp.png"),
 				BTranslationUtils::GetBitmap('PNG ',"HelpButtonDown.png"),
 				new BMessage(M_HELPBUTTON_PRESSED),resize,flags)
{
	get_ref_for_path(path,&fRef);
	ResizeToPreferred();
}

HelpButton::~HelpButton(void)
{
}

void HelpButton::AttachedToWindow(void)
{
	SetTarget(this);
}

void HelpButton::MessageReceived(BMessage *msg)
{
	if(msg->what==M_HELPBUTTON_PRESSED)
	{
		BRect frame = Window()->Frame();
		HelpButtonWindow *help = new HelpButtonWindow(frame,fRef);
		help->Show();
	}
	else
		DrawButton::MessageReceived(msg);
}

HelpButtonWindow::HelpButtonWindow(const BRect &frame,const entry_ref &helpfileref)
 : BWindow(frame,TRANSLATE("Help"),B_DOCUMENT_WINDOW_LOOK,
 			B_FLOATING_APP_WINDOW_FEEL,B_ASYNCHRONOUS_CONTROLS)
{
	BView *view = new BView(Bounds(),"back",B_FOLLOW_ALL,B_WILL_DRAW | B_FRAME_EVENTS);
	view->SetViewColor(235,235,255);
	AddChild(view);
	
	BRect r(Bounds().InsetByCopy(10,10));
	r.right -= B_V_SCROLL_BAR_WIDTH;
	
	BRect tr(Bounds().InsetByCopy(10,10));
	tr.right -= B_V_SCROLL_BAR_WIDTH + 10;
	r.bottom -= 5;
	
	fTextView = new BTextView(r,"textview",tr,B_FOLLOW_ALL,B_WILL_DRAW);
	fTextView->MakeEditable(false);
	fTextView->SetStylable(true);
	fTextView->SetWordWrap(true);
	
	BScrollView *sv = new BScrollView("scrollview",fTextView,B_FOLLOW_ALL,B_FRAME_EVENTS,false,true);
	view->AddChild(sv);
		
	BFile file(&helpfileref,B_READ_ONLY);
	SetTitle(helpfileref.name);
	BTranslationUtils::GetStyledText(&file,fTextView);
	fTextView->MakeFocus(true);
}


void HelpButtonWindow::FrameResized(float height, float width)
{
	fTextView->FrameResized(width, height);

	if (fTextView->DoesWordWrap())
	{
		BRect textRect;
		textRect = Bounds();
		textRect.InsetBy(10,10);
		textRect.right -= B_V_SCROLL_BAR_WIDTH + 10;
		fTextView->SetTextRect(textRect);
	}
}

