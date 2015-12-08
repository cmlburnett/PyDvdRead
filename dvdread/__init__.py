
__all__ = ['DVD']

import os

import _dvdread

Version = _dvdread.Version

class DVD(_dvdread.DVD):
    titles = None
    name = None

    def __init__(self, Path):
        _dvdread.DVD.__init__(self, Path)
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

