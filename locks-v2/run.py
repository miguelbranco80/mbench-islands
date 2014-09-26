#!/bin/env python
import datetime
import math
import socket
import subprocess
import time

Executables = {
	"SYNC": "./mbench_sync",
	"SPIN": "./mbench_spin",
	"MUTEX": "./mbench_mutex",
	"SYNC_NO_AFFINITY": "./mbench_sync_noaffinity",
	"SPIN_NO_AFFINITY": "./mbench_spin_noaffinity",
	"MUTEX_NO_AFFINITY": "./mbench_mutex_noaffinity"
}

Host = socket.gethostname()

"""
Arguments: 
name:		experiment name
counter2node:	list allocating each counter to memory node, e.g. [0, 2, 4]
           	means counter 0 is at node 0, counter 1 at node 2, counter 3 at node 4
thread2block:	list specifying which counter a thread uses.

Also uses global variable thread2core, which specifies the core to pin each thread to.
"""
def run(test, counter2node, thread2counter, duration = 10):
	f = open('run.cfg', 'w')
	f.write('ncounters = %d\n' % len(counter2node))
	for i, j in enumerate(counter2node):
		f.write('counter%d_node = %d\n' % (i, j))
	f.write('nthreads = %s\n' % len(thread2core))
	for i, j in enumerate(thread2core):
		f.write('thread%d_core = %d\n' % (i, j))
	for i, j in enumerate(thread2counter):
		f.write('thread%d_counter = %d\n' % (i, j))
	f.close()

	p = subprocess.Popen([Executables[test], 'run.cfg'], stdout=subprocess.PIPE)
	
	time.sleep(duration)
	p.terminate()
	p.wait()

	f = p.stdout
	stdout = f.readlines()
	f.close()

	throughput = [long(line.strip()) for line in stdout]
	throughput = throughput[1:] # Skip first measurement (start-up time)
	n = len(throughput)
	if n == 0:
		print 'No result during %s' % test
		return 0

	mean = 1.*sum(throughput)/n
	stddev = math.sqrt( sum([(t - mean) ** 2 for t in throughput]) / n )

	if (stddev / mean) > 0.10:
		print 'Unstable results during %s' % test

	return mean

def runN(*args):
	runs = []
	n = 5
	for i in xrange(0, n):
		runs.append(run(*args))
	mean = 1.*sum(runs)/n
	stddev = math.sqrt( sum([(r - mean) ** 2 for r in runs]) / n )

	return mean, stddev


def pretty_print(tests):
	print 'host;%s;time;%s' % (Host, datetime.datetime.now())
	print 'tests;%s' % ';'.join([name for name, results in tests])
	print 'mean;%s' % ';'.join([str(results[0]) for name, results in tests])
	print 'stddev;%s' % ';'.join([str(results[1]) for name, results in tests])


if 'diassrv6' in Host:
	thread2core = [i for i in xrange(0, 24)]

	for i in xrange(0,2):
		#execs = ['SYNC', 'SPIN', 'MUTEX']
		execs = ['SYNC', 'MUTEX']
		if i == 1:
			execs = ['%s_NO_AFFINITY' % e for e in execs]
		
		tests = []
		for test in execs:
			tests.append( ('%s: Round-robin' % test, runN(test, [0, 1, 2, 3], [i % 6 if (i % 6) < 4 else -1 for i in xrange(0, 24)]) ) )
			tests.append( ('%s: Same Socket' % test, runN(test, [0, 1, 2, 3], [i / 6 if (i % 6) < 4 else -1 for i in xrange(0, 24)]) ) )
			# The following test doesn't make sense because after first call, cachelines are owned exclusively be the other CPU
			#tests.append( ('%s: Same Socket non-NUMA' % test, runN(test, [3, 2, 1, 0], [i / 6 if (i % 6) < 4 else -1 for i in xrange(0, 24)]) ) )
		pretty_print(tests)

		tests = []
		for test in execs:
			tests.append( ('%s: SE' % test, runN(test, [0], [0 for i in xrange(0, 24)]) ) )
			tests.append( ('%s: SN' % test, runN(test, [i / 6 for i in xrange(0, 24)], [i for i in xrange(0, 24)]) ) )
			tests.append( ('%s: ISLANDS' % test, runN(test, [0, 1, 2, 3], [i / 6 for i in xrange(0, 24)]) ) )
			tests.append( ('%s: ISLANDS non-NUMA' % test, runN(test, [0, 1, 2, 3], [1,1,2,2,3,3, 0,0,2,2,3,3, 0,0,1,1,3,3, 0,0,1,1,2,2])) )
		pretty_print(tests)

