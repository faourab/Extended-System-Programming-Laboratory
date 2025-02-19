section .bss
    struct: resd 1
    buffer_reader: resb 600

section .data
    x_struct: dw 5
    x_num: dw 0xaa, 1,2,0x44,0x4f
    y_struct: dw 6
    y_num: dw 0xaa, 1,2,3,0x44,0x4f
    test: db "hello" , 0
    STATE: dw 0xACE1
    smaller: db 0
    part1_message: db "part1", 0 
    part2_message: db "part2", 0 
    part3_message: db "part3", 0 
    bigger: db 0
    MASK: dw 0x002D
    newline: db 10,0
    print_format: dw "%02hhx", 0
    word_print_format: db "%04hx", 0
    space: db " ", 0 
    count: db 0


section .text
global main
extern malloc
extern strlen
extern fgets
extern stdin
extern printf
extern free
extern puts
main:


stast:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]        ;argc
    mov ebx, [ebp+12]       ;argv

comparings:
    cmp eax, 1              
    je no_arguments          
    mov eax, [ebx + 4]       
    cmp word[eax], "-R"      ;"-R"
    je R
    cmp word[eax], "-I"      ;"-I"
    je I
    jmp exit

part1:
    add ecx,4
    dec ecx
    push part1_message
    call puts
    add esp,4
    ret

exit: ;exiting program
    pop ebp
    ret    

no_arguments:
    call part1
    push x_struct           
    push y_struct          
    call add_multi1       
    push eax               
    call print_multi_words   
    call free              
    add esp, 12            
    pop ebp
    ret                    

part2:
    add ecx,4
    dec ecx
    push part2_message
    call puts
    add esp,4
    ret
    
I:
    call part2
    call getmulti           
    push eax              
    call getmulti         
    push eax               
    call add_multi        
    push eax             
    call print_multi       
    call free               
    add esp, 4             
    call free              
    add esp, 4                     
    call free             
    add esp, 4            
    pop ebp
    ret       

part3:
    add ecx,4
    dec ecx
    push part3_message
    call puts
    add esp,4
    ret

R:
    call part3
    call PRmulti            
    push eax               
    call PRmulti            
    push eax               
    call add_multi         
    push eax                
    call print_multi        
    call free               
    add esp, 4             
    call free              
    add esp, 4                     
    call free             
    add esp, 4            
    pop ebp
    ret                                                                             

;/////////////////////   Task 1 a ///////////////////////////
print_multi:
    push ebp                  
    mov ebp, esp               
    pushad                      
    mov edi, [ebp+8]            
    movzx ebx, byte[edi]        ;struct length (struct.size)

;loop over the struct
looper:
    movzx ecx, byte[edi + ebx ]  ;always taking from the end (little endian)
    push ecx                    
    push print_format                 
    call printf        
    add esp, 8                  
    dec ebx               ;next     
    cmp bl, 0                   
    jne looper              ;continue if not the end
    
printing_new_line:
    push newline                ;"\n"
    call printf                 
    add esp, 4                  
    popad                       
    pop ebp                    
    ret                         ;return to main

print_multi_words:
    push ebp                    ; Save the base pointer
    mov ebp, esp                ; Set up the stack frame
    pushad                      ; Save all general-purpose registers
    mov edi, [ebp+8]            ; Address of the structure
    movzx ebx, byte[edi]        ; Get the size of the structure (1 byte)

print_words_loop:
    movzx ecx, word[edi + (ebx*2)]  ; Access with corrected alignment
    push ecx
    push word_print_format
    call printf
    add esp, 8
    dec ebx
    cmp ebx, 0
    jne print_words_loop
    push newline
    call printf
    add esp, 4
    popad
    pop ebp
    ret    

;/////////////////////   Task 1 b ///////////////////////////

 getmulti:                                                                                                                                                                                                                                                                                                                                                                                      
    push ebp                   
    mov ebp, esp       
    pushad        

input_reading:    
    push dword[stdin]          
    push 600                  
    push buffer_reader              
    call fgets                  ;fgets(buffer_reader, 600, stdin)
    add esp, 12     

input_size:
    push buffer_reader                
    call strlen                 ;eax = strlen(buffer_reader);
    add esp, 4                 
    mov edi, eax    

resign_the_size:        
    dec edi
    dec edi                
    shr eax, 1                  
    inc eax 

malloc_for_new_struct:             
    push eax                    
    call malloc                 ;eax = malloc(strlen result)
    mov dword[struct], eax      
    mov esi, eax                
    pop eax                     
    dec eax                    
    mov byte[esi], al         
    mov ecx, 1     

