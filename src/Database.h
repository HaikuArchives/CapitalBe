#ifndef DATABASE_H
#define DATABASE_H

#include "Account.h"
#include "CBLocale.h"
#include "CppSQLite3.h"
#include "Notifier.h"
#include "ObjectList.h"
#include <Locker.h>
#include <String.h>
#include <time.h>

class Fixed;
class BudgetEntry;
class TransactionType;
class TransactionData;
class ScheduledTransData;

BString
AccountTypeToString(const AccountType& type);

class Database : public Notifier {
  public:
	Database(const char* file = NULL);
	~Database(void);

	// ---------------------------------------------------------------------------
	// Old API functions (list-based)
	// ---------------------------------------------------------------------------
	Account* SetCurrentAccount(const int32& index);
	int32 SetCurrentAccount(Account* account);

	Account* CurrentAccount(void) const { return fCurrent; }

	Account* AccountAt(int32 index) { return fList.ItemAt(index); }

	Account* AccountByName(const char* name);
	Account* AccountByID(const uint32& accountid);

	int32 CountAccounts(void) { return fList.CountItems(); }

	void PrintToStream(void) const;

	int32 IndexOf(Account* acc) { return fList.IndexOf(acc); }

	void CloseAccount(Account* item);
	void ReopenAccount(Account* item);

	bool ImportFile(const entry_ref& ref);
	bool ExportFile(const entry_ref& ref);

	// ---------------------------------------------------------------------------
	// SQLite-based functions
	// ---------------------------------------------------------------------------
	void CreateFile(const char* path);
	status_t OpenFile(const char* path);
	void CloseFile(void);

	Account* AddAccount(
		const char* name, const AccountType& type, const char* status = "Open",
		const Locale* locale = NULL
	);
	bool RemoveAccount(const int& accountid);
	void RemoveAccount(Account* item);
	void RenameAccount(Account* item, const char* name);

	void AddBudgetEntry(const BudgetEntry& entry);
	bool RemoveBudgetEntry(const char* category);
	bool HasBudgetEntry(const char* category);
	bool GetBudgetEntry(const char* name, BudgetEntry& entry);
	int32 CountBudgetEntries(void);

	void SetAccountLocale(const uint32& accountid, const Locale& data);
	Locale LocaleForAccount(const uint32& id);
	void SetDefaultLocale(const Locale& data);
	Locale GetDefaultLocale(void);
	bool UsesDefaultLocale(const uint32& id);

	bool AddTransaction(
		const uint32& accountid, const uint32& id, const time_t& date, const TransactionType& type,
		const char* payee, const Fixed& amount, const char* category, const char* memo,
		const uint8& status = TRANS_OPEN
	);
	bool AddTransaction(TransactionData& data, const bool& newid = true);
	bool RemoveTransaction(const uint32& transid);
	uint32 NextTransactionID(void);
	bool HasTransaction(const uint32& transid);
	bool GetTransaction(const uint32& transid, TransactionData& data);
	bool GetTransaction(const uint32& transid, const uint32& accountid, TransactionData& data);
	void SetTransactionStatus(const uint32& transid, const uint8& status);

	bool GetTransferCounterpart(const uint32& transid, TransactionData& data);

	void AddScheduledTransaction(const ScheduledTransData& data, const bool& newid = true);
	void RemoveScheduledTransaction(const uint32& id);
	bool GetScheduledTransaction(const uint32& transid, ScheduledTransData& data);
	uint32 CountScheduledTransactions(void);

	void AddCategory(const char* name, const bool& isexpense);
	void RemoveCategory(const char* name);
	bool RenameCategory(const char* oldname, const char* newname);
	bool HasCategory(const char* name);
	bool IsCategoryExpense(const char* name);
	void SetCategoryExpense(const char* name, const bool& isexpense);
	void RecategorizeTransactions(const char* from, const char* to);

	void DBCommand(const char* command, const char* functionname);
	CppSQLite3Query DBQuery(const char* query, const char* functionname);

  private:
	int32 GetLastKey(const char* table, const char* column);

	// Used to allow split scheduled transactions
	bool InsertSchedTransaction(
		const uint32& id, const uint32& accountid, const time_t& startdate,
		const TransactionType& type, const char* payee, const Fixed& amount, const char* category,
		const char* memo, const TransactionInterval& interval, const time_t& nextdate,
		const int32& count = -1
	);

	Account* fCurrent;
	BObjectList<Account> fList;

	BLocker fLocker;
	BString fPath;
	CppSQLite3DB fDB;
};

BString
EscapeIllegalCharacters(const char* string);
BString
DeescapeIllegalCharacters(const char* string);

extern Locale gDefaultLocale;
extern Locale gCurrentLocale;
extern Database gDatabase;

#endif
