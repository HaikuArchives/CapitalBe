#include "Category.h"
#include "CBLocale.h"
#include "Database.h"
#include <File.h>
#include <stdio.h>
#include <stdlib.h>


Category::Category(const char* string, const Fixed& fixed, const char* memo)
{
	fList = new BObjectList<Category::CatItem>(20, true);
	if (string)
		fList->AddItem(new CatItem(string, fixed, memo));
}


Category::Category(const Category& from)
{
	fList = new BObjectList<Category::CatItem>(20, true);
	for (int32 i = 0; i < from.fList->CountItems(); i++) {
		CatItem* item = from.fList->ItemAt(i);
		if (item)
			fList->AddItem(new CatItem(*item));
	}
}


Category::~Category()
{
	delete fList;
}


void
Category::SetNameAt(const int32& index, const char* string)
{
	CatItem* item = fList->ItemAt(index);

	if (item)
		item->string = string;
}


void
Category::SetAmountAt(const int32& index, const Fixed& fixed)
{
	CatItem* item = fList->ItemAt(index);

	if (item)
		item->amount = fixed;
}


void
Category::SetMemoAt(const int32& index, const char* string)
{
	CatItem* item = fList->ItemAt(index);

	if (item)
		item->memo = string;
}


const char*
Category::NameAt(const int32& index) const
{
	CatItem* item = fList->ItemAt(index);

	return item ? item->string.String() : NULL;
}


Fixed
Category::AmountAt(const int32& index) const
{
	CatItem* item = fList->ItemAt(index);

	return item ? item->amount : Fixed(0);
}


const char*
Category::MemoAt(const int32& index) const
{
	CatItem* item = fList->ItemAt(index);

	return item ? item->memo.String() : NULL;
}


bool
Category::AddItem(const char* name, const Fixed& amount, const char* memo)
{
	if (!name)
		return false;

	CatItem* item = fList->ItemAt(0);
	if (item) {
		if (item->string.CountChars() == 0)
			fList->RemoveItemAt(0L);
	}
	fList->AddItem(new CatItem(name, amount, memo));

	return true;
}


bool
Category::HasItem(const char* name)
{
	// Prevent duplicates
	for (int32 i = 0; i < fList->CountItems(); i++) {
		CatItem* data = fList->ItemAt(i);
		if (data->string == name)
			return true;
	}
	return false;
}


bool
Category::RemoveItem(const char* name)
{
	if (!name)
		return false;

	// Find item and remove
	CatItem* item = NULL;
	for (int32 i = 0; i < fList->CountItems(); i++) {
		CatItem* data = fList->ItemAt(i);
		if (data->string == name) {
			item = data;
			break;
		}
	}

	// owns item, so it does the delete
	fList->RemoveItem(item);
	return true;
}


bool
Category::RemoveItem(const int32& index)
{
	CatItem* item = fList->RemoveItemAt(index);
	if (!item)
		return false;

	delete item;
	return true;
}


void
Category::Flatten(BFile* file)
{
	BString str("\t\t<Category>\n");

	for (int32 i = 0; i < fList->CountItems(); i++) {
		CatItem* data = fList->ItemAt(i);
		BString temp(data->string.String());
		IllegalCharsToEntities(&temp);
		str << "\t\t\t<CategoryEntry Name=\"" << temp << "\" Amount=\"" << data->amount.AsFixed()
			<< "\" />\n";
	}

	str << "\t\t</Category>\n";
	file->Write(str.String(), str.Length());
}


void
Category::PrintToStream() const
{
	BString str("<CAT>  ");

	for (int32 i = 0; i < fList->CountItems(); i++) {
		CatItem* data = fList->ItemAt(i);
		str << data->string.String() << "  " << data->amount.AsFixed();
		if (i == fList->CountItems() - 1)
			str << "  ";
		else
			str << "|";
	}
	str << "</CAT>";
	printf("%s", str.String());
}


Category&
Category::operator=(const Category& from)
{
	fList->MakeEmpty();

	for (int32 i = 0; i < from.CountItems(); i++) {
		CatItem* item = from.fList->ItemAt(i);
		fList->AddItem(new CatItem(item->string.String(), item->amount, item->memo.String()));
	}
	return *this;
}


void
Category::MakeEmpty()
{
	fList->MakeEmpty();
}


void
Category::Sort()
{
	fList->SortItems(CompareCatItem);
}


int
Category::CompareCatItem(const CatItem* item1, const CatItem* item2)
{
	if (!item1) {
		if (item2)
			return 1;
		else
			return 0;
	} else if (!item2) {
		if (item1)
			return -1;
		else
			return 0;
	}

	return item1->string.Compare(item2->string);
}
