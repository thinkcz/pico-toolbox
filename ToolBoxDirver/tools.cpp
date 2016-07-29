// various functions for support

#include "common.h"
#include "picostruct.h"
#include "os.h"


extern "C" {

	// externs
	NTSTATUS ZwQuerySection(
		IN HANDLE               SectionHandle,
		IN SECTION_INFORMATION_CLASS InformationClass,
		OUT PVOID               InformationBuffer,
		IN ULONG                InformationBufferSize,
		OUT PULONG              ResultLength OPTIONAL);

	NTSTATUS ZwQuerySystemInformation(
		IN ULONG SystemInformationClass, 
		IN PVOID SystemInformation, 
		IN ULONG SystemInformationLength, 
		OUT PULONG ReturnLength);

	/* tries to find NTDLL address using KnownDlls*/
	NTSTATUS FindNtdll(OUT void** ppv)
	{
		NTSTATUS status;
		OBJECT_ATTRIBUTES oa;

		DECLARE_CONST_UNICODE_STRING(us, L"\\KnownDlls\\ntdll.dll");

		InitializeObjectAttributes(&oa, (PUNICODE_STRING)&us, NULL, NULL, NULL);

		HANDLE hSection;
		if (0 <= (status = ZwOpenSection(&hSection, SECTION_QUERY, &oa)))
		{
			SECTION_IMAGE_INFORMATION sii;
			if (0 <= (status = ZwQuerySection(hSection, SectionImageInformation, &sii,
				sizeof(sii), 0)))
			{
				*ppv = sii.TransferAddress;
			}
			ZwClose(hSection);
		}

		return status;
	}

	

	PVOID FindKernelBase(OUT PULONG puSize OPTIONAL)
	{
		PRTL_PROCESS_MODULES pModules;
		ULONG uLen;
		NTSTATUS status;


	LRepeat:
		ZwQuerySystemInformation(SystemModuleInformation, (PVOID)&uLen, 0, &uLen);

		pModules = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(PagedPool, uLen, '--xS');
		if (!pModules) return NULL;

		status = ZwQuerySystemInformation(SystemModuleInformation, (PVOID)pModules, uLen, &uLen);
		if (!NT_SUCCESS(status)) {

			ExFreePoolWithTag(pModules, '--xS');
			if (STATUS_INFO_LENGTH_MISMATCH == status) goto LRepeat;
			return NULL;
		}

		for (ULONG i = 0; i < pModules->NumberOfModules; i++)
		{
			if (strstr((char*)pModules->Modules[i].FullPathName, "ntoskrnl.exe") ||		// 1 CPU
				strstr((char*)pModules->Modules[i].FullPathName, "ntkrnlmp.exe") ||		// N CPU, SMP
				strstr((char*)pModules->Modules[i].FullPathName, "ntkrnlpa.exe") ||		// 1 CPU, PAE
				strstr((char*)pModules->Modules[i].FullPathName, "ntkrpamp.exe") ||		// N CPU, SMP, PAE
				strstr((char*)pModules->Modules[i].FullPathName, "xNtKrnl.exe"))			// patched kernel
			{
				if (puSize) *puSize = pModules->Modules[i].ImageSize;
				PVOID pImageBase = pModules->Modules[i].ImageBase;
				ExFreePoolWithTag(pModules, '--xS');

				return pImageBase;
			}
		}

		ExFreePoolWithTag(pModules, '--xS');
		return NULL;
	}


	// big o'hack enabled registration of subsequent pico providers, in fact overwrite the other ones
	BOOLEAN EnablePicoRegistrations(IN BOOLEAN bEnable)
	{

		// get address of PsRegisterPicoProvider
		void* addr = (void*)PsRegisterPicoProvider;

		// find cmp ....
		INT32* pDis = (INT32*)((UINT_PTR)addr + 0x2e);

		// extract address
		UINT8* p_disbleRegistration = (UINT8*)((DWORD64)addr + 0x32 + *pDis);

		// just to be sure check we have really cmp
		// TODO:

		
		BOOLEAN old = (*p_disbleRegistration==0);

		*p_disbleRegistration = bEnable ? 0 : 1;

		return old;
	}


	// gets direct pointer to system's table of PICO provider's callbacks
	NTSTATUS GetPICOCallbacks(PPS_PICO_PROVIDER_ROUTINES* pppr)
	{
		//
		if (!pppr)
			return STATUS_INVALID_PARAMETER;


		// get address of PsRegisterPicoProvider
		void* addr = (void*)PsRegisterPicoProvider;

		// find cmp ....
		INT32* dis = (INT32*)((UINT_PTR)addr + 0x2e);

		// extract address
		UINT8* p_disbleRegistration = (UINT8*)((DWORD64)addr + 0x32 + *dis);


		PPS_PICO_PROVIDER_ROUTINES ppr = (PPS_PICO_PROVIDER_ROUTINES)ALIGN_UP_POINTER_BY(p_disbleRegistration + 1, 0x20);

		*pppr = ppr;

		return STATUS_SUCCESS;
	}



}