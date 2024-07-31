#ifndef HELPBUTTON_H
#define HELPBUTTON_H

#include <Button.h>

#define M_HELPBUTTON_PRESSED 'hbpr'

class HelpButton : public BButton {
public:
	HelpButton(const char* helpfilename, const char* anchor);
	~HelpButton(void);

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* msg);

private:
	// entry_ref fRef;
	BString fHelpfile;
	BString fAnchor;
};

#endif
