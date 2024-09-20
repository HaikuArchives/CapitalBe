/*
 * Copyright 2009. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 */
#ifndef _DSTRINGLIST_H
#define _DSTRINGLIST_H

#include <BeBuild.h>
#include <List.h>
#include <String.h>
#include <SupportDefs.h>

class DStringList {
public:
	DStringList(int32 itemsPerBlock = 20, bool owner = true);
	virtual ~DStringList();

	bool AddItem(const char* string);
	bool AddItem(const char* string, int32 atIndex);
	bool AddList(DStringList* newItems);
	bool AddList(DStringList* newItems, int32 atIndex);
	bool RemoveItem(BString* item);
	BString* RemoveItem(int32 index);
	bool RemoveItems(int32 index, int32 count);
	bool ReplaceItem(int32 index, BString* newItem);
	void MakeEmpty(bool freemem = false);

	//	void	SortItems();
	bool SwapItems(int32 indexA, int32 indexB) { return fList.SwapItems(indexA, indexB); }
	bool MoveItem(int32 fromIndex, int32 toIndex) { return fList.MoveItem(fromIndex, toIndex); }

	BString* FindItem(const char* string) const;
	BString* ItemAt(int32 index) const { return (BString*)fList.ItemAtFast(index); }
	BString* ItemAtFast(int32 index) const { return (BString*)fList.ItemAtFast(index); }
	BString* FirstItem() const { return (BString*)fList.FirstItem(); }
	BString* LastItem() const { return (BString*)fList.LastItem(); }
	BString* Items() const { return (BString*)fList.Items(); }

	bool HasItem(const char* string) const;
	bool HasItem(BString* item) const { return fList.HasItem(item); }

	int32 IndexOf(BString* item) const { return fList.IndexOf(item); }
	int32 CountItems() const { return fList.CountItems(); }
	bool IsEmpty() const { return fList.IsEmpty(); }

	void PrintToStream();

	//	void	DoForEach(bool (*func)(void *));
	//	void	DoForEach(bool (*func)(void *, void *), void *);

private:
	BList fList;
	bool fOwner;
};

#endif // _DSTRINGLIST_H
