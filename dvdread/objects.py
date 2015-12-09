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
    titles = None
    name = None

    def __init__(self, Path, TitleClass=None):
        if TitleClass == None: TitleClass = Title

        _dvdread.DVD.__init__(self, Path, TitleClass=TitleClass)
        self.titles = {}

    def GetTitle(self, titlenum):
        if titlenum in self.titles:
            return self.titles[titlenum]

        i = _dvdread.DVD.GetTitle(self, titlenum)
        self.titles[titlenum] = i
        return i;

    def GetAllTitles(self):
        num = self.NumberOfTitles

        return [self.GetTitle(i) for i in range(1, num+1)]

    def GetName(self):
        if self.name != None:
            return self.name

        with open(self.Path, 'rb') as f:
            f.seek(32808, os.SEEK_SET)
            self.name = f.read(32).decode('utf-8').strip()

        return self.name

    def GetNameTitleCase(self):
        n = self.GetName()
        n = n.replace('_', ' ')
        return " ".join( [z.capitalize() for z in n.split(' ')] )

class Title(_dvdread.Title):
    audios = None

    def __init__(self, DVD, IFONum, TitleNum, AudioClass=None, ChapterClass=None, SubpictureClass=None):
        if AudioClass == None: AudioClass = Audio
        if ChapterClass == None: ChapterClass = Chapter
        if SubpictureClass == None: SubpictureClass = Subpicture

        _dvdread.Title.__init__(self, DVD, IFONum, TitleNum, AudioClass=AudioClass, ChapterClass=ChapterClass, SubpictureClass=SubpictureClass)
        self.audios = {}

    def GetAudio(self, audionum):
        if audionum in self.audios:
            return self.audios[audionum]

        i = _dvdread.Title.GetAudio(self, audionum)
        self.audios[audionum] = i
        return i

    def GetAllAudios(self):
        num = self.NumberOfAudios

        return [self.GetAudio(i) for i in range(num)]

class Audio(_dvdread.Audio):
    def __init__(self, Title, AudioNum):
        _dvdread.Audio.__init__(self, Title, AudioNum)

class Chapter(_dvdread.Chapter):
    def __init__(self, Title, ChapterNum, StartCell, EndCell, LenMS, LenFancy):
        _dvdread.Chapter.__init__(self, Title, ChapterNum, StartCell, EndCell, LenMS, LenFancy)

class Subpicture(_dvdread.Subpicture):
    def __init__(self, Title, SubpictureNum):
        _dvdread.Subpicture.__init__(self, Title, SubpictureNum)

