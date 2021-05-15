org	0x7c00   

[BITS 16]

START:
	jmp	BOOT1_LOAD

;문자열 출력을 위한 함수들을 구현
print_char:
	mov	ah, 0x0E
	mov	bh, 0x00
	mov	bl, 0x07
	int	0x10
	ret

print_string:
	nextc:
		mov	al, [si]
		inc	si
		cmp	al, 0
		je	exit
		call	print_char
		jmp	nextc
		exit:	ret

;[0]을 [ ]로 다시 출력하기 위한 함수
RESET_CURSOR:
	mov	ah, 03h
	int	10h
	sub	dl, 3
	mov	ah, 2
	int	10h
	mov	si, blank
	call	print_string
	ret	

;화면에 출력된 모든 결과물을 지우는 함수
cls:
	pusha
	mov	ah, 0x00
	mov	al, 0x03
	int	0x10
	popa
	ret

BOOT1_LOAD:
	mov     ax, 0x0900 
        mov     es, ax
        mov     bx, 0x0

        mov     ah, 2	
        mov     al, 0x4		
        mov     ch, 0	
        mov     cl, 2	
        mov     dh, 0		
        mov     dl, 0x80

        int     0x13	
        jc      BOOT1_LOAD

;커널을 선택하는 창을 띄우는 프로시져
SELECT:
	call	cls
	mov	si, ssuos_1
	call	print_string
	mov	si, tab
	call	print_string
	mov	si, ssuos_2
	call	print_string
	mov	si, next_line
	call	print_string
	mov	si, ssuos_3
	call	print_string
	mov	dh, 0
	mov	dl, 0
	mov	bh, 0
	mov	ah, 2
	int	10h
	mov	si, select
	call	print_string
;hide blinking text cursor
	mov	ch,32
	mov	ah,1
	int	10h

;1번 커널을 선택하는 프로시져
CURSOR_SSU1:
;set chs variables' value
	mov byte [cylinder], 0
	mov byte [head], 0
	mov byte [sector], 6
;set previous cursor as [ ]
	call	RESET_CURSOR

;set cursor position
	mov	dh, 0
	mov	dl, 0
	mov	bh, 0
	mov	ah, 2
	int	10h
;print [0]
	mov	si, select
	call	print_string
;get key
	mov	ah, 00h
	int	16h
;compare
	cmp	ah, 0x4D
	je	CURSOR_SSU2
	cmp	ah, 0x50
	je	CURSOR_SSU3
	cmp	ah, 0x1C
	je	KERNEL_LOAD
	jne	CURSOR_SSU1


;2번 커널을 선택하는 프로시져
CURSOR_SSU2:
;set chs variables' value
	mov byte [cylinder], 9
	mov byte [head], 14
	mov byte [sector], 47
;set previous cusor as [ ]
	call	RESET_CURSOR

;set cursor position
	mov	dh, 0
	mov	dl, 12
	mov	bh, 0
	mov	ah, 2
	int	10h
;print [0]
	mov	si, select
	call	print_string

	mov	ah, 00h
	int	16h
	
;compare
	cmp	ah, 0x4B
	je	CURSOR_SSU1
	cmp	ah, 0x1C
	je	KERNEL_LOAD
	jne	CURSOR_SSU2

;3번 커널을 선택하는 프로시져
CURSOR_SSU3:
;set chs variables' value
	mov byte [cylinder], 14
	mov byte [head], 14
	mov byte [sector], 7
;set previous cusor as [ ]
	call	RESET_CURSOR

;set cursor position
	mov	dh, 1
	mov	dl, 0
	mov	bh, 0
	mov	ah, 2
	int	10h
;print [0]
	mov	si, select
	call	print_string

	mov	ah, 00h
	int	16h
	
;compare
	cmp	ah, 0x48
	je	CURSOR_SSU1
	cmp	ah, 0x1C
	je	KERNEL_LOAD
	jne	CURSOR_SSU3


;커널을 로드하는 프로시져
KERNEL_LOAD:
	mov     ax, 0x1000	
	mov     es, ax		
        mov     bx, 0x0		

        mov     ah, 2		
        mov     al, 0x3f
	mov	ch, [cylinder]
	mov	cl, [sector]
	mov	dh, [head]     
        mov     dl, 0x80

        int     0x13

        jc      KERNEL_LOAD

jmp		0x0900:0x0000

;필요한 변수를 선언
blank db "[ ]",0
select db "[O]",0
ssuos_1 db "[ ] SSUOS_1",0
ssuos_2 db "[ ] SSUOS_2",0
ssuos_3 db "[ ] SSUOS_3",0
ssuos_4 db "[ ] SSUOS_4",0
tab db "	",0
next_line db 13,10,0
cylinder db 0
head db 0
sector db 0
partition_num : resw 1

times   446-($-$$) db 0x00

;MBR Table
PTE:
partition1 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition2 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition3 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x98, 0x3a, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition4 db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
times 	510-($-$$) db 0x00
dw	0xaa55
