#ifndef HELPBUTTON_H
#define HELPBUTTON_H

#include <Button.h>
#include <Entry.h>
#include <Window.h>

#define M_HELPBUTTON_PRESSED 'hbpr'

class HelpButton : public BButton {
public:
	HelpButton(const char* title, const char* helpfilename);
	~HelpButton(void);

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* msg);

private:
	entry_ref GetHelpFile(const char* helpfilename);
	entry_ref fRef;
	const char* fWindowTitle;
};

#endif
