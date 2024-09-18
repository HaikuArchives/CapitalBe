#ifndef SPLITVIEWFILTER_H
#define SPLITVIEWFILTER_H

#include "SplitView.h"
#include <MessageFilter.h>
#include <String.h>

class SplitViewFilter : public BMessageFilter {
public:
	SplitViewFilter(SplitView* checkview);
	~SplitViewFilter();

	virtual filter_result Filter(BMessage* msg, BHandler** target);

private:
	SplitView* fView;
	BString fPayeeText;
};

#endif
