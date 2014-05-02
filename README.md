PyBitSet
========

PyBitSet is my first C-extension for Python.

This module exports a class, PyBitSet, that represents a bit set, i.e. a fixed-size container for integers.

Example of usage:

>>> p = PyBitSet(10, [1, 0, 4])
>>> p.to_bin_str()
0000010011
>>> len(p)
10
>>> int(p)
19
>>> p.flip_one(3)
>>> p.to_bin_str()
0000011011
>>> int(p)
27
>>> p[3]
1
>>> 3 in p
1
>>> 5 in p
False
>>> list(p)
[1, 1, 0, 1, 1, 0, 0, 0, 0, 0]
>>> p.flip_all()
>>> p.to_bin_str()
1111100100
>>> int(p)
996
