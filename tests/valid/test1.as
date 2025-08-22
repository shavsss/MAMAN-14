; Test basic functionality
mcro m1
add r1, r2
sub r3, #5
mcroend

.extern LIST
.entry MAIN

MAIN: mov #10, r0
      lea STR, r1
      m1
      jmp END

.data 5, -3, 100
STR: .string "Hello"

END: stop
