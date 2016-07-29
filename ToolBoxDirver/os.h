/* os undocumented and rare structures*/

#pragma once


/* The structure representing the System Service Table. */
typedef struct SystemServiceTable {
	UINT32* 	ServiceTable;
	UINT32* 	CounterTable;
	UINT32		ServiceLimit;
	UINT32*     ArgumentTable;
} SST;

typedef struct SSDT {
	UINT32* 	NtoskrnlTable;
	UINT32* 	Win32kTable;
	UINT32		t2;
	UINT32*     t3;
} SSDT;



/* Section query information classes*/
typedef enum _SECTION_INFORMATION_CLASS {
	SectionBasicInformation,
	SectionImageInformation
} SECTION_INFORMATION_CLASS;

typedef struct _SECTION_IMAGE_INFORMATION
{
	PVOID TransferAddress;
	ULONG ZeroBits;
	SIZE_T MaximumStackSize;
	SIZE_T CommittedStackSize;
	ULONG SubSystemType;
	union
	{
		struct
		{
			USHORT SubSystemMinorVersion;
			USHORT SubSystemMajorVersion;
		};
		ULONG SubSystemVersion;
	};
	ULONG GpValue;
	USHORT ImageCharacteristics;
	USHORT DllCharacteristics;
	USHORT Machine;
	UCHAR ImageContainsCode;
	UCHAR Spare1;
	ULONG LoaderFlags;
	ULONG ImageFileSize;
	ULONG Reserved[1];
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;


typedef enum _PSCREATEPROCESSNOTIFYTYPE {
	PsCreateProcessNotifyPico = 0
} PSCREATEPROCESSNOTIFYTYPE;


/* NtQueryInformationSystem things */
#define SystemModuleInformation     11

__if_not_exists(RTL_PROCESS_MODULE_INFORMATION) {
	typedef struct _RTL_PROCESS_MODULE_INFORMATION {
		HANDLE Section;                 // Not filled in
		PVOID MappedBase;
		PVOID ImageBase;
		ULONG ImageSize;
		ULONG Flags;
		USHORT LoadOrderIndex;
		USHORT InitOrderIndex;
		USHORT LoadCount;
		USHORT OffsetToFileName;
		UCHAR  FullPathName[256];
	} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

	typedef struct _RTL_PROCESS_MODULES {
		ULONG NumberOfModules;
		RTL_PROCESS_MODULE_INFORMATION Modules[1];
	} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;
}


extern  "C" NTSTATUS PsSetCreateProcessNotifyRoutineEx2(
	_In_ PSCREATEPROCESSNOTIFYTYPE NotifyType,
	_In_ PVOID NotifyInformation,
	_In_ BOOLEAN Remove
);
