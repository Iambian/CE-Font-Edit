.assume adl=1

;Export these definitions to other C/ASM sources

XDEF _GetScan
XDEF _PrintHexPair
XDEF _PrintLargeChar
XDEF _PrintSmallChar
XDEF _PopulateFileList
XDEF _GetSmallCharWidth
XDEF _SetupPalette
XDEF _GetCharLocation
XDEF _PrintChar
XDEF _GetFontBit
XDEF _SetFontBit
XDEF _ResFontBit
XDEF _InvFontBit
XDEF _GetDefaultLocation
XDEF _LoadCursors
XDEF _KeyToChar


;Import these definitions from othere sources (e.g. libraries)
XREF _kb_Scan
XREF _gfx_PrintChar
XREF _gfx_SetTextXY
XREF _gfx_GetTextX
XREF _gfx_GetTextY
XREF _gfx_SetTextFGColor

XREF _editbuf

GETSCAN_MAXDEBOUNCE     EQU   130
GETSCAN_REPEATDELAY     EQU   13

flags             EQU $D00080 ;As defined in ti84pce.inc
_LoadPattern      EQU $021164 ;''
_FindAlphaDn      EQU $020E90 ;''
_FindAlphaUp      EQU $020E8C ;''
_ChkFindSym       EQU $02050C ;''
_ChkInRam         EQU $021F98 ;'' NC if in RAM, C if in arc
_PopRealO1        EQU $0205DC ;''
_PopRealO2        EQU $0205D8 ;''
_PopRealO4        EQU $0205D0 ;''
_PushRealO1       EQU $020614 ;''
_PushRealO4       EQU $020608 ;''
_SetLocalizeHook  EQU $0213F0 ;''
_ClrLocalizeHook  EQU $0213F4 ;''
_SetFontHook      EQU $021454 ;''
_ClrFontHook      EQU $021458 ;''
_MovFrOp1         EQU $02032C ;''



prevDData         EQU $D005A1 ;''
lFont_record      EQU $D005A4 ;''
sFont_record      EQU $D005C5 ;''
Op1               EQU $D005F8 ;''
Op2               EQU $D00603 ;''
Op3               EQU $D0060E ;''
Op4               EQU $D00619 ;''
Op5               EQU $D00624 ;''
Op6               EQU $D0062F ;''

DRAW_BUFFER       EQU $E30014

;----------------------------------------------------------------------------
;Returns getcsc codes in lower 6 bits.
;bit 7 = Alpha
;bit 6 = 2nd
;Implementation notes: 2nd and alpha are filtered out of the getcsc results.

_GetScan:
      call  _kb_Scan
      ld    hl,$F5001E  ;kb group 0. grp1 = grp0-2, grp2 = grp1-2, etc
      ld    bc,256*7    ;set B to 7, clears C
getscan_mainloop:
      ld    a,l
      cp    a,$14      ;Checking group 5 for alpha
      jr    nz,getscan_skipalpha
      ld    a,(hl)
      res   7,a         ;filtering out ALPHA from general result to allow mult
      jr    getscan_contscan
getscan_skipalpha:
      cp    a,$12       ;Checking group 6 for 2nd
      ld    a,(hl)
      jr    nz,getscan_contscan
      res   5,a
getscan_contscan:
      or    a,a
      jr    z,getscan_nextgroup
      inc   c
      dec   c
      jr    z,getscan_specialarrows
getscan_scanloop:
      inc   c
      rrca
      jr    nc,getscan_scanloop
      jr    getscan_endscan
getscan_nextgroup:
      dec   hl
      dec   hl
      ld    a,c
      add   a,8
      ld    c,a
      djnz  getscan_mainloop
      ld    c,b         ;Loop ended with no keypress. Reset C.
getscan_endscan:
      ld    l,$14
      bit   7,(hl)      ;recheck for ALPHA
      jr    z,$+4
      set   7,c         ;ALPHA keycode bit 7
      ld    l,$12
      bit   5,(hl)
      jr    z,$+4
      set   6,c         ;2nd keycode bit 6
;---
      ;There's a flowchart on paper regarding the algorithm.
      ;First step is checking if kbd==0
      ld    hl,getscan_prevkey
      ld    a,c
      and   a,$3F
      jr    z,getscan_setandaccept
      ;Next check is if kbd == prevkey. if same, start dbnce
      cp    a,(hl)
      jr    z,getscan_checkdebounce
