.syntax unified
.cpu cortex-a9
.arm
.text

.global minemu_irq_trampoline
.type minemu_irq_trampoline, %function
.extern minemu_irq_dispatch

@ IRQ entry: build a trap frame on the IRQ stack, call the C dispatcher,
@ restore from the frame it returns, and resume the interrupted code.
minemu_irq_trampoline:
    sub sp, sp, #88             @ allocate one trap frame
    stmia sp, {r0-r12}          @ save r0-r12
    str lr, [sp, #52]           @ save IRQ lr
    mrs r1, spsr
    str r1, [sp, #56]           @ save interrupted CPSR
    ldr r1, =0x10000000         @ interrupt controller base
    ldr r1, [r1, #8]            @ read claim register
    str r1, [sp, #60]           @ exception_id = source id
    sub r1, lr, #4
    str r1, [sp, #64]           @ interrupted instruction address
    mov r1, #0
    str r1, [sp, #68]           @ dfsr (unused for IRQ)
    str r1, [sp, #72]           @ dfar (unused for IRQ)
    add r1, sp, #76
    stmia r1, {sp, lr}^         @ save banked user sp/lr
    mov r1, #0
    str r1, [sp, #84]           @ reserved
    mov r0, sp
    bl minemu_irq_dispatch      @ r0 = frame to resume
    mov sp, r0
    ldr lr, [sp, #52]
    ldr r1, [sp, #56]
    msr spsr_cxsf, r1
    add r1, sp, #76
    ldmia r1, {sp, lr}^         @ restore banked user sp/lr
    ldmia sp, {r0-r12}
    add sp, sp, #88
    subs pc, lr, #4             @ return and restore CPSR from SPSR
