"""
Pythonic wrappers for the C implemented objects. The idea is that some management is far easier to do in Python plus not implementing things in C avoids potential bugs and segfaults.

Note that these classes are passed back to the C objects to be created (see the various __init__ arguments that end in "Class").
"""

import _dvdread

import glob
import os
import subprocess

class Disc:
	"""
	Utility functions.
	"""

	@staticmethod
	def Check(globpath):
		"""
		Checks drives based on a glob path @globpath.
		Returned is a 3-tuple of (path, disc_type, and discid) where disc_type is 'br', 'dvd', or 'cd'.

		I have not tried multi-mode discs, so I have no idea what udevadm returns.
		My suspicion is that there are multiple media keys that would match.

		Sample output of udevadm that is expected is shown below (only serial number is removed).
		This drive is a BR/DVD/CD combo drive that currently has a CD inserted.
		The three keys of interest are ID_CDROM_MEDIA_CD=1, ID_CDROM_MEDIA_DVD=1, and ID-CDROM_MEDIA_BD=1.
		These indicate the type of media inserted.
		Depending on which is set, the appropriate function is called to return an ID for the media.

		$ udevadm info -q all /dev/sr0
		P: /devices/pci0000:00/0000:00:1f.2/ata1/host0/target0:0:1/0:0:1:0/block/sr0
		N: sr0
		L: -100
		S: cdrom
		S: cdrw
		S: disk/by-id/ata-HL-DT-ST_BD-RE_BH12LS38_K91B6A95648
		S: dvd
		S: dvdrw
		E: DEVLINKS=/dev/cdrom /dev/disk/by-id/ata-HL-DT-ST_BD-RE_XXXXXXXXXX /dev/cdrw /dev/dvd /dev/dvdrw
		E: DEVNAME=/dev/sr0
		E: DEVPATH=/devices/pci0000:00/0000:00:1f.2/ata1/host0/target0:0:1/0:0:1:0/block/sr0
		E: DEVTYPE=disk
		E: ID_ATA=1
		E: ID_ATA_FEATURE_SET_PM=1
		E: ID_ATA_FEATURE_SET_PM_ENABLED=0
		E: ID_ATA_SATA=1
		E: ID_ATA_SATA_SIGNAL_RATE_GEN1=1
		E: ID_BUS=ata
		E: ID_CDROM=1
		E: ID_CDROM_BD=1
		E: ID_CDROM_BD_R=1
		E: ID_CDROM_BD_RE=1
		E: ID_CDROM_CD=1
		E: ID_CDROM_CD_R=1
		E: ID_CDROM_CD_RW=1
		E: ID_CDROM_DVD=1
		E: ID_CDROM_DVD_PLUS_R=1
		E: ID_CDROM_DVD_PLUS_RW=1
		E: ID_CDROM_DVD_PLUS_R_DL=1
		E: ID_CDROM_DVD_R=1
		E: ID_CDROM_DVD_RAM=1
		E: ID_CDROM_DVD_RW=1
		E: ID_CDROM_MEDIA=1
		E: ID_CDROM_MEDIA_CD=1
		E: ID_CDROM_MEDIA_SESSION_COUNT=1
		E: ID_CDROM_MEDIA_TRACK_COUNT=10
		E: ID_CDROM_MEDIA_TRACK_COUNT_AUDIO=10
		E: ID_CDROM_MRW=1
		E: ID_CDROM_MRW_W=1
		E: ID_MODEL=HL-DT-ST_BD-RE_BH12LS38
		E: ID_MODEL_ENC=HL-DT-ST\x20BD-RE\x20\x20BH12LS38\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20
		E: ID_REVISION=1.00
		E: ID_SERIAL=XXXXXXXXXXXXXXXXXXXX
		E: ID_SERIAL_SHORT=XXXXXXXX
		E: ID_TYPE=cd
		E: MAJOR=11
		E: MINOR=0
		E: SUBSYSTEM=block
		E: TAGS=:uaccess:seat:systemd:
		E: USEC_INITIALIZED=2498180
		"""

		ret = []

		paths = glob.glob(globpath)
		for path in paths:
			# Call udevadm to get drive status
			lines = subprocess.check_output(['udevadm', 'info', '-q', 'all', path], universal_newlines=True).split('\n')

			for line in lines:
				parts = line.split(' ', 1)
				if len(parts) != 2: continue

				discid = None
				typ = None
				if parts[1] == 'ID_CDROM_MEDIA_CD=1':
					ret.append( (path, 'cd', Disc.cd_discid(path)) )
				elif parts[1] == 'ID_CDROM_MEDIA_DVD=1':
					ret.append( (path, 'dvd', Disc.dvd_discid(path)) )
				elif parts[1] == 'ID_CDROM_MEDIA_BD=1':
					ret.append( (path, 'br', Disc.br_discid(path)) )

		return ret

	@staticmethod
	def cd_discid(path):
		"""
		Gets the ID of the CD disc.
		This is from the cd-discid program that aggregates information about the tracks since CDs have no unique identifier otherwise.
		This is the same ID used for CDDB.
		"""

		return subprocess.check_output(['cd-discid', path]).decode('latin-1').split('\n')[0]

	@staticmethod
	def dvd_discid(path):
		"""
		Gets the ID of the DVD disc.
		This is the Volume id and volume size from the output of isoinfo.
		Volume id is not always sufficient to be unique enough to be usable.
		"""

		vid = None
		vsid = None
		sz = None

		lines = subprocess.check_output(['isoinfo', '-d', '-i', path]).decode('latin-1').split('\n')
		for line in lines:
			parts = line.split(':')
			if parts[0].lower().startswith('volume id'): vid = parts[1].strip()
			if parts[0].lower().startswith('volume set id'): vsid = parts[1].strip()
			if parts[0].lower().startswith('volume size'): sz = parts[1].strip()

		if vid == None:		raise ValueError("Did not find volume id in isoinfo output")
		if vsid == None:	raise ValueError("Did not find volume set id in isoinfo output")
		if sz == None:		raise ValueError("Did not find volume size in isoinfo output")

		return "%s - %s - %s" % (vid,vsid,sz)

	@staticmethod
	def br_discid(path):
		lines = subprocess.check_output(['blkid', '-s', 'LABEL', '-o', 'value', path]).decode('latin-1').split('\n')
		label = lines[0].strip()
		if not len(label):
			raise ValueError("Failed to get LABEL for '%s'" % path)

		lines = subprocess.check_output(['blkid', '-s', 'UUID', '-o', 'value', path]).decode('latin-1').split('\n')
		uuid = lines[0].strip()
		if not len(uuid):
			raise ValueError("Failed to get UUID for '%s'" % path)

		return "%s - %s" % (label, uuid)

	@staticmethod
	def br_getSize(path):
		label,uuid = __class__.br_discid(path).split(' - ')

		lines = subprocess.check_output(['blockdev', '--getbsz', path]).decode('latin-1').split('\n')
		blocksize = int(lines[0])

		lines = subprocess.check_output(['blockdev', '--getsize64', path]).decode('latin-1').split('\n')
		size = int(lines[0])

		blocks = int(size / blocksize)

		return (label,blocksize,blocks)

	@staticmethod
	def dd(inf, outf, blocksize, blocks, label):
		"""
		Perform a 'resumable' dd copy from @inf to @ouf using the given blocksize and number of blocks.
		The @label is used in exceptions to be descriptive.

		The resumable aspect:
		1) If @outf exists, then the sizes are compared
		2) If the @outf size is the same as what is expected then nothing is done
		3) If the @outf size is not a multiple of @blocksize, the remainder of the block is copied with blocksize=1
		4) If the @outf size is a multiple of @blocksize, the remainder of the blocks are copied by seek and skip appropriately.
		5) If @outf does not exist, then the entire @inf is copied.

		Commands used:
			Partial block copy:   ['dd', 'if=%s'%inf, 'of=%s'%outf, 'bs=1', 'count=%d' % (blocksize - (cursize % blocksize)), 'skip=%d'%cursize, 'seek=%d'%cursize]
			Remaining block copy: ['dd', 'if=%s'%inf, 'of=%s'%outf, 'bs=1', 'count=%d' % (blocksize - (cursize % blocksize)), 'skip=%d'%cursize, 'seek=%d'%cursize]
			Remaining block copy: ['dd', 'if=%s'%inf, 'of=%s'%outf, 'bs=%d' % blocksize, 'count=%d' % (blocksize - (cursize % blocksize)), 'skip=%d'%cursize, 'seek=%d'%cursize]
			Full block copy:      ['dd', 'if=%s'%inf, 'of=%s'%outf, 'bs=%d'%blocksize, 'count=%d'%blocks]

		Where cursize is given by os.path.getsize().
		"""

		# Get expected total size
		expectedsize = blocksize * blocks

		if os.path.exists(outf):
			cursize = os.path.getsize(outf)

			# If sizes are the same, then no need to copy
			if expectedsize == cursize:
				print("Disc already copied")
				return

			elif expectedsize < cursize:
				print("Disc already copied")
				# Um, ok, I guess it's done copying
				return

			else:
				# Partial copy
				print("Partial copy: blocksize=%d, blocks=%d, mod=%d" % (blocksize, blocks, blocksize - (cursize % blocksize)))

				# First, need to copy remaining block
				if cursize % blocksize != 0:
					print("Partial block copy")
					# Finish remainder of block
					args = ['dd', 'if=%s'%inf, 'of=%s'%outf, 'bs=1', 'count=%d' % (blocksize - (cursize % blocksize)), 'skip=%d'%cursize, 'seek=%d'%cursize]
					print(" ".join(args))
					ret = subprocess.call(args)
					if ret != 0:
						raise Exception("Failed to partial copy remaining block %d of disc '%s' to drive" % (int(cursize/blocksize)+1, label))

				# Get [possibly] updated size
				cursize = os.path.getsize(outf)

				print("Partial copy")
				# Finish remainder of disc
				#   * Blocks written: int(cursize / blocksize)
				#   * Remaining blocks: blocks - int(cursize / blocksize)
				#   * Starting block: int(cursize / blocksize)
				args = ['dd', 'if=%s'%inf, 'of=%s'%outf, 'bs=%d' % blocksize, 'count=%d' % (blocks - int(cursize / blocksize)), 'skip=%d'%int(cursize/blocksize), 'seek=%d'%int(cursize/blocksize)]
				print(" ".join(args))
				ret = subprocess.call(args)
				if ret != 0:
					raise Exception("Failed to copy remaining %d blocks (total %d) of disc '%s' to drive" % (blocks - int(cursize/blocksize)+1, blocks, label))

		else:
			# Dup entire disc to drive
			args = ['dd', 'if=%s'%inf, 'of=%s'%outf, 'bs=%d'%blocksize, 'count=%d'%blocks]
			print(" ".join(args))
			ret = subprocess.call(args)
			if ret != 0:
				raise Exception("Failed to copy disc '%s' to drive" % label)

	@staticmethod
	def dvd_GetSize(path):
		"""
		Get label, block size, and # of blocks from the disc (returned as a tuple in that order).
		Using block size and blocks, instead of not specifying them, per this page
		  https://www.thomas-krenn.com/en/wiki/Create_an_ISO_Image_from_a_source_CD_or_DVD_under_Linux
		Seems the more prudent choice.
		"""

		# Get information from isoinfo
		ret = subprocess.check_output(["isoinfo", "-d", "-i",path], universal_newlines=True)
		lines = ret.split("\n")

		label = None
		blocksize = None
		blocks = None

		# Have to iterate through the output to find the desired lines
		for line in lines:
			parts = line.split(':')
			parts = [p.strip() for p in parts]

			if parts[0] == 'Volume id':						label = parts[1]
			elif parts[0] == 'Logical block size is':		blocksize = int(parts[1])
			elif parts[0] == 'Volume size is':				blocks = int(parts[1])
			else:
				# Don't care
				pass

		# Make sure all three are present
		if label is None:		raise Exception("Could not find volume label")
		if blocksize is None:	raise Exception("Could not find block size")
		if blocks is None:		raise Exception("Could not find number of blocks")

		return (label,blocksize,blocks)

