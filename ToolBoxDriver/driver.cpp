
#include "driver.h"
#include "common.h"
#include "log.h"
#include "tools.h"
#include "os.h"
#include "intrin.h"
#include "picostruct.h"

#include "PicoMon.h"


extern "C" {


//global data
PICODRV_DATA gPicoDrv;

// forwards
DRIVER_INITIALIZE DriverEntry;
static DRIVER_UNLOAD	DriverpDriverUnload;
NTSTATUS IoctlDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS IoctlCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);



#if defined(ALLOC_PRAGMA)
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverpDriverUnload)
#pragma alloc_text(PAGE, IoctlDeviceControl)
#pragma alloc_text(PAGE, IoctlCreateClose)
#endif

NTSTATUS RegisterDeviceAndLink(PDRIVER_OBJECT drvobj)
{
	NTSTATUS status = STATUS_SUCCESS;
	// create control device and corresponding DOS link
	
	DECLARE_CONST_UNICODE_STRING(ntUnicodeString, NT_DEVICE_NAME);
	DECLARE_CONST_UNICODE_STRING(ntWin32NameString, DOS_DEVICE_NAME);

	PDEVICE_OBJECT  deviceObject = NULL;    


	status = IoCreateDevice(
		drvobj,                   // Our Driver Object
		0,                              // We don't use a device extension
		(PUNICODE_STRING)&ntUnicodeString,    // Device name 
		FILE_DEVICE_UNKNOWN,            // Device type
		FILE_DEVICE_SECURE_OPEN,     // Device characteristics
		FALSE,                          // Not an exclusive device
		&deviceObject);

	if ( !NT_SUCCESS(status) )
		return status;
	
	status = IoCreateSymbolicLink(
		(PUNICODE_STRING)&ntWin32NameString, 
		(PUNICODE_STRING)&ntUnicodeString);

	if (!NT_SUCCESS(status))
	{

		// Delete everything that this routine has allocated.
		PICODRV_LOG_ERROR("Couldn't create symbolic link\n");
		IoDeleteDevice(deviceObject);		
	}
	return status;
}


NTSTATUS UnRegisterDeviceAndLink(PDRIVER_OBJECT drvobj)
{
	// s break
	PDEVICE_OBJECT deviceObject = drvobj->DeviceObject;
	DECLARE_CONST_UNICODE_STRING( uniWin32NameString, DOS_DEVICE_NAME );

	//
	// Delete the link from our device name to a name in the Win32 namespace.
	//

	NTSTATUS status = IoDeleteSymbolicLink((PUNICODE_STRING)&uniWin32NameString);

	if (deviceObject != NULL)
	{
		IoDeleteDevice(deviceObject);
	}

	return status;

}

