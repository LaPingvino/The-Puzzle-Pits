extrn           _flagtable:byte               

;_mydat  segment para public 'data'
;OldInt9 dw 0000,0000
;_mydat  ends

;_mycode segment para public 'code'
.model large
.code
public  _SetKb,_ResetKb
;assume  cs:_mycode,ds:_mydat
;assume  cs:_mycode

OldInt9 dw 0,0

;New interrupt 09 handler
int9    proc
			cli
			push cx
			push bx
			push ax
			push ds
			mov  ax,SEG _flagtable   ;set up addressing
			mov  ds,ax                                                       ;for data
WaitFor:   in   al,64h              ;is status port-key done?
	   test al,02h              ;check if controller done
	   loopnz WaitFor
	   in   al,60h              ;get the code
	   xor      ah,ah               ;clear ah
	   shl  ax,1                ;ah contains high bit
	   shr  al,1                ;al contains just low bits
	   xor  ah,1                ;Invert-Make code should be 1
	   xor  bh,bh
	   mov  bl,al                                                 ;load bx with scan code
	   mov  _flagtable[bx],ah   ;send it to table

;now make keyboard controller happy
	   or   al,80h     ;set KB enable bit
	   out  61h,al     ;write to control port
	   xchg ah,al      ;send original value back again
	   out  61h,al

;clean up and exit interrupt
	   mov  al,20h
	   out  20h,al      ;tell chip we're done
	   pop  ds
	   pop  ax
	   pop  bx
	   pop  cx
	   sti
	   iret
int9  endp

;install keyboard handler at this startup
_SetKb  proc far
	   push ds       ;save regs
	   push es
	   push ax
	   push dx
	   mov  al,9h    ;request keyboard interrupt
	   mov  ah,35h   ;DOS get interrupt vector
	   int  21h      ;DOS call
	   mov  ax,seg OldInt9  ;get address to save it at
	   mov  ds,ax
	   mov  OldInt9[0],bx   ;save offset
	   mov  OldInt9[2],es   ;save segment
	   mov  ax,seg int9     ;get new segment
	   mov  ds,ax
	   mov  dx,offset int9  ;get new offset
	   mov  al,9h           ;change keyboard interrupt
	   mov  ah,25h          ;DOS change interrupt vector
	   int  21h             ;DOS call
	   pop  dx
	   pop  ax
	   pop  es
	   pop  ds
	   retf
_SetKb  endp

;Remove kb handler
_ResetKb  proc far
	   push ds
	   push ax
	   push dx
	   mov  ax,seg OldInt9   ;get address to restore from
	   mov  ds,ax
	   mov  dx,OldInt9[0]    ;restore offset
	   mov  ax,OldInt9[2]    ;restore segment
	   mov  ds,ax
	   mov  al,9h            ;change kb interrupt
	   mov  ah,25h           ;DOS change interrupt vector
	   int  21h              ;DOS call
	   pop  dx
	   pop  ax
	   pop  ds
	   retf
_ResetKb  endp
;_mycode   ends
;ends

end
