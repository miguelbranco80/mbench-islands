import sys

if len(sys.argv) != 2:
	print 'Missing input file name'
	sys.exit(0)

f = open(sys.argv[1], 'r')
l = f.readlines()
f.close()

print l[0].strip()
l = l[1:] # skip first line

i = 0
while i<len(l):
	val = 0
	for j in xrange(0, 5):
		val += float(l[i+j].split()[2])
	x, y = l[i].split()[:2]
	print x,y,val/5
	i += 5
	


