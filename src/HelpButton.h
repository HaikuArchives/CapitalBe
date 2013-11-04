#ifndef HELPBUTTON_H
#define HELPBUTTON_H

#include "DrawButton.h"
#include <Entry.h>
#include <Window.h>

#define M_HELPBUTTON_PRESSED 'hbpr'

class HelpButton : public DrawButton
{
public:
	HelpButton(const BPoint &location, const char *name, const entry_ref &ref,
				const int32 &resize = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				const int32 &flags = B_WILL_DRAW);
	HelpButton(const BPoint &location, const char *name, const char *path,
				const int32 &resize = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				const int32 &flags = B_WILL_DRAW);
	~HelpButton(void);
	void AttachedToWindow(void);
	void MessageReceived(BMessage *msg);

private:
	entry_ref fRef;
};

#endif
