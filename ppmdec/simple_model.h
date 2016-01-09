/*
 *--------------------------------------------------------------------------
 * A simple frequency model.
 *
 * This keeps a list of symbols and their frequencies, approximately
 * sorted by symbol frequency. We allow for a single symbol to periodically
 * move up the list when emitted, effectively doing a single step of
 * bubble sort periodically. This means it's largely the same complexity
 * irrespective of alphabet size.
 * It's more efficient on strongly biased distributions than random data.
 *
 * There is no escape symbol, so the model is tailored to relatively
 * stationary samples (although we do have occasional normalisation to
 * avoid frequency counters getting too high).
 *--------------------------------------------------------------------------
 */

// Shrinking this to 1<<10 gives 2-3% smaller qualities, but 50% longer
#define MAX_FREQ (1<<16)-32

template <int NSYM>
struct SIMPLE_MODEL {
    enum { STEP=8 };

    SIMPLE_MODEL();
    inline void encodeSymbol(RangeCoder *rc, uint16_t sym);
    inline int encodeNearSymbol(RangeCoder *rc, uint16_t sym, int dist);
    inline uint16_t decodeSymbol(RangeCoder *rc);

protected:
    void   normalize();

    uint32_t TotFreq;  // Total frequency
    uint32_t BubCnt;   // Periodic counter for bubble sort step

    // Array of Symbols approximately sorted by Freq. 
    struct SymFreqs {
	uint16_t Symbol;
	uint16_t Freq;
    } sentinel, F[NSYM+1];
};


template <int NSYM>
SIMPLE_MODEL<NSYM>::SIMPLE_MODEL() {
    for ( int i=0; i<NSYM; i++ ) {
	F[i].Symbol = i;
	F[i].Freq   = 1;
    }

    TotFreq         = NSYM;
    sentinel.Symbol = 0;
    sentinel.Freq   = MAX_FREQ; // Always first; simplifies sorting.
    BubCnt          = 0;

    F[NSYM].Freq = 0; // terminates normalize() loop. See below.
}


template <int NSYM>
void SIMPLE_MODEL<NSYM>::normalize() {
    /* Faster than F[i].Freq for 0 <= i < NSYM */
    TotFreq=0;
    for (SymFreqs *s = F; s->Freq; s++) {
	s->Freq -= s->Freq>>1;
	TotFreq += s->Freq;
    }
}

template <int NSYM>
inline void SIMPLE_MODEL<NSYM>::encodeSymbol(RangeCoder *rc, uint16_t sym) {
    SymFreqs *s = F;
    uint32_t AccFreq  = 0;

    while (s->Symbol != sym)
	AccFreq += s++->Freq;

    rc->Encode(AccFreq, s->Freq, TotFreq);
    s->Freq += STEP;
    TotFreq += STEP;

    if (TotFreq > MAX_FREQ)
	normalize();

    /* Keep approx sorted */
    if (((++BubCnt&15)==0) && s[0].Freq > s[-1].Freq) {
	SymFreqs t = s[0];
	s[0] = s[-1];
	s[-1] = t;
    }
}

template <int NSYM>
inline int SIMPLE_MODEL<NSYM>::encodeNearSymbol(RangeCoder *rc, uint16_t sym,
						  int dist) {
    SymFreqs *s = F;
    uint32_t AccFreq  = 0;
    int new_sym;

    while ( ABS((signed int)s->Symbol - (signed int)sym) > dist )
	AccFreq += s++->Freq;

    rc->Encode(AccFreq, s->Freq, TotFreq);
    s->Freq += STEP;
    TotFreq += STEP;

    if (TotFreq > MAX_FREQ)
	normalize();

    new_sym = s->Symbol;

    /* Keep approx sorted */
    if (((++BubCnt&15)==0) && s[0].Freq > s[-1].Freq) {
	SymFreqs t = s[0];
	s[0] = s[-1];
	s[-1] = t;
    }

    return new_sym;
}

template <int NSYM>
inline uint16_t SIMPLE_MODEL<NSYM>::decodeSymbol(RangeCoder *rc) {
    SymFreqs* s = F;
    uint32_t freq = rc->GetFreq(TotFreq);
    uint32_t AccFreq;

    for (AccFreq = 0; (AccFreq += s->Freq) <= freq; s++);
    AccFreq -= s->Freq;

    rc->Decode(AccFreq, s->Freq, TotFreq);
    s->Freq += STEP;
    TotFreq += STEP;

    if (TotFreq > MAX_FREQ)
	normalize();

    /* Keep approx sorted */
    if (((++BubCnt&15)==0) && s[0].Freq > s[-1].Freq) {
	SymFreqs t = s[0];
	s[0] = s[-1];
	s[-1] = t;
	return t.Symbol;
    }

    return s->Symbol;
}
