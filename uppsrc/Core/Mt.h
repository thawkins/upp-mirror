#ifndef _DEBUG
inline void AssertST() {}
#endif

#ifdef _MULTITHREADED

class Callback;

class Thread {
#ifdef PLATFORM_WIN32
	HANDLE     handle;
#endif
#ifdef PLATFORM_POSIX
	pthread_t  handle;
#endif

public:
	bool       Run(Callback cb);

	void       Detach();
	int        Wait();

	bool       IsOpen() const     { return handle; }

#ifdef PLATFORM_WIN32
	HANDLE      GetHandle() const { return handle; }
#endif
#ifdef PLATFORM_POSIX
	pthread_t   GetHandle() const { return handle; }
#endif

	void        Priority(int percent); // 0 = lowest, 100 = normal

	static void Start(Callback cb);

	static void Sleep(int ms);

	static bool IsST();
	static int  GetCount();
	static void ShutdownThreads();
	static bool IsShutdownThreads();


	Thread();
	~Thread()   { Detach(); }

private:
	void operator=(const Thread&);
	Thread(const Thread&);
};

#ifdef _DEBUG
inline void AssertST() { ASSERT(Thread::IsST()); }
#endif

void ReadMemoryBarrier();
void WriteMemoryBarrier();

class Semaphore {
#ifdef PLATFORM_WIN32
	HANDLE     handle;
#else
	sem_t      sem;
#endif

public:
	void       Wait();
	void       Release();

	Semaphore();
	~Semaphore();
};

class StaticSemaphore {
	volatile Semaphore *semaphore;
	byte                buffer[sizeof(Semaphore)];

	void Initialize();

public:
	Semaphore& Get()             { ReadMemoryBarrier(); if(!semaphore) Initialize(); return *const_cast<Semaphore *>(semaphore); }
	operator Semaphore&()        { return Get(); }
	void Wait()                  { Get().Wait(); }
	void Release()               { Get().Release(); }
};

#ifdef PLATFORM_WIN32

typedef LONG Atomic;

inline int  AtomicInc(volatile Atomic& t)             { return InterlockedIncrement((Atomic *)&t); }
inline int  AtomicDec(volatile Atomic& t)             { return InterlockedDecrement((Atomic *)&t); }
inline int  AtomicXAdd(volatile Atomic& t, int incr)  { return InterlockedExchangeAdd((Atomic *)&t, incr); }

class Mutex {
protected:
	CRITICAL_SECTION section;

	Mutex(int)         {}

public:
	void  Enter()                { EnterCriticalSection(&section); }
	void  Leave()                { LeaveCriticalSection(&section); }

	Mutex()                      { InitializeCriticalSection(&section); }
	~Mutex()                     { DeleteCriticalSection(&section); }

	struct Lock;
};

#endif

#ifdef PLATFORM_POSIX

typedef _Atomic_word Atomic;

inline int  AtomicInc(volatile Atomic& t)             { using namespace __gnu_cxx; __atomic_add(&t, 1); return t; }
inline int  AtomicDec(volatile Atomic& t)             { using namespace __gnu_cxx; __atomic_add(&t, -1); return t; }
inline int  AtomicXAdd(volatile Atomic& t, int incr)  { using namespace __gnu_cxx; return __exchange_and_add(&t, incr); }

class Mutex {
protected:
	pthread_mutex_t  mutex[1];
	void            *threadid;
	int              count;

	Mutex(int)         {}

public:
	void  Enter();
	void  Leave();

	struct Lock;

	Mutex();
	~Mutex();
};

#endif

inline int  AtomicRead(const volatile Atomic& t)      { ReadMemoryBarrier(); return t; }
inline void AtomicWrite(volatile Atomic& t, int val)  { WriteMemoryBarrier(); t = val; }

struct Mutex::Lock {
	Mutex& s;
	Lock(Mutex& s) : s(s) { s.Enter(); }
	~Lock()                         { s.Leave(); }
};

class StaticMutex {
	volatile Mutex *section;
	byte                      buffer[sizeof(Mutex)];

	void Initialize();

public:
	Mutex& Get()         { if(!section) Initialize(); return *const_cast<Mutex *>(section); }
	operator Mutex&()    { return Get(); }
	void Enter()                   { Get().Enter();}
	void Leave()                   { Get().Leave(); }
};

#define INTERLOCKED \
for(bool i_b_ = true; i_b_;) \
	for(static UPP::StaticMutex i_ss_; i_b_;) \
		for(UPP::Mutex::Lock i_ss_lock__(i_ss_); i_b_; i_b_ = false)

struct H_l_ : Mutex::Lock {
	bool b;
	H_l_(Mutex& cs) : Mutex::Lock(cs) { b = true; }
};

#define INTERLOCKED_(cs) \
for(UPP::H_l_ i_ss_lock__(cs); i_ss_lock__.b; i_ss_lock__.b = false)

void Set__(volatile bool& b);

#define ONCELOCK \
for(static volatile bool o_b_; ReadMemoryBarrier(), !o_b_;) \
	for(static StaticMutex o_ss_; !o_b_;) \
		for(Mutex::Lock o_ss_lock__(o_ss_); !o_b_; WriteMemoryBarrier(), Set__(o_b_))

#define ONCELOCK_(o_b_) \
for(static StaticMutex o_ss_; ReadMemoryBarrier(), !o_b_;) \
	for(Mutex::Lock o_ss_lock__(o_ss_); !o_b_; WriteMemoryBarrier(), Set__(o_b_))

template <class U, class V>
void dirty_trick_MemoryBarrier(U& ptr, V data) {
	WriteMemoryBarrier();
	ptr = data;
}

#define ONCELOCK_PTR(ptr, init) \
if((ReadMemoryBarrier(), !ptr)) { \
	static StaticMutex cs; \
	cs.Enter(); \
	if(!ptr) \
		dirty_trick_MemoryBarrier(ptr, init); \
	cs.Leave(); \
}

#else

typedef int Atomic;

inline int  AtomicRead(const volatile Atomic& t)      { return t; }
inline void AtomicWrite(volatile Atomic& t, int data) { t = data; }

inline int  AtomicInc(volatile Atomic& t)             { ++t; return t; }
inline int  AtomicDec(volatile Atomic& t)             { --t; return t; }
inline int  AtomicXAdd(volatile Atomic& t, int incr)  { Atomic x = t; t += incr; return x; }

class Mutex {
public:
	void  Enter()                {}
	void  Leave()                {}

	Mutex()            {}
	~Mutex()           {}

	struct Lock;
};

typedef Mutex StaticMutex;

struct Mutex::Lock {
	Lock(Mutex&) {}
	~Lock()                {}
};

#define INTERLOCKED
#define INTERLOCKED_(x)

#define ONCELOCK \
for(static bool o_b_; !o_b_; o_b_ = true)

#define ONCELOCK_(o_b_) \
for(; !o_b_; o_b_ = true) \

#define ONCELOCK_PTR(ptr, init) \
if(!ptr) ptr = init;

inline void ReadMemoryBarrier() {}
inline void WriteMemoryBarrier() {}

#ifdef _DEBUG
inline void AssertST() {}
#endif

#endif

typedef Mutex CriticalSection; // deprecated
typedef StaticMutex StaticCriticalSection; // deprecated
