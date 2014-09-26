import pylab
import numpy
import mpl_toolkits.mplot3d.axes3d as p3
import glob

DEFAULT_NAME = 'run_*.avg'

def graph(fname):
	f = open(fname, 'r')
	ll = f.readlines()
	f.close()

	bank = ll[0]
	ll = ll[1:]

	fig = pylab.figure()
	ax = p3.Axes3D(fig)

	x = []
	y = []
	z = []
	for l in ll:
		l = l.strip()
		if not l:
			continue
		
		_x, _y, _z = l.split()
		x.append(int(_x))
		y.append(int(_y))
		z.append(float(_z))
	
	ax.scatter3D(x, y, z)
	ax.set_xlabel('Producer Core')
	ax.set_ylabel('Consumer Core')
	ax.set_zlabel('Time elapsed (s)')
	ax.set_title('Memory bank %s' % bank)
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
	print 'Using file %s' % files[i]
        graph(files[i])

