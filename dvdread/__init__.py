
__all__ = ['DVD', 'Title', 'Chapter', 'Audio', 'Subpicture', 'tnode', 'node', 'DVDToXML']

# Import and get C's version
import _dvdread
Version = _dvdread.Version

# Get C object wrappers
from .objects import DVD, Title, Chapter, Audio, Subpicture

# Get XML implementation
from .xml import tnode, node

def DVDToXML(device):
	with DVD(device) as d:
		d.Open()

		root = node('dvd', numtitles=d.NumberOfTitles, parser="pydvdread %s"%Version)
		root.AddChild( tnode('device', d.Path) )
		root.AddChild( tnode('name', d.GetName(), fancy=d.GetNameTitleCase()) )
		root.AddChild( tnode('vmg_id', d.VMGID) )
		root.AddChild( tnode('provider_id', d.ProviderID) )

		titles = root.AddChild( node('titles') )

		for i in range(1, d.NumberOfTitles+1):
			t = d.GetTitle(i)


			title = titles.AddChild( node('title', idx=t.TitleNum) )
			title.AddChild( tnode('length', t.PlaybackTime, fancy=t.PlaybackTimeFancy) )
			title.AddChild( node('picture', aspectratio=t.AspectRatio, framerate=t.FrameRate, width=t.Width, height=t.Height) )
			title.AddChild( node('angle', num=t.NumberOfAngles) )

			audios = title.AddChild( node('audios', num=t.NumberOfAudios) )
			for j in range(1, t.NumberOfAudios+1):
				a = t.GetAudio(j)
				audio = audios.AddChild( node('audio', idx=j, langcode=a.LangCode, language=a.Language, format=a.Format, samplingrate=a.SamplingRate) )

			chapters = title.AddChild( node('chapters', num=t.NumberOfChapters) )
			for j in range(1, t.NumberOfChapters+1):
				c = t.GetChapter(j)
				chapter = chapters.AddChild( node('chapter', idx=j) )
				chapter.AddChild( tnode('length', c.Length, fancy=c.LengthFancy) )
				chapter.AddChild( node('cells', start=c.StartCell, end=c.EndCell) )


			subpictures = title.AddChild( node('subpictures', num=t.NumberOfSubpictures) )
			for j in range(1, t.NumberOfSubpictures+1):
				s = t.GetSubpicture(j)
				subpicture = subpictures.AddChild( node('subpicture', idx=j, langcode=s.LangCode, language=s.Language) )

	return root.OuterXMLPretty

