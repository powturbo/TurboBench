#!/usr/bin/env python2
#-*- encoding: utf-8 -*-

import sys
import os
import subprocess
import random
import hashlib
import argparse

def _dump(testdata, **kwargs):
    dump_path = os.path.join(sys.path[0], "fuzzdump_" + hashlib.md5(testdata).hexdigest())
    if not os.path.exists(dump_path):
        os.mkdir(dump_path)
    for kwargs_name, kwargs_data in kwargs.items():
        with open(os.path.join(dump_path, kwargs_name), "wb") as f:
            f.write(kwargs_data)
    return None

def _run_fuzzy_round(zling_demo_bin, fuzzy_size):
    fuzzy_size = random.randint(0, fuzzy_size)
    fuzzy_input = str().join(map(lambda _: chr(random.randrange(256)), xrange(fuzzy_size)))

    for level in range(0, 4):
        pipe = subprocess.Popen(
                [zling_demo_bin, "e%s" % level], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        enc_outdata, enc_errdata = pipe.communicate(fuzzy_input)

        pipe = subprocess.Popen(
                [zling_demo_bin, "d"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        dec_outdata, dec_errdata = pipe.communicate(enc_outdata)

        if dec_outdata != fuzzy_input:
            _dump(fuzzy_input, **{
                "fuzzy_input": fuzzy_input,
                "enc_out": enc_outdata,
                "enc_err": enc_errdata,
                "dec_out": dec_outdata,
                "dec_err": dec_errdata,
            })
            return False
    return True

if __name__ == "__main__":
    ## init arguments
    argparser = argparse.ArgumentParser()
    argparser.add_argument("--zling_demo_bin", type=str)
    argparser.add_argument("--fuzzy_size", type=int)
    argparser.add_argument("--fuzzy_round", type=int)
    args = argparser.parse_args()
    zling_demo_bin = args.zling_demo_bin
    fuzzy_size     = args.fuzzy_size
    fuzzy_round    = args.fuzzy_round

    fuzzy_succ = True
    for n in range(fuzzy_round):
        if _run_fuzzy_round(zling_demo_bin, fuzzy_size):
            sys.stderr.write(".")
        else:
            fuzzy_succ = False
            sys.stderr.write("X")
    sys.stderr.write("\n")

    if fuzzy_succ:
        sys.stderr.write("libzling_fuzzy test passed!\n")
    else:
        sys.stderr.write("libzling_fuzzy test failed!\n")
        sys.exit(-1)
