.code

JumpToKernel PROC
	mov rsp, rdx
	jmp r8
JumpToKernel ENDP

END