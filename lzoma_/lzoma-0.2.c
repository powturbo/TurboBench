#ifdef __GNUC__
#define _FILE_OFFSET_BITS 64
#define _fseeki64 fseeko64
#define _ftelli64 ftello64
#endif
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include"divsufsort.h"

typedef unsigned char U8;
typedef unsigned int U32;

#define longlen 5400
#define hugelen 0x060000
#define breaklz 512
#define breaklen 4
#define lzmagic 0x02aFFe00*2
#define lzshift(top) (9*top>>3)
#define lzlow(total) (total<49550?80:total<652631?60:48)

U8 isBPE[8192];

static inline void set_bpe(U8 a,U8 b){
	int ab=a;
	ab<<=5;
	ab+=b>>3;
	isBPE[ab]|=(1<<(b&7));
}

static inline void unset_bpe(U8 a,U8 b){
	int ab=a;
	ab<<=5;
	ab+=b>>3;
	isBPE[ab]&=~(1<<(b&7));
}

static inline int hasBPE(U8 a,U8 b){
	int ab=a;
	ab<<=5;
	ab+=b>>3;
	return isBPE[ab]&(1<<(b&7));
}

#define BPE 1024
int bpeLastOfs[BPE];
int bpeNum, bpeHead;

void bpe_init(){
	bpeNum=bpeHead=0;
	memset(isBPE,0,8192);
}

void bpePush(U8 *buf, int pos){
	if(pos<2)return;
	U8 a=buf[pos-2], b=buf[pos-1];
	if(hasBPE(a,b))return;
	if(bpeNum==BPE){
		int prev_pos=bpeLastOfs[bpeHead];
		U8 pa=buf[prev_pos], pb=buf[prev_pos+1];
		unset_bpe(pa,pb);
	}
	bpeLastOfs[bpeHead++]=pos-2;
	if(bpeHead==BPE) bpeHead=0;
	if(bpeNum<BPE) bpeNum++;

	set_bpe(a,b);
}

int find_bpes(U8 *buf, int n, int *offsets, int *rofs, int *totals){
	int I[256][256], i=1, cnt=0;

	bpe_init();
	offsets[0]=offsets[1]=rofs[0]=rofs[1]=-1;
	totals[0]=0;
	totals[1]=1;
	if(buf[1]==buf[0]&&buf[2]==buf[1]){
		offsets[1]=rofs[1]=0;
		totals[1]=1;
		cnt++;
	}
	for(n--;++i<n;){
		int cur_head=bpeHead;
		bpePush(buf,i);
		if(bpeHead!=cur_head)
		I[buf[i-2]][buf[i-1]]=cur_head;
		totals[i]=bpeNum+1;
		if(buf[i]==buf[i-1]&&buf[i]==buf[i+1]){
			offsets[i]=i-1;
			rofs[i]=0;
			cnt++;
			continue;
		}
		if(hasBPE(buf[i],buf[i+1])){
			int index=I[buf[i]][buf[i+1]];
			offsets[i]=bpeLastOfs[index];
			index=bpeHead-index;
			if(index<0) index+=BPE;
			rofs[i]=index+1;
			cnt++;
			continue;
		}
		offsets[i]=-1;
		rofs[i]=-1;
	}
	return cnt;
}

int cntBPEs(U8 *buf, int n){
	int I[256][256], i=1, cnt=0;

	bpe_init();
	if(buf[1]==buf[0]&&buf[2]==buf[1])cnt++;
	for(n--;++i<n;){
		int cur_head=bpeHead;
		bpePush(buf,i);
		if(buf[i]==buf[i-1]&&buf[i]==buf[i+1]){
			cnt++;
			continue;
		}
		if(hasBPE(buf[i],buf[i+1])){
			cnt++;
			continue;
		}
	}
	return cnt;
}

#define MINOLEN 1
#define MINLZ 2
int level, match_level;
//FILE *flzlit=NULL, *flit=NULL, *folz=NULL, *flen=NULL, *fdist=NULL;

