#ifndef TRANSEDITWIN_H
#define TRANSEDITWIN_H

#include <Window.h>
#include <Message.h>
#include "SplitView.h"

class TransactionData;

class TransactionEditWindow : public BWindow
{
public:
	TransactionEditWindow(const BRect &frame, const TransactionData &trans);
	void MessageReceived(BMessage *msg);

private:
	SplitView *fSplitView;
};

#endif