getscan_setandaccept:
      ;Runs if KBD==0 or (KBD and KBD!=prevkey)
      ld    (hl),a
      inc   hl
      ld    (hl),0
      ld    a,c
      ret
;I probably shouldn't call this "debounce" but meh. More like "controlled bounce"
getscan_checkdebounce:
      ;Now check if debounce at its limit
      inc   hl
      ld    a,(hl)
      cp    a,GETSCAN_MAXDEBOUNCE
      jr    nc,getscan_checkifdpad
      ;Not at limit. Move towards limit and clear kbd.
      inc   (hl)
getscan_clearnormalkeys:
      ld    a,c
      and   a,$C0       ;keep the 2nd/alpha states, though.
      ret
getscan_checkifdpad:
      ld    a,c
      and   a,$3F
      ;Added extra keys to debouncer.
      cp    a,$33
      jr    z,getscan_rundebouncer
      cp    a,$32
      jr    z,getscan_rundebouncer
      ;Debounce arrow keys
      cp    a,9
      jr    nc,getscan_clearnormalkeys
getscan_rundebouncer:
      ld    a,(hl)
      sub   a,GETSCAN_REPEATDELAY
      ld    (hl),a
      ld    a,c
      ret
;A=%URLD: %0001=1,%0010=2,%0100=3,%1000=4,%0011=5,%0101=6,%1010=7,%1100=8
getscan_specialarrows:
      push  hl
            call  _kb_Scan    ;rescan to give time for diagonals
            ld    a,($F5001E)
            sbc   hl,hl
            ld    l,a
            ld    bc,getscan_arrowmap
            add   hl,bc
            ld    c,(hl)
      pop   hl
      jr    getscan_endscan
getscan_arrowmap: db 0,1,2,5,3,6,0,0,4,0,7,0,8,0,0,0
getscan_prevkey:  db 0
getscan_curtime:  db 0

;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
;arg0 = uint8_t. prints char to screen
_PrintHexPair:
      pop   bc
      pop   hl
      push  hl
      push  bc
      ld    a,L
      rra
      rra
      rra
      rra
      push  hl
            call  printhexpair_hex
      pop   hl
      ld    a,L
;got the trick from z80bits      
printhexpair_hex:
      or    a,$F0
      daa
      add   a,$A0
      adc   a,$40
      ld    L,a
      push  hl
            call  _gfx_PrintChar
      pop   hl
      ret


;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
;arg0 = file type (0x06=protprog,0x15=appvar)
;returns: list length
XREF _filelist_len
XREF _filelist
_PopulateFileList:      
      ld    iy,flags
      push  ix
            ld    hl,6
            add   hl,sp
            ld    l,(hl)
            ld    h,0
            ld    (Op1),hl
            ld    e,h               ;E is our counter
            ld    hl,_filelist      ;HL is our cur pointer      
populatefilelist_loop:
            push  de
                  push  hl
populatefilelist_filenotfont:
                        call  _FindAlphaUp
                        jr    c,populatefilelist_end
                        call  getfontstruct
                        jr    c,populatefilelist_filenotfont
                        ex    (sp),hl     ;cur pointer, on stack: fontstruct
                        ld    de,Op1
                        ex    de,hl
                        push  bc
                              ld    bc,9
                              ldir        ;load filetype and name
                        pop   bc
                        xor   a,a
                        ld    (de),a      ;force the null terimator
                        inc   de
                  pop   hl
                  ex    de,hl
                  ld    (hl),de           ;load fonstruct
                  inc   hl
                  inc   hl
                  inc   hl
                  ld    (hl),c            ;load size
                  inc   hl
                  ld    (hl),b
                  inc   hl                ;arrived at next location
            pop   de
            inc   e           ;load fonts until table is full. Nobody is going to
            jr    nz,populatefilelist_loop      ;keep that many fonts on a system
            ;... right?
            dec   e
            jr    populatefilelist_ran_out_of_space
populatefilelist_end:
                  pop   hl
            pop   de
populatefilelist_ran_out_of_space:
            ld    a,e
            ld    (_filelist_len),a
      pop   ix
      ret

;-----------------------------------------
getfontstruct:
      call  _ChkFindSym
      jr    c,getfontstruct_failure
      call  _ChkInRam
      ex    de,hl
      jr    nc,getfontstruct_isinram
      ret   nc
      ld    de,9
      add   hl,de
      ld    e,(hl)
      add   hl,de
      inc   hl
