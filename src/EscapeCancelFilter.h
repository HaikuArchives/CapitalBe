#ifndef ESCAPE_CANCEL_FILTER_H
#define ESCAPE_CANCEL_FILTER_H

// This is for any window which doesn't already have an AutoTextControl in it.

#include <Handler.h>
#include <MessageFilter.h>

class EscapeCancelFilter : public BMessageFilter {
  public:
	EscapeCancelFilter(void) : BMessageFilter(B_PROGRAMMED_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN) {}

	~EscapeCancelFilter(void) {}

	filter_result Filter(BMessage *msg, BHandler **target) {
		int32 rawchar;
		msg->FindInt32("raw_char", &rawchar);

		if (rawchar == B_ESCAPE) {
			BLooper *loop = (*target)->Looper();
			if (loop) {
				BMessenger msgr(loop);
				msgr.SendMessage(B_QUIT_REQUESTED);
				return B_SKIP_MESSAGE;
			}
		}
		return B_DISPATCH_MESSAGE;
	}
};


#endif
