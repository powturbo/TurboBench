
struct my_jmpbuf {
  qword rip,rsp;
};

#define ASM __asm__ volatile

INLINE
int my_setjmp( my_jmpbuf* regs ) {
  qword r;

  ASM ("\
   movq %%rsp,8(%1); \
   call 1f; \
1: popq 0(%1); \
  " : "=a"(r) : "b"(regs),"a"(0) : "%rcx","%rdx","%rsi","%rdi","%rbp","%r8","%r9","%r10","%r11","%r12","%r13","%r14","%r15"
  );

  return r;
}

INLINE
void my_jmp( my_jmpbuf* regs, int ) {
  ASM ("\
  xchg %0,%%rsp; \
  jmp *%1; \
  " :  : "d"(regs->rsp),"b"( ((byte*)regs->rip)+2 ),"a"(1) : 
  );
}

typedef my_jmpbuf m_jmp_buf[1];
#define jmp_buf m_jmp_buf
#define longjmp my_jmp
#define setjmp  my_setjmp

