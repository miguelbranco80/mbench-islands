Test lock contention using NUMA
===============================

Creates multiple threads, each "pinned" to a core.

Creates multiple counters, each allocated in a NUMA memory node.

Then, each thread accesses one counter and increments it.

To increment the counter, it's possible to use GCC's built-in `__sync_fetch_add` or pthread's spinlocks or mutexes.

To compile:
```
make
```

Tu run, see ``run.py`` for an example on how to generate the configuration needed.
The script currently supports ``diassrv2``, ``diassrv6`` or ``diascld20``.
