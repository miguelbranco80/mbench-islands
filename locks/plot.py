import pylab
import numpy
import mpl_toolkits.mplot3d.axes3d as p3

import glob

DEFAULT_NAME = 'log-*'

ticks = ['r-', 'b-', 'g-', 'c-', 'k-', 'y-', 'k--', 'r--']

def graph(fname):
        f = open(fname, 'r')
        ll = f.readlines()
        f.close()

        fig = pylab.figure()
	ax = p3.Axes3D(fig)

	vals = []
	map = None
	x = []
	y = []
	z = []
	title = None
	nloops = None

        for l in ll:
                l = l.strip()
                if not l:
                        continue

                l = l.split()
                if l[0] == 'Mapping':
			# previous run
			if map:
				x.append(map[0])
				y.append(map[1])
				z.append(numpy.average(vals))
				vals = []
				map = None

			for i, j in enumerate(l[1:]):
				j = int(j)
				if j < 0: continue
				i -= 1
				if j < i:
					map = j, i
					break

                elif l[0] == 'Took':
                        vals.append(float(l[1]))

		elif l[0] == 'Using':
			s = ' '.join(l[1:])[:-1]
			if not title:
				title = s
			else:
				title += ' & '+s

		elif l[0] == 'Looping':
			nloops = l[1]

	if nloops:
		title = nloops+'x '+title

	ax.scatter3D(x, y, z)
        ax.set_xlabel('Core')
	ax.set_ylabel('Core')
        ax.set_zlabel('Time elapsed (s)')
        ax.set_title(title)
        pylab.show()

if __name__ == '__main__':
	files = glob.glob(DEFAULT_NAME)
	files.sort()
	for i, f in enumerate(files):
		print '[%d] %s' % (i, f)
	while True:
	        i = raw_input('Enter file: ')
		try: i = int(i)
		except: continue
		if i < 0 or i > len(files)-1:
			continue
		else:
			break
	
        graph(files[i])