parsing_input:
    mov ebx, 0                  ;reset
    mov bh, byte[buffer_reader + edi]  ;take the first char
    dec edi                         
    cmp bh, 'a'                 ;check if the digit is a number or letter
    jge character                  
    sub bh, '0'                
    jmp swaping

character:
    sub bh, 'a'                
    add bh, 0xa                

swaping:
    mov bl, bh                  
    mov bh, 0                  
    cmp edi, 0
    jl construct
    mov bh, byte[buffer_reader + edi]  ;take the second char
    dec edi                     
    cmp bh, 'a'                 ;check if the second digit is a number or letter
    jge second_character                   
    add bh, 0xa       

second_character:
    sub bh, 'a'                
    add bh, 0xa                

construct:
    shl bh, 4                 
    or bl, bh                 

add_struct:
    mov byte[esi + ecx], bl    
    inc ecx                    
    cmp edi, 0                  
    jge parsing_input           ;jump to end if we finished

end:
    jmp saving_value                      
    ret                                               

;/////////////////////   Task 2 a ///////////////////////////

MaxMin:
    movzx edx, byte[ebx]      
    movzx ecx, byte[eax]                
    cmp ecx, edx                 
    jae no_swap                     ;jae checks unsigned numbers
    xchg eax, ebx             
    no_swap:            
    ret                           

;/////////////////////   Task 2 b ///////////////////////////

add_multi:
    push ebp              
    mov ebp, esp              
    pushad                     
    mov eax, [ebp+8]            
    mov ebx, [ebp+12]          
    
print_inputs:
    push ebx                    
    call print_multi            ;print_multi(second input) 
    add esp, 4   
    push eax                  
    call print_multi          ;print_multi(first input) 
    add esp, 4              
    call MaxMin                 ;swap eax and ebx so that eax is the bigger number              

saving_each_input_for_looping:
    mov edi, ebx    
    mov esi, eax                 
    movzx eax, byte[edi]        
    mov byte[smaller], al 
    movzx eax, byte[esi]        
    mov byte[bigger], al   
    inc eax
    inc eax              
    push eax                  
    call malloc                
    mov dword[struct], eax    
    pop ecx                    
    ;sub ecx,
    dec ecx  
    mov byte[eax], cl   

update_values:        
    mov ecx, 0                  
    mov edx, 0                 
    inc esi                     ;esi is now pointed to the first input num
    inc edi                     ;edi is now pointed to the second input num
    inc eax                     ;eax is now pointed to the result  num

loop_on_nums_for_adding:
    xor ecx, ecx                ; Clear carry flag
    movzx ebx, word[esi]        ; Load word from first number
    add ebx, ecx                ; Add previous carry
    movzx ecx, word[edi]        ; Load word from second number
    add ebx, ecx                ; Add numbers
    mov word[eax], bx           ; Store result
    shr ebx, 16 

update_the_values:
    add edx, 2
    add esi, 2
    add edi, 2
    add eax, 2
    movzx ecx, byte[smaller]
    add ecx, ecx                ; Multiply by 2 for word-size comparison
    cmp dl, cl
    jb loop_on_nums_for_adding
    ; Handle remaining digits from longer number
    movzx ecx, byte[bigger]
    add ecx, ecx                ; Multiply by 2 for word-size comparison
    cmp dl, cl
    jae skip_remaining

remain_loop:
    movzx ebx, word[esi]
    add ebx, ecx                ; Add carry from previous operation
    mov word[eax], bx
    shr ebx, 16                 ; Get carry for next iteration
    mov ecx, ebx                ; Save carry
    add edx, 2
    add esi, 2
    add eax, 2
    movzx ebx, byte[bigger]
    add ebx, ebx                ; Multiply by 2 for word-size comparison
    cmp dl, bl
    jb remain_loop

skip_remaining:
    test ecx, ecx               ; Check if there's still a carry
    jz saving_value
    mov word[eax], cx           ; Store final carry if exists

saving_value:
    popad
    pop ebp
    mov eax, [struct]
    ret    

add_multi1:
    push ebp              
    mov ebp, esp              
    pushad                     
    mov eax, [ebp+8]           ; First number struct pointer
    mov ebx, [ebp+12]          ; Second number struct pointer
    ; Print inputs
    push ebx                    
    call print_multi_words      
    add esp, 4   
    push eax                  
    call print_multi_words      
    add esp, 4              

    
    ; Store original pointers for length comparison
    push eax                   ; Save first struct pointer
    push ebx                   ; Save second struct pointer
    
    ; Get and compare lengths
    movzx ecx, word[eax]       ; Get first length
    movzx edx, word[ebx]       ; Get second length
    
    ; Store the smaller and bigger lengths
    cmp ecx, edx
    jbe first_is_smaller
    mov byte[bigger], cl
    mov byte[smaller], dl
    jmp lengths_stored
