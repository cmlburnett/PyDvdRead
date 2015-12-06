from distutils.core import setup, Extension

majv = 1
minv = 0

dvdread4 = Extension(
	'dvdread',
	define_macros = [
		('MAJOR_VERSION', str(majv)),
		('MINOR_VERSION', str(minv))
	],
	libraries = ['dvdread'],
	sources = ['src/dvdread.c']
)

setup(
	name = 'dvdread',
	version = '%d.%d'%(majv,minv),
	description = 'Python wrapper for libdvdread4',
	author = 'Colin ML Burnett',
	author_email = 'cmlburnett@gmail.com',
	url = "",
	ext_modules = [dvdread4]
)