getfontstruct_isinram:
      ld    bc,0
      ld    c,(hl)
      inc   hl
      ld    b,(hl)
      inc   hl
      ld    de,getfontstuct_header
getfontstruct_cmploop:
      ld    a,(de)
      inc   de
      cpi
      jr    nz,getfontstruct_failure
      or    a,a
      jr    nz,getfontstruct_cmploop
      ld    de,(hl)     ;DE=offset
      ex    de,hl
      add   hl,de       ;HL= &fontstruct, DE=location of offset table
      or    a,a
      ret
getfontstruct_failure:
      or    a,a
      sbc   hl,hl
      scf
      ret
getfontstuct_header:
db $EF,$7B,$18,$0C,"FNTPK",0
sizeof_fontheader equ $-getfontstuct_header



;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
;arg0 = char, arg1 = fontid (1=lfont)
_GetDefaultLocation:
      push  ix
            ld    hl,6
            add   hl,sp
            ld    a,(hl)
            inc   hl
            inc   hl
            inc   hl
            dec   (hl)
            jr    z,gdefloc_islarge
            call  gscw_usedefault
            jr    gdefloc_end
gdefloc_islarge:
            call  glcp_usedefault
gdefloc_end:
      pop   ix
      ret
      
      
;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
;arg0 = char, arg1 = pointer_to_fonstruct, arg3 = fontid (1=lfont)
_PrintChar:
      ld    de,0
      ld    hl,3+6
      add   hl,sp
      ld    a,(hl)
printchar_backontrack:
      or    a,a
      jp    m,printchar_forcedefaults
      ld    hl,_PrintLargeChar
      jr    nz,printchar_notsmall
      ld    hl,_PrintSmallChar
printchar_notsmall:
      add   hl,de
      jp    (hl)
printchar_forcedefaults:
      and   a,$7F
      ld    de,-4       ;if defaulting, jump backwards to default setter
      jr    printchar_backontrack
      
;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
;arg0 = char, arg1 = pointer_to_fonstruct, arg3 = fontid (1=lfont)
_GetCharLocation:
      push  ix
            ld    ix,6
            add   ix,sp
            ld    hl,(ix+3)
            push  hl
                  ld    hl,(ix+0)
                  push  hl
                        dec   (ix+6)
                        jr    z,getcharloc_largechar
                        call  _GetSmallCharWidth
                        jr    getcharloc_continue
getcharloc_largechar:   call  getLargeCharPtr
getcharloc_continue:
                  pop   af
            pop   af
      pop   ix
      ret
      
;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
;arg0 = char, arg1 = pointer_to_fonstruct
getLargeCharPtr:
      ld    hl,3
      add   hl,sp
      ld    a,(hl)      ;character
      or    a,a
      jr    z,getanysizecharacter_useeditbuf
      inc   hl
      inc   hl
      inc   hl
      ld    de,(hl)
      sbc   hl,hl
      ld    l,a
      add   hl,de       ;address to char mapping
      ld    L,(hl)
      inc   L
getlargechar_instrsmc:
      jr    z,glcp_usedefault
      dec   L
      ld    h,28
      mlt   hl
      inc   h
      add   hl,de
      ret
glcp_usedefault:
      ld    iy,flags
      ld    bc,(iy+$34)
      ld    (iy+$35),0
      set   2,(iy+$32)
      push  bc
            call  _LoadPattern
      pop   bc
      res   2,(iy+$32)
      ld    (iy+$34),bc
      ld    hl,lFont_record+1
      ret
getanysizecharacter_useeditbuf:
      ld    hl,_editbuf
      ret
      
;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
;arg0 = char, arg1 = pointer_to_fonstruct
PrintDefaultLargeChar:
      ld    a,$18 ;jr opcode
      jr    $+4
