from distutils.core import setup, Extension
 
module1 = Extension('ads', 
        sources = ['ads.cpp'],
        extra_compile_args=["-g"],
        libraries=["bio4716", "biodaq"])
 
setup (name = 'ads',
        version = '0.1',
        description = 'module',
        ext_modules = [module1])
