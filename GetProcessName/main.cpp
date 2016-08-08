#include "stdafx.h"
#include "windows.h"
#include "tchar.h"
#include "stdio.h"
#include "psapi.h"
#include "winternl.h"
#include "ntstatus.h"

#pragma comment (lib,"ntdll.lib")


// undocumented version of _SYSTEM_PROCESS_INFORMATION
// thanks to Lada Zezula www.zezula.net
typedef struct _UNDOC_SYSTEM_PROCESS_INFORMATION
{
	ULONG NextEntryOffset;
	ULONG NumberOfThreads;
	LARGE_INTEGER SpareLi1;
	LARGE_INTEGER SpareLi2;
	LARGE_INTEGER SpareLi3;
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER KernelTime;
	UNICODE_STRING ImageName;
	LONG BasePriority;
	HANDLE UniqueProcessId;
	HANDLE InheritedFromUniqueProcessId;
	ULONG HandleCount;
	ULONG SessionId;
	ULONG_PTR PageDirectoryBase;

	//
	// This part corresponds to VM_COUNTERS_EX.
	// NOTE: *NOT* THE SAME AS VM_COUNTERS!
	//
	SIZE_T PeakVirtualSize;
	ULONG VirtualSize;
	SIZE_T PageFaultCount;
	SIZE_T PeakWorkingSetSize;
	SIZE_T WorkingSetSize;
	SIZE_T QuotaPeakPagedPoolUsage;
	SIZE_T QuotaPagedPoolUsage;
	SIZE_T QuotaPeakNonPagedPoolUsage;
	SIZE_T QuotaNonPagedPoolUsage;
	SIZE_T PagefileUsage;
	SIZE_T PeakPagefileUsage;
	SIZE_T PrivatePageCount;

	//
	// This part corresponds to IO_COUNTERS
	//
	LARGE_INTEGER ReadOperationCount;
	LARGE_INTEGER WriteOperationCount;
	LARGE_INTEGER OtherOperationCount;
	LARGE_INTEGER ReadTransferCount;
	LARGE_INTEGER WriteTransferCount;
	LARGE_INTEGER OtherTransferCount;

	//SYSTEM_THREAD_INFORMATION TH[1];
} UNDOC_SYSTEM_PROCESS_INFORMATION, *PUNDOC_SYSTEM_PROCESS_INFORMATION;





NTSTATUS find_process_name(BOOL bShowMinimalOnly, HANDLE pid = 0)
{
	unsigned char* p_buffer = NULL;
	ULONG size = 0;
	NTSTATUS status = 0;
	UNDOC_SYSTEM_PROCESS_INFORMATION spi;
	
	memset(&spi, 0, sizeof(spi));

	status = NtQuerySystemInformation(SystemProcessInformation, &spi, sizeof(spi), &size);
	
	if (status != 0xC0000004)
		return status;
	// allocate buffer
	p_buffer = new unsigned char[size];
	status = NtQuerySystemInformation(SystemProcessInformation, p_buffer, size, &size);

	if (status != STATUS_SUCCESS)
		return status;
	
	
	PUNDOC_SYSTEM_PROCESS_INFORMATION pspi;

	pspi = (PUNDOC_SYSTEM_PROCESS_INFORMATION)(p_buffer);
	while (pspi) {

		// hic sunt leones				
		PUNICODE_STRING p_uname = &(UNICODE_STRING)pspi->ImageName;

		if (!pid || pspi->UniqueProcessId == pid) {
			
			// kind of heuristic to find out if process is minimal process
			// comes out from simple fact that minimal processes have no
			// windows handles
			BOOL bMinimal = (pspi->HandleCount == 0);

			if ((bShowMinimalOnly && bMinimal) || (!bShowMinimalOnly))
				if (p_uname->Length != 0)
					printf("Process: (%d) %wZ\n", pspi->UniqueProcessId, p_uname);
		}

		if (!pspi->NextEntryOffset)
			break;

		// next record
		pspi = (PUNDOC_SYSTEM_PROCESS_INFORMATION)((UINT_PTR)pspi + pspi->NextEntryOffset);
	}
	return STATUS_SUCCESS;

}




int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE pid = 0; // place PICO process pid here, otherwise all will be printed

	/*
		if first parameter bShowMinimalOnly is TRUE, then it lists only
		processes which is likely to be minimal or PICO (based on handles count)	
	*/

	NTSTATUS status = find_process_name(TRUE, pid);
	
	if (status != STATUS_SUCCESS)
		printf("Failed with ntstatus (%p)\n", status);
	
	return 0;
}