_PrintLargeChar:              ;-3
      ld    a,$28 ;jr z opcode
      ld    (getlargechar_instrsmc),a
      push  ix                ;-6
      ;-
      ld    ix,6
      add   ix,sp	      ;-3 ret, -6 first push, -9 second push, etc
      call  _gfx_GetTextY
      push  hl                ;-9  texty arg
      call  _gfx_GetTextX
      push  hl                ;-12 textx arg
      call  printchar_setcolor
      ld    hl,(ix+3)
      push  hl
            ld    hl,(ix+0)
            push  hl
                  call getLargeCharPtr
            pop   af
      pop   af
      push  hl
            ld    L,(ix-9)    ;get ypos
            ld    h,160
            mlt   hl
            add   hl,hl
            ld    de,(ix-12)  ;get xpos
            add   hl,de
            ld    bc,(DRAW_BUFFER)
            add   hl,bc
            ex    de,hl
            ld    bc,14
            add   hl,bc
            ld    (ix-12),hl  ;change xpos
            ex    de,hl
      pop   de
      ld    c,14
printlargechar_drawloop:
      push  hl
            ld    b,5
            call  printchar_drawbits
            ld    b,7
            call  printchar_drawbits
            ld    hl,320
            ld    a,c
      pop   bc
      add   hl,bc
      ld    c,a
      dec   c
      jr    nz,printlargechar_drawloop
printlargechar_end:
      call  _gfx_SetTextXY    ;arguments were pushed to stack already
      pop   af
      pop   af    ;unwind stack args for settextxy
      pop   ix
      ret
;------------------------------------------------------------------------------
;arg0 = char, arg1 = objpointer
;returns: HL=address_to_smallchar_object (get byte at it to retrieve width)
_GetSmallCharWidth:
      ld    hl,3
      add   hl,sp
      ld    a,(hl)      ;character
      or    a,a
      jr    z,getanysizecharacter_useeditbuf       ;gotta draw something.
      inc   hl
      inc   hl
      inc   hl
      ld    de,(hl)
      sbc   hl,hl
      ld    l,a
      add   hl,de       ;address to char mapping
      ld    L,(hl)
      inc   L
getsmallchar_instrsmc:        ;change between jr z and unconditional
      jr    z,gscw_usedefault
      dec   L
      ld    a,(de)      ;numchars in array
      ld    c,a
      ld    b,28
      mlt   bc
      inc   b           ;sizeof array+largefont
      ld    h,25
      mlt   hl          ;smallfont char
      add   hl,bc       ;offset from start of data struct to sfont chr
      add   hl,de       ;address completed
      ret
gscw_usedefault:
      ld    iy,flags
      ld    bc,(iy+$34)
      ld    (iy+$35),0
      res   2,(iy+$32)
      push  bc
            call  _LoadPattern
      pop   bc
      ld    (iy+$34),bc
      ld    hl,sFont_record
      ret
      
;------------------------------------------------------------------------------
;arg0 = char, arg1 = objpointer
;returns: HL=address_to_smallchar_object (get byte at it to retrieve width)
PrintDefaultSmallChar:
      ld    a,$18 ;jr opcode
      jr    $+4
_PrintSmallChar:
      ld    a,$28 ;jr z opcode
      ld    (getlargechar_instrsmc),a
      push  ix                ;-6
      ;-
      ld    ix,6
      add   ix,sp	      ;-3 ret, -6 first push, -9 second push, etc
      call  _gfx_GetTextY
      push  hl                ;-9  texty arg
      call  _gfx_GetTextX
      push  hl                ;-12 textx arg
      call  printchar_setcolor
      ld    hl,(ix+3)
      push  hl
            ld    hl,(ix+0)
            push  hl
                  call  _GetSmallCharWidth
            pop   af
      pop   af
      ld    a,(hl)            ;char width
      inc   hl
      or    a,a
      jr    z,printsmallchar_end    ;don't try to draw 0 width chars
      push  hl
            ld    L,(ix-9)    ;get ypos
            ld    h,160
            mlt   hl
            add   hl,hl
            ld    de,(ix-12)  ;get xpos
            add   hl,de
            ld    bc,(DRAW_BUFFER)
            add   hl,bc
            ex    de,hl
            ld    bc,0
            ld    c,a
            add   hl,bc
            ld    (ix-12),hl  ;change xpos
            ex    de,hl
      pop   de
      ld    c,12
      cp    a,9
      jr    nc,printsmallchar_drawlong
      ld    (printsmallchar_drawshort+1),a
printsmallchar_drawshort:
      ld    b,0   ;smc'd in.
      push  hl
            call  printchar_dawbitscarefully
            ld    hl,320
            ld    a,c
      pop   bc
      add   hl,bc
      ld    c,a
      dec   c
      jr    nz,printsmallchar_drawshort
      jr    printsmallchar_end
