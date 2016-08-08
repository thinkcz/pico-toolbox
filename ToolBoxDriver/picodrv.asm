
.code



	



; =======================================================================================
; Trampoline entry point
; =======================================================================================

Trampoline proc
;
; Don't save fastcall registers (RCX, RDX, R8, R9) and ignore first four XMM0-3 registers as well (they're used for the floating args)
; Jump to kernel-mode with using NtDeviceIoControlFile function. Prepare its input parameters.
;
    add			rsp, 		38h
	pop			rbx
	jmp 		rcx

Trampoline endp

END