//int bestOfs[MAX_SIZE]; /* best way to start, assuming we do not start with OLD OFFSET code */
//int bestLen[MAX_SIZE]; /* best way to start, assuming we do not start with OLD OFFSET code */
//int cache[MAX_SIZE]; /* best possible result if we start with lz or letter code */
#define cache same
#define bestOfs same3
#define bestLen samelen
int *same, *same3, *samelen, *useOlz, *olzLen, *useOlz2, *olzLen2, *cacheRLE, *RLE;
int *SA, *ISA, *LCP, *Prev, *Next;
U8 *T=0, *O;// text to be encoded

static inline int lenEnc(int num,int total){
	if(total<257)return 8;
	register int res=0,x=1;
	int top = lzlow(total);
	for(;(x+=x)<total+top;++res){
		if(x & lzmagic)top=lzshift(top);
		if(x>=breaklz){
			if(num<top)goto doneit;
			num+=top;
			total+=top;
			top+=top;
		}
	}
	if(num>=x-total)res++;
doneit:
	return res;
}

static inline int lenEncOL(int num){//num>=2
	if(num<4)return 2;// 00 01
	num-=2;
#define SKEW 1
	return SKEW+((31-__builtin_clz(num))<<1);
}

static inline int lenEncL(int num){//num>=2
	if(num<4)return 2;num-=2;// 00 01
	if(num<4)return 1+2;// 0 0=2 1=3
	if(num<8)return 1+4; // 10 00=4 01=5 10=6 11=7
	if(num<16)return 1+6; // 110 xxx 8-15
	if(num<32)return 1+8;
	if(num<64)return 1+10;
	if(num<128)return 1+12;
	if(num<256)return 1+14;
	if(num<512)return 1+16;
	if(num<1024)return 1+18;
	if(num<2048)return 1+20;
	if(num<4096)return 1+22;
	if(num<8192)return 1+24;
	if(num<16384)return 1+26;
	if(num<32768)return 1+28;
	if(num<65536)return 1+30;
	if(num<131072)return 1+32;
	if(num<262144)return 1+34;
	if(num<524288)return 1+36;
	return 1+38;
}

int lastpos, outpos;
U32 bit_cnt;

static inline void putbit(int bit){
	bit_cnt>>=1;
	if(bit_cnt==0){
		lastpos=outpos;
		*(U32*)(O+lastpos)=0;
		outpos+=4;
		bit_cnt=0x80000000;
	}
	if(bit) *(U32*)(O+lastpos)|=bit_cnt;
}

U32 stlet=0, stlz=0, stolz=0, bitslzlen=0, bitsolzlen=0, bitslen=0, bitsdist=0, bitslit=0;

static inline void putenc(int num,int total, int break_at){
	static char bits[100];
	int res=0, x=1, obyte=1;
//	if(fdist)fwrite(&num,1,4,fdist);
	bits[0]=bits[1]=bits[2]=bits[3]=bits[4]=bits[5]=bits[6]=bits[7]=0;
	int top=lzlow(total);
	for(;(x+=x)<512||x<total+top;bits[res++]=2){
		if(x & lzmagic)top=lzshift(top);
		if(x>=break_at){
			if(num<top)goto doneit;
			num+=top;
			total+=top;
			top+=top;
		}
	}
	if(num>=(x-=total))num+=x, bits[res++]=2;

doneit:
	for(;res<8;res++)bits[res++]=2;
	for(x=res;x;)
		if(bits[--x]==2){
			bits[x]=num&1;
			num>>=1;
		}
	if(obyte){
		U8 b=0;
		for(;x<8;x++)if(bits[x])b|=128>>x;
		O[outpos++]=b;
	}
	for(;x<res;)putbit(bits[x++]);
	bitsdist+=res;
}

static inline void putencL(int num, int break_at){
	static char bits[100];
//	if(flen) fwrite(&num,1,4,flen);
	if(num<2){bitslen+=2; putbit(0);putbit(num);return;}
	int res=0, x=1;
	putbit(1);bitslen++;

	for(num-=2;;bits[res++]=2)
		if((x+=x)>=break_at){
			if(num<(x>>1)){bits[res++]=0; break;}
			bits[res++]=1;
			num-=x>>1;
		}
	for(x=res;x;)
		if(bits[--x]==2){
			bits[x]=num&1;
			num>>=1;
		}
	for(;x<res;)putbit(bits[x++]);
	bitslen+=res;
}