printsmallchar_drawlong:
      sub   a,8
      ld    (printsmallchar_drawlongsmc+1),a
printsmallchar_drawlongloop:
      push  hl
            ld    b,8
            call  printchar_drawbits
printsmallchar_drawlongsmc:
            ld    b,0   ;smc'd in
            call  printchar_dawbitscarefully
            ld    hl,320
            ld    a,c
      pop   bc
      add   hl,bc
      ld    c,a
      dec   c
      jr    nz,printsmallchar_drawlongloop
printsmallchar_end:
      ;call  _gfx_SetTextXY
      pop   af
      pop   af
      pop   ix
      ret

;--------------------------------------
printchar_setcolor:
      push  hl
            call  _gfx_SetTextFGColor
            ld    l,a
            ex    (sp),hl
            call  _gfx_SetTextFGColor
      pop   hl
      ld    a,l
      ld    (printchar_color_smc),a
      ret
      
printchar_dawbitscarefully:
      dec   b
      ret   m           ;yeah, no. Don't draw if zerosized or negative.
      inc   b
printchar_drawbits:     ;in: B=bits to draw, DE=bitfield, HL=bufptr
      ld    a,(de)
      inc   de
printchar_drawbitsloop:
      add   a,a
      jr    nc,$+4
printchar_color_smc equ $+1
      ld    (hl),0
      inc   hl
      djnz  printchar_drawbitsloop
      ret

;------------------------------------------------------------------------------      
;------------------------------------------------------------------------------ 

;arg0 = x, arg1 = y, arg2 = fontid

_GetFontBit:
      call  fontbit_getmask
      and   a,(hl)
      ret
_SetFontBit:
      call  fontbit_getmask
      or    a,(hl)
      ld    (hl),a
      ret
_ResFontBit:
      call  fontbit_getmask
      cpl
      and   a,(hl)
      ld    (hl),a
      ret
_InvFontBit:
      call  fontbit_getmask
      ld    c,a
      xor   a,(hl)
      ld    (hl),a
      and   a,c   ;get the bit that was changed and return it
      ret

fontbit_getmask:
      ;ld    a,2
      ;ld    (-1),a
      ld    de,0
      ld    hl,_editbuf
      ld    iy,6
      add   iy,sp
      ld    e,(iy+3)    ;y, save in E for offsetting
      ld    c,(iy+0)    ;x, save in C for ease of reference
      ld    a,(iy+6)    ;fontid
      or    a,a
      jr    nz,fontbit_getmasklarge
      ld    a,(hl)
      inc   hl
      cp    a,9
      jr    nc,fontbit_getmaskshortshort
      add   hl,de
      add   hl,de       ;get correct row
      ld    a,c         ;x
      and   a,$F8
      jr    z,$+3
      inc   hl          ;if gt 7, is second byte
      ld    a,c
      and   a,7
      jr    fontbit_maskout
fontbit_getmaskshortshort
      add   hl,de
      ld    a,c
      and   a,7
      jr    fontbit_maskout
fontbit_getmasklarge:
      add   hl,de
      add   hl,de
      ld    a,c
      cp    a,5
      jr    c,fontbit_getmasklargefirstbyte
      sub   a,5
      inc   hl
fontbit_getmasklargefirstbyte:
fontbit_maskout:  ;hl=adr, A = modded X
      ld    b,a
      or    a,a
      ld    a,$80
      ret   z     ;modded x was 0. Mask is automatically $80.
fontbit_maskoutloop:
      rrca
      djnz  fontbit_maskoutloop
      ret
      
      
      
      
      
      






     
;------------------------------------------------------------------------------      
;------------------------------------------------------------------------------      
      
_SetupPalette:
	LD    HL,0E30019h
	RES   0,(HL)       ;Reset BGR bit to make our mapping correct
	LD	BC,0
	LD	IY,0E30200h  ;Address of palette
