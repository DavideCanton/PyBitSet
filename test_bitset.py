__author__ = 'Davide Canton'

from pyBitSet import PyBitSet
import itertools as it
import random

if __name__ == "__main__":
    length = 20
    init_val = {random.randint(0, length - 1) for _ in range(5)}
    bitset = PyBitSet(length, init_val)
    print(init_val)
    bin_str = bitset.to_bin_str()
    print(bin_str)
    print(bitset)
    print(set(bitset.elems()))

    new_val = {random.randint(0, length - 1) for _ in range(5)}
    print(new_val)
    bitset.update(new_val)
    print(bitset)
    print(bitset.to_bin_str())
    print(set(it.compress(range(length), bitset)))

    assert set(it.compress(range(length), bitset)) == set(bitset.elems())