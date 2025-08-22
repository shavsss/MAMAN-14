; Test with errors
mov r0, r8     ; Invalid register
add #600, r0   ; Value out of range
undefined_inst r0, r1  ; Unknown instruction
stop
