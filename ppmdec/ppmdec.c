/****************************************************************************
 *  This file is part of the ao0ac project a derivative of PPMd project     *
 *  PPMd written and distributed to public domain by Dmitry Shkarin 1997,   *
 *  1999-2001, 2006                                                         *
 *  Converted to an order 0 adaptive arithmetic code by Kennon Conrad, 2015 *
 ****************************************************************************/
#include "ppmdec.h"

enum { TOP = 1 << 24, BOT = 1 << 14, UP_FREQ = 20, MAX_FREQ = 0x3FFF,
       BUF_SIZE = 0x1000 };

struct SYM_DATA {
  unsigned char Symbol;
  unsigned char Pad;
  unsigned short Freq;
};

#define StartModel() {\
    int i;\
    Range.scale = 257; Range.low=Range.high=0;\
    for (i = 0; i < 256; i++) {\
        Data[i].Symbol = i;\
        Data[i].Freq = 1;\
    }\
}

#define rescale()\
{\
    unsigned char index = 1;\
    Range.scale = (Data[0].Freq = (Data[0].Freq + 1) >> 1) + 1;\
    do {\
        Range.scale += Data[index].Freq = (Data[index].Freq + 1) >> 1;\
    } while (++index);\
}

#define update()\
{\
    unsigned SymFreq = Data[FoundIndex].Freq += UP_FREQ;\
    if (SymFreq > Data[FoundIndex-1].Freq) {\
        unsigned t1=*(unsigned *)&Data[FoundIndex];\
        *(unsigned *)&Data[FoundIndex] = *(unsigned *)&Data[FoundIndex-1];\
        while (--FoundIndex && (SymFreq > Data[FoundIndex-1].Freq)) {\
            *(unsigned *)&Data[FoundIndex] = *(unsigned *)&Data[FoundIndex-1];\
        }\
        *(unsigned *)&Data[FoundIndex] = t1;\
    }\
}\

#define encodeSymbol(symbol)\
{\
    unsigned char i = 1;\
    unsigned LoCnt = Data[0].Freq;\
    if (Data[0].Symbol == symbol) {\
        Range.high = LoCnt;\
        FoundIndex = 0;\
        Data[0].Freq = LoCnt + UP_FREQ;\
        Range.low = 0;\
        goto a;\
    }\
    while (Data[i].Symbol != symbol) {\
        LoCnt += Data[i].Freq;\
        if (++i == 0) {\
            Range.low = LoCnt;\
            FoundIndex = -1;\
            Range.high = Range.scale;\
            goto a;\
        }\
    }\
    Range.high = (Range.low = LoCnt) + Data[i].Freq;\
    FoundIndex = i;\
    update();\
	a:;\
}

/*#define ReadByte(File, e)                            \
{                                                    \
    if (InCharNum == NumInChar) {                    \
        if ((++NumReads & 0x7FF) == 0);              \
            PrintInfo(DecodedFile,EncodedFile);      \
        NumInChar = fread(InData,1,BUF_SIZE,File);   \
        if (NumInChar) {                             \
            Symbol = (int)InData[0];                 \
            InCharNum = 1;                           \
        }                                            \
        else                                         \
            Symbol = -1;                             \
    }                                                \
    else                                             \
        Symbol = (int)InData[InCharNum++];           \
}
#define WriteByte(Value, File)                       \
{                                                    \
    OutData[OutCharNum++] = (unsigned char)(Value);  \
    if (OutCharNum == BUF_SIZE) {                    \
        fflush(File);                                \
        _fwrite(OutData,1,BUF_SIZE,File);             \
        OutCharNum = 0;                              \
    }                                                \
}*/

#define ReadByte(__in, __in_) (__in<__in_?(*__in++):-1)
#define WriteByte(__x, __out) *__out++ = __x
 
