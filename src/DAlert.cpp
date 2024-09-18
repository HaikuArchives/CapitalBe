//------------------------------------------------------------------------------
//	Copyright (c) 2001-2006, Haiku
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Alert.cpp
//	Author:			Erik Jaesler (erik@cgsoftware.com)
//	Description:	DAlert displays a modal alert window.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <ControlLook.h>
#include <string.h>

// System Includes -------------------------------------------------------------
#include <IconUtils.h>
#include <Invoker.h>
#include <Looper.h>
#include <Message.h>
#include <MessageFilter.h>

#include "DAlert.h"
#include <Bitmap.h>
#include <Button.h>
#include <Screen.h>
#include <TextView.h>
#include <View.h>

#include <Beep.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Resources.h>
#include <TranslationUtils.h>

#include <Autolock.h>

// #define DEBUG_ALERT
#ifdef DEBUG_ALERT
#define FTRACE(x) fprintf(x)
#else
#define FTRACE(x) /* nothing */
#endif

// Default size of the Alert window.
#define DEFAULT_RECT BRect(0, 0, 450, 75)
#define max(LHS, RHS) ((LHS) > (RHS) ? (LHS) : (RHS))


namespace CapitalBe {


// Globals ---------------------------------------------------------------------
static const unsigned int kAlertButtonMsg = 'ALTB';
static const int kSemTimeOut = 50000;

static const int kButtonBottomOffset = 9;
static const int kDefButtonBottomOffset = 6;
static const int kButtonRightOffset = 6;
static const int kButtonSpaceOffset = 6;
static const int kButtonOffsetSpaceOffset = 26;
static const int kButtonMinOffsetSpaceOffset = kButtonOffsetSpaceOffset / 2;
static const int kButtonLeftOffset = 62;
static const int kButtonUsualWidth = 75;

static const int kWindowIconOffset = 27;
static const int kWindowMinWidth = 450;
static const int kWindowMinOffset = 12;
static const int kWindowOffsetMinWidth = 335;

static const int kIconStripeWidth = 30;

static const int kTextLeftOffset = 10;
static const int kTextIconOffset = kWindowIconOffset + kIconStripeWidth - 2;
static const int kTextTopOffset = 6;
static const int kTextRightOffset = 10;
static const int kTextBottomOffset = 45;

//------------------------------------------------------------------------------
class TAlertView : public BView {
public:
	TAlertView(BRect frame);
	TAlertView(BMessage* archive);
	~TAlertView();

	static TAlertView* Instantiate(BMessage* archive);
	status_t Archive(BMessage* archive, bool deep = true);

	virtual void Draw(BRect updateRect);

	// These functions (or something analogous) are missing from libbe.so's
	// dump.  I can only assume that the bitmap is a public var in the
	// original implementation -- or DAlert is a friend of TAlertView.
	// Neither one is necessary, since I can just add these.
	void SetBitmap(BBitmap* Icon) { fIconBitmap = Icon; }
	BBitmap* Bitmap() { return fIconBitmap; }

private:
	BBitmap* fIconBitmap;
};

//------------------------------------------------------------------------------
// I'm making a guess based on the name and TextEntryAlert's implementation that
// this is a BMessageFilter.  I'm not sure, but I think I actually prefer how
// TextEntryAlert does it, but there are clearly no message filtering functions
// on DAlert so here we go.
class _DAlertFilter_ : public BMessageFilter {
public:
	_DAlertFilter_(DAlert* Alert);
	~_DAlertFilter_();

