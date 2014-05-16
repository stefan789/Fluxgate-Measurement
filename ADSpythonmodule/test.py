import adsinstantmod as a

def func():
    print a.readFGvalue()

if __name__ == "__main__":
    import timeit
    t = timeit.Timer("func()", setup="from __main__ import func")
    g = t.repeat(100,1)
    print(min(g), max(g))
