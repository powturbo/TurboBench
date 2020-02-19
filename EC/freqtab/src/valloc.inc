
void* VAlloc( void* lpAddress, size_t dwSize, uint flAllocationType=0, uint flProtect=0 ) {
  char* r; // = _aligned_malloc( 0x1000, dwSize );

  size_t d, l = dwSize+sizeof(void*)+(0x1000-1)+(16-1);
  l -= l % 16;

  r = (char*)malloc( l );
  if( r ) {
    d = (r - ((char*)0)) & (0x1000-1);
    l = d + sizeof(void*) + (0x1000-1); l-=l % 0x1000;
    d = l - d;
    *(char**)(r+d-sizeof(void*)) = r;
    r+= d;
  }

  return r;
}

uint VFree( void* lpAddress, size_t dwSize=0, uint dwFreeType=0 ) {
  char* r = (char*)lpAddress;
  if( r ) {
    r = *(char**)(r-sizeof(void*));
    free( r );
  }
  return 1;
}


