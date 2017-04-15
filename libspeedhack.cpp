#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <dlfcn.h>
#include <cstdio>
#include <cmath>
#include <cassert>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

static timeval& operator *= (timeval &tv, double x)
{
	double sec = tv.tv_sec * x, usec = tv.tv_usec * x;
	tv.tv_sec = floor(sec);
	usec += 1000000 * (sec - tv.tv_sec);
	tv.tv_usec = fmod(usec, 1000000);
	tv.tv_sec += usec / 1000000;
	return tv;
}

static timeval& operator += (timeval &tv, const timeval &x)
{
	tv.tv_sec += x.tv_sec;
	tv.tv_usec += x.tv_usec;
	if (tv.tv_usec >= 1000000) {
		tv.tv_usec -= 1000000;
		tv.tv_sec += 1;
	}
	return tv;
}

static timeval& operator -= (timeval &tv, const timeval &x)
{
	tv.tv_sec -= x.tv_sec;
	tv.tv_usec -= x.tv_usec;
	if (tv.tv_usec < 0) {
		tv.tv_usec += 1000000;
		tv.tv_sec -= 1;
	}
	return tv;
}

static timeval operator * (const timeval &tv, double x)
{
	timeval res(tv);
	res *= x;
	return res;
}

static timeval operator * (double x, const timeval &tv)
{
	return tv * x;
}

static timeval operator + (const timeval &tv, const timeval &x)
{
	timeval res(tv);
	res += x;
	return res;
}

static timeval operator - (const timeval &tv, const timeval &x)
{
	timeval res(tv);
	res -= x;
	return res;
}

static timespec& operator *= (timespec &tv, double x)
{
	double sec = tv.tv_sec * x, usec = tv.tv_nsec * x;
	tv.tv_sec = floor(sec);
	usec += 1000000000 * (sec - tv.tv_sec);
	tv.tv_nsec = fmod(usec, 1000000000);
	tv.tv_sec += usec / 1000000000;
	return tv;
}

static timespec& operator += (timespec &tv, const timespec &x)
{
	tv.tv_sec += x.tv_sec;
	tv.tv_nsec += x.tv_nsec;
	if (tv.tv_nsec >= 1000000000) {
		tv.tv_nsec -= 1000000000;
		tv.tv_sec += 1;
	}
	return tv;
}

static timespec& operator -= (timespec &tv, const timespec &x)
{
	tv.tv_sec -= x.tv_sec;
	tv.tv_nsec -= x.tv_nsec;
	if (tv.tv_nsec < 0) {
		tv.tv_nsec += 1000000000;
		tv.tv_sec -= 1;
	}
	return tv;
}

static timespec operator * (const timespec &tv, double x)
{
	timespec res(tv);
	res *= x;
	return res;
}

static timespec operator * (double x, const timespec &tv)
{
	return tv * x;
}

static timespec operator + (const timespec &tv, const timespec &x)
{
	timespec res(tv);
	res += x;
	return res;
}

static timespec operator - (const timespec &tv, const timespec &x)
{
	timespec res(tv);
	res -= x;
	return res;
}

static timeval timezero, _timezero;
static timespec
	realtimezero, realtimecoarsezero,
	monotoniczero, monotoniccoarsezero, monotonicrawzero,
	boottimezero, processzero, threadzero,
	_realtimezero, _realtimecoarsezero,
	_monotoniczero, _monotoniccoarsezero, _monotonicrawzero,
	_boottimezero, _processzero, _threadzero;

static int (*gettimeofday_orig)(timeval *tv, struct timezone *tz);
static int (*clock_gettime_orig)(clockid_t clk_id, timespec *tp);

static double speedup = 1;
static int fd;

