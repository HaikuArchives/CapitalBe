#include "CategoryWindow.h"

#include <Button.h>
#include <CheckBox.h>
#include <OutlineListView.h>
#include <LayoutBuilder.h>
#include <ListItem.h>
#include <Message.h>
#include <Messenger.h>
#include <Region.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextView.h>
#include <View.h>

#include "AutoTextControl.h"
#include "Database.h"
#include "EscapeCancelFilter.h"
#include "Preferences.h"
#include "Translate.h"

enum
{
	M_SHOW_ADD_WINDOW='shaw',
	M_SHOW_REMOVE_WINDOW,
	M_SHOW_EDIT_WINDOW,
	M_EDIT_CATEGORY,
	M_ADD_CATEGORY,
	M_REMOVE_CATEGORY,
	M_NAME_CHANGED
};

class CategoryItem : public BStringItem
{
public:
	CategoryItem(const BString &name);
	void DrawItem(BView *owner, BRect frame, bool complete = false);
};

class CategoryView : public BView
{
public:
	CategoryView(const char *name, const int32 &flags);
	void AttachedToWindow(void);
	void MessageReceived(BMessage *msg);

private:
	float RefreshCategoryList(void);
	
	BOutlineListView	*fListView;
	
	BButton		*fAddButton,
				*fRemoveButton,
				*fEditButton;
	
	CategoryItem *fIncomeItem,
				*fSpendingItem;
	float		fBestWidth;
};

class CategoryInputWindow : public BWindow
{
public:
	CategoryInputWindow(const BRect &frame, BView *target);
	void MessageReceived(BMessage *msg);

private:
	AutoTextControl	*fNameBox;
	
	BButton		*fOKButton,
				*fCancelButton;
	
	BCheckBox	*fExpenseBox;
	BView	 	*fTarget;
};

class CategoryEditWindow : public BWindow
{
public:
	CategoryEditWindow(const BRect &frame, const char *oldname, BView *target);
	void MessageReceived(BMessage *msg);

private:
	AutoTextControl	*fNameBox;
	
	BButton		*fOKButton,
				*fCancelButton;
	
	BString 	fOldName;
	
	BView	 	*fTarget;
};

class CategoryRemoveWindow : public BWindow
{
public:
	CategoryRemoveWindow(const BRect &frame, const char *from, BView *target);
	void MessageReceived(BMessage *msg);
	void FrameResized(float w, float h);
private:
	
	BOutlineListView	*fListView;
	
	CategoryItem	*fIncomeItem,
					*fSpendingItem;
	
	BTextView	*fDirections;
	
	BButton		*fOKButton,
				*fCancelButton;
	BView	 	*fTarget;
	BScrollView *fScrollView;
};

CategoryView::CategoryView(const char *name, const int32 &flags)
 :	BView(name,flags)
{
	BString temp;
	SetViewColor(240,240,240);
	
	// the buttons
	temp = TRANSLATE("Edit"); temp+="…";
	fEditButton = new BButton("editbutton",temp.String(), new BMessage(M_SHOW_EDIT_WINDOW));
	
	temp = TRANSLATE("Remove"); temp+="…";
	fRemoveButton = new BButton("removebutton",temp.String(), new BMessage(M_SHOW_REMOVE_WINDOW));
		
	temp = TRANSLATE("Add"); temp+="…";
	fAddButton = new BButton("addbutton",temp.String(), new BMessage(M_SHOW_ADD_WINDOW));
		
	// the category list
	fListView = new BOutlineListView("categorylist",B_SINGLE_SELECTION_LIST,
									B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE);
	BScrollView *sv = new BScrollView("scrollview",fListView,0,false,true);
	
	fIncomeItem = new CategoryItem(TRANSLATE("Income"));
	fSpendingItem = new CategoryItem(TRANSLATE("Spending"));

	RefreshCategoryList();
	
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(15, 15)
		.AddGrid(1.0f, 1.0f)
			.Add(sv, 0, 0, 4)
			.AddGrid(1.0f, 1.0f, 0, 1, 4)
				.Add(fEditButton, 0, 0)
				.Add(fRemoveButton, 1, 0)
				.Add(fAddButton, 2, 0)
				.AddGlue(3, 0)					
			.End()
		.End()
	.End();	
}

