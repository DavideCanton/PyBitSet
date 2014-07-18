PyBitSet
========

PyBitSet is my first C-extension for Python.

This module exports a class, PyBitSet, that represents a bit set, i.e. a fixed-size container for integers.

Example of usage:

```python
>>> from pyBitSet import PyBitSet
>>> p = PyBitSet(10, [1, 0, 4, 5])
>>> p.to_bin_str()
0000110011
>>> p.elems()
[0, 1, 4, 5]
>>> p[5] = 0
>>> p.to_bin_str()
0000010011
>>> p.elems()
[0, 1, 4]
>>> p.nnz
3
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
True
>>> 5 in p
False
>>> list(p)
[1, 1, 0, 1, 1, 0, 0, 0, 0, 0]
>>> p.flip_all()
>>> p.to_bin_str()
1111100100
>>> int(p)
996
>>> p.nnz
6
>>> p.update({1,2,3})
>>> p.nnz
5
>>> p.elems()
[2, 6, 7, 8, 9]
```
