/*
 * Copyright 2009. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 */
#ifndef SPLITVIEWFILTER_H
#define SPLITVIEWFILTER_H

#include <MessageFilter.h>
#include <String.h>
#include "SplitView.h"

class SplitViewFilter : public BMessageFilter {
public:
	SplitViewFilter(SplitView* checkview);
	~SplitViewFilter();

	virtual filter_result Filter(BMessage* msg, BHandler** target);

private:
	SplitView* fView;
	BString fPayeeText;
};

#endif	// SPLITVIEWFILTER_H
