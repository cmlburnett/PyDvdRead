import _dvdread

import os

# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------
# Pythonic wrappers for the C implemented objects.
# The idea is that some management is far easier to do in Python plus not implementing
# things in C avoids potential bugs and segfaults.
#
# Note that these classes are passed back to the C objects to be created (see the
# various __init__ arguments that end in "Class".

class DVD(_dvdread.DVD):
	"""
	Entry class into parsing the DVD structure.
	Pass the device path to the init function, and then call Open() to initiate reading.
	Best to use the `with` keyword to ensure Python calls the Close() function when done.

	A DVD has titles.
	A title has chapters, audio tracks, and subpictures ("subtitles").
	"""

	titles = None
	name = None

	def __init__(self, Path, TitleClass=None):
		"""
		Initializes a DVD object and requires the path to the DVD device to query.
		@Path: device path.
		@TitleClass: python-level class to use when creating title objects.
		"""

		if TitleClass == None: TitleClass = Title

		_dvdread.DVD.__init__(self, Path, TitleClass=TitleClass)
		self.titles = {}

	def GetTitle(self, titlenum):
		"""
		Get the object for the given title. Title objects are cached.
		@titlenum: the title number to query starting with one.
		"""

		if titlenum in self.titles:
			return self.titles[titlenum]

		i = _dvdread.DVD.GetTitle(self, titlenum)
		self.titles[titlenum] = i
		return i;

	def GetAllTitles(self):
		"""
		Gets a tuple of all the title objects starting with title one.
		"""

		num = self.NumberOfTitles

		return tuple([self.GetTitle(i) for i in range(1, num+1)])

	def GetName(self):
		"""
		Get the name of the DVD disc in UTF-8.
		"""
		if self.name != None:
			return self.name

		with open(self.Path, 'rb') as f:
			f.seek(32808, os.SEEK_SET)
			self.name = f.read(32).decode('utf-8').strip()

		return self.name

	def GetNameTitleCase(self):
		"""
		Takes the DVD name, changes underscores for spaces, and uses title case.
		Returns a slightly more human-friendly name of the disc.
		"""

		n = self.GetName()
		n = n.replace('_', ' ')
		return " ".join( [z.capitalize() for z in n.split(' ')] )

class Title(_dvdread.Title):
	"""
	Class that represents a DVD title.
	It should be invoked only by calling DVD.GetTitle() and never manually.
	"""

	audios = None

	def __init__(self, DVD, IFONum, TitleNum, AudioClass=None, ChapterClass=None, SubpictureClass=None):
		"""
		Initializes a title object and should never be called manually.
		@DVD: DVD object this title belongs to.
		@IFONum: internal IFO number needed to access title information.
		@TitleNum: title number (DVD starts with one)
		@AudioClass: python-level class to use when creating audio objects.
		@ChapterClass: python-level class to use when creating chapter objects.
		@SubpictureClass: python-level class to use when creating subpicture objects.
		"""

		if AudioClass == None: AudioClass = Audio
		if ChapterClass == None: ChapterClass = Chapter
		if SubpictureClass == None: SubpictureClass = Subpicture

		_dvdread.Title.__init__(self, DVD, IFONum, TitleNum, AudioClass=AudioClass, ChapterClass=ChapterClass, SubpictureClass=SubpictureClass)
		self.audios = {}

	def GetAudio(self, audionum):
		"""
		Gets the specified audio track.
		@audionum: audio track number starting with one.
		"""

		if audionum in self.audios:
			return self.audios[audionum]

		i = _dvdread.Title.GetAudio(self, audionum)
		self.audios[audionum] = i
		return i

	def GetAllAudios(self):
		"""
		Gets a tuple of all the audio track objects starting with audio track one.
		"""

		num = self.NumberOfAudios

		return tuple([self.GetAudio(i) for i in range(1, num+1)])

class Audio(_dvdread.Audio):
	"""
	Class that represents a DVD title's audio track.
	It should be invoked only by calling Title.GetAudio() and never manually.
	This class is presumed unless a different one is supplied to the Title initializer method.
	"""

	def __init__(self, Title, AudioNum):
		"""
		Initializes an audio track object and should never be called manually.
		@Title: DVD title object this audio track belongs to.
		@AudioNum: audio track number (starts with one)
		"""

		_dvdread.Audio.__init__(self, Title, AudioNum)

class Chapter(_dvdread.Chapter):
	"""
	Class that represents a DVD title's chapter.
	It should be invoked only by calling Title.GetChapter() and never manually.
	This class is presumed unless a different one is supplied to the Title initializer method.
	"""

	def __init__(self, Title, ChapterNum, StartCell, EndCell, LenMS, LenFancy):
		_dvdread.Chapter.__init__(self, Title, ChapterNum, StartCell, EndCell, LenMS, LenFancy)

class Subpicture(_dvdread.Subpicture):
	"""
	DVD parlance is "subpicture" ("subp") rather than subtitle.
	Class that represents a DVD title's audio track.
	It should be invoked only by calling Title.GetSubpicture() and never manually.
	This class is presumed unless a different one is supplied to the Title initializer method.
	"""

	def __init__(self, Title, SubpictureNum):
		_dvdread.Subpicture.__init__(self, Title, SubpictureNum)

