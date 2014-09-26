#!/bin/env python
import glob
import datetime
import math
import socket
import subprocess
import sys
import time


def get_mean(values):
	return 1. * sum(values) / len(values)

def get_stddev(values):
	mean = get_mean(values)
	return math.sqrt( sum([(v - mean) ** 2 for v in values]) / len(values) )

def run_once(cmdline, duration):
	p = subprocess.Popen(cmdline.split(), stdout=subprocess.PIPE)
	
	time.sleep(duration)
	p.terminate()
	p.wait()

	f = p.stdout
	stdout = f.readlines()
	f.close()

	throughput = [long(line.strip()) for line in stdout]
	throughput = throughput[1:] # Skip first measurement (start-up time)

	if len(throughput) == 0:
		print 'No results during', cmdline
		sys.exit(1)

	mean = get_mean(throughput)
	stddev = get_stddev(throughput)

	#if (stddev / mean) > 0.10:
	#	print 'Unstable results during', cmdline
	#	sys.exit(1)

	return mean

def run_many(cmdline, duration, nruns):
	runs = []
	for i in xrange(0, nruns):
		runs.append( run_once(cmdline, duration) )

	mean = get_mean(runs)
	stddev = get_stddev(runs)

	return mean, stddev


# SE
f = open('run-se.cfg', 'w')
f.write('ncounters = 1\n')
f.write('counter0_node = 0\n')
f.write('nthreads = 80\n')
for i in xrange(0, 80):
	if i < 40:
		f.write('thread%d_core = %d\n' % (i, i + 1))
	elif i == 40:
		f.write('thread40_core = 0\n')
	else:
		f.write('thread%d_core = %d\n' % (i, i + 40))
for i in xrange(0, 80):
	f.write('thread%d_counter = 0\n' % i)
f.close()

# SN
f = open('run-sn.cfg', 'w')
f.write('ncounters = 80\n')
for i in xrange(0, 80):
	f.write('counter%d_node = %d\n' % (i, i / 10))
f.write('nthreads = 80\n')
for i in xrange(0, 80):
	if i < 40:
		f.write('thread%d_core = %d\n' % (i, i + 1))
	elif i == 40:
		f.write('thread40_core = 0\n')
	else:
		f.write('thread%d_core = %d\n' % (i, i + 40))
for i in xrange(0, 80):
	f.write('thread%d_counter = %d\n' % (i, i))
f.close()	

# Islands NUMA
f = open('run-islands-numa.cfg', 'w')
f.write('ncounters = 8\n')
for i in xrange(0, 8):
	f.write('counter%d_node = %d\n' % (i, i))
f.write('nthreads = 80\n')
for i in xrange(0, 80):
	if i < 40:
		f.write('thread%d_core = %d\n' % (i, i + 1))
	elif i == 40:
		f.write('thread40_core = 0\n')
	else:
		f.write('thread%d_core = %d\n' % (i, i + 40))
for i in xrange(0, 80):
	f.write('thread%d_counter = %d\n' % (i, i / 10))
f.close()

# Islands non-NUMA
f = open('run-islands-nonnuma.cfg', 'w')
f.write('ncounters = 8\n')
for i in xrange(0, 8):
	f.write('counter%d_node = %d\n' % (i, i))
f.write('nthreads = 80\n')
for i in xrange(0, 80):
	if i < 40:
		f.write('thread%d_core = %d\n' % (i, i + 1))
	elif i == 40:
		f.write('thread40_core = 0\n')
	else:
		f.write('thread%d_core = %d\n' % (i, i + 40))
for i in xrange(0, 80):
	f.write('thread%d_counter = %d\n' % (i, i % 8))
f.close()


print 'host;%s' % socket.gethostname()
print 'time;%s' % datetime.datetime.now()

for cmd in glob.glob("mbench_*"):
	cfgs = glob.glob('run*.cfg')
	cfgs.sort()
	for cfg in cfgs:
		mean, stddev = run_many('./%s %s' % (cmd, cfg), 15, 10)

		print
		print 'test;configuration;mean;stddev'
		print '%s;%s;%s;%s' % (cmd, cfg, mean, stddev)

