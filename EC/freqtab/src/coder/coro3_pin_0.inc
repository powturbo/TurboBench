
#pragma once

struct coro3_pin_0 {
  byte* ptr;
  byte* beg;
  byte* end;
  uint  f_EOF;
  word  base_offs;
  word  r_code;

  uint getinplen()  { return end-ptr; } //-V110
  uint getinpleft() { return end-ptr; } //-V110
  uint getinpsize() { return ptr-beg; } //-V110

  uint getoutlen()  { return end-ptr; } //-V110
  uint getoutleft() { return end-ptr; } //-V110
  uint getoutsize() { return ptr-beg; } //-V110

  void addinp( byte* inp,uint inplen ) { addbuf(inp,inplen); }
  void addout( byte* out,uint outlen ) { addbuf(out,outlen); }

  void addbuf( byte* buf,uint len ) {
    beg = ptr = buf;
    end = &buf[len];
  }

};
