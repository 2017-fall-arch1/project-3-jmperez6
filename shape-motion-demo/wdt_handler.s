<<<<<<< HEAD
	.global addPts
	.global FeetJump
=======
	.global CanInterrupt
>>>>>>> a6b083134665f79f37c31e5681c692d9ee2d7e0f
	.file	"_wdt_handler.c"
.text
	.balign 2
	.global	WDT
	.section	__interrupt_vector_11,"ax",@progbits
	.word	WDT
	.text
	

	.extern redrawScreen
	.extern wdt_c_handler
WDT:
; start of function
; attributes: interrupt 
; framesize_regs:     24
; framesize_locals:   0
; framesize_outgoing: 0
; framesize:          24
; elim ap -> fp       26
; elim fp -> sp       0
; saved regs: R4 R5 R6 R7 R8 R9 R10 R11 R12 R13 R14 R15
	; start of prologue
	PUSH	R15
	PUSH	R14
	PUSH	R13
	PUSH	R12
	PUSH	R11
	PUSH	R10
	PUSH	R9
	PUSH	R8
	PUSH	R7
	PUSH	R6
	PUSH	R5
	PUSH	R4
	; end of prologue
	CALL	#wdt_c_handler
	; start of epilogue
	POP	R4
	POP	R5
	POP	R6
	POP	R7
	POP	R8
	POP	R9
	POP	R10
	POP	R11
	POP	R12
	POP	R13
	POP	R14
	POP	R15
	cmp	#0, &redrawScreen
	jz	ball_no_move
	and	#0xffef, 0(r1)	; clear CPU off in saved SR
ball_no_move:	
	RETI
	.size	WDT, .-WDT
	.local	count
	.comm	count,1,1
	.ident	"GCC: (GNU) 4.9.1 20140707 (prerelease (msp430-14r1-364)) (GNUPro 14r1) (Based on: GCC 4.8 GDB 7.7 Binutils 2.24 Newlib 2.1)"

addPts:	
	add #1, R12
	ret
	
FeetJump: 			;R12 is moving layer, R13 is layer R14 is velocity
	mov #0, R4		;temporary variable

	mov R14, 4(R12)		;ml->velocity.axes[1] = Bodyvelocity

	;; begin vec2Add()
	mov 4(R12), R4		;temp = ml->velocity.axes[1]
	add 12(R13), R4		;temp += layer->posNext.axes[1]
	;; end vec2Add()
	add R14, R4		;temp += Bodyvelocity
	mov R4, 12(R13)		;layer->posNext.axes[1] = temp 

	ret
