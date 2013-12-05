NAME = capitalbe
TYPE = APP
RDEFS = src/capitalbe.rdef
SRCS = src/AboutWindow.cpp src/Account.cpp src/AccountListItem.cpp \
	src/AccountSettingsWindow.cpp src/App.cpp src/AutoTextControl.cpp src/Budget.cpp \
	src/BudgetReport.cpp src/BudgetWindow.cpp src/CashFlowReport.cpp src/Category.cpp src/CategoryBox.cpp \
	src/CategoryWindow.cpp src/CheckNumBox.cpp src/CheckView.cpp src/ColorTools.cpp src/ColumnListView.cpp \
	src/ColumnTypes.cpp src/CppSQLite3.cpp src/CurrencyBox.cpp src/DAlert.cpp src/Database.cpp \
	src/DateBox.cpp src/DrawButton.cpp src/DStringList.cpp src/Fixed.cpp src/HelpButton.cpp \
	src/Import.cpp src/Language.cpp src/LanguageRoster.cpp src/Locale.cpp src/MainWindow.cpp \
	src/NavTextBox.cpp src/NetWorthReport.cpp src/Notifier.cpp src/NumBox.cpp src/PayeeBox.cpp \
	src/Preferences.cpp src/PrefWindow.cpp src/QuickTrackerItem.cpp src/ReconcileItem.cpp \
	src/ReconcileWindow.cpp src/RegisterView.cpp src/ReportGrid.cpp src/ReportWindow.cpp \
	src/ScheduleAddWindow.cpp src/ScheduledExecutor.cpp src/ScheduledTransData.cpp \
	src/ScheduledTransItem.cpp src/ScheduleListWindow.cpp src/SplitItem.cpp src/SplitView.cpp \
	src/SplitViewFilter.cpp src/StickyDrawButton.cpp src/TextFile.cpp src/TimeSupport.cpp \
	src/Transaction.cpp src/TransactionData.cpp src/TransactionEditWindow.cpp src/TransactionItem.cpp \
	src/TransactionLayout.cpp src/TransactionReport.cpp src/TransactionView.cpp src/TransferWindow.cpp
	
LIBS = be tracker translation sqlite3

include $(BUILDHOME)/etc/makefile-engine
