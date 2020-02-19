
struct my_jmpbuf {
  uint eip,esp;
};

#define ASM __asm__ volatile

INLINE
int my_setjmp( my_jmpbuf* regs ) {
  int r;

  ASM ("\
   movl %%esp,4(%1); \
   call 1f; \
1: popl 0(%1); \
  " : "=a"(r) : "b"(regs),"a"(0) : "%ecx","%edx","%esi","%edi","%ebp"
  );

  return r;
}

INLINE
void my_jmp( my_jmpbuf* regs, int ) {
  ASM ("\
  xchg %0,%%esp; \
  jmp *%1; \
  " :  : "d"(regs->esp),"b"( ((byte*)regs->eip)+2 ),"a"(1) : 
  );
}

typedef my_jmpbuf m_jmp_buf[1];
#define jmp_buf m_jmp_buf
#define longjmp my_jmp
#define setjmp  my_setjmp

