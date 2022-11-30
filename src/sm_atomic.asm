_text SEGMENT
atomic_compare_exchange PROC
        .FRAME
        .ENDPROLOGUE
        ; dst <- $rdi
        ; expected <- $rsi
        ; desired <- $rdx

        ;mov     r8d, DWORD PTR desired$[rsp]
        ;mov     edx, DWORD PTR expected_value$[rsp]
        ;  mov     rcx, QWORD PTR dst$[rsp]

        mov     rax, qword ptr [rsi]
        lock    cmpxchg qword ptr [rdi], rdx
        mov     rcx, rax
        sete    al
        je      exit
        mov     qword ptr [rsi], rcx
exit:
        ret
atomic_compare_exchange ENDP
_text ENDS
END