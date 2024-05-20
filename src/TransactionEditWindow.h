#ifndef TRANSEDITWIN_H
#define TRANSEDITWIN_H

#include <Message.h>
#include <Window.h>
#include "SplitView.h"

class TransactionData;

class TransactionEditWindow : public BWindow {
public:
	TransactionEditWindow(const TransactionData& trans);
	void MessageReceived(BMessage* msg);

private:
	SplitView* fSplitView;
};

#endif
