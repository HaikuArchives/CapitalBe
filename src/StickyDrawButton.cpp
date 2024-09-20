/*
 * Copyright 2009. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 */
#include "StickyDrawButton.h"


StickyDrawButton::StickyDrawButton(BRect frame, const char* name, BBitmap* up, BBitmap* down,
	BMessage* msg, const int32& resize, const int32& flags)
	:
	BButton(frame, name, "", msg, resize, flags),
	fUp(up),
	fDown(down),
	fDisabledUp(NULL),
	fDisabledDown(NULL),
	fButtonState(B_CONTROL_OFF)
{
}


StickyDrawButton::~StickyDrawButton()
{
	delete fUp;
	delete fDown;
	delete fDisabledUp;
	delete fDisabledDown;
}


void
StickyDrawButton::ResizeToPreferred()
{
	if (fUp)
		ResizeTo(fUp->Bounds().Width(), fUp->Bounds().Height());
	else if (fDown)
		ResizeTo(fDown->Bounds().Width(), fDown->Bounds().Height());
	else if (fDisabledUp)
		ResizeTo(fDisabledUp->Bounds().Width(), fDisabledUp->Bounds().Height());
	else if (fDisabledDown)
		ResizeTo(fDisabledDown->Bounds().Width(), fDisabledDown->Bounds().Height());
}


void
StickyDrawButton::SetBitmaps(BBitmap* up, BBitmap* down)
{
	delete fUp;
	delete fDown;

	fUp = up;
	fDown = down;
}


void
StickyDrawButton::SetDisabled(BBitmap* disabledup, BBitmap* disableddown)
{
	delete fDisabledUp;
	delete fDisabledDown;

	fDisabledUp = disabledup;
	fDisabledDown = disableddown;
}


void
StickyDrawButton::MouseUp(BPoint pt)
{
	BButton::MouseUp(pt);
	fButtonState = (fButtonState == B_CONTROL_ON) ? B_CONTROL_OFF : B_CONTROL_ON;
	Invalidate();
}


void
StickyDrawButton::SetState(int32 value)
{
	if (fButtonState != value) {
		if (value == B_CONTROL_ON)
			fButtonState = B_CONTROL_OFF;
		else
			fButtonState = B_CONTROL_ON;
		Invalidate();
	}
}


void
StickyDrawButton::Draw(BRect update)
{
	// if down
	if (fButtonState == B_CONTROL_ON) {
		if (!IsEnabled()) {
			if (fDisabledDown)
				DrawBitmap(fDisabledDown, BPoint(0, 0));
			else
				StrokeRect(Bounds());
			return;
		} else {
			if (fDown)
				DrawBitmap(fDown, BPoint(0, 0));
			else
				StrokeRect(Bounds());
			return;
		}
	} else if (!IsEnabled()) {
		if (fDisabledUp)
			DrawBitmap(fDisabledUp, BPoint(0, 0));
		else
			StrokeRect(Bounds());
		return;
	} else {
		if (fUp)
			DrawBitmap(fUp, BPoint(0, 0));
		else
			StrokeRect(Bounds());
	}
}