	virtual filter_result Filter(BMessage* msg, BHandler** target);

private:
	DAlert* fAlert;
};


static float
width_from_label(BButton* button)
{
	// BButton::GetPreferredSize() does not return the minimum width
	// required to fit the label. Thus, the width is computed here.
	return button->StringWidth(button->Label()) + 20.0f;
}

//	#pragma mark - DAlert


DAlert::DAlert(const char* title, const char* text, const char* button1, const char* button2,
	const char* button3, button_width width, alert_type type)
	:
	BWindow(DEFAULT_RECT, title, B_MODAL_WINDOW,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	_InitObject(text, button1, button2, button3, width, B_EVEN_SPACING, type);
}


DAlert::DAlert(const char* title, const char* text, const char* button1, const char* button2,
	const char* button3, button_width width, button_spacing spacing, alert_type type)
	:
	BWindow(DEFAULT_RECT, title, B_MODAL_WINDOW,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	_InitObject(text, button1, button2, button3, width, spacing, type);
}


DAlert::~DAlert()
{
	// Probably not necessary, but it makes me feel better.
	if (fAlertSem >= B_OK)
		delete_sem(fAlertSem);
}


DAlert::DAlert(BMessage* data)
	:
	BWindow(data)
{
	fInvoker = NULL;
	fAlertSem = -1;
	fAlertVal = -1;

	fTextView = (BTextView*)FindView("_tv_");

	fButtons[0] = (BButton*)FindView("_b0_");
	fButtons[1] = (BButton*)FindView("_b1_");
	fButtons[2] = (BButton*)FindView("_b2_");

	if (fButtons[2])
		SetDefaultButton(fButtons[2]);
	else if (fButtons[1])
		SetDefaultButton(fButtons[1]);
	else if (fButtons[0])
		SetDefaultButton(fButtons[0]);

	TAlertView* master = (TAlertView*)FindView("_master_");
	if (master)
		master->SetBitmap(_InitIcon());

	// Get keys
	char key;
	for (int32 i = 0; i < 3; ++i) {
		if (data->FindInt8("_but_key", i, (int8*)&key) == B_OK)
			fKeys[i] = key;
	}

	int32 temp;
	// Get alert type
	if (data->FindInt32("_atype", &temp) == B_OK)
		fMsgType = (alert_type)temp;

	// Get button width
	if (data->FindInt32("_but_width", &temp) == B_OK)
		fButtonWidth = (button_width)temp;

	AddCommonFilter(new _DAlertFilter_(this));
}


BArchivable*
DAlert::Instantiate(BMessage* data)
{
	if (!validate_instantiation(data, "DAlert"))
		return NULL;

	return new DAlert(data);
}


status_t
DAlert::Archive(BMessage* data, bool deep) const
{
	status_t ret = BWindow::Archive(data, deep);

	// Stow the text
	if (ret == B_OK)
		ret = data->AddString("_text", fTextView->Text());

	// Stow the alert type
	if (ret == B_OK)
		ret = data->AddInt32("_atype", fMsgType);

	// Stow the button width
	if (ret == B_OK)
		ret = data->AddInt32("_but_width", fButtonWidth);

	// Stow the shortcut keys
	if (fKeys[0] || fKeys[1] || fKeys[2]) {
		// If we have any to save, we must save something for everyone so it
		// doesn't get confusing on the unarchive.
		if (ret == B_OK)
			ret = data->AddInt8("_but_key", fKeys[0]);
		if (ret == B_OK)
			ret = data->AddInt8("_but_key", fKeys[1]);
		if (ret == B_OK)
			ret = data->AddInt8("_but_key", fKeys[2]);
	}

	return ret;
}


void
DAlert::SetShortcut(int32 index, char key)
{
	if (index >= 0 && index < 3)
		fKeys[index] = key;
}


char
DAlert::Shortcut(int32 index) const
{
	if (index >= 0 && index < 3)
		return fKeys[index];

	return 0;
}


int32
DAlert::Go()
{
	fAlertSem = create_sem(0, "AlertSem");
	if (fAlertSem < B_OK) {
		Quit();
		return -1;
	}

	// Get the originating window, if it exists
	BWindow* window = dynamic_cast<BWindow*>(BLooper::LooperForThread(find_thread(NULL)));
	this->CenterIn(window->Frame());

	Show();

	// Heavily modified from TextEntryAlert code; the original didn't let the
	// blocked window ever draw.
	if (window) {
		status_t err;
		for (;;) {
			do {
				err = acquire_sem_etc(fAlertSem, 1, B_RELATIVE_TIMEOUT, kSemTimeOut);
				// We've (probably) had our time slice taken away from us
			} while (err == B_INTERRUPTED);

			if (err == B_BAD_SEM_ID) {
				// Semaphore was finally nuked in MessageReceived
				break;
			}
			window->UpdateIfNeeded();
		}
	} else {
		// No window to update, so just hang out until we're done.
		while (acquire_sem(fAlertSem) == B_INTERRUPTED) {
		}
	}

	// Have to cache the value since we delete on Quit()
	int32 value = fAlertVal;
	if (Lock())
		Quit();

	return value;
}


status_t
DAlert::Go(BInvoker* invoker)
{
	fInvoker = invoker;
	Show();
	return B_OK;
}


void
DAlert::MessageReceived(BMessage* msg)
{
	if (msg->what != kAlertButtonMsg)
		return BWindow::MessageReceived(msg);

	int32 which;
	if (msg->FindInt32("which", &which) == B_OK) {
		if (fAlertSem < B_OK) {
			// Semaphore hasn't been created; we're running asynchronous
			if (fInvoker) {
				BMessage* out = fInvoker->Message();
				if (out
					&& (out->ReplaceInt32("which", which) == B_OK
						|| out->AddInt32("which", which) == B_OK))
					fInvoker->Invoke();
			}
			PostMessage(B_QUIT_REQUESTED);
		} else {
			// Created semaphore means were running synchronously
			fAlertVal = which;

			// TextAlertVar does release_sem() below, and then sets the
			// member var.  That doesn't make much sense to me, since we
			// want to be able to clean up at some point.  Better to just
			// nuke the semaphore now; we don't need it any more and this
			// lets synchronous Go() continue just as well.
			delete_sem(fAlertSem);
			fAlertSem = -1;
		}
	}
}


void
DAlert::FrameResized(float newWidth, float newHeight)
{
	BWindow::FrameResized(newWidth, newHeight);
}


BButton*
DAlert::ButtonAt(int32 index) const
{
	if (index >= 0 && index < 3)
		return fButtons[index];

	return NULL;
}


BTextView*
DAlert::TextView() const
{
	return fTextView;
}


BHandler*
DAlert::ResolveSpecifier(BMessage* msg, int32 index, BMessage* specifier, int32 form,
	const char* property)
{
	return BWindow::ResolveSpecifier(msg, index, specifier, form, property);
}


status_t
DAlert::GetSupportedSuites(BMessage* data)
{
	return BWindow::GetSupportedSuites(data);
}


void
DAlert::DispatchMessage(BMessage* msg, BHandler* handler)
{
	BWindow::DispatchMessage(msg, handler);
}


void
DAlert::Quit()
{
	BWindow::Quit();
}


bool
DAlert::QuitRequested()
{
	return BWindow::QuitRequested();
}


status_t
DAlert::Perform(perform_code d, void* arg)
{
	return BWindow::Perform(d, arg);
}


void
DAlert::_ReservedAlert1()
{
}


void
DAlert::_ReservedAlert2()
{
}


void
DAlert::_ReservedAlert3()
{
}


void
DAlert::_InitObject(const char* text, const char* button0, const char* button1, const char* button2,
	button_width width, button_spacing spacing, alert_type type)
{
	fInvoker = NULL;
	fAlertSem = -1;
	fAlertVal = -1;
	fButtons[0] = fButtons[1] = fButtons[2] = NULL;
	fTextView = NULL;
	fKeys[0] = fKeys[1] = fKeys[2] = 0;
	fMsgType = type;
	fButtonWidth = width;

	// Set up the "_master_" view
	TAlertView* masterView = new TAlertView(Bounds());
	AddChild(masterView);
	masterView->SetBitmap(_InitIcon());

	// Must have at least one button
	if (button0 == NULL) {
		debugger("DAlerts must have at least one button.");
		button0 = "";
	}

	BMessage protoMsg(kAlertButtonMsg);
	protoMsg.AddInt32("which", 0);
	// Set up the buttons
	int buttonCount = 0;
	fButtons[buttonCount] = new BButton(BRect(0, 0, 0, 0), "_b0_", button0, new BMessage(protoMsg),
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	masterView->AddChild(fButtons[buttonCount]);
	++buttonCount;

	if (button1) {
		protoMsg.ReplaceInt32("which", 1);
		fButtons[buttonCount] = new BButton(BRect(0, 0, 0, 0), "_b1_", button1,
			new BMessage(protoMsg), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);

		masterView->AddChild(fButtons[buttonCount]);
		++buttonCount;
	}
	if (button2) {
		protoMsg.ReplaceInt32("which", 2);
		fButtons[buttonCount] = new BButton(BRect(0, 0, 0, 0), "_b2_", button2,
			new BMessage(protoMsg), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);

		masterView->AddChild(fButtons[buttonCount]);
		++buttonCount;
	}

	SetDefaultButton(fButtons[buttonCount - 1]);

	// Find the widest button only if the widest value needs to be known.
	float maxWidth = 0;
	if (fButtonWidth == B_WIDTH_FROM_WIDEST) {
		for (int i = 0; i < buttonCount; ++i) {
			float temp = width_from_label(fButtons[i]);
			maxWidth = max(maxWidth, temp);
		}
	}

	for (int i = buttonCount - 1; i >= 0; --i) {
		// Determine the button's size
		float buttonWidth = 0, buttonHeight = 0;
		fButtons[i]->GetPreferredSize(&buttonWidth, &buttonHeight);
		if (fButtonWidth == B_WIDTH_FROM_WIDEST)
			buttonWidth = maxWidth;
		else if (fButtonWidth == B_WIDTH_FROM_LABEL)
			buttonWidth = width_from_label(fButtons[i]);
		else // B_WIDTH_AS_USUAL
			buttonWidth = max(buttonWidth, kButtonUsualWidth);
		fButtons[i]->ResizeTo(buttonWidth, buttonHeight);

		// Determine the button's placement
		float buttonX, buttonY;
		buttonY = Bounds().bottom - buttonHeight;
		if (i == buttonCount - 1) {
			// The right-most button
			buttonX = Bounds().right - fButtons[i]->Frame().Width() - kButtonRightOffset;

			// This hack is put in place because placement under Zeta looks goofy
			// because there is no border around default buttons anymore
			if (B_BEOS_VERSION > 0x0510)
				buttonY -= kButtonBottomOffset;
			else
				buttonY -= kDefButtonBottomOffset;
		} else {
			buttonX
				= fButtons[i + 1]->Frame().left - fButtons[i]->Frame().Width() - kButtonSpaceOffset;
			buttonY -= kButtonBottomOffset;
			if (i == 0) {
				if (spacing == B_OFFSET_SPACING) {
					if (buttonCount == 3)
						buttonX -= kButtonOffsetSpaceOffset;
					else {
						// If there are two buttons, the left wall of
						// button0 needs to line up with the left wall
						// of the TextView.
						buttonX = (masterView->Bitmap()) ? kTextIconOffset : kTextLeftOffset;
						if (fButtons[i + 1]->Frame().left - (buttonX + fButtons[i]->Frame().Width())
							< kButtonMinOffsetSpaceOffset) {
							// Recompute buttonX using min offset space
							// if using the current buttonX would not
							// provide enough space or cause an overlap.
							buttonX = fButtons[i + 1]->Frame().left - fButtons[i]->Frame().Width()
								- kButtonMinOffsetSpaceOffset;
						}
					}
				} else if (buttonCount == 3)
					buttonX -= 3;
			}
		}
		fButtons[i]->MoveTo(buttonX, buttonY);
	} // for (int i = buttonCount - 1; i >= 0; --i)

	// Adjust the window's width, if necessary
	float totalWidth = kButtonRightOffset;
	totalWidth += fButtons[buttonCount - 1]->Frame().right - fButtons[0]->Frame().left;
	if (masterView->Bitmap())
		totalWidth += kIconStripeWidth + kWindowIconOffset;
	else
		totalWidth += kWindowMinOffset;

	if (spacing == B_OFFSET_SPACING) {
		totalWidth -= 2;
		if (buttonCount == 3)
			totalWidth = max(kWindowOffsetMinWidth, totalWidth);
		else
			totalWidth = max(kWindowMinWidth, totalWidth);
	} else {
		totalWidth += 5;
		totalWidth = max(kWindowMinWidth, totalWidth);
	}
	ResizeTo(totalWidth, Bounds().Height());

	// Set up the text view
	BRect textViewRect(kTextLeftOffset, kTextTopOffset, Bounds().right - kTextRightOffset,
		Bounds().bottom - kTextBottomOffset);
	if (masterView->Bitmap())
		textViewRect.left = kTextIconOffset;

	fTextView = new BTextView(textViewRect, "_tv_", textViewRect, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW);
	masterView->AddChild(fTextView);

	fTextView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fTextView->MakeEditable(false);
	fTextView->SetStylable(true);
	fTextView->MakeSelectable(true);
	fTextView->SetWordWrap(true);

	// Set up text - boldface the title, skip a line, and then show the message.
	BFont font(be_bold_font);
	font.SetSize(be_plain_font->Size() + 3.0);
	fTextView->SetFontAndColor(&font);
	fTextView->Insert(Title());
	fTextView->Insert("\n\n", 2);
	fTextView->SetFontAndColor(be_plain_font);
	fTextView->Insert(text);

	// Now resize the TextView vertically so that all the text is visible
	float textHeight = fTextView->TextHeight(0, fTextView->CountLines());
	textViewRect.OffsetTo(0, 0);
	textHeight -= textViewRect.Height();
	ResizeBy(0, textHeight);
	fTextView->ResizeBy(0, textHeight);
	textViewRect.bottom += textHeight;
	fTextView->SetTextRect(textViewRect);

	AddCommonFilter(new _DAlertFilter_(this));
}


BBitmap*
DAlert::_InitIcon()
{
	BBitmap* icon = NULL;

	const char* iconName;
	switch (fMsgType) {
		case B_INFO_ALERT:
			icon = BTranslationUtils::GetBitmap('VICN', "dialog-information");
			break;
		case B_IDEA_ALERT:
			icon = BTranslationUtils::GetBitmap('VICN', "dialog-idea");
			break;
		case B_WARNING_ALERT:
			icon = BTranslationUtils::GetBitmap('VICN', "dialog-warning");
			break;
		case B_STOP_ALERT:
			icon = BTranslationUtils::GetBitmap('VICN', "dialog-error");
			break;

		default:
			// Alert type is either invalid or B_EMPTY_ALERT;
			// either way, we're not going to load an icon
			return NULL;
	}

	if (!icon) {
		// If there's no icon, it's an empty alert indeed.
		fMsgType = B_EMPTY_ALERT;
	}

	return icon;
}

//------------------------------------------------------------------------------
//	#pragma mark - TAlertView


TAlertView::TAlertView(BRect frame)
	:
	BView(frame, "TAlertView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	fIconBitmap(NULL)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
}


TAlertView::TAlertView(BMessage* archive)
	:
	BView(archive),
	fIconBitmap(NULL)
{
}


TAlertView::~TAlertView()
{
	delete fIconBitmap;
}


TAlertView*
TAlertView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "TAlertView"))
		return NULL;

	return new TAlertView(archive);
}


