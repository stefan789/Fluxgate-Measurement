import ads
import matplotlib.pyplot as plt
import numpy as np
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('file')
args = parser.parse_args()
print(args)

a = ads.Ads(clkRate=16384, chStart=0, chCount=3, samples=65536)
a.samples = 65536
a.setRangeTo625()

r = np.array(a.readFGfast()).reshape((-1,3))
print(r.shape)

print(np.mean(r[:,0]))
print(np.mean(r[:,1]))
print(np.mean(r[:,2]))


np.save(args.file, r)

fig, axes = plt.subplots(nrows=2, ncols=1)

axes[0].plot(r[:,0])
axes[0].plot(r[:,1])
axes[0].plot(r[:,2])


axes[1].plot(r[:,1])
#plt.show()