class DVD(_dvdread.DVD):
	"""
	Entry class into parsing the DVD structure.
	Pass the device path to the init function, and then call Open() to initiate reading.
	Best to use the `with` keyword to ensure Python calls the Close() function when done.

	A DVD has titles.
	A title has chapters, audio tracks, and subpictures ("subtitles").
	"""

	def __init__(self, Path, TitleClass=None):
		"""
		Initializes a DVD object and requires the path to the DVD device to query.
		@Path: device path.
		@TitleClass: python-level class to use when creating title objects.
		"""

		if TitleClass is None: TitleClass = Title

		_dvdread.DVD.__init__(self, Path, TitleClass=TitleClass)
		self.titles = {}

	def __enter__(self):
		return self

	def __exit__(self, type, value, tb):
		# Close, always
		try:
			self.Close()
		except Exception:
			pass

		# Don't suppress any exceptions
		return False

	def GetTitle(self, titlenum):
		"""
		Get the object for the given title. Title objects are cached.
		@titlenum: the title number to query starting with one.
		"""

		if not self.IsOpen:
			raise AttributeError("GetTitle: disc is not open")

		if titlenum in self.titles:
			return self.titles[titlenum]

		i = _dvdread.DVD.GetTitle(self, titlenum)
		self.titles[titlenum] = i
		return i;

	def GetName(self):
		"""
		Get the name of the DVD disc in UTF-8.
		"""

		if not self.IsOpen:
			raise AttributeError("GetName: disc is not open")

		if self.name is not None:
			return self.name

		with open(self.Path, 'rb') as f:
			# Random seek location I found in lsdvd...no idea what it means or where it is
			f.seek(32808, os.SEEK_SET)
			self.name = f.read(32).decode('utf-8').strip()

		return self.name

	def GetNameTitleCase(self):
		"""
		Takes the DVD name, changes underscores for spaces, and uses title case.
		Returns a slightly more human-friendly name of the disc.
		"""

		if not self.IsOpen:
			raise AttributeError("GetNameTitleCase: disc is not open")

		n = self.GetName()
		n = n.replace('_', ' ')
		return " ".join( [z.capitalize() for z in n.split(' ')] )

class Title(_dvdread.Title):
	"""
	Class that represents a DVD title.
	It should be invoked only by calling DVD.GetTitle() and never manually.
	"""

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

		if AudioClass is None: AudioClass = Audio
		if ChapterClass is None: ChapterClass = Chapter
		if SubpictureClass is None: SubpictureClass = Subpicture

		_dvdread.Title.__init__(self, DVD, IFONum, TitleNum, AudioClass=AudioClass, ChapterClass=ChapterClass, SubpictureClass=SubpictureClass)

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

