Test memory accesses using NUMA
===============================

Creates multiple threads, each "pinned" to a core.

Creates multiple memory blocks, each allocated in a NUMA memory node. A memory "block" is an array of longs.

Then, each thread accesses one block, swapping elements in the block. It puts element 0 in position N-1, 1 in N-2, etc... This forces reads and writes from the block, which may be located in a close or furthest node.

Run in either diassrv2, diassrv6 or diascld20 with:
```
  make
  python run.py
```

The Python script takes care of generating the required config file for the given machine.

Run `numactl --hardware` to know the topology of the machine.

Note: In Intel machines, there are only local or remote nodes (the QPI fully connects all CPUs/sockets). On AMD machines, there's more levels.