int old_ofs=0;
int was_letter=1;

void initout(void){
	O[0]=T[old_ofs=0];
	outpos=bit_cnt=was_letter=1;
}

static inline int min(int a,int b){
	return a<b? a:b;
}

static inline void putLZ(int offset,int length,int used){
//	if(flzlit) fprintf(flzlit,"%c",1);
	putbit(1); bitslzlen++;
	offset=~offset;
	if(was_letter){ bitsolzlen++;
		was_letter=0;
		if(old_ofs==offset){
			stolz++;
//			if(folz) fprintf(folz,"%c",0);
			putbit(0);
			putencL(length-MINOLEN,breaklen);
			return;
		}
//		if(folz) fprintf(folz,"%c",1);
		putbit(1);
	}
	length-=MINLZ;
	stlz++;
#if LZX
	int h=min(used,hugelen);
	int l=min(used,longlen);
	int total=used+h+l;
	if(length==0)
		putenc(offset,total,breaklz);
	else if(length==1)
		putenc(offset+l,total,breaklz);
	else
		putenc(offset+l+h,total,breaklz),
		putencL(length-2,breaklen);
#else
//	out_lzpos[used-offset-1]=1;
//	out_lzwas[used]=used-offset-1+1;
	if(offset+1>=longlen)length--;
	if(offset+1>=hugelen)length--;
	putenc(offset,used,breaklz);
	putencL(length-MINLZ+2,breaklen);
#endif

	old_ofs=offset;
}

static inline void putC(U8 b){
//	if(flzlit) fprintf(flzlit,"%c",0);
//	if(flit) fprintf(flit,"%c",b);
	putbit(0); bitslzlen++;
	O[outpos++]=b; bitslit+=8;
	was_letter++;
	stlet++;
}

static inline int lenLZ(int offset, int length, int used){ // offset>=1, length>=2,
	int res=1; /* 1 bit = not a letter */
#if LZX
	int l=min(used,longlen), h=min(used,hugelen);
	used+=l+h;
	offset--;
	if(length==2)return res+lenEnc(offset,used);
	if(length==3)return res+lenEnc(offset+l,used);
	res+=lenEnc(offset+l+h,used);
	res+=lenEncL(length-MINLZ+2-2);
#else
	if(offset>=longlen)length--;
	if(offset>=hugelen)length--;
	res+=lenEnc(--offset,used);
	res+=lenEncL(length-MINLZ+2);
#endif
	return res;
}

static inline int len_olz_minus_lz(int offset, int length, int used){ // offset>=1, length>=2,
	return 2+lenEncOL(length+2-MINOLEN)-lenLZ(offset,length,used);
}

static inline int cmpstr(int i,int j){
	int res=0,b;
	for(;;res+=b){
		if(T[i]!=T[j])return res;
		b=RLE[j];
		if(!b)return res;
		if(b>RLE[i])return res+RLE[i];
		i+=b;j+=b;
	}
}

int cmpstrsort(int *o,int *p){
	int b, i=*o, j=*p;
	for(;;j+=b){
		if(b=T[i]-T[j])return b;
		b=RLE[j];
		if(!b)return 1; // first string is longer
		if(b>RLE[i])b=RLE[i];
		if(!b)return-1; // second string is longer
		i+=b;
	}
}

