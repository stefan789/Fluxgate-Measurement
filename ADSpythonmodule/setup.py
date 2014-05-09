from distutils.core import setup, Extension
 
module1 = Extension('adsinstantmod', 
                    sources = ['adsinstantmod.cpp'], 
                    extra_compile_args=["-g"], 
                    libraries=['bio4716', 'biodaq'])
 
setup (name = 'ADS Module',
        version = '0.1',
        description = 'module to read out advantech usb4716 in instant mode',
        ext_modules = [module1])
