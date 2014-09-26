#!/bin/env python
import datetime
import math
import socket
import subprocess
import time

"""
Arguments: 
name:		experiment name
blocksize:	block size :-)
batchsize:	batch size :-)
nblocks:	number of blocks :-)
block2node:	list allocating each block to memory node, e.g. [0, 2, 4]
           	means block 0 is at node 0, block 1 at node 2, block 3 at node 4
thread2core:	maps each thread to a core

Also uses global variable thread2core, which specifies the core to pin each thread to.
"""
def run(numa, blocksize, batchsize, thread2block, thread2core, nblocks = 0, block2node = [], duration = 10):
	if block2node and nblocks > 0: assert(len(block2node) == nblocks)
	elif block2node: nblocks = len(block2node)
	elif nblocks == 0: assert(False)

	print 'thread2block', thread2block
	print 'thread2core', thread2core
	print 'block2node', block2node

	f = open('run.cfg', 'w')
	f.write('blocksize = %d\n' % blocksize)
	f.write('batchsize = %d\n' % batchsize)
	f.write('nblocks = %d\n' % nblocks)
	f.write('nthreads = %d\n' % len(thread2block))
	for i, j in enumerate(thread2block):
		f.write('thread%d_block = %d\n' % (i, j))
	for i, j in enumerate(thread2core):
		f.write('thread%d_core = %d\n' % (i, j))
	if numa:
		for i, j in enumerate(block2node):
			f.write('block%d_node = %d\n' % (i, j))
	f.close()

	if numa:
		p = subprocess.Popen(['./affinity', 'run.cfg'], stdout=subprocess.PIPE)
	else:
		p = subprocess.Popen(['./no_affinity', 'run.cfg'], stdout=subprocess.PIPE)
	time.sleep(duration)
	p.terminate()
	p.wait()

	f = p.stdout
	stdout = f.readlines()
	f.close()

	throughput = [int(line.strip()) for line in stdout]
	throughput = throughput[1:] # Skip first measurement (start-up time)
	n = len(throughput)

	mean = 1.*sum(throughput)/n
	stddev = math.sqrt( sum([(t - mean) ** 2 for t in throughput]) / n )

	if (stddev / mean) > 0.15:
		print 'Unstable results during run:', mean, stddev
	else:
		print mean, stddev

	return mean


def runN(*args, **optargs):
	runs = []
	n = 3
	for i in xrange(0, n):
		runs.append(run(*args, **optargs))
	mean = 1.*sum(runs)/n
	stddev = math.sqrt( sum([(r - mean) ** 2 for r in runs]) / n )
	print 
	print mean, stddev
	print

	return mean, stddev


host = socket.gethostname()

if 'diassrv6' in host:
	# Not using HyperThreading
	thread_to_core = [i for i in xrange(0, 24)]
	nsockets = 4

	tests_nblocks = [2, 4, 6, 12]
	blocksize = 10000000

elif 'diascld20' in host:
	# Using HyperThreading
	thread_to_core = [0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23]
	nsockets = 2

	tests_nblocks = [2, 4, 6, 12, 24]
	blocksize = 10000000

elif 'diassrv2' in host:
	# AMD
	thread_to_core = [i for i in xrange(0, 16)]
	nsockets = 4

	tests_nblocks = [2, 4, 8, 16]
	blocksize = 10000000

elif 'diassrv4' in host:
	# Niagara T2 w/ 4 sockets
	thread_to_core = [i for i in xrange(0, 256)]
	nsockets = 4

	tests_nblocks = [4, 16]
	blocksize = 10000000

elif 'diassrv8' in host:
	# OctoSocket Intel
	thread_to_core = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 0, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119]
	nsockets = 8

	tests_nblocks = [8, 16]
	blocksize = 1000000

else:
	print 'Unknown host', host


nthreads = len(thread_to_core)
print 'nthreads', nthreads
ncores_per_socket = nthreads / nsockets
print 'ncores per socket', ncores_per_socket

tests = []
for nblocks in tests_nblocks:
#	for batchsize in [1, 10, 100, 1000, 10000, 100000, 1000000]:
#	for batchsize in [1, 100, 10000, 1000000]:
	for batchsize in [1, 1000000]:

		assert((nblocks % nsockets) == 0)
		assert((nthreads % nblocks) == 0)

		thread_to_block = [i / (nthreads / nblocks) for i in xrange(0, nthreads)]

		tests.append(('NUMA-aware (Blocks %d, Batch %d)' % (nblocks, batchsize),
			      runN(True,
				   blocksize,
	 			   batchsize,
				   thread_to_block,
				   thread_to_core,
				   block2node = [i / (nblocks / nsockets) for i in xrange(0, nblocks)])))
		tests.append(('NUMA worst-case (Blocks %d, Batch %d)' % (nblocks, batchsize),
			      runN(True,
				   blocksize,
				   batchsize,
				   thread_to_block,
				   thread_to_core,
				   block2node = [nsockets - 1 - (i / (nblocks / nsockets)) for i in xrange(0, nblocks)])))
		tests.append(('NUMA-unaware (Blocks %d, Batch %d)' % (nblocks, batchsize),
			      runN(False,
				   blocksize,
				   batchsize,
				   thread_to_block,
				   thread_to_core,
				   nblocks = nblocks)))

		#if 'diassrv2' in host:
		#	Do also intermediate!!
		# 	e.g. tests['Intermediate %d' % batchsize] = run(blocksize, [0,0,0,0, 0,0,0,0, 2,2,2,2, 2,2,2,2], [i for i in xrange(0, 16)])

print 'host;%s;time;%s' % (host, datetime.datetime.now())
print 'tests;%s' % ';'.join([t[0] for t in tests])
print 'mean;%s' % ';'.join([str(t[1][0]) for t in tests])
print 'stddev;%s' % ';'.join([str(t[1][1]) for t in tests])