void init_same(int n){
	unsigned short bb;
	int i=n-1, r=RLE[i]=1, m;
	U8 b=T[i], *A=T, *B, *C=T+n;
	for(;i;RLE[i]=r)
		if(T[--i]==b)++r;
		else b=T[i], r=1;
	RLE[n]=0;
	for(i=65536;i;LCP[i]=0)same3[--i]=~longlen;
	for(;i<n-1;same3[bb]=i++)
		if((same[i]=same3[bb=T[i]<<8|T[i+1]])>=0)samelen[i]=1+cmpstr(i+1,same[i]+1);
	same[i]=-1;
	//puts("init_same done.");
	if(n>500){
		for(r=256;r;)LCP[--r+65536]=0;	// for bucketA & bucketB
		_divsufsort(T,SA,LCP,n);
	}else{
		for(r=n;SA[--r]=r;);
		qsort(SA,n,sizeof(int),(int(*)(const void*,const void*))cmpstrsort);
	}
	//puts("suffix sort done.");
	// compute lcp array
	for(;ISA[SA[i]]=i--;);
	for(;++i<n;)if(r=ISA[i]){
		B=A+SA[--r];
		for(m=SA[r]<i?C-A:C-B;A[i]==*B&&m;++B)++A,--m;
		if(LCP[r]=A-T)A--;
	}
	for(Prev[LCP[n-1]=i=0]=-1;++i<n;)Prev[i]=LCP[i-1]>1? i-1:-1;
	for(i=T[n--]=0;i<n;i++)Next[i]=LCP[i]>1? i+1:-1;
	Next[n]=-1;
	//puts("sort done.");
}

static inline int max(int a,int b){
	return a>b? a:b;
}

#define CHECK_OLZ \
int jjj, d=level, tmp=lenLZ(i-pos,len,i), olen=0;\
for(k=len;++k<left-2;){\
	tmp+=9;\
	if(bestOfs[i+k]==pos-i){\
		int tmp2=tmp+len_olz_minus_lz(i-pos,bestLen[i+k],i+k)+cache[i+k];\
		if(tmp2<res || tmp2==res && _bestOfs<pos-i){\
			res=tmp2;\
			_bestOfs=pos-i;\
			_bestLen=len;\
			_useOlz=k-len;\
			_olzLen=bestLen[i+k];\
			_useOlz2=0;\
		}\
	}\
	if(olen==0){\
		olen=cmpstr(i+k,pos+k);\
		for(j=MINOLEN;j<olen;j+=cacheRLE[i+k+j]){\
			int tmp2=tmp+2+lenEncOL(j+2-MINOLEN)+cache[i+k+j];\
			if(tmp2<res || tmp2==res && _bestOfs<pos-i){\
				res=tmp2;\
				_bestOfs=pos-i;\
				_bestLen=len;\
				_useOlz=k-len;\
				_olzLen=j;\
				_useOlz2=0;\
			}\
		}\
		if(olen>=MINOLEN){\
			int tmp2=tmp+2+lenEncOL(olen+2-MINOLEN)+cache[i+k+olen];\
			if(bestLen[i+k+olen]==1){\
				int jj=0,p;\
				for(;++jj<7;){\
					if(bestLen[p=i+k+olen+jj]>1&&bestOfs[p]==pos-i){\
						tmp2+=len_olz_minus_lz(i-pos,bestLen[p],p);\
						break;\
					}\
					int olen2=cmpstr(p,pos+k+olen+jj);\
					for(jjj=MINOLEN;jjj<=olen2;jjj+=cacheRLE[p+jjj]){\
						int tmp3=jj*9+2-cache[i+k+olen]+lenEncOL(jjj+2-MINOLEN)+cache[p+jjj];\
						if(tmp3<0){tmp3+=tmp2;\
							if(tmp3<res || tmp3==res && _bestOfs<pos-i){\
								res=tmp3;\
								_bestOfs=pos-i;\
								_bestLen=len;\
								_useOlz=k-len;\
								_olzLen=olen;\
								_useOlz2=jj;\
								_olzLen2=jjj;\
							}\
						}\
					}\
					break;\
				}\
			}\
			if(tmp2<res || tmp2==res && _bestOfs<pos-i){\
				res=tmp2;\
				_bestOfs=pos-i;\
				_bestLen=len;\
				_useOlz=k-len;\
				_olzLen=olen;\
				_useOlz2=0;\
			}\
		}\
	}else--olen;\
	if(bestOfs[i+k]&&!--d)break;\
}


