import sys
from glob import glob
from setuptools import setup, Extension
from pybind11.setup_helpers import Pybind11Extension, build_ext

__version__ = "0.0.1"

# Mixing C and C++ is still not working. We force cxx_std to be 14 and force clang++ on macOS to
# allow compilation.
if sys.platform == 'darwin':
    import os
    compiler = os.environ.get("CC", "clang")
    if not compiler.endswith("++"):
        os.environ["CC"] = compiler + "++"

ext_modules = [
    Pybind11Extension(
        "libalac",
        sources=["bindings.cpp"] + sorted(glob("libalac/*.cpp") + glob("libalac/*.c") + glob("libaes/*.c")),
        language="c++",
        cxx_std=14,
        define_macros = [('VERSION_INFO', __version__)],
    ),
]

setup(
    name="pyalac",
    version=__version__,
    author="SchlaubiSchlump",
    url="https://github.com/SchlaubiSchlump/pyalac",
    description="Python bindings for the apple ALAC library.",
    long_description="",
    ext_modules=ext_modules,
    install_requires=["pybind11>=2.6"],
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    #packages=[]
)
