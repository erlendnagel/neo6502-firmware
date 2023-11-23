; ***************************************************************************************
; ***************************************************************************************
;
;      Name :      support.asm
;      Authors :   Paul Robson (paul@robsons.org.uk)
;      Date :      23rd November 2023
;      Reviewed :  No
;      Purpose :   Support functions
;
; ***************************************************************************************
; ***************************************************************************************

; ***************************************************************************************
;
;					  Send message in the following 2 bytes
;
; ***************************************************************************************

KSendMessage:
		jsr		KWaitMessage 				; wait for command to be released.

		sta 	KSMReturnA+1 				; save A reloaded at end.
		pla 								; pop return address to the read instruction
		sta 	KSMRAddress+1 			
		pla
		sta 	KSMRAddress+2

		jsr 	KSMReadAdvance 				; read the command.
		pha 								; save, write it after the command.
		jsr 	KSMReadAdvance 				; read the function number
		sta 	DFunction 					
		pla
		sta 	DCommand 					; save the command, starting the message.
KSMAdvanceReturn:		
		jsr 	KSMReadAdvance 				; use jmp indirect so advance it again.
KSMReturnA:		
		lda 	#$FF 						; original A value
		jmp 	(KSMRAddress+1)

KSMReadAdvance:
		inc 	KSMRAddress+1 				; pre-inc because of 6502 RTS behaviour
		bne 	KSMRAddress
		inc 	KSMRAddress+2
KSMRAddress:
		lda 	$FFFF 						; holds the return address.
		rts

; ***************************************************************************************
;
;							Write character inlined (following byte)
;
; ***************************************************************************************

KWriteCharacterInLine:
		sta 	KSMReturnA+1 				; save A reloaded at end.
		pla 								; pop return address to the read instruction
		sta 	KSMRAddress+1 			
		pla
		sta 	KSMRAddress+2
		jsr 	KSMReadAdvance 				; output a character
		jsr 	KWriteCharacter
		bra 	KSMAdvanceReturn

; ***************************************************************************************
;
;							Write A to the current console
;
; ***************************************************************************************

KWriteCharacter:	
		pha
		sta 	DParameters 				; sending A
		stz 	DFunction 					; we don't inline it because inline uses it
		lda 	#1
		sta 	DCommand
		pla
		rts

; ***************************************************************************************
;
;							Read keystroke from the console
;
; ***************************************************************************************

KReadCharacter:
		jsr 	KSendMessage 				; send command 1,1 read keyboard
		.byte 	1,1
		lda 	DParameters 				; read result
		beq 	KReadCharacter 				; no key, yet.
		rts

; ***************************************************************************************
;
;			Wait for the handler process to acknowledge or return a value
;
; ***************************************************************************************

KWaitMessage:
		pha
KWaitMessage1:
		lda 	DCommand 					; wait until the handler has finished.
		bne 	KWaitMessage1
		pla
		rts
	