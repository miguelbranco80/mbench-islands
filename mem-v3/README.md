Test memory accesses using NUMA
===============================

Creates multiple threads, each "pinned" to a core.

Creates multiple memory blocks, each allocated in a NUMA memory node. A memory "block" is an array of longs.

Then, multiple threads access a block, swapping elements in the block. Each thread swaps element in position 1 with element in position N-1, then 2 with N-2, etc.

(Actually each thread swaps element `(1 + offset) % N` with `(N - 1 + offset) % N`; each thread starts with a different offset. This minimizes any caching effects.)

Run in either diassrv2, diassrv6 or diascld20 with:
```
  make
  python run.py
```

The Python script takes care of generating the required config file for the given machine.

Run `numactl --hardware` to know the topology of the machine.

Note: In Intel machines, there are only local or remote nodes (the QPI fully connects all CPUs/sockets). On AMD machines, there's more levels.

You may also want to experiment with -O0, -O2, etc. I've seen absolute differences in performance but the relative differences are the same.
