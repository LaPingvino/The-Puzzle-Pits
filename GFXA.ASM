;Assembly Language Graphics and I/O Routines
;10.21.95 By Abe Pralle

extrn _keytable:byte

.model large
public _setmode,_cls,_screenswap,_setpalette
public _initmouse,_readmbutton,_relpos
public  _setkb,_resetkb

.code
OldInt9 dw 0,0


_setmode PROC
  ARG  mode:WORD
  push bp
  mov  bp,sp
  mov  ax,mode
  mov  ah,0
  int  10h
  pop  bp
  ret
_setmode ENDP

_cls PROC
  ARG screen:DWORD
    push bp
    mov  bp,sp
    push di
    les  di,screen
    mov  cx,32000
    mov  ax,0
    rep  stosw
    pop  di
    pop  bp
    ret
_cls ENDP

;  blitscreen( void far* buffer )
;    move a full 320 x 200 frame to video ram from buffer

_screenswap     PROC
  ARG  screen:DWORD
  push bp
  mov  bp,sp
  push es
  push ds
  push di
  push si
  mov  ax, 0a000h
  mov  es, ax
  xor  di, di
  lds  si, screen
  mov  cx, 32000
  mov  dx,03dah   ;// VGA status register 1 
wait_for_retrace:
  in   al,dx    ;//get value of status reg 1 
  test al,08h  ;//retrace in progress? 
  jnz  wait_for_retrace
  cld
  rep  movsw
  pop  si
  pop  di
  pop  ds
  pop  es
  pop  bp
  ret
_screenswap     ENDP

_setpalette PROC
  ARG  regs:DWORD
  push bp              ; Save BP
  mov  bp,sp           ; Set up stack pointer
  les  dx,regs         ; Point ES:SX at palette registers
  mov  ah,10h          ; Specify BIOS function 10h
  mov  al,12h          ; ...subfunction 12h
  mov  bx,0            ; Start with first register
  mov  cx,64           ; Set 64 registers
  int  10h             ; Call video BIOS
  pop  bp              ; Restore BP
  ret
_setpalette     ENDP

_initmouse PROC
  ;call mouse driver initialization routine
  mov  ax,0
  int  33h
  ret
_initmouse ENDP

_readmbutton PROC
  ;read mouse buttons
  mov  ax,3
  int  33h
  mov  ax,bx
  ret
_readmbutton ENDP

_relpos PROC
  ARG x:DWORD, y:DWORD
  push bp
  mov  bp,sp
  mov  ax,000bh
  int  33h
  les  bx,x
  mov  [es:bx],cx
  les  bx,y
  mov  [es:bx],dx
  pop  bp
  ret
_relpos ENDP

;New interrupt 09 handler
int9 PROC
  cli
  push cx
  push bx
  push ax
  push ds
  mov  ax,SEG _keytable   ;set up addressing
  mov  ds,ax               ;for data
WaitFor:   
  in   al,64h              ;is status port-key done?
  test al,02h              ;check if controller done
  loopnz WaitFor
  in   al,60h              ;get the code
  xor  ah,ah               ;clear ah
  shl  ax,1                ;ah contains high bit
  shr  al,1                ;al contains just low bits
  xor  ah,1                ;Invert the Make code (1=pushed)
  xor  bh,bh
  mov  bl,al               ;load bx with scan code
  mov  _keytable[bx],ah   ;send it to table
  cmp  ah,0               ;was our key released instead of pressed?
  jz   nostore0           ; then skip this next instr
  mov  _keytable[0],bl    ;& also put the key into keytable[0]
nostore0:

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
int9  ENDP

;install keyboard handler at this startup
_setkb PROC 
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
_setkb  endp

;Remove kb handler
_resetkb  proc 
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
_resetkb  endp

END

