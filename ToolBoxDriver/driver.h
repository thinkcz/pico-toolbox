#pragma once

// tools commands
#define TOOL_QUERY		0x1
#define TOOL_LOAD		0x2
#define TOOL_UNLOAD		0x3

// dispatcher prototype
typedef unsigned int TOOL_DISPATCHER(unsigned int uCmd, void* pData, size_t sDataSize);

#define MAX_TOOLS 10


#define PICOIOCTL_TYPE 40000
//
// The IOCTL function codes from 0x800 to 0xFFF are for customer use.
//


#define IOCTL_PICOIOCTL_ACTIVATE_TOOL \
    CTL_CODE( PICOIOCTL_TYPE, 0x904, METHOD_BUFFERED, FILE_ANY_ACCESS  )


#define DRIVER_FUNC_INSTALL     0x01
#define DRIVER_FUNC_REMOVE      0x02

#define DRIVER_NAME         L"picodrv"

#define NT_DEVICE_NAME      L"\\Device\\picodrv"
#define DOS_DEVICE_NAME     L"\\DosDevices\\picodrvtool"


typedef struct _PICO_DRV_START_CONTEXT
{
	unsigned int  ProcessId;
	unsigned int  ThreadId;
} PICO_DRV_START_CONTEXT;

typedef struct _PICO_CONTEXT {
	int a;
} PICO_CONTEXT, *PPICO_CONTEXT;




typedef struct _PICODRV_DATA {

	void* pNTDll;
	void* pKernelBase;
	unsigned long ulKernelSize;
	unsigned int pActiveTool;
	TOOL_DISPATCHER* ToolsRegistered[MAX_TOOLS];
	size_t sToolsSize;

} PICODRV_DATA;





