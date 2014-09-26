import commands
import os
import sys

for socket in xrange(0, 4):

	data = []
	out = 'run_%d.log' % socket
	if os.path.exists(out):
		print 'File %s already exists!' % out
		sys.exit(0)	

	f = open(out, 'w')
	f.write('%d\n' % socket)
	f.close()

	for i in xrange(0, 16):
		for j in xrange(0, 16):
			if i == j: continue
			for k in xrange(0,5):
				o = commands.getoutput('./test_memxchg %d %d %d' % (socket, i, j)).strip()
				f = open(out, 'a')
				f.write('%d %d %s\n' % (i, j, o))
				f.close()