int pack(int n){
	int res;
	int i=n-1,j,k;

	if(!n)return 0;
	init_same(n);
	cache[i]=9; /* last letter cannot be packed as a lz */
	cacheRLE[i]=cacheRLE[n]=bestLen[i]=1;
	bestOfs[i]=useOlz[i]=useOlz2[i]=0;
	k=Prev[j=ISA[i--]];
	if(k>=0)LCP[k]=min(LCP[k],LCP[j]),Next[k]=Next[j];
	if(Next[j]>=0)Prev[Next[j]]=k;

	for(;i>0;i--){
		int left=n-i;
		int res;
		int pos;
		int max_match;
		int len;
		int _bestOfs=0;
		int _bestLen=1;
		int _useOlz=0;
		int _useOlz2=0;
		int _olzLen=0;
		int _olzLen2=0;
		int match_check_max = match_level;
		int notskip = 1;

		res=9+cache[j=i+1];
		if(bestOfs[j]){
			res++;
			if(T[i]==T[j]){
				if(T[i]==T[i-1]){
					if(T[i]==T[i+bestOfs[j]]){
						if((bestLen[j]>3)||(bestLen[j]==3&&-bestOfs[j]<hugelen)|| (-bestOfs[j]<longlen)){
							int tmp=cache[j]-lenLZ(-bestOfs[j],bestLen[j],j)+lenLZ(-bestOfs[j],bestLen[j]+1,i);
							if(tmp<=res){
								res=tmp;
								_bestOfs=bestOfs[j];
								_bestLen=bestLen[j]+1;
								_useOlz=useOlz[j];
								_olzLen=olzLen[j];
							}
							if(_bestLen>4)
							notskip = 0;
						}
					}
				}
			}
		}
#if 0
		if(0&&bpe_ofs[i]>=0){
			int tmp=len_bpe(i)+cache[i+2];
			len=2;
			pos=bpe_ofs[i];
			if(tmp<res){
				res=tmp;
				_bestOfs=pos-i;
				_bestLen=len;
				_useOlz=0;
				_olzLen=0;
			}
			if(len<left){
				CHECK_OLZ
			}
		}
#endif
		int medium = ISA[i];
		pos=same[i];
		if(pos<0)goto done;

		if(notskip){
			int tmp=0;
			len=samelen[i];
			int ll=(i-pos>=longlen)+(i-pos>=hugelen);
			for(j=MINLZ+ll;j<=len;j+=cacheRLE[i+j]){
				int tmp2=tmp+lenLZ(i-pos,2-MINLZ+j,i)+cache[i+j];
				if(tmp2<res){
					res=tmp2;
					_bestOfs=pos-i;
					_bestLen=j;
					_useOlz=0;
					_olzLen=0;
				}
			}
			max_match=len;
			if(len<left && len>1+ll){
				CHECK_OLZ
			}
		}
		if(max_match<MINLZ) max_match=MINLZ;
		if(notskip)
		for(;;){
			int slen=samelen[pos];
			pos=same[pos];
			if(i-pos>=longlen||pos<0)break;
			if(len>slen)len=slen;
			else if(len==slen)len+=cmpstr(i+len,pos+len);
			if(len>max_match){
				int tmp=0;
				for(j=max_match+1;j<=len;j+=cacheRLE[i+j]){
					int tmp2=tmp+lenLZ(i-pos,j-MINLZ+2,i)+cache[i+j];
					if(tmp2<res){
						res=tmp2;
						_bestOfs=pos-i;
						_bestLen=j;
						_useOlz=0;
						_olzLen=0;
					}
				}
				max_match=len;
			}
			if(len<left && len>1){
				CHECK_OLZ
			}
		}
		max_match=_bestLen+1;
		if(max_match<MINLZ)max_match=MINLZ;
		if(!notskip)goto done;
		int top=Prev[medium];
		int bottom=Next[medium];
		int len0=top>=0?LCP[top]:0;
		int len1=bottom>=0?LCP[medium]:0;
		int minOfs=i+1;
		while((top>=0 || bottom>=0)&&--match_check_max){
			if(len0>len1){
				pos=SA[top];
				len=len0;
				top=Prev[top];
				len0 = top>=0 ? min(len0,LCP[top]):0;
			}else{
				pos=SA[bottom];
				len=len1;
				len1 = bottom>=0 ? min(len1,LCP[bottom]):0;
				bottom=Next[bottom];
			}
			if(i-pos<longlen||len<=MINLZ+1 && i-pos>=hugelen) continue; // we already checked it
			if(len<=MINLZ) goto done;
			if(minOfs>i-pos){
				minOfs=i-pos;//we are checking matches in decreasing order. we need to check next matches only if those are shorter
				int tmp=0, ll=(i-pos>=hugelen)?1:0;
				for(j=MINLZ+1+ll;j<=len;j+=cacheRLE[i+j]){
					int tmp2=tmp+lenLZ(i-pos,j-MINLZ+2,i)+cache[i+j];
					if(tmp2<res || tmp2==res && _bestOfs<pos-i){
						res=tmp2;
						_bestOfs=pos-i;
						_bestLen=j;
						_useOlz=0;
						_olzLen=0;
					}
				}
			}
			if(len<left){
				CHECK_OLZ
			}
		}
done:
		if((k=Prev[medium])>=0)
			LCP[k]=min(LCP[k],LCP[medium]), Next[k]=Next[medium];
		if(Next[medium]>=0)Prev[Next[medium]]=k;

		bestOfs[i]=_bestOfs;
		bestLen[i]=_bestLen;
		useOlz[i]=_useOlz;
		olzLen[i]=_olzLen;
		useOlz2[i]=_useOlz2;
		olzLen2[i]=_olzLen2;
		cache[i]=res;
		for(pos=0;cache[++pos+i]>=res;);
		cacheRLE[i]=pos;
		//i&0xFFF||printf("\r%d left ",i);
	}
	fflush(stdout);
	res=8+cache[1];
	//printf("\rres=%d\n",res);
	res+=7;
	res>>=3; //printf("res bytes=%d ",res>>=3);
	if(res>n)return n;

	/* now we can easily generate compressed stream */
	initout();

	for(i=1;i<n;)
		if(bestLen[i]==1)putC(T[i++]);
		else{
			int k,ofs,len,k2,len2;
dolz:
			putLZ(ofs=bestOfs[i],bestLen[i],i);
			len=olzLen[i];
			len2=olzLen2[i];
			k=useOlz[i];
			k2=useOlz2[i];
			i+=bestLen[i];
			if(k>0){
				for(;putC(T[i++]),--k;);
				if((useOlz[i])&&(len==bestLen[i])&&(ofs==bestOfs[i])) goto dolz;
				putLZ(ofs,len,i);
				i+=len;
				if(k2>0){
					for(;putC(T[i++]),--k2;);
					if((useOlz[i])&&(len2==bestLen[i])&&(ofs==bestOfs[i])) goto dolz;
					putLZ(ofs,len2,i);
					i+=len2;
				}
			}
		}
	//printf("out bytes=%d ",outpos);
/*
	int cnt=0;
	FILE *f=fopen("lzpos","wb");
	for(i=0;i<n;){
		fprintf(f,"%c",out_lzpos[i]);
		out_lzcnt[i]=cnt+=out_lzpos[i++];
	}
	fclose(f);
	f=fopen("lzrolz","wb");
	for(i=0;i<n;i++){
		int ofs=out_lzwas[i];
		if(ofs==0) continue;
		ofs--;
		int b1=out_lzcnt[ofs];
		int b2=out_lzcnt[i];
		b2-=b1;
		fwrite(&b2,1,4,f);
	}
	fclose(f);
*/
	return outpos;
}