status_t
TAlertView::Archive(BMessage* archive, bool deep)
{
	return BView::Archive(archive, deep);
}


void
TAlertView::Draw(BRect updateRect)
{
	// Here's the fun stuff
	if (fIconBitmap) {
		/*		BRect stripeRect = Bounds();
				stripeRect.right = kIconStripeWidth;
		//		SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
				SetHighColor(220,220,220);
				FillRect(stripeRect);
		*/
		SetDrawingMode(B_OP_OVER);
		DrawBitmapAsync(fIconBitmap, BPoint(5, 10));
		SetDrawingMode(B_OP_COPY);
	}
}

//------------------------------------------------------------------------------
//	#pragma mark - _DAlertFilter_


_DAlertFilter_::_DAlertFilter_(DAlert* alert)
	:
	BMessageFilter(B_KEY_DOWN),
	fAlert(alert)
{
}


_DAlertFilter_::~_DAlertFilter_()
{
}


filter_result
_DAlertFilter_::Filter(BMessage* msg, BHandler** target)
{
	if (msg->what == B_KEY_DOWN) {
		char byte;
		if (msg->FindInt8("byte", (int8*)&byte) == B_OK) {
			for (int i = 0; i < 3; ++i) {
				if (byte == fAlert->Shortcut(i) && fAlert->ButtonAt(i)) {
					char space = ' ';
					fAlert->ButtonAt(i)->KeyDown(&space, 1);

					return B_SKIP_MESSAGE;
				}
			}
		}
	}

	return B_DISPATCH_MESSAGE;
}


} // namespace CapitalBe
