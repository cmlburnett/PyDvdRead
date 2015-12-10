import sys
from distutils.core import setup, Extension

majv = 1
minv = 0

if sys.version_info < (3,):
	print("This library is only tested with Python 3.4")
	sys.exit(1)

dvdread4 = Extension(
	'_dvdread',
	define_macros = [
		('MAJOR_VERSION', str(majv)),
		('MINOR_VERSION', str(minv))
	],
        include_dirs = ['/usr/include'],
	libraries = ['dvdread'],
	sources = ['src/dvdread.c']
)

setup(
	name = 'dvdread',
	version = '%d.%d'%(majv,minv),
	description = 'Python wrapper for libdvdread4',
	author = 'Colin ML Burnett',
	author_email = 'cmlburnett@gmail.com',
	url = "https://github.com/cmlburnett/PyDvdRead",
	packages = ['dvdread'],
	ext_modules = [dvdread4],
	classifiers = [
		'Programming Language :: Python :: 3.4'
	]
)