void CategoryView::AttachedToWindow(void)
{
	fListView->SetTarget(this);
	fEditButton->SetTarget(this);
	fAddButton->SetTarget(this);
	fRemoveButton->SetTarget(this);
	
	fListView->Select(0);
	
	fListView->MakeFocus(true);
}

void CategoryView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_SHOW_ADD_WINDOW:
		{
			BRect r(Window()->Frame());
			CategoryInputWindow *catwin = new CategoryInputWindow(BRect(100,100,300,225),this);
			catwin->MoveTo(r.left + ((Bounds().Width()-catwin->Bounds().Width())/2),
							r.top + ((Bounds().Height()-catwin->Bounds().Height())/2) );
			catwin->Show();
			break;
		}
		case M_SHOW_REMOVE_WINDOW:
		{
			int32 index = fListView->CurrentSelection();
			CategoryItem *item = (CategoryItem*)fListView->ItemAt(index);
			
			if(!item || item==fIncomeItem || item==fSpendingItem)
				break;
			
			BRect r(Window()->Frame());
			CategoryRemoveWindow *catwin = new CategoryRemoveWindow(r,item->Text(),this);
			catwin->MoveTo(r.left + ((Bounds().Width()-catwin->Bounds().Width())/2),
							r.top + ((Bounds().Height()-catwin->Bounds().Height())/2) );
			catwin->Show();
			break;
		}
		case M_SHOW_EDIT_WINDOW:
		{
			int32 index = fListView->CurrentSelection();
			CategoryItem *item = (CategoryItem*)fListView->ItemAt(index);
			
			if(!item || item==fIncomeItem || item==fSpendingItem)
				break;
			
			BRect r(Window()->Frame());
			CategoryEditWindow *catwin = new CategoryEditWindow(r,item->Text(),this);
			catwin->MoveTo(r.left + ((Bounds().Width()-catwin->Bounds().Width())/2),
							r.top + ((Bounds().Height()-catwin->Bounds().Height())/2) );
			catwin->Show();
			break;
		}
		case M_ADD_CATEGORY:
		{
			BString name;
			bool expense;
			if(msg->FindString("name",&name)!=B_OK ||
					msg->FindBool("expense",&expense)!=B_OK)
				break;
			
			if(name.ICompare(TRANSLATE("Income"))==0 || name.ICompare(TRANSLATE("Spending"))==0 ||
				name.ICompare(TRANSLATE("Split"))==0)
			{
				ShowAlert(TRANSLATE("Can't use this category name"),
							TRANSLATE("Capital Be uses the words 'Income','Spending', and 'Split' "
							"for managing categories, so you can't use them as category names. "
							"Please choose a different name for your new category."));
				break;
			}
			
			gDatabase.AddCategory(name.String(),expense);
			RefreshCategoryList();
			break;
		}
		case M_REMOVE_CATEGORY:
		{
			BString newcat;
			if(msg->FindString("newcat",&newcat)!=B_OK)
				break;
			
			int32 index = fListView->CurrentSelection();
			CategoryItem *item = (CategoryItem*)fListView->ItemAt(index);
			
			if(!item || item==fIncomeItem || item==fSpendingItem)
				break;
			
			gDatabase.RecategorizeTransactions(item->Text(), newcat.String());
			gDatabase.RemoveCategory(item->Text());
			RefreshCategoryList();
			
			break;
		}
		case M_EDIT_CATEGORY:
		{
			
			BString newname;
			if(msg->FindString("newname",&newname)!=B_OK)
				break;
			
			int32 index = fListView->CurrentSelection();
			CategoryItem *item = (CategoryItem*)fListView->ItemAt(index);
			
			if(!item || item==fIncomeItem || item==fSpendingItem)
				break;
			
			gDatabase.RecategorizeTransactions(item->Text(), newname.String());
			gDatabase.RemoveCategory(item->Text());
			RefreshCategoryList();
			break;
		}
		default:
		{
			BView::MessageReceived(msg);
			break;
		}
	}
}

