#ifndef APP_H
#define APP_H

#include <Application.h>
#include <Message.h>

#define M_QUIT_NOW 'qnow'

class App : public BApplication
{
public:
	App(void);
	~App(void);
	void MessageReceived(BMessage *msg);
};

extern bool gRestartApp;

#endif