int dorle(signed long n){
	long i;
	U8 rlebuf[1000000];
	U8 *rptr=rlebuf;
	U8 *out=O;
	*out++=T[0];
	for(i=1;i<n;){
		//printf("RLE i=%d to %d\n",i,out-O);
		*out++=T[i];
		if(T[i] != T[i-1])i++;
		else{
			U8 b=T[i], cnt=0;
			for(;b==T[i+1]&&cnt<255&&i<n-1;cnt++)i++;
			*rptr++=b+cnt;
			i++;
		}
	}
	memcpy(out,rlebuf,rptr-rlebuf);
	out+=rptr-rlebuf;
	n=out-O;
	//printf("after RLE n=%d\n",n);
	memcpy(T,O,n);
	return n;
}

void e8(signed long n){
	long i, *op;
	for(i=0; i<n-5;){
		U8 b = T[i];
		if((b == 0xF) && (T[i+1] & 0xF0) == 0x80){
			i++;
			b = 0xe8;
		}
		i++;
		b &= 0xFE;
		if(b == 0xe8){
			op = (long *)(T+i);
			if(*op >= -i && *op < n-i){
				*op += i;
			} else if( *op >= n-i && *op < n ){
				*op -= n; // to [-i,1]
			}
			i+=4;
		}
	}
}