first_is_smaller:
    mov byte[bigger], dl
    mov byte[smaller], cl
lengths_stored:
    
    ; Move pointers past the length fields to the actual numbers
    pop ebx                    ; Restore second struct pointer
    pop eax                    ; Restore first struct pointer
    add eax, 2                 ; Skip past length field
    add ebx, 2                 ; Skip past length field
    
    ; Save number array pointers
    mov esi, eax               ; First number array
    mov edi, ebx               ; Second number array
    
    ; Allocate result structure
    movzx eax, byte[bigger]    ; Get bigger length
    inc eax                    ; Add 1 for possible carry
    push eax                   ; Save length for malloc
    shl eax, 1                 ; Multiply by 2 (for words)
    add eax, 2                 ; Add 2 bytes for length field
    push eax
    call malloc
    add esp, 4
    mov dword[struct], eax     ; Save result struct pointer
    pop ecx                    ; Restore length
    
    ; Set length in result struct
    dec ecx
    mov word[eax], cx          ; Store length
    add eax, 2                 ; Point to result array
    
    ; Initialize for addition
    xor edx, edx               ; Clear carry flag
    mov byte[count], 0         ; Initialize counter

add_numbers1:
    movzx ebx, word[esi]       ; Load word from first number
    movzx ecx, word[edi]       ; Load word from second number
    add ebx, ecx               ; Add numbers
    add ebx, edx               ; Add carry
    xor edx, edx               ; Clear carry flag
    cmp ebx, 0x10000           ; Check for carry
    jb no_carry1
    mov edx, 1                 ; Set carry
    sub ebx, 0x10000           ; Adjust result
no_carry1:
    mov word[eax], bx          ; Store result
    add esi, 2                 ; Next word in first number
    add edi, 2                 ; Next word in second number
    add eax, 2                 ; Next word in result
    inc byte[count]            ; Increment counter
    movzx ecx, byte[smaller]
    cmp byte[count], cl
    jb add_numbers1

process_remaining1:
    movzx ecx, byte[bigger]
    cmp byte[count], cl
    jae check_final_carry1
    
process_bigger1:
    movzx ebx, word[esi]       ; Load remaining word
    add ebx, edx               ; Add carry
    xor edx, edx               ; Clear carry flag
    cmp ebx, 0x10000           ; Check for carry
    jb no_carry2
    mov edx, 1                 ; Set carry
    sub ebx, 0x10000           ; Adjust result
no_carry2:
    mov word[eax], bx          ; Store result
    add esi, 2                 ; Next word
    add eax, 2                 ; Next result position
    inc byte[count]            ; Increment counter
    movzx ecx, byte[bigger]
    cmp byte[count], cl
    jb process_bigger1

check_final_carry1:
    test edx, edx               ; Check if final carry exists
    jz done1
    mov word[eax], 1           ; Store final carry
    mov eax, [struct]          ; Get result struct pointer
    movzx ecx, word[eax]       ; Get current length
    inc ecx                    ; Increment length
    mov word[eax], cx          ; Store new length

done1:
    popad
    pop ebp
    mov eax, [struct]
    ret
;/////////////////////   Task 3  ///////////////////////////               

testing_perpose:
    push ebp                   
    mov ebp, esp                
    pushad  
    push test
    call printf

PRmulti:
    push ebp                   
    mov ebp, esp                
    pushad                      

looping_a_random_size:
    call rand_num                            ;return a random number    
    cmp al, 0                 
    je looping_a_random_size                               ;repeat till the size isn't zero
    movzx ebx, al               
    add ebx, 1                  
    push ebx                   
    call malloc                              ;calling malloc(size=bl)
    mov dword[struct], eax      
    pop ebx                     
    sub ebx,1                    
    mov byte[eax], bl           
    mov esi, eax              
    mov edx, 0                  
looping_a_random_array_num_for_struct:
    call rand_num                        ;return a random number 
    mov byte[esi + edx + 1], al 
    inc edx                
    sub ebx,1                 
    jnz looping_a_random_array_num_for_struct   ;continue till we are not done

    jmp saving_value11       
    ret                         

rand_num:
    push ebp                    
    mov ebp, esp               
    pushad                     

random_generator:
    mov bx, 0              
    movzx eax, word[STATE] 
    and ax, [MASK]          
    jnp parity            
    mov bx, 0x8000          

parity:
    movzx eax, word[STATE]  
    shr ax, 1               
    or ax, bx              
    mov word[STATE], ax    

saving_value_and:
    popad                     
    pop ebp                     
    movzx eax, word[STATE]      
    ret        

saving_value11:
    popad                      
    pop ebp                    
    mov eax, [struct]           
    ret   

_exit:
endd:
    pop ebp
    ret    
