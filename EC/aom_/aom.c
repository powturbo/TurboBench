#include "../aom/aom_dsp/entenc.h"
#include "../aom/aom_dsp/entdec.h"

#define MNDEF0(_m_)     aom_cdf_prob      _m_[17] 		 
#define MNDEF1(_m_,_n_) aom_cdf_prob _m_[_n_][17]

#define MNDEC0(_m_)     { int j;                          for(j = 0; j <= 16; j++) _m_[j]    = OD_ICDF((j+1)*0x800); }
#define MNDEC1(_m_,_n_) { int i,j; for(i=0; i < _n_; i++) for(j = 0; j <= 16; j++) _m_[i][j] = OD_ICDF((j+1)*0x800); }

#define mn4c(_m_,_y_)         { od_ec_encode_cdf_q15(&enc, _y_, _m_, 16); update_cdf(_m_, _y_, 16); }
#define mn8c(_mh_, _ml_, _x_) { unsigned _yh = (_x_)>>4,_yl=(_x_) & 0xf; mn4c(_mh_, _yh); mn4c(_ml_[_yh], _yl); }

#define mn4d(_m_,_y_)         { _y_ = od_ec_decode_cdf_q15(&dec, _m_, 16); update_cdf(_m_, _y_, 16); }
#define mn8d(_mh_, _ml_, _x_) { unsigned _yh,_yl; mn4d(_mh_,_yh); mn4d(_ml_[_yh],_yl); _x_ = _yh << 4| _yl; }

int aomenc( unsigned char *in, unsigned inlen,  unsigned char *out, unsigned outsize) { 
  unsigned char *ip,*op; 														
  unsigned s;
  MNDEF0(mh); MNDEF1(ml,16);
  MNDEC0(mh); MNDEC1(ml,16);

  od_ec_enc enc;
  od_ec_enc_init(&enc, inlen);
  od_ec_enc_reset(&enc);			
  for(ip = in; ip < in+inlen; ip++) mn8c(mh,ml,ip[0]);
  op = od_ec_enc_done(&enc, &s);
  memcpy(out, op, s);
  od_ec_enc_clear(&enc);
  return s; 
}

int aomdec( unsigned char *in, unsigned inlen, unsigned char *out, unsigned outlen ) { 
  unsigned char *op; 										
  MNDEF0(mh); MNDEF1(ml,16);
  MNDEC0(mh); MNDEC1(ml,16);
  od_ec_dec dec;
  od_ec_dec_init(&dec, in, inlen);
  for(op=out; op < out+outlen;op++) mn8d(mh,ml,*op);
  return outlen; 																																			 
}

