section .bss
characterr_ : resb 1
section .data
enterr_ db 10
infile dd 0
outfile dd 1
section .text
global _start
global system_call
;extern main
extern strlen
_start:
    pop    dword ecx    ; ecx = argc
    mov    esi,esp      ; esi = argv
    ;; lea eax, [esi+4*ecx+4] ; eax = envp = (4*ecx)+esi+4
    mov     eax,ecx     ; put the number of arguments into eax
    shl     eax,2       ; compute the size of argv in bytes
    add     eax,esi     ; add the size to the address of argv 
    add     eax,4       ; skip NULL at the end of argv
    push    dword eax   ; char *envp[]
    push    dword esi   ; char* argv[]
    push    dword ecx   ; int argc

    call    main        ; int main( int argc, char *argv[], char *envp[] )

    mov     ebx,eax
    mov     eax,1
    int     0x80
    nop
        
system_call:
    push    ebp             ; Save caller state
    mov     ebp, esp
    sub     esp, 4          ; Leave space for local var on stack
    pushad                  ; Save some more caller state

    mov     eax, [ebp+8]    ; Copy function args to registers: leftmost...        
    mov     ebx, [ebp+12]   ; Next argument...
    mov     ecx, [ebp+16]   ; Next argument...
    mov     edx, [ebp+20]   ; Next argument...
    int     0x80            ; Transfer control to operating system
    mov     [ebp-4], eax    ; Save returned value...
    popad                   ; Restore caller state (registers)
    mov     eax, [ebp-4]    ; place returned value where caller can see it
    add     esp, 4          ; Restore caller state
    pop     ebp             ; Restore caller state
    ret                     ; Back to caller

main:
    push    ebp             ; Save caller state
    mov     ebp, esp
    pushad                  ; Save some more caller state
    ;argc
    mov edi, [ebp+8]
    ;argv
    mov esi, [ebp+12]
print_loop:
    mov ebx, [esi]
    cmp word[ebx], "-i"
    je in_open
    cmp word[ebx], "-o"
    je out_open
print_argument:
    push dword [esi]
    call strlen
    add esp, 4
    mov edx, eax
    mov eax, 4
    mov ebx, 1
    mov ecx, [esi]
    int 0x80
    mov eax, 4
    mov ebx, 1
    mov ecx, enterr_
    int 0x80

    add esi, 4
    sub edi, 1
    cmp edi, 0
    jne print_loop

encode:
    ; Read one byte from infile
    mov eax, 3
    mov ebx, dword[infile]
    mov ecx, characterr_
    mov edx, 1
    int 0x80
    cmp eax, 0
    jle exiit_ ; Exit if end of file

    ; Check if character is 'z', map to 'a'
    cmp byte[characterr_], 'z'
    je map_to_a

    ; Check if character is 'Z', map to 'A'
    cmp byte[characterr_], 'Z'
    je map_to_A

    ; Check if character is between 'a' and 'y', increment by 1
    cmp byte[characterr_], 'a'
    jl check_uppercase ; If below 'a', check uppercase range
    cmp byte[characterr_], 'y'
    jle add_one_lowercase

check_uppercase:
    ; Check if character is between 'A' and 'Y', increment by 1
    cmp byte[characterr_], 'A'
    jl skipp ; Skip non-alphabetic characters
    cmp byte[characterr_], 'Y'
    jle add_one_uppercase
    jmp skipp

map_to_a:
    mov byte[characterr_], 'a'
    jmp output

map_to_A:
    mov byte[characterr_], 'A'
    jmp output

add_one_lowercase:
    add byte[characterr_], 1
    jmp output

add_one_uppercase:
    add byte[characterr_], 1
    jmp output

skipp:
output:
    ; Write the processed character to outfile
    mov eax, 4
    mov ebx, dword[outfile]
    mov ecx, characterr_
    mov edx, 1
    int 0x80
    jmp encode ; Continue processing next character
exiit_:
    popad                   ; Restore caller state (registers)
    pop     ebp             ; Restore caller state
    ret                     ; Back to caller
    
out_open:
    mov eax, 5
    add ebx, 2
    mov ecx, 1101o
    mov edx, 644o
    int 0x80
    cmp eax, 0
    jl exiit_
    mov dword[outfile], eax
    jmp print_argument
in_open:
    mov eax, 5
    add ebx, 2
    mov ecx, 0
    int 0x80
    cmp eax, 0
    jl exiit_
    mov dword[infile], eax
    jmp print_argument