void e8back(U8 *buf,long n){
	long i;
	signed long *op;
	for(i=0; i<n-5;){
		U8 b = buf[i];
		if((b == 0xF) && (buf[i+1] & 0xF0) == 0x80){
			i++;
			b = 0xe8;
		}
		b &= 0xFE;
		i++;

		if(b == 0xe8){
			op = (long *)(buf+i);
			if(*op >= -i && *op < 0){
				*op += n;
			} else if( *op >= 0 && *op < n ){
				*op -= i;
			}
			i+=4;
		}
	}
}

#define getbit ((bits=bits&0x7fffffff?(resbits=bits,bits+bits):(src+=4,resbits=*((U32*)(src-4)),(resbits<<1)+1)),resbits>>31)

#define getcode(bits, src, ptotal) {\
	int total = (ptotal);\
	ofs=0;\
	long res=0;\
	int x=1;\
	int top=0;\
	/*if (total > 256) {*/\
		 top=lzlow(total);\
		 x=256;\
		 res=*src++;\
	/*}*/\
	while((x+=x)<total+top) {\
		if(x & lzmagic)\
			top=lzshift(top);\
		/*if (x>=512) {*/\
			if(res<top)goto getcode_doneit;\
			ofs-=top;\
			total+=top;\
			top+=top;\
		/*}*/\
		res+=res+getbit;\
	}\
	x-=total;\
	if (res>=x)res+=res+getbit-x;\
getcode_doneit: \
	ofs+=res;\
	/*fprintf(stderr,"ofs=%d total=%d\n",ofs,ptotal);*/\
}

#define getlen(bits, src) {\
	long res=0;\
	int x=1;\
	if(getbit==0){\
		len+=getbit;\
		goto getlen_0bit;\
	}\
	for(len+=2;;len+=x+=x){\
		res+=res+getbit;\
		if(getbit==0)break;\
	}\
	len+=res;\
getlen_0bit: ;\
}

