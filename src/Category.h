/*
 * Copyright 2009. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 */
#ifndef TYPES_H
#define TYPES_H

#include "Fixed.h"
#include "ObjectList.h"

#include <File.h>
#include <String.h>

class Category {
public:
	Category(const char* string = NULL, const Fixed& fixed = 0, const char* memo = NULL);
	Category(const Category& from);
	~Category();

	void SetNameAt(const int32& index, const char* string);
	void SetAmountAt(const int32& index, const Fixed& fixed);
	void SetMemoAt(const int32& index, const char* string);

	const char* NameAt(const int32& index) const;
	const char* MemoAt(const int32& index) const;
	Fixed AmountAt(const int32& index) const;

	bool AddItem(const char* name, const Fixed& amount, const char* memo = NULL);
	bool HasItem(const char* name);
	bool RemoveItem(const char* name);
	bool RemoveItem(const int32& index);

	int32 CountItems() const { return fList->CountItems(); }

	void Flatten(BFile* file);
	void PrintToStream() const;
	void Sort();

	Category& operator=(const Category& from);
	void MakeEmpty();

private:
	class CatItem {
	public:
		CatItem(const char* str = NULL, const Fixed& fixed = 0, const char* str2 = NULL)
		{
			string = str;
			amount = fixed;
			memo = str2;
		}

		CatItem(const CatItem& from)
		{
			string = from.string;
			amount = from.amount;
			memo = from.memo;
		}

		BString string;
		Fixed amount;
		BString memo;
	};

	static int CompareCatItem(const CatItem* item1, const CatItem* item2);
	BObjectList<Category::CatItem>* fList;
};

#endif //TYPES_H
