
struct Counter {

  word P; 

  void Update( const int c, const int wr, const int Mw ) {
    int dp = P + Mw-SCALE + ((SCALE-2*Mw)&(-c));
    dp = ((dp*wr)>>SCALElog);
    int q = P - dp + c;
    P = q & mSCALE;
  }

  void update( const int c, const int wr, const int mw ) {
    int dp = c ? mw : (SCALE-mw);
    dp = ((P-dp)*wr) >> SCALElog;
    int q = P - dp + c;
    P = q & mSCALE;
  }

};
