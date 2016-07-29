#include "common.h"
#include "os.h"
#include "driver.h"
#include "log.h"
#include "tools.h"

#include "PicoMon.h"
#include "picostruct.h"


extern "C" {

	static struct {

		PPS_PICO_GET_ALLOCATED_PROCESS_IMAGE_NAME pGetAllocatedProcessImageName;
		PPS_PICO_PROVIDER_PROCESS_EXIT pOriginalPicoProviderProcessExit;
		PPS_PICO_PROVIDER_ROUTINES pRoutines;

	} gData;

	// hooked pico provider's routine for exit process 
	// it is not necessary for getting name
	// it's here just to show that implementation of process name
	// in moment of ProcessNotifyCallbackEx is not setup correctly
	// on exit it returns correct name of linux proccess
	VOID PicoProviderProcessExit(_In_ PEPROCESS Process)
	{
		PICODRV_LOG_INFO("PICOMON: Exitting PICO process:");
		PICODRV_LOG_INFO("    EPROCESS: %p", Process);
		
		if (gData.pGetAllocatedProcessImageName != NULL) {


			// now the name is always valid
			PUNICODE_STRING puName = NULL;
			gData.pGetAllocatedProcessImageName(Process, &puName);

			// just to be sure
			if ((puName) && (puName->Length) && (puName->Buffer)) {
				PICODRV_LOG_INFO("    NAME: %wZ", puName);
			}

		}
		
		gData.pOriginalPicoProviderProcessExit(Process);
	}


	void ProcessNotifyCallbackEx(IN PEPROCESS pProcessObj, IN HANDLE ProcessId, IN PPS_CREATE_NOTIFY_INFO pCreateInfo OPTIONAL)
	{
	
		// test if this process is PICO
		if ((pCreateInfo) && (pCreateInfo->Flags & 0x80000000)) {
			
			PICODRV_LOG_INFO("PICOMON: New PICO process:");
			PICODRV_LOG_INFO("    EPROCESS/PID: %p %p", pProcessObj, ProcessId );

			
			// now try to its name by querying provider directly
			if (gData.pGetAllocatedProcessImageName != NULL) {

				// NOTE: there is obvious bug in lxsscore.sys the name returned on this notify callback is not
				// yet setup so it returns mostly bash for all other processes
				PUNICODE_STRING puName = NULL;
				gData.pGetAllocatedProcessImageName(pProcessObj, &puName);

				// just to be sure
				if ((puName) && (puName->Length) && (puName->Buffer)) {
					PICODRV_LOG_INFO("   NAME: %wZ", puName);
				}

			}

		}

	}



static unsigned int PicoMon_Load(void* pData, size_t sDataSize)
{
	PICODRV_LOG_INFO("PICOMON: PICO process monitor loading...");

	RtlZeroMemory(&gData, sizeof(gData));

	NTSTATUS status = PsSetCreateProcessNotifyRoutineEx2(PsCreateProcessNotifyPico, ProcessNotifyCallbackEx, FALSE);
	
	// now the tricky part, first check if there is already pico provider installed

	PPS_PICO_PROVIDER_ROUTINES pRoutines;

	PICODRV_LOG_INFO("PICOMON: now trying to get pico callbacks...");
	
	if NT_SUCCESS(GetPICOCallbacks(&pRoutines)) {
		// now we have pointer to all calbacks, so let's get one for getting
		// name of process and store it. 

		// now try to check if all data are OK
		// FIXME: beware of concurrent access
		if ((pRoutines->Size != 0) && (pRoutines->GetAllocatedProcessImageName != NULL)) {
			gData.pRoutines = pRoutines;
			// this should be address of routine inside lxsscore.sys
			gData.pGetAllocatedProcessImageName = pRoutines->GetAllocatedProcessImageName;
					
			
			PICODRV_LOG_INFO("PICOMON: callback 'GetAllocatedProcessImageName' resides at %p", gData.pGetAllocatedProcessImageName);

			// hook process exit to show its name on exit
			gData.pOriginalPicoProviderProcessExit = pRoutines->ExitProcess;
			
			PICODRV_LOG_INFO("PICOMON: callback 'ProcessExit' resides at %p", pRoutines->ExitProcess);


			if (pRoutines->ProtectedRanges) {
				PICODRV_LOG_INFO("PICOMON: provider registered these ranges:");
				// just enyumerate protected ranges
				for (auto idx = 0; idx < pRoutines->ProtectedRanges->AddressRangeCount; idx++)
					PICODRV_LOG_INFO("PICOMON: %p - %p ",
						pRoutines->ProtectedRanges->AddressRanges[idx].Address,
						(UINT_PTR)pRoutines->ProtectedRanges->AddressRanges[idx].Address +
						pRoutines->ProtectedRanges->AddressRanges[idx].Size - 1);
			}
			// redirect call back to us
			pRoutines->ExitProcess = PicoProviderProcessExit;
		}
		else {
			PICODRV_LOG_ERROR("PICOMON: callbacks don't seem to be valid or linux subsystem is not installed");
		}
	}

	

	return 0;
}

static unsigned int PicoMon_UnLoad(void* pData, size_t sDataSize)
{
	PICODRV_LOG_INFO("PICOMON: PICO process monitor unloading...");
	
	// unregister process notification
	PsSetCreateProcessNotifyRoutineEx2(PsCreateProcessNotifyPico, ProcessNotifyCallbackEx, TRUE);
	if (gData.pOriginalPicoProviderProcessExit)
	{
		gData.pRoutines->ExitProcess = gData.pOriginalPicoProviderProcessExit;
	}
	return 0;
}



unsigned int PicoMon_ToolDispatcher(unsigned int uCmd, void* pData, size_t sDataSize)
{
	switch (uCmd)
	{

		case TOOL_QUERY:
			if ((sDataSize == sizeof(unsigned long)) && (pData) &&
				(TOOL_PICOMON_UID == *((unsigned long*)pData)))
				return 1;
			else
				return 0;
			break;

		case TOOL_LOAD:
			return PicoMon_Load(pData, sDataSize);

		case TOOL_UNLOAD:
			return PicoMon_UnLoad(pData, sDataSize);


	}

}

}