float CategoryView::RefreshCategoryList(void)
{
	if(fListView->CountItems()>0)
	{
		if(fListView->HasItem(fIncomeItem))
			fListView->RemoveItem(fIncomeItem);
		if(fListView->HasItem(fSpendingItem))
			fListView->RemoveItem(fSpendingItem);
		
		fListView->MakeEmpty();
	}
	fListView->AddItem(fIncomeItem);
	fListView->AddItem(fSpendingItem);
	
	int32 maxchars;
	float maxlength;
	if(strlen(TRANSLATE("Income"))>strlen(TRANSLATE("Spending")))
	{
		maxchars = strlen(TRANSLATE("Income"));
		maxlength = StringWidth(TRANSLATE("Income"));
	}
	else
	{
		maxchars = strlen(TRANSLATE("Spending"));
		maxlength = StringWidth(TRANSLATE("Spending"));
	}
	
	CppSQLite3Query query = gDatabase.DBQuery("select * from categorylist order by name desc",
											"CategoryView::CategoryView");
	while(!query.eof())
	{
		int expense = query.getIntField(1);
		BString name = query.getStringField(0);
		
		if(expense==0)
			fListView->AddUnder(new CategoryItem(DeescapeIllegalCharacters(name.String())),
									fSpendingItem);
		else
			fListView->AddUnder(new CategoryItem(DeescapeIllegalCharacters(name.String())),
									fIncomeItem);
		
		if(name.CountChars()>maxchars)
		{
			maxchars = name.CountChars();
			maxlength = StringWidth(name.String());
		}
		
		query.nextRow();
	}
	
	return maxlength;
}

CategoryWindow::CategoryWindow(const BRect &frame)
 :	BWindow(frame,TRANSLATE("Categories"),B_DOCUMENT_WINDOW_LOOK,B_NORMAL_WINDOW_FEEL,
 			B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
{
	AddCommonFilter(new EscapeCancelFilter);
	
	CategoryView *view = new CategoryView("categoryview" ,B_WILL_DRAW);
		
	AddShortcut('A',B_COMMAND_KEY, new BMessage(M_SHOW_ADD_WINDOW),view);
	AddShortcut('R',B_COMMAND_KEY, new BMessage(M_REMOVE_CATEGORY),view);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(0)
		.Add(view)
	.End();
}

CategoryItem::CategoryItem(const BString &name)
 :	BStringItem(name.String())
{
}

void CategoryItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	if(IsSelected())
	{
		owner->SetHighColor(GetColor(BC_SELECTION_FOCUS));
		owner->SetLowColor(GetColor(BC_SELECTION_FOCUS));
	}
	else
	{
		owner->SetHighColor(255, 255, 255);
		owner->SetLowColor(255, 255, 255);
	}
	owner->FillRect(frame);
	if(IsSelected())
	{
		owner->SetHighColor(100, 100, 100);
		owner->StrokeRect(frame);
	}
	
	owner->SetHighColor(0,0,0);
	
	BRegion region(frame);
	owner->ConstrainClippingRegion(&region);
	owner->DrawString(Text(),BPoint(frame.left+1,frame.bottom-2));
	owner->ConstrainClippingRegion(NULL);
}

