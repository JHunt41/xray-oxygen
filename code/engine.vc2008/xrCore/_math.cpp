#include "stdafx.h"
#pragma hdrstop
#pragma warning(disable: 4005)

#include <process.h>
#include <powerbase.h>
#pragma comment (lib, "PowrProf.lib")
#include <mmsystem.h>

// Initialized on startup
XRCORE_API Fmatrix Fidentity;
XRCORE_API Dmatrix Didentity;
XRCORE_API CRandom Random;

typedef struct _PROCESSOR_POWER_INFORMATION 
{
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;
 
namespace	FPU 
{
	XRCORE_API void 	m24		(void)	{
		_control87	( _PC_24,   MCW_PC );
		_control87	( _RC_CHOP, MCW_RC );
	}
	XRCORE_API void 	m24r	(void)	{
		_control87	( _PC_24,   MCW_PC );
		_control87	( _RC_NEAR, MCW_RC );
	}
	XRCORE_API void 	m53		(void)	{
		_control87	( _PC_53,   MCW_PC );
		_control87	( _RC_CHOP, MCW_RC );
	}
	XRCORE_API void 	m53r	(void)	{
		_control87	( _PC_53,   MCW_PC );
		_control87	( _RC_NEAR, MCW_RC );
	}
	XRCORE_API void 	m64		(void)	{
		_control87	( _PC_64,   MCW_PC );
		_control87	( _RC_CHOP, MCW_RC );
	}
	XRCORE_API void 	m64r	(void)	{
		_control87	( _PC_64,   MCW_PC );
		_control87	( _RC_NEAR, MCW_RC );
	}

	void initialize()				
	{
		 m24r();
		::Random.seed(u32(CPU::GetCLK() % (1i64 << 32i64)));
	}
};

namespace CPU 
{
    XRCORE_API u64 qpc_freq;
	XRCORE_API u32 qpc_counter = 0;
	XRCORE_API processor_info ID;

	XRCORE_API u64 QPC()
	{
		u64 _dest;
		QueryPerformanceCounter(PLARGE_INTEGER(&_dest));
		qpc_counter++;
		return _dest;
	}

    u64 getProcessorFrequencyGeneral()
    {
        u64 start, end;
        u32 dwStart, dwTest;

        dwTest = timeGetTime();
        do { dwStart = timeGetTime(); } while (dwTest == dwStart);
        start = GetCLK();
        while (timeGetTime() - dwStart < 1000);
        end = GetCLK();
        return end - start;
    }

    typedef struct _PROCESSOR_POWER_INFORMATION 
	{
		unsigned long Number;
        unsigned long MaxMhz;
        unsigned long CurrentMhz;
        unsigned long MhzLimit;
        unsigned long MaxIdleState;
        unsigned long CurrentIdleState;
    } PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;
    u64 getProcessorFrequency(u32 logicalProcessorCount)
    {
        PROCESSOR_POWER_INFORMATION* pInfo = reinterpret_cast<PROCESSOR_POWER_INFORMATION*> (alloca(sizeof(PROCESSOR_POWER_INFORMATION) * logicalProcessorCount));
        LONG retCode = CallNtPowerInformation(ProcessorInformation, nullptr, 0, pInfo, sizeof(PROCESSOR_POWER_INFORMATION) * logicalProcessorCount);
        if (retCode != 0x0l)
        {
            return getProcessorFrequencyGeneral();
        }
        return pInfo->MhzLimit * u64(1000000);
    }
};

bool g_initialize_cpu_called = false;

//------------------------------------------------------------------------------------
void _initialize_cpu	(void) 
{
	Msg("* Vendor CPU: %s", CPU::ID.vendor);
    Msg("* Detected CPU: %s", CPU::ID.modelName);

//	DUMP_PHASE;

	string256	features;	xr_strcpy(features,sizeof(features),"RDTSC");
    if (CPU::ID.hasFeature(CPUFeature::MMX))    xr_strcat(features,", MMX");
	if (CPU::ID.hasFeature(CPUFeature::AMD_3DNow)) xr_strcat(features, ", 3DNow!");
	if (CPU::ID.hasFeature(CPUFeature::AMD_3DNowExt)) xr_strcat(features, ", 3DNowExt!");
    if (CPU::ID.hasFeature(CPUFeature::SSE))    xr_strcat(features,", SSE");
    if (CPU::ID.hasFeature(CPUFeature::SSE2))   xr_strcat(features,", SSE2");
    if (CPU::ID.hasFeature(CPUFeature::SSE3))   xr_strcat(features,", SSE3");
    if (CPU::ID.hasFeature(CPUFeature::MWait))  xr_strcat(features, ", MONITOR/MWAIT");
    if (CPU::ID.hasFeature(CPUFeature::SSSE3))  xr_strcat(features,", SSSE3");
    if (CPU::ID.hasFeature(CPUFeature::SSE41))  xr_strcat(features,", SSE4.1");
    if (CPU::ID.hasFeature(CPUFeature::SSE42))  xr_strcat(features,", SSE4.2");
	if (CPU::ID.hasFeature(CPUFeature::HT))     xr_strcat(features, ", HTT");
	if (CPU::ID.hasFeature(CPUFeature::AVX))    xr_strcat(features, ", AVX");
#ifndef AI32_USE
	else Debug.do_exit("X-Ray x64 using AVX anyway!");
#endif
	if (CPU::ID.hasFeature(CPUFeature::AVX2))   xr_strcat(features, ", AVX2");
	if (CPU::ID.hasFeature(CPUFeature::SSE4a))  xr_strcat(features, ", SSE4.a");
	if (CPU::ID.hasFeature(CPUFeature::MMXExt)) xr_strcat(features, ", MMXExt");

	Msg("* CPU features: %s" , features );
	Msg("* CPU cores/threads: %d/%d \n", CPU::ID.n_cores, CPU::ID.n_threads);

    LARGE_INTEGER Freq;
    QueryPerformanceFrequency(&Freq);
    CPU::qpc_freq = Freq.QuadPart;

	Fidentity.identity		();	// Identity matrix
	Didentity.identity		();	// Identity matrix
	pvInitializeStatics		();	// Lookup table for compressed normals
	FPU::initialize			();
	_initialize_cpu_thread	();

	g_initialize_cpu_called = true;
}

// per-thread initialization
#include <xmmintrin.h>
const int _MM_DENORMALS_ZERO = 0x0040;
const int _MM_FLUSH_ZERO = 0x8000;

IC void _mm_set_flush_zero_mode(u32 mode)
{
	_mm_setcsr((_mm_getcsr() & ~_MM_FLUSH_ZERO) | (mode));
}

IC void _mm_set_denormals_zero_mode(u32 mode)
{
	_mm_setcsr((_mm_getcsr() & ~_MM_DENORMALS_ZERO) | (mode));
}

static	bool _denormals_are_zero_supported	= true;
void debug_on_thread_spawn	();

void _initialize_cpu_thread	()
{
	debug_on_thread_spawn	();
	// fpu & sse 
	FPU::m24r	();
	if (CPU::ID.hasFeature(CPUFeature::SSE))
	{
		_mm_set_flush_zero_mode(_MM_FLUSH_ZERO);
		if (_denormals_are_zero_supported)	{
			__try	{
				_mm_set_denormals_zero_mode(_MM_DENORMALS_ZERO);
			} __except(EXCEPTION_EXECUTE_HANDLER) {
				_denormals_are_zero_supported	= false;
			}
		}
	}
}

// threading API 
#pragma pack(push,8)
struct THREAD_NAME	{
	DWORD	dwType;
	const char*	szName;
	DWORD	dwThreadID;
	DWORD	dwFlags;
};

void thread_name(const char* name)
{
	THREAD_NAME		tn;
	tn.dwType		= 0x1000;
	tn.szName		= name;
	tn.dwThreadID	= DWORD(-1);
	tn.dwFlags		= 0;
	__try
	{
		RaiseException(0x406D1388, 0, sizeof(tn) / sizeof(size_t), (size_t*)&tn);
	}
	__except(EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}
#pragma pack(pop)

struct	THREAD_STARTUP
{
	thread_t*	entry;
	char*		name;
	void*		args;
};
void __cdecl thread_entry(void*	_params)
{
	// initialize
	THREAD_STARTUP*		startup	= (THREAD_STARTUP*)_params;
	thread_name			(startup->name);
	thread_t*			entry = startup->entry;
	void*				arglist	= startup->args;
	xr_delete			(startup);
	_initialize_cpu_thread		();

	// call
	entry(arglist);
}

void thread_spawn(thread_t*	entry, const char*	name, unsigned	stack, void* arglist )
{
	Debug._initialize	(false);

	auto* startup		= new THREAD_STARTUP();
	startup->entry		= entry;
	startup->name		= (char*)name;
	startup->args		= arglist;
	_beginthread		(thread_entry,stack,startup);
}

void spline1(float t, Fvector *p, Fvector *ret)
{
	const float t2 = t * t;
	const float t3 = t2 * t;
	float m[4];

	ret->x = 0.0f;
	ret->y = 0.0f;
	ret->z = 0.0f;
	m[0] = (0.5f * ((-1.0f * t3) + (2.0f * t2) + (-1.0f * t)));
	m[1] = (0.5f * ((3.0f * t3) + (-5.0f * t2) + (0.0f * t) + 2.0f));
	m[2] = (0.5f * ((-3.0f * t3) + (4.0f * t2) + (1.0f * t)));
	m[3] = (0.5f * ((1.0f * t3) + (-1.0f * t2) + (0.0f * t)));

	for (u32 i = 0; i < 4; i++)
	{
		ret->x += p[i].x * m[i];
		ret->y += p[i].y * m[i];
		ret->z += p[i].z * m[i];
	}
}

void spline2(float t, Fvector *p, Fvector *ret)
{
	const float s = 1.0f - t;
	const float t2 = t * t;
	const float t3 = t2 * t;
	float m[4];

	m[0] = s*s*s;
	m[1] = 3.0f*t3 - 6.0f*t2 + 4.0f;
	m[2] = -3.0f*t3 + 3.0f*t2 + 3.0f*t + 1;
	m[3] = t3;

	ret->x = (p[0].x*m[0] + p[1].x*m[1] + p[2].x*m[2] + p[3].x*m[3]) / 6.0f;
	ret->y = (p[0].y*m[0] + p[1].y*m[1] + p[2].y*m[2] + p[3].y*m[3]) / 6.0f;
	ret->z = (p[0].z*m[0] + p[1].z*m[1] + p[2].z*m[2] + p[3].z*m[3]) / 6.0f;
}

const float beta1 = 1.0f;
const float beta2 = 0.8f;

void spline3( float t, Fvector *p, Fvector *ret )
{
	float	s= 1.0f - t;
	float   t2 = t * t;
	float   t3 = t2 * t;
	float	b12=beta1*beta2;
	float	b13=b12*beta1;
	float	delta=2.0f-b13+4.0f*b12+4.0f*beta1+beta2+2.0f;
	float	d=1.0f/delta;
	float	b0=2.0f*b13*d*s*s*s;
	float	b3=2.0f*t3*d;
	float	b1=d*(2*b13*t*(t2-3*t+3)+2*b12*(t3-3*t2+2)+2*beta1*(t3-3*t+2)+beta2*(2*t3-3*t2+1));
	float	b2=d*(2*b12*t2*(-t+3)+2*beta1*t*(-t2+3)+beta2*t2*(-2*t+3)+2*(-t3+1));

	ret->x = p[0].x*b0+p[1].x*b1+p[2].x*b2+p[3].x*b3;
	ret->y = p[0].y*b0+p[1].y*b1+p[2].y*b2+p[3].y*b3;
	ret->z = p[0].z*b0+p[1].z*b1+p[2].z*b2+p[3].z*b3;
}
