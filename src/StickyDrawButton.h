#ifndef _STICKY_DRAWBUTTON_H
#define _STICKY_DRAWBUTTON_H

#include <Bitmap.h>
#include <Button.h>
#include <View.h>
#include <Window.h>

class StickyDrawButton : public BButton {
  public:
	StickyDrawButton(
		BRect frame, const char *name, BBitmap *up, BBitmap *down, BMessage *msg,
		const int32 &resize, const int32 &flags
	);
	~StickyDrawButton(void);

	void Draw(BRect update);

	void SetBitmaps(BBitmap *up, BBitmap *down);
	void ResizeToPreferred(void);
	void SetDisabled(BBitmap *disabledup, BBitmap *disableddown);
	void MouseUp(BPoint pt);

	//	void	MessageReceived(BMessage *msg);

	int32 GetState(void) { return fButtonState ? 1 : 0; };

	void SetState(int32 value);

  private:
	BBitmap *fUp, *fDown, *fDisabledUp, *fDisabledDown;

	// true if in state two
	int32 fButtonState;
};

#endif
