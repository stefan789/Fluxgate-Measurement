from distutils.core import setup, Extension

module1 = Extension('ads',
        sources = ['ads.cpp']
    )

setup (name = 'ads',
        version = '0.1',
        description = 'module',
        ext_modules = [module1]
)
