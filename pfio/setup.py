from distutils.core import setup, Extension
import numpy 

setup(name = 'pfio', version = '1.0',  \
   ext_modules = [Extension('pfio', ['pfio.c'],
                  include_dirs=[numpy.get_include()]),
    ],
)