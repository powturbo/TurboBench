# Builds the C99 decoder as a shared object inside the package; aceapex/__init__.py loads it
# with ctypes. csrc/ is a verbatim copy of ../c (checked by scripts/cdec_test.sh); setuptools
# refuses sources outside the package tree. Needs a C compiler and libzstd (headers + library).
from setuptools import setup, Extension
setup(ext_modules=[Extension("aceapex._aceapex_decode", sources=["csrc/aceapex_decode.c"], include_dirs=["csrc"],
                             libraries=["zstd"], extra_compile_args=["-std=c99", "-O2"],
                             # the reader keeps the documented FSE_CHUNK knob for LEGACY archives (src/ax_env.h)
                             define_macros=[("ACEAPEX_ENV_TUNING", "1")])])
