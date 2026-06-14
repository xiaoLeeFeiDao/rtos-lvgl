.syntax unified
.cpu cortex-m3
.thumb

.section .vectors, "a"
.global _vectors
_vectors:
    .word _stack_top           /* SP initial value */
    .word Reset_Handler        /* Reset */
    .word Default_Handler      /* NMI */
    .word Default_Handler      /* HardFault */
    .word Default_Handler      /* MemManage */
    .word Default_Handler      /* BusFault */
    .word Default_Handler      /* UsageFault */
    .word 0,0,0,0              /* Reserved */
    .word Default_Handler      /* SVCall */
    .word Default_Handler      /* DebugMon */
    .word 0                    /* Reserved */
    .word Default_Handler      /* PendSV */
    .word Default_Handler      /* SysTick */
    /* External interrupts 0-67 */
    .rept 68
    .word Default_Handler
    .endr

.section .text
.global Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
    /* Copy .data from FLASH to SRAM */
    ldr r0, =_etext
    ldr r1, =_sdata
    ldr r2, =_edata
1:  cmp r1, r2
    bge 2f
    ldr r3, [r0], #4
    str r3, [r1], #4
    b 1b

    /* Zero .bss */
2:  ldr r1, =_sbss
    ldr r2, =_ebss
    mov r3, #0
3:  cmp r1, r2
    bge 4f
    str r3, [r1], #4
    b 3b

4:  bl main
    b .

Default_Handler:
    b .
