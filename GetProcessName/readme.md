========================================================================
    Getting name of  PICO process (bash, init, etc.)
========================================================================

This is currently the only way how to get meaningful name of PICO process,
through native API call.

Tested in Windows 10 Enterprise Insider build 1439

Notes:

     PICO providers inside KM have callback registered for getting ntoskrnl
     know what is the name of PICO process, however for some unknown reason 
     (maybe it is still WIP), the only place which references this callback
     is ExpGetProcessInformation which is refrenced by NtQueryInformationProcess
     classes 5, 57, 148. The name in case lxsscore.sys is not full name including 
     path but only simple process name.
     

Tools used:
      IDA + WinDbg

