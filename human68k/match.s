* match.s -- optional optimized asm version of longest match in deflate.c
* Copyright (C) 1992-1993 Jean-loup Gailly
*
* Adapted for X68000 by NIIMI Satoshi <a01309@cfi.waseda.ac.jp>
* Adapted for the Amiga by Carsten Steger <stegerc@informatik.tu-muenchen.de>
* using the code in match.S.
* The major change in this code consists of removing all unaligned
* word accesses, because they cause 68000-based machines to crash.
* For maximum speed, UNALIGNED_OK can be defined.
* The program will then only run on 68020-based machines, though.


Cur_Match       reg     d0      ; Must be in d0!
Best_Len        reg     d1
Loop_Counter    reg     d2
Scan_Start      reg     d3
Scan_End        reg     d4
Limit           reg     d5
Chain_Length    reg     d6
Scan_Test       reg     d7
Scan            reg     a0
Match           reg     a1
Prev_Address    reg     a2
Scan_Ini        reg     a3
Match_Ini       reg     a4

MAX_MATCH       equ     258
MIN_MATCH       equ     3
WSIZE           equ     32768
MAX_DIST        equ     WSIZE-MAX_MATCH-MIN_MATCH-1


        .xref   _max_chain_length
        .xref   _prev_length
        .xref   _prev
        .xref   _window
        .xref   _strstart
        .xref   _good_match
        .xref   _match_start
        .xref   _nice_match


        .xdef   _match_init
        .xdef   _longest_match

        .text
        .even


_match_init:
        rts


_longest_match:
        move.l  4(sp),Cur_Match
.ifdef  UNALIGNED_OK
        movem.l d2-d6/a2-a4,-(sp)
.else
        movem.l d2-d7/a2-a4,-(sp)
.endif
        move.l  _max_chain_length,Chain_Length
        move.l  _prev_length,Best_Len
        lea     _prev,Prev_Address
        lea     _window+MIN_MATCH,Match_Ini
        move.l  _strstart,Limit
        move.l  Match_Ini,Scan_Ini
        add.l   Limit,Scan_Ini
        subi.w  #MAX_DIST,Limit
        bhi.b   limit_ok
        moveq   #0,Limit
limit_ok:
        cmp.l   _good_match,Best_Len
        bcs.b   length_ok
        lsr.l   #2,Chain_Length
length_ok:
        subq.l  #1,Chain_Length

.ifdef  UNALIGNED_OK
        move.w  -MIN_MATCH(Scan_Ini),Scan_Start
        move.w  -MIN_MATCH-1(Scan_Ini,Best_Len.w),Scan_End
.else
        move.b  -MIN_MATCH(Scan_Ini),Scan_Start
        lsl.w   #8,Scan_Start
        move.b  -MIN_MATCH+1(Scan_Ini),Scan_Start
        move.b  -MIN_MATCH-1(Scan_Ini,Best_Len.w),Scan_End
        lsl.w   #8,Scan_End
        move.b  -MIN_MATCH(Scan_Ini,Best_Len.w),Scan_End
.endif

        bra.b   do_scan

long_loop:

.ifdef  UNALIGNED_OK
        move.w  -MIN_MATCH-1(Scan_Ini,Best_Len.w),Scan_End
.else
        move.b  -MIN_MATCH-1(Scan_Ini,Best_Len.w),Scan_End
        lsl.w   #8,Scan_End
        move.b  -MIN_MATCH(Scan_Ini,Best_Len.w),Scan_End
.endif

short_loop:
        lsl.w   #1,Cur_Match
        move.w  0(Prev_Address,Cur_Match.l),Cur_Match
        cmp.w   Limit,Cur_Match
        dbls    Chain_Length,do_scan
        bra.b   return

do_scan:
        move.l  Match_Ini,Match
        add.l   Cur_Match,Match

.ifdef  UNALIGNED_OK
        cmp.w   -MIN_MATCH-1(Match,Best_Len.w),Scan_End
        bne.b   short_loop
        cmp.w   -MIN_MATCH(Match),Scan_Start
        bne.b   short_loop
.else
        move.b  -MIN_MATCH-1(Match,Best_Len.w),Scan_Test
        lsl.w   #8,Scan_Test
        move.b  -MIN_MATCH(Match,Best_Len.w),Scan_Test
        cmp.w   Scan_Test,Scan_End
        bne.b   short_loop
        move.b  -MIN_MATCH(Match),Scan_Test
        lsl.w   #8,Scan_Test
        move.b  -MIN_MATCH+1(Match),Scan_Test
        cmp.w   Scan_Test,Scan_Start
        bne.b   short_loop
.endif

        move.w  #(MAX_MATCH-MIN_MATCH),Loop_Counter
        move.l  Scan_Ini,Scan
scan_loop:
        cmpm.b  (Match)+,(Scan)+
        dbne    Loop_Counter,scan_loop

        sub.l   Scan_Ini,Scan
        addq.l  #(MIN_MATCH-1),Scan
        cmp.l   Best_Len,Scan
        bls.b   short_loop
        move.l  Scan,Best_Len
        move.l  Cur_Match,_match_start
        cmp.l   _nice_match,Best_Len
        bcs.b   long_loop
return:
        move.l  Best_Len,d0
.ifdef  UNALIGNED_OK
        movem.l (sp)+,d2-d6/a2-a4
.else
        movem.l (sp)+,d2-d7/a2-a4
.endif
        rts

        end
