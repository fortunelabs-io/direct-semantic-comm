/*
 * startup.s - minimal Cortex-M4 vector table and reset handler for
 * STM32F411CEU6. Only the entries this firmware needs (reset, and the
 * mandatory stack pointer/NMI/HardFault slots). Extend the vector table
 * before wiring any actual EXTI-based capture interrupt in Tier 1; this
 * file intentionally does not populate one yet.
 */
    .syntax unified
    .cpu cortex-m4
    .thumb

/* Coprocessor access control register. The build selects a hard-float ABI
 * (-mfloat-abi=hard -mfpu=fpv4-sp-d16), so the compiler is free to emit
 * FPU instructions, and executing one with the FPU disabled is a
 * HardFault. Nothing here uses floating point today, which is exactly why
 * this is worth doing now: the trap would otherwise lie dormant until the
 * first line of arithmetic in Tier 1 and present as a lockup rather than
 * as a missing enable. */
    .equ CPACR, 0xE000ED88
    .equ CPACR_CP10_CP11_FULL, (0xF << 20)

    .section .isr_vector, "a", %progbits
    .type g_pfnVectors, %object
g_pfnVectors:
    .word _estack
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .size g_pfnVectors, . - g_pfnVectors

    .section .text.Reset_Handler
    .weak Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    ldr r0, =_estack
    mov sp, r0

    /* enable CP10/CP11 (the FPU) before any code that might use it */
    ldr r0, =CPACR
    ldr r1, [r0]
    orr r1, r1, #CPACR_CP10_CP11_FULL
    str r1, [r0]
    dsb
    isb

    /* copy .data from flash to RAM */
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata
CopyData:
    cmp r1, r2
    bge CopyDataDone
    ldr r3, [r0], #4
    str r3, [r1], #4
    b CopyData
CopyDataDone:

    /* zero .bss */
    ldr r0, =_sbss
    ldr r1, =_ebss
    movs r2, #0
ZeroBss:
    cmp r0, r1
    bge ZeroBssDone
    str r2, [r0], #4
    b ZeroBss
ZeroBssDone:

    bl main
    b .

    .section .text.NMI_Handler,"ax",%progbits
    .weak NMI_Handler
    .type NMI_Handler, %function
NMI_Handler:
    b .

    .section .text.HardFault_Handler,"ax",%progbits
    .weak HardFault_Handler
    .type HardFault_Handler, %function
HardFault_Handler:
    b .