elif 'diascld20' in Host:
	thread2core = [0, 2, 4, 6, 8, 10, 1, 3, 5, 7, 9, 11]

	for i in xrange(0,2):
		#execs = ['SYNC', 'SPIN', 'MUTEX']
		execs = ['SYNC', 'MUTEX']
		if i == 1:
			execs = ['%s_NO_AFFINITY' % e for e in execs]

		tests = []
		for test in execs:
			tests.append( ('%s: Round-robin' % test, runN(test, [0, 1], [i % 6 if (i % 6) < 2 else -1 for i in xrange(0, 12)]) ) )
			tests.append( ('%s: Same Socket' % test, runN(test, [0, 1], [i / 6 if (i % 6) < 2 else -1 for i in xrange(0, 12)]) ) )
			#tests.append( ('%s: Same Socket non-NUMA' % test, runN(test, [1, 0], [i / 6 if (i % 6) < 2 else -1 for i in xrange(0, 12)]) ) )
		pretty_print(tests)

		tests = []
		for test in execs:
			tests.append( ('%s: SE' % test, runN(test, [0], [0 for i in xrange(0, 12)]) ) )
			tests.append( ('%s: SN' % test, runN(test, [i / 6 for i in xrange(0, 12)], [i for i in xrange(0, 12)]) ) )
			tests.append( ('%s: ISLANDS' % test, runN(test, [0, 1], [i / 6 for i in xrange(0, 12)]) ) )
			tests.append( ('%s: ISLANDS non-NUMA' % test, runN(test, [1, 0], [i % 2 for i in xrange(0, 12)]) ) )
		pretty_print(tests)

elif 'diassrv2' in Host:
	thread2core = [i for i in xrange(0, 16)]

	for i in xrange(0,2):
		#execs = ['SYNC', 'SPIN', 'MUTEX']
		execs = ['SYNC', 'MUTEX']
		if i == 1:
			execs = ['%s_NO_AFFINITY' % e for e in execs]

		tests = []
		for test in execs:
			tests.append( ('%s: Round-robin' % test, runN(test, [0, 1, 2, 3], [i % 4 for i in xrange(0, 16)]) ) )
			tests.append( ('%s: Same Socket' % test, runN(test, [0, 1, 2, 3], [i / 4 for i in xrange(0, 16)]) ) )
			#tests.append( ('%s: Same Socket non-NUMA' % test, runN(test, [3, 2, 1, 0], [i / 4 for i in xrange(0, 16)]) ) )
		pretty_print(tests)

		tests = []
		for test in execs:
			tests.append( ('%s: SE' % test, runN(test, [0], [0 for i in xrange(0, 16)]) ) )
			tests.append( ('%s: SN' % test, runN(test, [i / 4 for i in xrange(0, 16)], [i for i in xrange(0, 16)]) ) )
			tests.append( ('%s: ISLANDS' % test, runN(test, [0, 1, 2, 3], [i / 4 for i in xrange(0, 16)]) ) )
			tests.append( ('%s: ISLANDS non-NUMA' % test, runN(test, [3, 2, 1, 0], [i % 4 for i in xrange(0, 16)]) ) )
		pretty_print(tests)