static void unpack(U8 *src, U8 *dst, int left){
	int ofs=-1, len;
	U32 bits=0x80000000, resbits;
	left--;

copyletter:
	*dst++=*src++;
	left--;len=-1;

get_bit:
	if(left<0)return;
	if(getbit==0)goto copyletter;
	if(len<0)if(len=!getbit)goto uselastofs;
	len=2;
	getcode(bits,src,dst-O);
	if(++ofs>=longlen)len++;
	if(ofs>=hugelen)len++;
	ofs=-ofs;
uselastofs:
	getlen(bits,src);
	for(left-=len;*dst=dst[ofs],++dst,--len;);
	goto get_bit;
}
#if 0
int main(int i, char**v){
	if(i<3)usage:printf(
"LZOMA v0.2 - A LZ based file compressor (c)2015,2016 Alexandr Efimov, xezz\n\n"
"To compress: %s c[#1,#2,#3] infile [outfile]\n"
"#1       [0-511] block size. (1+#1) MB. default is 16MB\n"
"#2       [0-7] level\n"
"#3       [0-100000000] match level\n"
"outfile  default name is infile.lzm\n\n"
"Examles:\n"
"lzoma c in out\n"
"lzoma c0,7 in out\n"
"lzoma c,1 in out\n\n"
"To decompress: %s d infile [outfile]\n"
"outfile  default name is infile.out\n"
, *v,*v), exit(1);
	char *c=v[1], d=*c, *name=v[2], *name2=i<4?(char*)malloc(strlen(name)+4): v[3];
	if(!name2)nomem:puts("out of memory"),exit(1);
	FILE *ip,*op;
	clock_t t=clock();
	if(i<4)
		strcpy(name2, name),
		strcat(name2, d=='c'?".lzm":".out");
	if(ip=fopen(name2,"rb"))printf("Warning: overwrite \"%s\"\n",name2), fclose(ip);
	else printf("output=%s\n", name2);

	ip=fopen(name,"rb");
	op=fopen(name2,"wb");
	if(!ip || !op)puts("File open error at the beginning."), exit(1);
	if(_fseeki64(ip,0,SEEK_END))
		puts("Fseek failed"), exit(1);
	long long il=_ftelli64(ip), ol;
	if(il<1)
		puts(il?"Ftell failed":"0byte"), exit(1);
	rewind(ip);
	if(d=='c'){
		int A[]={15,2,999},a=i=0,l,n=0,bs;
		for(l=sizeof(A)/sizeof(A[0]);*++c&&i<l;)
		if(*c<58&&*c>47){if(a<9)n=n*10+*c-48,a++;}
		else{if(a)A[i]=n;i++;a=n=0;}
		if(a&&i<l)A[i]=n;

		bs=(A[0]&511)+1<<20;
		if(bs>il)bs=il;if(bs<65536)bs=65536;
		l=sizeof(int);
		T=malloc(bs*(l*5+1));
		same=malloc(bs*l*9+l*2);

		if(!T||!same)goto nomem;

		O=T+bs, SA=(int*)O, ISA=SA+bs, LCP=ISA+bs, Prev=LCP+bs, Next=Prev+bs;
		same3=same+bs, samelen=same3+bs, useOlz=samelen+bs, useOlz2=useOlz+bs, olzLen=useOlz2+bs, olzLen2=olzLen+bs, RLE=olzLen2+bs, cacheRLE=RLE+bs+1;

		printf("block size:%d, compression level:[%d, %d]\n",bs,level=A[1]+1,match_level=A[2]+1);
		while((n=fread(T,1,bs,ip))>0){
			printf("got %d bytes, packing %s into %s...\n",n,name,name2);
			int b1=cntBPEs(T,n), use_e8=1;
			e8(n);
			int b2=cntBPEs(T,n);
			printf("stats noe8 %d e8 %d\n",b1,b2);
			if(b2<=b1)use_e8=0, puts("reverted e8"), e8back(T,n);

			b1=pack(n);
			if(b1+5<n)
				fwrite(&b1,4,1,op),
				fwrite(&n,4,1,op),
				fwrite(&use_e8,1,1,op),
				fwrite(O,1,b1,op);
			else i=-n,
				fwrite(&i,4,1,op),
				fwrite(T,1,n,op);
		}
		printf("closing files let=%d lz=%d olz=%d\n",stlet,stlz,stolz);
		printf("bits lzlit=%d let=%d olz=%d match=%d len=%d time=%2.3f\n",bitslzlen,bitslit,bitsolzlen,bitsdist,bitslen,(double)(clock()-t)/CLOCKS_PER_SEC);
		free(T);free(same);
	}
	else if(d=='d'){
		int n;
		while(fread(&n,1,4,ip)>3) {
			int use_e8=0;
			if(n<0)i=-n;
			else fread(&i,1,4,ip), fread(&use_e8,1,1,ip);
			if(!T){
				if(!(T=malloc(i*2+9)))goto nomem;
				O=T+i+9;
			}
			if(n<0){fread(O,1,i,ip);goto done;}
			fread(T,1,n,ip);
			unpack(T,O,i);
	done:
			if(use_e8)e8back(O,i);
			fwrite(O,1,i,op);
		}free(T);
	}
	else goto usage;
	free(name2);
	ol=_ftelli64(op);
	printf("size:%lld -> ",il);
	printf("%lld ",ol);
	printf("(%2.2f%%)\ntime:%2.3f s\n",100.0*ol/il,(double)(clock()-t)/CLOCKS_PER_SEC);
	fclose(ip);fclose(op);
}
#endif