CategoryInputWindow::CategoryInputWindow(const BRect &frame, BView *target)
 :	BWindow(frame,TRANSLATE("Add Category"),B_FLOATING_WINDOW_LOOK,B_MODAL_APP_WINDOW_FEEL,
 			B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE |
 			B_NOT_V_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS),
 	fTarget(target)
{
	BString temp;
	AddCommonFilter(new EscapeCancelFilter);
	AddShortcut('W',B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	
	BView *view = new BView("background",B_WILL_DRAW);
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(0)
		.Add(view)
	.End();
	view->SetViewColor(240,240,240);
	
	temp = TRANSLATE("Category Name"); temp+=":";
	fNameBox = new AutoTextControl("namebox",temp.String(),
									"",new BMessage(M_NAME_CHANGED));
	fNameBox->SetCharacterLimit(32);
	
	fExpenseBox = new BCheckBox("expensebox",TRANSLATE("Spending Category"),NULL);
	fExpenseBox->SetValue(B_CONTROL_ON);
	
	fOKButton = new BButton("okbutton",TRANSLATE("Cancel"),
						new BMessage(M_ADD_CATEGORY));
		fOKButton->SetLabel(TRANSLATE("OK"));	
	fOKButton->MakeDefault(true);

	fCancelButton = new BButton("cancelbutton",TRANSLATE("Cancel"),
						new BMessage(B_QUIT_REQUESTED));

	fNameBox->MakeFocus(true);
	
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(10)
		.Add(fNameBox)
		.Add(fExpenseBox)
		.AddGrid(1.0f, 1.0f)
			.Add(fCancelButton, 0, 0)
			.Add(fOKButton, 1, 0)
		.End()
	.End();
}

void CategoryInputWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_NAME_CHANGED:
		{
			if(strlen(fNameBox->Text())>0)
			{
				if(!fOKButton->IsEnabled())
					fOKButton->SetEnabled(true);
			}
			else
			{
				if(fOKButton->IsEnabled())
					fOKButton->SetEnabled(false);
			}
			break;
		}
		case M_ADD_CATEGORY:
		{
			BMessenger msgr(fTarget);
			msg->AddString("name",fNameBox->Text());
			msg->AddBool("expense",fExpenseBox->Value() == B_CONTROL_ON);
			msgr.SendMessage(msg);
			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}

CategoryRemoveWindow::CategoryRemoveWindow(const BRect &frame, const char *from, BView *target)
 :	BWindow(frame,TRANSLATE("Remove Category"),B_FLOATING_WINDOW_LOOK,B_MODAL_APP_WINDOW_FEEL,
 			B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_AUTO_UPDATE_SIZE_LIMITS),
 	fTarget(target)
{
	rgb_color backcolor = {240,240,240,255};
	
	AddCommonFilter(new EscapeCancelFilter);
	AddShortcut('W',B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	BView *view = new BView("background",B_WILL_DRAW | B_FRAME_EVENTS);
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(0)
		.Add(view)
	.End();
	view->SetViewColor(backcolor);
			
	fDirections = new BTextView("directions");
	fDirections->MakeEditable(false);
	
	BString directions(
		TRANSLATE("Please choose a new category for all transactions currently in the "
					"%%CATEGORY_NAME%% category."));
	directions.ReplaceFirst("%%CATEGORY_NAME%%",from);
	fDirections->SetText(directions.String());
	fDirections->SetViewColor(backcolor);
	fDirections->SetWordWrap(true);
	
	fOKButton = new BButton("okbutton",TRANSLATE("Cancel"),
						new BMessage(M_REMOVE_CATEGORY));
	
	fOKButton->SetLabel(TRANSLATE("OK"));

	fCancelButton = new BButton("cancelbutton",TRANSLATE("Cancel"),
						new BMessage(B_QUIT_REQUESTED));
			
	fListView = new BOutlineListView("categorylist",B_SINGLE_SELECTION_LIST,
									B_WILL_DRAW | B_FRAME_EVENTS |
									B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE);
	fScrollView = new BScrollView("scrollview",fListView,0,
										false,true);
		
	fIncomeItem = new CategoryItem(TRANSLATE("Income"));
	fSpendingItem = new CategoryItem(TRANSLATE("Spending"));
	fListView->AddItem(fIncomeItem);
	fListView->AddItem(fSpendingItem);
	
	CppSQLite3Query query = gDatabase.DBQuery("select * from categorylist order by name desc",
											"CategoryView::CategoryView");
	
	int32 maxchars;
	float maxlength;
	if(strlen(TRANSLATE("Income"))>strlen(TRANSLATE("Spending")))
	{
		maxchars = strlen(TRANSLATE("Income"));
		maxlength = view->StringWidth(TRANSLATE("Income"));
	}
	else
	{
		maxchars = strlen(TRANSLATE("Spending"));
		maxlength = view->StringWidth(TRANSLATE("Spending"));
	}
	
	while(!query.eof())
	{
		int expense = query.getIntField(1);
		BString name = DeescapeIllegalCharacters(query.getStringField(0));
		
		if(name.Compare(from)==0)
		{
			query.nextRow();
			continue;
		}
		
		if(expense==0)
			fListView->AddUnder(new CategoryItem(name.String()),
									fSpendingItem);
		else
			fListView->AddUnder(new CategoryItem(name.String()),
									fIncomeItem);
		
		if(name.CountChars()>maxchars)
		{
			maxchars = name.CountChars();
			maxlength = view->StringWidth(name.String());
		}
		
		query.nextRow();
	}

	BLayoutBuilder::Group<>(view, B_VERTICAL, 2.0f)
		.SetInsets(10)
		.Add(fDirections, 1)
		.Add(fScrollView, 2)
		.AddGrid(1.0f, 1.0f)
			.AddGlue(0, 0)
			.Add(fCancelButton, 1, 0)
			.Add(fOKButton, 2, 0)
		.End()
	.End();
}

void CategoryRemoveWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_REMOVE_CATEGORY:
		{
			int32 index = fListView->CurrentSelection();
			CategoryItem *item = (CategoryItem*)fListView->ItemAt(index);
			
			if(!item || item==fIncomeItem || item==fSpendingItem)
				break;
			
			BMessenger msgr(fTarget);
			msg->AddString("newcat",item->Text());
			msgr.SendMessage(msg);
			
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}

void CategoryRemoveWindow::FrameResized(float w, float h)
{

}

CategoryEditWindow::CategoryEditWindow(const BRect &frame, const char *oldname, BView *target)
 :	BWindow(frame,TRANSLATE("Edit Category"),B_FLOATING_WINDOW_LOOK,B_MODAL_APP_WINDOW_FEEL,
 			B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE |
 			B_AUTO_UPDATE_SIZE_LIMITS),
 	fOldName(oldname),
 	fTarget(target)
{
	BString temp;
	AddCommonFilter(new EscapeCancelFilter);
	AddShortcut('W',B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	
	BView *view = new BView("background",B_WILL_DRAW | B_FRAME_EVENTS);	
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(0)
		.Add(view)
	.End();
	view->SetViewColor(240,240,240);
	
	temp = TRANSLATE("Category Name"); temp << ": " << fOldName;
	BStringView *oldnameView = new BStringView("oldname",temp.String());	
	
	temp = TRANSLATE("New Category Name"); temp+=":";
	fNameBox = new AutoTextControl("namebox",temp.String(),"",
									new BMessage(M_NAME_CHANGED));
	fNameBox->SetCharacterLimit(32);
	
	fOKButton = new BButton("okbutton",TRANSLATE("Cancel"),
						new BMessage(M_EDIT_CATEGORY));
	fOKButton->SetFlags(fOKButton->Flags() | B_FRAME_EVENTS);
	fOKButton->SetLabel(TRANSLATE("OK"));
	fOKButton->MakeDefault(true);

	fCancelButton = new BButton("cancelbutton",TRANSLATE("Cancel"),
						new BMessage(B_QUIT_REQUESTED));
	
	fOKButton->SetEnabled(false);
	fNameBox->MakeFocus(true);

	BLayoutBuilder::Group<>(view, B_VERTICAL)
		.SetInsets(10)
		.Add(oldnameView)
		.Add(fNameBox)
		.AddGrid(1.0f, 1.0f)
			.Add(fCancelButton, 0, 0)
			.Add(fOKButton, 1, 0)
		.End()
	.End();
}

void CategoryEditWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case M_NAME_CHANGED:
		{
			if(strlen(fNameBox->Text())>0)
			{
				if(!fOKButton->IsEnabled())
					fOKButton->SetEnabled(true);
			}
			else
			{
				if(fOKButton->IsEnabled())
					fOKButton->SetEnabled(false);
			}
			break;
		}
		case M_EDIT_CATEGORY:
		{
			BMessenger msgr(fTarget);
			msg->AddString("newname",fNameBox->Text());
			msgr.SendMessage(msg);
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}
