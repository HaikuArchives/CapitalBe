#ifndef NOTIFIER_H
#define NOTIFIER_H

#include <List.h>
#include <Message.h>

#define WATCH_CREATE 0x1
#define WATCH_DELETE 0x2
#define WATCH_RENAME 0x4
#define WATCH_CHANGE 0x8
#define WATCH_SELECT 0x10
#define WATCH_REDRAW 0x20
#define WATCH_MASS_EDIT 0x40

#define WATCH_ACCOUNT 0x0000000100000000LL
#define WATCH_TRANSACTION 0x0000000200000000LL
#define WATCH_BUDGET 0x0000000400000000LL
#define WATCH_SCHED_TRANSACTION 0x0000000800000000LL
#define WATCH_LOCALE 0x0000001000000000LL

#define WATCH_EVENTS 0xFFFFFFFF00000000LL
#define WATCH_ACTIONS 0xFFFFFFFF
#define WATCH_ALL 0xFFFFFFFFFFFFFFFFLL

void PrintNotification(const uint64& value, const BMessage* msg);

class Observer {
public:
	Observer(const uint64& flags = WATCH_ALL)
	{
		AddWatch(flags);
		fEnabled = true;
	}

	virtual ~Observer() {}

	void AddWatch(const uint64& flags) { fFlags |= flags; }

	void RemoveWatch(const uint64& flags) { fFlags &= ~flags; }

	bool IsWatching(const uint64& flags) { return fFlags & flags; }

	// Implement this to do whatever you want it to.
	virtual void HandleNotify(const uint64& value, const BMessage* msg) {}

	virtual void SetObserving(const bool& value) { fEnabled = value; }

	bool IsObserving() const { return fEnabled; }

private:
	uint64 fFlags;
	bool fEnabled;
};

class Notifier {
public:
	Notifier() { fEnabled = true; }

	virtual ~Notifier() {}

	// If these methods are subclassed, please make sure that these versions
	// are also called.
	virtual void AddObserver(Observer* obs) { fObserverList.AddItem(obs); }

	virtual void RemoveObserver(Observer* obs) { fObserverList.RemoveItem(obs); }

	bool HasObserver(Observer* obs) { return fObserverList.HasItem(obs); }

	virtual void Notify(const uint64& value, const BMessage* msg = NULL)
	{
		if (!fEnabled)
			return;

		for (int32 i = 0; i < fObserverList.CountItems(); i++) {
			Observer* obs = (Observer*)fObserverList.ItemAt(i);

			if (!obs)
				continue;

			if (obs->IsObserving() && obs->IsWatching(value))
				obs->HandleNotify(value, msg);
		}
	}

	void SetNotify(const bool& value) { fEnabled = value; }

	bool IsNotifying() const { return fEnabled; }

private:
	BList fObserverList;
	bool fEnabled;
};

#endif
