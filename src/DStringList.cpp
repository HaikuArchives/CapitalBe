#include "DStringList.h"
#include <stdio.h>


DStringList::DStringList(int32 itemsPerBlock, bool owner)
	:
	fList(itemsPerBlock),
	fOwner(owner)
{
}


DStringList::~DStringList()
{
	MakeEmpty(fOwner);
}


bool
DStringList::AddItem(const char* string)
{
	if (!string)
		return false;

	return fList.AddItem(new BString(string));
}


bool
DStringList::AddItem(const char* string, int32 atIndex)
{
	if (!string)
		return false;

	return fList.AddItem(new BString(string), atIndex);
}


bool
DStringList::AddList(DStringList* newItems)
{
	if (!newItems)
		return false;

	for (int32 i = 0; i < newItems->CountItems(); i++)
		fList.AddItem(newItems->ItemAtFast(i));
	return true;
}


bool
DStringList::AddList(DStringList* newItems, int32 atIndex)
{
	if (!newItems)
		return false;

	for (int32 i = 0; i < newItems->CountItems(); i++)
		fList.AddItem(newItems->ItemAtFast(i), atIndex);
	return true;
}


bool
DStringList::RemoveItem(BString* item)
{
	return fList.RemoveItem(item);
}


BString*
DStringList::RemoveItem(int32 index)
{
	return (BString*)fList.RemoveItem(index);
}


bool
DStringList::RemoveItems(int32 index, int32 count)
{
	return (BString*)fList.RemoveItems(index, count);
}


bool
DStringList::ReplaceItem(int32 index, BString* newItem)
{
	return fList.ReplaceItem(index, newItem);
}


bool
DStringList::HasItem(const char* string) const
{
	if (!string)
		return false;

	for (int32 i = 0; i < CountItems(); i++) {
		BString* item = ItemAt(i);
		if (item->Compare(string) == 0)
			return true;
	}
	return false;
}


BString*
DStringList::FindItem(const char* string) const
{
	if (!string)
		return NULL;

	for (int32 i = 0; i < CountItems(); i++) {
		BString* item = ItemAt(i);
		if (item->Compare(string) == 0)
			return item;
	}
	return NULL;
}


void
DStringList::MakeEmpty(bool freemem)
{
	if (freemem) {
		BString* str;
		for (int32 i = 0; i < fList.CountItems(); i++) {
			str = (BString*)fList.ItemAtFast(i);
			if (str)
				delete str;
		}
	}
	fList.MakeEmpty();
}


void
DStringList::PrintToStream()
{
	BString* str;

	printf("DStringList: %ld items\n", fList.CountItems());

	for (int32 i = 0; i < fList.CountItems(); i++) {
		str = ItemAt(i);
		printf("%ld: %s\n", i, (str) ? str->String() : NULL);
	}
}
