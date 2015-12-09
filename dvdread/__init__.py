
__all__ = ['DVD']

import os

import _dvdread

Version = _dvdread.Version

class DVD(_dvdread.DVD):
    titles = None
    name = None

    def __init__(self, Path):
        _dvdread.DVD.__init__(self, Path, TitleClass=Title)
        self.titles = {}

    def GetTitle(self, titlenum):
        if titlenum in self.titles:
            return self.titles[titlenum]

        i = _dvdread.DVD.GetTitle(self, titlenum)
        self.titles[titlenum] = i
        return i;

    def GetAllTitles(self):
        num = self.NumberOfTitles

        return [self.GetTitle(i) for i in range(num)]

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

    def __init__(self, DVD, IFONum, TitleNum):
        _dvdread.Title.__init__(self, DVD, IFONum, TitleNum, AudioClass=Audio, ChapterClass=Chapter, SubpictureClass=Subpicture)
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

