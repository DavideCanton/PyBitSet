from distutils.core import setup, Extension

pyBitSet_module = Extension('pyBitSet',
                            define_macros=[('MAJOR_VERSION', '1'),
                                           ('MINOR_VERSION', '0')],
                            include_dirs=['C:\Python34\include'],
                            sources=['pybitset.c'])

setup(name='PyBitSet',
      version='1.0',
      description='PyBitSet module',
      author='Davide Canton',
      author_email='davide.canton5@gmail.com',
      long_description='''
PyBitSet is my first C-extension for Python.

This module exports a class, PyBitSet, that represents a bit set, i.e. a fixed-size container for integers.
''',
      ext_modules=[pyBitSet_module])