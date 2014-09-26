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
block2node:	list allocating each block to memory node, e.g. [0, 2, 4]
           	means block 0 is at node 0, block 1 at node 2, block 3 at node 4
thread2block:	list specifying which block a thread uses.

Also uses global variable thread2core, which specifies the core to pin each thread to.
"""
def run(blocksize, block2node, thread2block, duration = 10):
	print block2node
	print thread2block
	f = open('run.cfg', 'w')
	f.write('blocksize = %d\n' % blocksize)
	f.write('nblocks = %d\n' % len(block2node))
	for i, j in enumerate(block2node):
		f.write('block%d_node = %d\n' % (i, j))
	f.write('nthreads = %s\n' % len(thread2block))
	for i, j in enumerate(thread2block):
		f.write('thread%d_block = %d\n' % (i, j))
	#for i, j in enumerate(thread2core):
	#	f.write('thread%d_core = %d\n' % (i, j))
	for i,j in enumerate(thread2block):
		f.write('thread%d_core = %d\n' % (i, thread2core[j]))
	f.close()

	p = subprocess.Popen(['./main', 'run.cfg'], stdout=subprocess.PIPE)
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

	print mean, stddev
	print

	return mean, stddev


host = socket.gethostname()
tests = {}

if 'diassrv6' in host or 'diascld20' in host:

	if 'diassrv6' in host:
		thread2core = [i for i in xrange(0, 24)]
		nsockets = 4
		ncorespersocket = 6
	else:
		thread2core = [0, 2, 4, 6, 8, 10, 1, 3, 5, 7, 9, 11]
		nsockets = 2
		ncorespersocket = 6

	for i in xrange(1, ncorespersocket + 1):
		tests['Local_%d' % i] = run(10000000, [j / ncorespersocket for j in xrange(0, nsockets * ncorespersocket)], [ j + ncorespersocket * socket for socket in xrange(nsockets) for j in xrange(0, i)])
		tests['Remote_%d' % i] = run(10000000, [nsockets - 1 - (j / ncorespersocket) for j in xrange(0, nsockets * ncorespersocket)], [j + ncorespersocket * socket for socket in xrange(nsockets) for j in xrange(0, i)])

elif 'diassrv2' in host:
	thread2core = [i for i in xrange(0, 16)]
	nsockets = 4
	ncorespersocket = 6

	for i in xrange(1, ncorespersocket + 1):
		tests['Local_%d' % i] = run(1000000, [j / ncorespersocket for j in xrange(0, nsockets * ncorespersocket)], [i for i in xrange(0, 16)])
		tests['Furthest_%d' % i] = run(1000000, [nsockets - 1 - (j / ncorespersocket) for j in xrange(0, nsockets * ncorespersocket)], [i for i in xrange(0, 16)])
		tests['Intermediate_%d' % i] = run(1000000, [0,0,0,0, 0,0,0,0, 2,2,2,2, 2,2,2,2], [i for i in xrange(0, 16)])

print 'host;%s;time;%s' % (host, datetime.datetime.now())
keys = tests.keys()
keys.sort()
print 'tests;%s' % ';'.join(keys)
print 'mean;%s' % ';'.join([str(tests[test][0]) for test in keys])
print 'stddev;%s' % ';'.join([str(tests[test][1]) for test in keys])


