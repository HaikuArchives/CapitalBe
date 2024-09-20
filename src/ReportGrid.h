/*
 * Copyright 2009. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 */
#ifndef GRID_H
#define GRID_H

#include "Fixed.h"
#include "ObjectList.h"
#include <List.h>
#include <String.h>

class ReportGrid {
public:
	ReportGrid(const uint32& width, const uint32& height);
	~ReportGrid();

	const char* RowTitle(const int32& index);
	void SetRowTitle(const int32& index, const char* title);
	int32 FindTitle(const char* title);

	status_t ValueAt(const uint32& x, const uint32& y, Fixed& fixed);
	status_t SetValue(const uint32& x, const uint32& y, const Fixed& fixed);

	void AddItem(const int32& index = -1);
	void RemoveItem(const int32& index);
	int32 CountItems() const { return fList.CountItems(); }

	void MakeEmpty();
	void Sort();

	void PrintToStream();

	uint32 Width() const { return fWidth; }
	uint32 Height() const { return (uint32)fList.CountItems(); }

private:
	class GridRecord {
	public:
		GridRecord(const uint32& size, const char* title = NULL);
		~GridRecord();

		const char* Title() const { return fTitle.String(); }

		void SetTitle(const char* title) { fTitle = title; }

		status_t ValueAt(const uint32& index, Fixed& fixed);
		status_t SetValue(const uint32& index, const Fixed& fixed);

	private:
		BObjectList<Fixed> fList;
		BString fTitle;
	};

	static int CompareGridrecord(const GridRecord* item1, const GridRecord* item2);

	BObjectList<GridRecord> fList;
	uint32 fWidth;
};

#endif // GRID_H
