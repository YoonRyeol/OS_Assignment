BITS 16				;이하 코드를 16비트 모드에서 수행한다.
org		0x7C00		;메모리 0x7C00번지에 코드를 올린다.

jmp start			;start로 점프한다.

msg_start db "Hello, Yeol's World",13,10,0

print_char:			;int 10h, ah Eh를 사용하여 문자를 출력
	mov ah, 0x0E
	mov bh, 0x00
	mov bl, 0x07
	int 0x10
	ret

print_string:			;print_char을 이용하여 문자열의 모든 문자를 출력한다.
	nextc:
		mov al, [si]
		inc si
		cmp al, 0
		je exit
		call print_char
		jmp nextc
		exit: ret
cls:				;int 10h ah 0h를 사용하여 화면을 지운다.
	pusha
	mov ah, 0x00
	mov al, 0x03
	int 0x10
	popa
	ret

start:
	mov ax, 0xb800
	mov es, ax
	mov ax, 0x00
	mov bx, 0
	mov cx, 80*25*2

	call cls		;화면을 초기화
	mov si, msg_start	;si레지스터에 문자열의 주소값을 대입
	call print_string	;문자열을 출력
