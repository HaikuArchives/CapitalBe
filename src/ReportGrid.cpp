#include "ReportGrid.h"
#include <stdio.h>


ReportGrid::ReportGrid(const uint32& width, const uint32& height)
	:
	fList(20, true),
	fWidth(width)
{
	for (uint32 i = 0; i < height; i++)
		AddItem();
}


ReportGrid::~ReportGrid()
{
}


const char*
ReportGrid::RowTitle(const int32& index)
{
	ReportGrid::GridRecord* record = fList.ItemAt(index);

	return (record) ? record->Title() : NULL;
}


void
ReportGrid::SetRowTitle(const int32& index, const char* title)
{
	ReportGrid::GridRecord* record = (ReportGrid::GridRecord*)fList.ItemAt(index);

	if (record)
		record->SetTitle(title);
}


int32
ReportGrid::FindTitle(const char* title)
{
	if (!title || fList.CountItems() == 0)
		return -1;

	for (int32 i = 0; i < fList.CountItems(); i++) {
		ReportGrid::GridRecord* record = fList.ItemAt(i);
		if (strcmp(record->Title(), title) == 0)
			return i;
	}
	return -1;
}


status_t
ReportGrid::ValueAt(const uint32& x, const uint32& y, Fixed& fixed)
{
	if (y >= (uint32)fList.CountItems() || x >= fWidth)
		return B_ERROR;

	ReportGrid::GridRecord* record = fList.ItemAt(y);

	if (!record)
		return B_ERROR;

	return record->ValueAt(x, fixed);
}


status_t
ReportGrid::SetValue(const uint32& x, const uint32& y, const Fixed& fixed)
{
	if (y >= (uint32)fList.CountItems() || x >= fWidth)
		return B_ERROR;

	ReportGrid::GridRecord* record = (ReportGrid::GridRecord*)fList.ItemAt(y);

	if (!record)
		return B_ERROR;

	return record->SetValue(x, fixed);
}


int
ReportGrid::CompareGridrecord(const ReportGrid::GridRecord* item1,
	const ReportGrid::GridRecord* item2)
{
	//	ReportGrid::GridRecord *listitem1 = *((ReportGrid::GridRecord**)item1);
	//	ReportGrid::GridRecord *listitem2 = *((ReportGrid::GridRecord**)item2);

	//	return strcmp(listitem1->Title(),listitem2->Title());
	return strcmp(item1->Title(), item2->Title());
}


void
ReportGrid::AddItem(const int32& index)
{
	if (index < 0)
		fList.AddItem(new ReportGrid::GridRecord(fWidth));
	else
		fList.AddItem(new ReportGrid::GridRecord(fWidth), index);
}


void
ReportGrid::RemoveItem(const int32& index)
{
	ReportGrid::GridRecord* item = fList.RemoveItemAt(index);
	if (item)
		delete item;
}


void
ReportGrid::MakeEmpty()
{
	fList.MakeEmpty();
}


void
ReportGrid::Sort()
{
	fList.SortItems(CompareGridrecord);
}


void
ReportGrid::PrintToStream()
{
	if (CountItems() == 0)
		printf("Grid is empty\n");
	else {
		for (int32 i = 0; i < CountItems(); i++) {
			ReportGrid::GridRecord* item = fList.ItemAt(i);

			printf("Row %ld: %s ", i, item->Title());
			for (uint32 j = 0; j < fWidth; j++) {
				Fixed f;
				item->ValueAt(j, f);
				printf("%.2f ", f.AsFloat());
			}
			printf("\n");
		}
	}
	printf("\n");
}

//---------------------------------------------------------------------------------


ReportGrid::GridRecord::GridRecord(const uint32& size, const char* title)
	:
	fList(size),
	fTitle(title)
{
	for (uint32 i = 0; i < size; i++)
		fList.AddItem(new Fixed());
}


ReportGrid::GridRecord::~GridRecord()
{
	for (int32 i = 0; i < fList.CountItems(); i++) {
		Fixed* item = (Fixed*)fList.ItemAt(i);
		delete item;
	}
}


status_t
ReportGrid::GridRecord::ValueAt(const uint32& index, Fixed& fixed)
{
	Fixed* item = (Fixed*)fList.ItemAt(index);

	if (!item)
		return B_ERROR;

	fixed = *item;
	return B_OK;
}


status_t
ReportGrid::GridRecord::SetValue(const uint32& index, const Fixed& fixed)
{
	Fixed* item = (Fixed*)fList.ItemAt(index);

	if (!item)
		return B_ERROR;

	*item = fixed;
	return B_OK;
}
