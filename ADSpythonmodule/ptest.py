import adsinstantmod as a
import numpy as np

if __name__ == "__main__":
    res = []
    for i in range(1):
        print(i)
        d = a.readFGvalue()
        res.append(d)
    np.save("fg", np.asarray(res))

