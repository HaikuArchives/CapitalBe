#ifndef HELP_H
#define HELP_H

#include <Button.h>

#define M_HELP 'help'


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

void openDocumentation(const char* helpfilename, const char* anchor);
BBitmap* getHelpIcon(void);

#endif
