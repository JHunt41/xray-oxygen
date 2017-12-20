#include "stdafx.h"
#include "cpuid.h"

#include <Windows.h>
#include <array>
#include <bitset>
#include <memory>
#include <vector>  
#include <string>  
#include <intrin.h>

static void CleanDups(char* s, char c = ' ')
{
	if (*s == 0)
		return;
	char* dst = s;
	char* src = s + 1;
	while (*src != 0)
	{
		if (*src == c && *dst == c)
			++src;
		else
			*++dst = *src++;
	}
	*++dst = 0;
}

using ulong_t = unsigned long long;
using long_t = long long;

unsigned long countSetBits(ulong_t bitMask)
{
	unsigned long LSHIFT = sizeof(ulong_t) * 8 - 1;
	unsigned long bitSetCount = 0;
	long_t bitTest = static_cast<ulong_t>(1) << LSHIFT;

	for (unsigned long i = 0; i <= LSHIFT; ++i)
	{
		bitSetCount += ((bitMask & bitTest) ? 1 : 0);
		bitTest /= 2;
	}

	return bitSetCount;
}

unsigned int query_processor_info(processor_info* pinfo)
{
	std::memset(pinfo, 0, sizeof(processor_info));

	std::bitset<32> f_1_ECX;
	std::bitset<32> f_1_EDX;
	std::bitset<32> f_1_EBX;
	std::bitset<32> f_81_EDX;
	std::bitset<32> f_81_ECX;

	std::vector<std::array<int, 4>> data;
	std::array<int, 4> cpui;

	__cpuid(cpui.data(), 0);
	const int nIds = cpui[0];

	for (int i = 0; i <= nIds; ++i)
	{
		__cpuidex(cpui.data(), i, 0);
		data.push_back(cpui);
	}

	memset(pinfo->vendor, 0, sizeof(pinfo->vendor));
	*reinterpret_cast<int*>(pinfo->vendor) = data[0][1];
	*reinterpret_cast<int*>(pinfo->vendor + 4) = data[0][3];
	*reinterpret_cast<int*>(pinfo->vendor + 8) = data[0][2];

	pinfo->isAmd = std::strncmp(pinfo->vendor, "AuthenticAMD", 12) == 0;
	pinfo->isIntel = !pinfo->isAmd;

	// load bitset with flags for function 0x00000001
	if (nIds >= 1)
	{
		f_1_ECX = data[1][2];
		f_1_EDX = data[1][3];
	}

	if (nIds >= 7)
	{
		f_1_EBX = data[7][1];
	}

	// load bitset with flags for function 0x00000007
	__cpuid(cpui.data(), 0x80000000);
	const int nExIds_ = cpui[0];
	data.clear();

	for (int i = 0x80000000; i <= nExIds_; ++i)
	{
		__cpuidex(cpui.data(), i, 0);
		data.push_back(cpui);
	}
	// load bitset with flags for function 0x80000001
	if (nExIds_ >= 0x80000001)
	{
		f_81_ECX = data[1][2];
		f_81_EDX = data[1][3];
	}
	memset(pinfo->modelName, 0, sizeof(pinfo->modelName));

	// Interpret CPU brand string if reported
	if (nExIds_ >= 0x80000004)
	{
		memcpy(pinfo->modelName, data[2].data(), sizeof(cpui));
		memcpy(pinfo->modelName + 16, data[3].data(), sizeof(cpui));
		memcpy(pinfo->modelName + 32, data[4].data(), sizeof(cpui));
	}

	//Added sv3nk
	CleanDups(pinfo->vendor);
	CleanDups(pinfo->modelName);
	//end

	if (f_1_EDX[23])           pinfo->features |= static_cast<unsigned>(CPUFeature::MMX);
	if (f_1_EDX[25])           pinfo->features |= static_cast<unsigned>(CPUFeature::SSE);
	if (f_1_EDX[26])           pinfo->features |= static_cast<unsigned>(CPUFeature::SSE2);

	//Added sv3nk: AMD Features
	if (pinfo->isAmd)
	{
		// 3DNow!
		if (f_81_EDX[31])		pinfo->features |= static_cast<unsigned>(CPUFeature::AMD_3DNow);
		if (f_81_EDX[30])		pinfo->features |= static_cast<unsigned>(CPUFeature::AMD_3DNowExt);
		// SSE & MMX
		if (f_1_ECX[6])			pinfo->features |= static_cast<unsigned>(CPUFeature::SSE4a);
		if (f_81_EDX[22])		pinfo->features |= static_cast<unsigned>(CPUFeature::MMXExt);
	}
	//End


	if (pinfo->isAmd)
		//

		if (f_1_ECX[0])            pinfo->features |= static_cast<unsigned>(CPUFeature::SSE3);
	if (f_1_ECX[9])            pinfo->features |= static_cast<unsigned>(CPUFeature::SSSE3);
	if (f_1_ECX[19])           pinfo->features |= static_cast<unsigned>(CPUFeature::SSE41);
	if (f_1_ECX[20])           pinfo->features |= static_cast<unsigned>(CPUFeature::SSE42);

	//Added sv3nk: AVX
	if (f_1_ECX[28])           pinfo->features |= static_cast<unsigned>(CPUFeature::AVX);
	if (f_1_EBX[5])			   pinfo->features |= static_cast<unsigned>(CPUFeature::AVX2);
	if (f_1_EBX[16])           pinfo->features |= static_cast<unsigned>(CPUFeature::AVX512F);
	if (f_1_EBX[26])           pinfo->features |= static_cast<unsigned>(CPUFeature::AVX512PF);
	if (f_1_EBX[27])           pinfo->features |= static_cast<unsigned>(CPUFeature::AVX512ER);
	if (f_1_EBX[28])           pinfo->features |= static_cast<unsigned>(CPUFeature::AVX512CD);
	//End
	__cpuid(cpui.data(), 1);

	//Edit sv3nk
	if ((cpui[2] & 0x8) > 0)   pinfo->features |= static_cast<unsigned>(CPUFeature::MWait);

	pinfo->family = (cpui[0] >> 8) & 0xf;
	pinfo->model = (cpui[0] >> 4) & 0xf;
	pinfo->stepping = cpui[0] & 0xf;

	// Calculate available processors
	ulong_t pa_mask_save, sa_mask_stub = 0;
	GetProcessAffinityMask(GetCurrentProcess(), &pa_mask_save, &sa_mask_stub);

	unsigned long returnedLength = 0;
	unsigned long byteOffset = 0;
	GetLogicalProcessorInformation(nullptr, &returnedLength);

	auto buffer = std::make_unique<unsigned char[]>(returnedLength);
	auto ptr = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(buffer.get());
	GetLogicalProcessorInformation(ptr, &returnedLength);

	auto processorCoreCount = 0u;
	auto logicalProcessorCount = 0u;

	while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnedLength)
	{
		switch (ptr->Relationship)
		{
		case RelationProcessorCore:
			processorCoreCount++;

			// A hyperthreaded core supplies more than one logical processor.
			logicalProcessorCount += countSetBits(ptr->ProcessorMask);
			break;

		default:
			break;
		}

		byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		ptr++;
	}

	if (logicalProcessorCount != processorCoreCount) pinfo->features |= static_cast<unsigned>(CPUFeature::HT);

	// All logical processors
	pinfo->n_threads = logicalProcessorCount;
	pinfo->affinity_mask = unsigned(pa_mask_save);
	pinfo->n_cores = processorCoreCount;
	return pinfo->features;
}

processor_info::processor_info()
{
	features = query_processor_info(&*this);
}