#ifndef SPLIT_ITEM_H
#define SPLIT_ITEM_H

#include <ListItem.h>
#include <String.h>
#include "Fixed.h"

class SplitItem : public BStringItem
{
public:
	SplitItem(void);
	~SplitItem(void);
//	void DrawItem(BView *owner, BRect frame, bool complete = false);
	
	void SetCategory(const char *value);
	const char *GetCategory(void) const;
	
	void SetAmount(const Fixed &fixed);
	Fixed GetAmount(void) const;
	
	void SetMemo(const char *value);
	const char *GetMemo(void) const;
	
private:
	void UpdateLabel(void);
	BString fName;
	Fixed fAmount;
	BString fMemo;
};

#endif
