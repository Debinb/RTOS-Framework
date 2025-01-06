; Created on: Sep 17, 2024
; Author: debin
;-----------------------------------------------------------------------------
; Device includes, defines, and assembler directives
;-----------------------------------------------------------------------------

	.def setPSP
	.def getPSP
	.def setMSP
	.def getMSP
	.def setASP
	.def setTMPL
	.def MPUFaultCause
	.def getSVCnum
	.def popREGS
	.def pushREGS
	.def ReadFromR1
;-----------------------------------------------------------------------------
; Register values and large immediate values
;-----------------------------------------------------------------------------

LINK_REG_VALUE 				.field 0xFFFFFFFD

.thumb
.const

setPSP:
	MSR PSP, R0			;setting Process Stack Pointer (PSP) to use R0 value
	ISB 				;Instruction Barrier
	BX LR				;MSR - Move to Special Register

getPSP:
	MRS R0, PSP			;Moves PSP reg value to R0
	ISB
	BX LR

setMSP:
	MSR MSP, R0			;Moves R0 value to MSP register
	ISB
	BX LR

getMSP:
	MRS R0, MSP 		;Moves MSP reg value to R0
	ISB
	BX LR

setASP:					;Setting ASP bit for changing to PSP
	MRS R0, CONTROL		;Moves Control reg value to R0
	ORR R0, R0, #2		;Sets the ASP bit to 1 by performing bitwise OR (2^1) and stores in R0
	MSR CONTROL, R0		;Moves the value of 1 into CONTROL reg... so ASP bit is 1 now
	ISB
	BX LR

setTMPL:				;sets the TMPL bit and changes to unpriveleged mode
	MRS R0, CONTROL		;Moves value from control reg to R0
	ORR R0, R0, #1		;Bitwise OR to set the TMPL bit
	MSR CONTROL, R0
	ISB
	BX LR

MPUFaultCause:
	MOVW R0, #0xFFFF
	MOVT R0, #0xFFFF
	MOV PC, R0
	BX LR

getSVCnum:
	MRS R0, PSP
	LDR R1, [R0, #24]	;Load the value at PSP + 24 bytes (PC saved during exception)
	SUB R1, R1, #2		;Move back two bytes to point to SVC instruction
	LDRH R2, [R1]		;Load halfword of R1 into R2
	AND R2, R2, #0xFF   ;Mask with 0xFF to get immediate value
	MOV R0, R2
	ISB
	BX LR

popREGS:				;restoring registers LR, R11-R4
	MRS R0, PSP

	LDR LR, LINK_REG_VALUE
	ADD R0, #4
	LDR R11, [R0]
	ADD R0, #4
	LDR R10, [R0]
	ADD R0, #4
	LDR R9, [R0]
	ADD R0, #4
	LDR R8, [R0]
	ADD R0, #4
	LDR R7, [R0]
	ADD R0, #4
	LDR R6, [R0]
	ADD R0, #4
	LDR R5, [R0]
	ADD R0, #4
	LDR R4, [R0]
	ADD R0, #4

	MSR PSP, R0
	ISB
	BX LR

pushREGS:				;saving registers R4-R11
	MRS R0, PSP

	SUB R0, R0, #4
	STR R4, [R0]
	SUB R0, R0, #4
	STR R5, [R0]
	SUB R0, R0, #4
	STR R6, [R0]
	SUB R0, R0, #4
	STR R7, [R0]
	SUB R0, R0, #4
	STR R8, [R0]
	SUB R0, R0, #4
	STR R9, [R0]
	SUB R0, R0, #4
	STR R10, [R0]
	SUB R0, R0, #4
	STR R11, [R0]
	SUB R0, R0, #4

	MSR PSP, R0
	ISB
	BX LR

ReadFromR1:
	MOV R1, R0
	BX LR


