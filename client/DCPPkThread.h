#if !defined(DCPPK_THREAD_H__3006956B_7C69_4DAD_9596_A49E1BD007D5__INCLUDED_)
#define DCPPK_THREAD_H__3006956B_7C69_4DAD_9596_A49E1BD007D5__INCLUDED_

#include "Thread.h"
#include "CriticalSection.h"

// DCPP_EXTENDER BEGINS
class UsefulThread : public Thread
{
public:

	UsefulThread()
	{
		mShouldRun = false;
		mHasBeenStarted = false;
		mShouldDestroyItself = false;
	}

protected:
	CriticalSection cs;

	bool mShouldRun;

public:

	GETSET(bool, mHasBeenStarted, HasBeenStarted);

	GETSET(bool, mShouldDestroyItself, ShouldDestroyItself);

	void destroyAfterRun()
	{
		setShouldDestroyItself(true);
	}

	virtual int run()
	{
		if(mShouldDestroyItself)
		{
			delete this;
		}
		return 0;
	}

	void start()
	{
		mShouldRun = true;
		Thread::start();
		mHasBeenStarted = true;
	}


	void join()
	{
		while(shouldRun())
		{
			Thread::sleep(50);
		}
	}

	bool shouldRun()
	{
		return mShouldRun;
	}

	void stopASAP()
	{
		Lock l(cs);
		mShouldRun = false;
	}
};

class ThreadedLowPriority : public UsefulThread
{
public:

	ThreadedLowPriority() : UsefulThread()
	{
	}

public:
	void start()
	{
		UsefulThread::start();
		setThreadPriority(Thread::Priority::LOW);
	}
};
// DCPP_EXTENDER ENDS

#endif // !defined(DCPPK_THREAD_H__3006956B_7C69_4DAD_9596_A49E1BD007D5__INCLUDED_)
