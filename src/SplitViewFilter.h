#ifndef SPLITVIEWFILTER_H
#define SPLITVIEWFILTER_H

#include <MessageFilter.h>
#include <String.h>
#include "SplitView.h"

class SplitViewFilter : public BMessageFilter {
public:
	SplitViewFilter(SplitView* checkview);
	~SplitViewFilter(void);
	virtual filter_result Filter(BMessage* msg, BHandler** target);

private:
	SplitView* fView;
	BString fPayeeText;
};

#endif
