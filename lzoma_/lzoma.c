
int lzomaunpack( unsigned char *in, int inlen, unsigned char *out, int outlen) {
  unsigned char *ifd=in,ifd_=in+inlen,*ofd=out;
  int n,n_unp;
  char shift;

  //ifd=open(argv[1],O_RDONLY|O_BINARY);
  //ofd=open(argv[2],O_WRONLY|O_TRUNC|O_CREAT|O_BINARY,511);
  int current_history = 0;
  int ofs = 0;
  int use_e8=0;
  uint8_t header[8];
  _fread(header,1,8,ifd,ifd_);
  if (header[0] != (AuthorID >> 8) ||
      header[1] != (AuthorID & 0xFF) ||
      header[2] != AlgoID[0] ||
      header[3] != AlgoID[1] ||
      header[4] != AlgoID[2] ||
      header[5] != AlgoID[3] ||
      header[6] != Version) {
    fprintf(stderr, "Unsupported compressed data format\n");
    return 1;
  }
  int block_size = 32*1024 << (header[7] & 0xF);
  int history_size = block_size << 4;
  in_buf = (uint8_t *)malloc(block_size);
  out_buf = (uint8_t *)malloc(history_size); // history is 16*block_size

  uint32_t blk;
  while(_fread(&blk,1,4,ifd,ifd_)==4) {
    //if (use_e8) e8(out_buf,n_unp);
    n = blk & (block_size-1);
    if (blk & BLOCK_STORED) {
      n_unp = n;
    } else if (blk & BLOCK_LAST) {
      _fread(&n_unp,1,4,ifd,ifd_);
    } else {
      n_unp = block_size;
    }
    /*
    if (n != n_unp && !current_history) 
      read(ifd,&use_e8,1);
    else
      use_e8 = 0;
    */
    //long unsigned tsc = (long unsigned)__rdtsc();
    if (n == n_unp) {
      _fread(out_buf,1,n_unp,ifd,ifd_);
      _fwrite(out_buf+ofs,1,n_unp,ofd);
    } else {
      _fread(in_buf,1,n,ifd,ifd_);
#ifdef ASM_X86
#error Asm version not yet updated for recent format changes. Please use C version right now.
      unpack_x86(in_buf, out_buf, n_unp);
#else
      unpack_c(current_history, in_buf, out_buf+ofs, out_buf, n_unp);
#endif
      //tsc=(long unsigned)__rdtsc()-tsc;
      //printf("tsc=%lu\n",tsc);
      //if (use_e8) e8back(out_buf,n_unp);
      _fwrite(out_buf+ofs,1,n_unp,ofd);
    }
    if (blk & BLOCK_LAST)
      break;
    ofs+=n_unp;
    ofs &= (history_size-1);
    current_history += n_unp;
    if (current_history > history_size-block_size)
      current_history = history_size-block_size;
  }

  /*close(ifd);
  close(ofd);
  return 0;*/
  return ifd - in;
}