;palette index format: IIRRGGBB palette entry: IBBBBBGG GGGRRRRR
setupPaletteLoop:
	LD	HL,0
	;PROCESS BLUE. TARGET 0bbii0--
	LD	A,B
	RRCA               ;BIIRRGGB
	LD    E,A          ;Keep for red processing
	RRCA               ;BBIIRRGG
	LD	C,A          ;Keep for green processing
	RRCA               ;GBBIIRRG
	AND	A,01111000b  ;0BBII000
	LD	H,A          ;Blue set.
	;PROCESS GREEN. TARGET ii0000gg, MASK LOW NIBBLE INTO HIGH BYTE
	LD    A,C           ;BBIIRRGG
	XOR	H            ;xxxxxxyy
	AND	A,00000011b  ;keep low bits to mask back to original
	XOR	H            ;0BBII0GG
	LD	H,A          ;Green high set (------GG)
	LD	L,B          ;Green low set  (II------)
	;PROCESS RED. TARGET 000rrii0
	LD	A,B          ;IIRRGGBB
	RLC   A            ;IRRGGBBI      
	RLC   A            ;RRGGBBII      
	RLC   A            ;RGGBBIIR
	XOR	E            ;-----xx-
	AND	A,00000110b
	XOR	E            ;biiRRIIb
	XOR   A,L          ;---xxxx-
	AND   A,00011110b
	XOR	L            ;IIxRRIIx
	AND	A,11011110b  ;II0RRII0
	LD	L,A
      SET   7,H
	LD	(IY+0),HL
	LEA   IY,IY+2
	INC   B
	JR    NZ,setupPaletteLoop
	RET
      
      
;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
;This routine only exists because C doesn't allow definition of binary
;constants, which is pretty important when directly defining 1bpp graphics.

;a0=idx/chr, a1=*data
XREF _gfx_SetCharData
;no args. chars loaded at graphx codepoints 1-5
_LoadCursors:
      ld    c,1
      ld    b,5
      ld    hl,loadcursors_data
loadcursors_loop:
      push  hl
            push  bc
                  call  _gfx_SetCharData
            pop   bc
      pop   hl
      ld    de,8
      add   hl,de
      inc   c
      djnz  loadcursors_loop
      ret
      
loadcursors_data:
;+0 - solid block
db 11111110b
db 11111110b
db 11111110b
db 11111110b
db 11111110b
db 11111110b
db 11111110b
db 11111110b
;+1 - "A" alpha
db 11111110b
db 11001110b
db 10110110b
db 10110110b
db 10000110b
db 10110110b
db 10110110b
db 11111110b
;+2 - 'a' alpha
db 11111110b
db 11001110b
db 11110110b
db 11000110b
db 10110110b
db 10110110b
db 11000110b
db 11111110b
;+3 - '1' numeric
db 11111110b
db 11101110b
db 11001110b
db 11101110b
db 11101110b
db 11101110b
db 11000110b
db 11111110b
;+4 - checkboard pattern
db 01010100b
db 10101010b
db 01010100b
db 10101010b
db 01010100b
db 10101010b
db 01010100b
db 10101010b

;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
;Allows more optimized and fine-tuned bit manipulations in assembly.

;arg0 = key, arg1 = mode/cursor (1:none,2:alpha,3:lower,4:num,5:block)
;returns: A = char
_KeyToChar:
      ld    hl,6
      add   hl,sp
      ld    b,(hl)
      res   7,b   ;remove persist flag
      dec   hl
      dec   hl
      dec   hl
      ld    a,(hl)
      dec   a     ;align results to keygroups
      cp    a,9   ;arrow keys disallowed
      jr    c,keytochar_disallow
      cp    a,48  ;keys higher than alpha disallowed (no printables)
      jr    nc,keytochar_disallow
      djnz  keytochar_nums    ;no mode? do only nums
      djnz  keytochar_alpha
      djnz  keytochar_lower
      djnz  keytochar_nums
      ;anything above 4 is disallowed.
keytochar_disallow:
      xor   a,a
      ret
keytochar_nums:
      ld    hl,keytochar_numtable
      ld    bc,10
      ld    e,$30+10
      jr    keytochar_compare
keytochar_lower:
      ld    e,$61+26
      jr    keytochar_alpha+2
keytochar_alpha:
      ld    e,$41+26
      ld    hl,keytochar_alphatable
      ld    bc,26
keytochar_compare:
      cpir
      jr    nz,keytochar_disallow
      ld    a,c
      cpl
      add   a,e
      ret   ;62

keytochar_numtable:
;  0  1  2  3  4  5  6  7  8  9  
db 33,34,26,18,35,27,19,36,25,20
keytochar_alphatable:
;  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z
db 47,39,31,46,38,30,22,14,45,37,29,21,13,44,36,28,20,12,43,35,27,19,11,42,34,26
;Total: 98 bytes