#define decodeSymbol()\
{\
    unsigned char i = 1;\
    unsigned count;\
    unsigned HiCnt = Data[0].Freq;\
    if ((count = (code - low) / (range /= Range.scale)) < HiCnt) {\
        Range.high = HiCnt;\
        FoundIndex = 0;\
        Data[0].Freq = HiCnt + UP_FREQ;\
        Range.low = 0;\
        goto ret;\
    }\
    while ((HiCnt += Data[i].Freq) <= count) {\
        if (++i == 0) {\
            Range.low = HiCnt;\
            FoundIndex = -1;\
            Range.high = Range.scale;\
            goto ret;\
        }\
    }\
    Range.low = (Range.high = HiCnt) - Data[i].Freq;\
    FoundIndex = i;\
    update(); \
	ret:;\
}

unsigned ppmdenc(unsigned char *DecodedFile, unsigned inlen, unsigned char* EncodedFile)
{
  struct SYM_DATA Data[256];
  struct SUBRANGE { unsigned low, high, scale; } Range; 
  unsigned low = 0, range = (unsigned)-1;
  int FoundIndex;
  int Symbol;
  unsigned short NumReads=0;
  unsigned char *e = DecodedFile+inlen,*_EncodedFile = EncodedFile;
  //unsigned short NumInChar = 0;
  //unsigned short InCharNum = 0;
  //unsigned short OutCharNum = 0;
  StartModel();
    while (1) {
        Symbol = ReadByte(DecodedFile, e);
        encodeSymbol(Symbol);
        low += Range.low * (range /= Range.scale);
        range *= Range.high - Range.low;
        while ((low ^ (low + range)) < TOP || range < BOT && ((range = -low & (BOT - 1)), 1)) {
            WriteByte(low >> 24, EncodedFile);
            range <<= 8;
            low <<= 8;
        }
        if ((Range.scale += UP_FREQ) > MAX_FREQ)
            rescale();
        if (FoundIndex < 0)
            break;
    }
    while (low ^ (low + range)) {
        WriteByte(low >> 24, EncodedFile);
        low <<= 8;
        range <<= 8;
    }
    //fwrite(OutData,1,OutCharNum,EncodedFile);
    //PrintInfo(DecodedFile,EncodedFile);
	return EncodedFile - _EncodedFile;
}
unsigned ppmddec(unsigned char *EncodedFile, unsigned inlen, unsigned char *DecodedFile, unsigned outlen)
{
  struct SYM_DATA Data[256];
  struct SUBRANGE { unsigned low, high, scale; } Range;
  unsigned low=0, code, range=(unsigned)-1;
  int FoundIndex;
  int Symbol;
  //unsigned short NumReads;
    unsigned char *e = EncodedFile+inlen,*_DecodedFile=DecodedFile;
    unsigned short NumInChar = 0;
    unsigned short InCharNum = 0;
    unsigned short OutCharNum = 0;
    //NumReads = 0;
    low = 0;
    range = (unsigned)-1;
    StartModel();
    code = 0;
	int i;
    for (i = 4; i != 0; i--) {
        Symbol = ReadByte(EncodedFile, e);
        code = (code << 8) | Symbol;
    }
    while (1) {
        decodeSymbol();
        low += range * Range.low;
        range *= Range.high - Range.low;
        if (FoundIndex < 0)
            break;
        WriteByte(Data[FoundIndex].Symbol, DecodedFile);
        while ((low ^ (low + range)) < TOP || range < BOT && ((range= -low & (BOT - 1)), 1)) {
            Symbol = ReadByte(EncodedFile, e);
            code = (code << 8) | Symbol;
            range <<= 8;
            low <<= 8;
        }
        if ((Range.scale += UP_FREQ) > MAX_FREQ)
          rescale();
    }
    //_fwrite(OutData,1,OutCharNum,DecodedFile);
    //PrintInfo(DecodedFile,EncodedFile);
	return DecodedFile - _DecodedFile;
}