static void fix_timescale() {
	if (fd < 0) {
		return;
	}
	static char s[16];
	int n = read(fd, s, sizeof(s) - 1);
	if (n <= 0) {
		return;
	}
	s[n] = 0;
	double news;
	if (sscanf(s, "%lf", &news) < 1) {
		return;
	}
	
	if (gettimeofday_orig) {
		timeval tv;
		gettimeofday_orig(&tv, nullptr);
		_timezero += (tv - timezero) * speedup;
		timezero = tv;
	}
	if (clock_gettime_orig) {
		timespec tv;
		clock_gettime_orig(CLOCK_REALTIME, &tv);
		_realtimezero += (tv - realtimezero) * speedup;
		realtimezero = tv;
		clock_gettime_orig(CLOCK_REALTIME_COARSE, &tv);
		_realtimecoarsezero += (tv - realtimecoarsezero) * speedup;
		realtimecoarsezero = tv;
		clock_gettime_orig(CLOCK_MONOTONIC, &tv);
		_monotoniczero += (tv - monotoniczero) * speedup;
		monotoniczero = tv;
		clock_gettime_orig(CLOCK_MONOTONIC_COARSE, &tv);
		_monotoniccoarsezero += (tv - monotoniccoarsezero) * speedup;
		monotoniccoarsezero = tv;
		clock_gettime_orig(CLOCK_MONOTONIC_RAW, &tv);
		_monotonicrawzero += (tv - monotonicrawzero) * speedup;
		monotonicrawzero = tv;
		clock_gettime_orig(CLOCK_BOOTTIME, &tv);
		_boottimezero += (tv - boottimezero) * speedup;
		boottimezero = tv;
		clock_gettime_orig(CLOCK_PROCESS_CPUTIME_ID, &tv);
		_processzero += (tv - processzero) * speedup;
		processzero = tv;
		clock_gettime_orig(CLOCK_THREAD_CPUTIME_ID, &tv);
		_threadzero += (tv - threadzero) * speedup;
		threadzero = tv;
	}
	speedup = news;
}

extern "C" void init_libspeedhack()
{
	gettimeofday_orig = decltype(gettimeofday_orig)(dlsym(RTLD_NEXT,"gettimeofday"));
	clock_gettime_orig = decltype(clock_gettime_orig)(dlsym(RTLD_NEXT,"clock_gettime"));
	fd = open("/tmp/speedhack_pipe", O_RDONLY | O_NONBLOCK);
	fix_timescale();
}

extern "C" int gettimeofday(timeval *tv, struct timezone *tz)
{
	fix_timescale();
	int val = gettimeofday_orig(tv, tz);
	*tv = _timezero + (*tv - timezero) * speedup;
	return val;
}

extern "C" int clock_gettime(clockid_t clk_id, timespec *tp)
{
	fix_timescale();
	timespec z, _z;
	switch (clk_id) {
	case CLOCK_REALTIME:
		z = realtimezero;
		_z = _realtimezero;
		break; 
	case CLOCK_REALTIME_COARSE:
		z = realtimecoarsezero;
		_z = _realtimecoarsezero;
		break; 
	case CLOCK_MONOTONIC:
		z = monotoniczero;
		_z = _monotoniczero;
		break; 
	case CLOCK_MONOTONIC_COARSE:
		z = monotoniccoarsezero;
		_z = _monotoniccoarsezero;
		break; 
	case CLOCK_MONOTONIC_RAW:
		z = monotonicrawzero;
		_z = _monotonicrawzero;
		break; 
	case CLOCK_BOOTTIME:
		z = boottimezero;
		_z = _boottimezero;
		break; 
	case CLOCK_PROCESS_CPUTIME_ID:
		z = processzero;
		_z = _processzero;
		break; 
	case CLOCK_THREAD_CPUTIME_ID:
		z = threadzero;
		_z = _threadzero;
		break; 
	default:
		assert(0);
	}
	int val = clock_gettime_orig(clk_id, tp);
	*tp = _z + (*tp - z) * speedup;
	return val;
}

extern "C" int settimeofday(const timeval *tv, const struct timezone *tz)
{
	assert(0);
}

extern "C" int clock_settime(clockid_t clk_id, const timespec *tp)
{
	assert(0);
}

