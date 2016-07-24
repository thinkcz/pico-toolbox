#include "stdafx.h"
#include "windows.h"
#include "tchar.h"
#include "stdio.h"
#include "psapi.h"
#include "winternl.h"
#include "ntstatus.h"

#pragma comment (lib,"ntdll.lib")



NTSTATUS find_process_name(HANDLE pid = 0)
{
	unsigned char* p_buffer = NULL;
	ULONG size = 0;
	NTSTATUS status = 0;
	SYSTEM_PROCESS_INFORMATION spi;
	
	memset(&spi, 0, sizeof(spi));

	status = NtQuerySystemInformation(SystemProcessInformation, &spi, sizeof(spi), &size);
	
	if (status != 0xC0000004)
		return status;
	// allocate buffer
	p_buffer = new unsigned char[size];
	status = NtQuerySystemInformation(SystemProcessInformation, p_buffer, size, &size);

	if (status != STATUS_SUCCESS)
		return status;
	
	
	PSYSTEM_PROCESS_INFORMATION pspi;

	pspi = (SYSTEM_PROCESS_INFORMATION*)(p_buffer);
	while (pspi) {

		// hic sunt leones				
		PUNICODE_STRING p_uname = (PUNICODE_STRING)pspi->Reserved2;

		if (!pid || pspi->UniqueProcessId == pid) {

			if (p_uname->Length != 0)
				printf("Process: (%d) %wZ \n", pspi->UniqueProcessId, p_uname);

		}

		if (!pspi->NextEntryOffset)
			break;

		// next record
		pspi = (SYSTEM_PROCESS_INFORMATION*)((UINT_PTR)pspi + pspi->NextEntryOffset);
	}
	return STATUS_SUCCESS;

}




int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE pid = 0; // place PICO process pid here, otherwise all will be printed


	NTSTATUS status = find_process_name(pid);
	
	if (status != STATUS_SUCCESS)
		printf("Failed with ntstatus (%p)\n", status);
	
	return 0;
}