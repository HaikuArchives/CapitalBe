#ifndef HELPBUTTON_H
#define HELPBUTTON_H

#include <Button.h>
#include <Entry.h>
#include <Window.h>

#define M_HELPBUTTON_PRESSED 'hbpr'

class HelpButton : public BButton
{
public:
	HelpButton(const char *name, const char *path);
	~HelpButton(void);

	void MessageReceived(BMessage *msg);
private:
	entry_ref fRef;
};

#endif
