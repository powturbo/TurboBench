import pysam, subprocess, random, sys, os
random.seed(20260812)                      # детерминированно
fa = pysam.FastaFile('ref.fa')
L  = fa.get_reference_length('chr1')
N  = int(sys.argv[1]) if len(sys.argv)>1 else 1000
BIN = os.path.expanduser('~/aceapex/aceapex_fai')
env = dict(os.environ, ACEAPEX_BS='16384')

cases = []
# края и особые случаи — обязательно
for a,b in [(1,1),(1,60),(1,100),(L,L),(L-1,L),(9999,10001),(10000,10050),
            (49,52),(50,51),(51,100),(16383,16385),(1048575,1048577)]:
    cases.append((a,b))
while len(cases) < N:
    a = random.randint(1, L)
    ln = random.choice([1,2,49,50,51,60,100,999,16001,65536])
    b = min(a+ln-1, L)
    cases.append((a,b))

bad = 0
for i,(a,b) in enumerate(cases):
    r = subprocess.run([BIN,'r','--in','a.aet','--out','/tmp/s.bin',
                        '--fai','ours.fai','--range',f'chr1:{a}-{b}','--view','sequence'],
                       env=env, capture_output=True)
    got = open('/tmp/s.bin','rb').read().decode()
    exp = fa.fetch('chr1', a-1, b)
    if got != exp:
        bad += 1
        if bad <= 3:
            print(f'  MISMATCH chr1:{a}-{b}  got {len(got)} exp {len(exp)}')
    if (i+1) % 200 == 0:
        print(f'  {i+1}/{len(cases)}, расхождений {bad}', flush=True)
print(f'\nпроверено {len(cases)}, расхождений {bad}')
sys.exit(1 if bad else 0)