/*------------------------------- DRIVER ENTRY --------------------------------*/
NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) 
{
  UNREFERENCED_PARAMETER(registry_path);

  PAGED_CODE();  
 
  NTSTATUS status = STATUS_UNSUCCESSFUL;
  
  // log file name creation
  static const wchar_t kLogFilePath[] = L"\\SystemRoot\\" DRIVER_NAME L".log";
  static const auto kLogLevel =
      (IsReleaseBuild()) ? kLogPutLevelInfo | kLogOptDisableFunctionName
                         : kLogPutLevelDebug | kLogOptDisableFunctionName;
  
  // no active tool
  gPicoDrv.pActiveTool = NULL;
  gPicoDrv.sToolsSize = 0;


  // now register tools
  gPicoDrv.ToolsRegistered[++gPicoDrv.sToolsSize] = PicoMon_ToolDispatcher;
  gPicoDrv.pActiveTool = 1;


  // register unload function
  driver_object->DriverUnload = DriverpDriverUnload;
  

  // Request NX Non-Paged Pool when available
  ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

  // Initialize log functions
  bool need_reinitialization = false;
  status = LogInitialization(kLogLevel, kLogFilePath);
  if (status == STATUS_REINITIALIZATION_NEEDED) {
	  need_reinitialization = true;
  } else if (!NT_SUCCESS(status)) {
    return status;
  }


  // register for basic IRPs
  driver_object->MajorFunction[IRP_MJ_CREATE] = IoctlCreateClose;
  driver_object->MajorFunction[IRP_MJ_CLOSE] = IoctlCreateClose;
  driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoctlDeviceControl;


  status = RegisterDeviceAndLink(driver_object);

  if (!NT_SUCCESS(status)) {
	  LogTermination();
	  return status;
  }


  // Register re-initialization for the log functions if needed
  if (need_reinitialization) {
    LogRegisterReinitialization(driver_object);
  }

  // try to get some useful stuff

  gPicoDrv.pKernelBase = FindKernelBase(&gPicoDrv.ulKernelSize);

  PICODRV_LOG_INFO("Found kernel base at %p with size of %08x",
	  gPicoDrv.pKernelBase,
	  gPicoDrv.ulKernelSize);

  if NT_SUCCESS(FindNtdll(&gPicoDrv.pNTDll)) {
	  PICODRV_LOG_INFO("Found ntdll.dll at %p", gPicoDrv.pNTDll);
  }
  

  

  // here is place for tools


  if (gPicoDrv.pActiveTool) {
	  PICODRV_LOG_INFO("Loading tool...");
	  gPicoDrv.ToolsRegistered[gPicoDrv.pActiveTool](TOOL_LOAD, NULL, NULL);
  }


  PICODRV_LOG_INFO("The picodrv toolbox driver has been installed.");
  return status;
}



// Unload handler
static void DriverpDriverUnload(
    PDRIVER_OBJECT driver_object) {
  UNREFERENCED_PARAMETER(driver_object);
  PAGED_CODE();
  

  if (gPicoDrv.pActiveTool) {
	  PICODRV_LOG_INFO("Un loading tool...");
	  gPicoDrv.ToolsRegistered[gPicoDrv.pActiveTool](TOOL_UNLOAD, NULL, NULL);
  }


  UnRegisterDeviceAndLink(driver_object);

  LogTermination();
  
}



NTSTATUS
IoctlCreateClose(
	PDEVICE_OBJECT DeviceObject,
	PIRP Irp
)

{
	UNREFERENCED_PARAMETER(DeviceObject);

	PAGED_CODE();

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}



NTSTATUS
IoctlDeviceControl(
	PDEVICE_OBJECT DeviceObject,
	PIRP Irp
)


{
	PIO_STACK_LOCATION  irpSp;// Pointer to current stack location
	NTSTATUS            ntStatus = STATUS_SUCCESS;// Assume success
	ULONG               inBufLength; // Input buffer length
	ULONG               outBufLength; // Output buffer length
	PCHAR               inBuf, outBuf; // pointer to Input and output buffer
	//PCHAR               data = "This String is from Device Driver !!!";
	//size_t              datalen = strlen(data) + 1;//Length of data including null
	PMDL                mdl = NULL;
	PCHAR               buffer = NULL;

	UNREFERENCED_PARAMETER(DeviceObject);

	PAGED_CODE();

	irpSp = IoGetCurrentIrpStackLocation(Irp);
	inBufLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	outBufLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

	if (!inBufLength || !outBufLength)
	{
		ntStatus = STATUS_INVALID_PARAMETER;
		goto End;
	}

	//
	// Determine which I/O control code was specified.
	//

	switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
	{

	case IOCTL_PICOIOCTL_ACTIVATE_TOOL:
		
		PICODRV_LOG_INFO("Magic is about to happen\n");
		
		//AbrakaDabra((PCHAR)Irp->AssociatedIrp.SystemBuffer);

		break;



	default:

		//
		// The specified I/O control code is unrecognized by this driver.
		//

		ntStatus = STATUS_INVALID_DEVICE_REQUEST;
		
		break;
	}

End:
	//
	// Finish the I/O operation by simply completing the packet and returning
	// the same status as in the packet itself.
	//

	Irp->IoStatus.Status = ntStatus;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return ntStatus;
}


}  // extern "C"
