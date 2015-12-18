#include "dvdread.h"


// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// Helper functions

static long
dvdtimetoms(dvd_time_t *t)
{
	long ms = 0;
	ms += ((t->hour & 0x0F) >> 0) * 3600 * 1000;
	ms += ((t->hour & 0xF0) >> 4) * 3600 * 1000 * 10;
	ms += ((t->minute & 0x0F) >> 0) * 60 * 1000;
	ms += ((t->minute & 0xF0) >> 4) * 60 * 1000 * 10;
	ms += ((t->second & 0x0F) >> 0) * 1000;
	ms += ((t->second & 0xF0) >> 4) * 1000 * 10;

	int f = t->frame_u >> 6;
	if (f == 1) // 25 fps = 40 ms per frame
	{
		ms += ((t->frame_u & 0x0F) >> 0) * 40;
		ms += ((t->frame_u & 0x30) >> 4) * 40 * 10;
	}
	else if (f == 3) // 29.97 fps = 33.36670003337 ms per frame
	{
		ms += ((t->frame_u & 0x0F) >> 0) * 33.3667;
		ms += ((t->frame_u & 0x30) >> 4) * 33.3667 * 10;
	}
	else
	{
		// "Illegal" values anyway, so ignore their minor contribution to @ms
	}

	return ms;
}

static PyObject*
dvdtimetofancy(long ms, int framerate)
{
	long h,m,s,f;

	if (framerate == 1)
	{
		f = (ms % 1000) / 40;
	}
	else if (framerate == 3)
	{
		f = (int)((ms % 1000) / 33.3667);
	}
	else
	{
		f = 0;
	}

	// Get rid of milliseconds
	ms = ms / 1000;

	// Get seconds
	s = ms % 60;
	ms /= 60;

	// Get minutes
	m = ms % 60;
	ms /= 60;

	// Get hours
	h = ms;

	// Format as string
	return PyUnicode_FromFormat("%02d:%02d:%02d.%02d", h, m, s, f);
}

#define SC(a,b) ((((uint16_t)a)<<8)+((uint16_t)b))
static PyObject*
LangCodeToName(uint16_t langcode)
{
	switch(langcode)
	{
		case SC('\0','\0'): return PyUnicode_FromString("Unknown");
		case SC(' ',' '): return PyUnicode_FromString("Not Specified");
		case SC('a','a'): return PyUnicode_FromString("Afar");
		case SC('a','b'): return PyUnicode_FromString("Abkhazian");
		case SC('a','f'): return PyUnicode_FromString("Afrikaans");
		case SC('a','m'): return PyUnicode_FromString("Amharic");
		case SC('a','r'): return PyUnicode_FromString("Arabic");
		case SC('a','s'): return PyUnicode_FromString("Assamese");
		case SC('a','y'): return PyUnicode_FromString("Aymara");
		case SC('a','z'): return PyUnicode_FromString("Azerbaijani");
		case SC('b','a'): return PyUnicode_FromString("Bashkir");
		case SC('b','e'): return PyUnicode_FromString("Byelorussian");
		case SC('b','g'): return PyUnicode_FromString("Bulgarian");
		case SC('b','h'): return PyUnicode_FromString("Bihari");
		case SC('b','i'): return PyUnicode_FromString("Bislama");
		case SC('b','n'): return PyUnicode_FromString("Bengali; Bangla");
		case SC('b','o'): return PyUnicode_FromString("Tibetan");
		case SC('b','r'): return PyUnicode_FromString("Breton");
		case SC('c','a'): return PyUnicode_FromString("Catalan");
		case SC('c','o'): return PyUnicode_FromString("Corsican");
		case SC('c','s'): return PyUnicode_FromString("Czech");
		case SC('c','y'): return PyUnicode_FromString("Welsh");
		case SC('d','a'): return PyUnicode_FromString("Dansk");
		case SC('d','e'): return PyUnicode_FromString("Deutsch");
		case SC('d','z'): return PyUnicode_FromString("Bhutani");
		case SC('e','l'): return PyUnicode_FromString("Greek");
		case SC('e','n'): return PyUnicode_FromString("English");
		case SC('e','o'): return PyUnicode_FromString("Esperanto");
		case SC('e','s'): return PyUnicode_FromString("Espanol");
		case SC('e','t'): return PyUnicode_FromString("Estonian");
		case SC('e','u'): return PyUnicode_FromString("Basque");
		case SC('f','a'): return PyUnicode_FromString("Persian");
		case SC('f','i'): return PyUnicode_FromString("Suomi");
		case SC('f','j'): return PyUnicode_FromString("Fiji");
		case SC('f','o'): return PyUnicode_FromString("Faroese");
		case SC('f','r'): return PyUnicode_FromString("Francais");
		case SC('f','y'): return PyUnicode_FromString("Frisian");
		case SC('g','a'): return PyUnicode_FromString("Gaelic");
		case SC('g','d'): return PyUnicode_FromString("Scots Gaelic");
		case SC('g','l'): return PyUnicode_FromString("Galician");
		case SC('g','n'): return PyUnicode_FromString("Guarani");
		case SC('g','u'): return PyUnicode_FromString("Gujarati");
		case SC('h','a'): return PyUnicode_FromString("Hausa");
		case SC('h','e'): return PyUnicode_FromString("Hebrew");
		case SC('h','i'): return PyUnicode_FromString("Hindi");
		case SC('h','r'): return PyUnicode_FromString("Hrvatski");
		case SC('h','u'): return PyUnicode_FromString("Magyar");
		case SC('h','y'): return PyUnicode_FromString("Armenian");
		case SC('i','a'): return PyUnicode_FromString("Interlingua");
		case SC('i','d'): return PyUnicode_FromString("Indonesian");
		case SC('i','e'): return PyUnicode_FromString("Interlingue");
		case SC('i','k'): return PyUnicode_FromString("Inupiak");
		case SC('i','n'): return PyUnicode_FromString("Indonesian");
		case SC('i','s'): return PyUnicode_FromString("Islenska");
		case SC('i','t'): return PyUnicode_FromString("Italiano");
		case SC('i','u'): return PyUnicode_FromString("Inuktitut");
		case SC('i','w'): return PyUnicode_FromString("Hebrew");
		case SC('j','a'): return PyUnicode_FromString("Japanese");
		case SC('j','i'): return PyUnicode_FromString("Yiddish");
		case SC('j','w'): return PyUnicode_FromString("Javanese");
		case SC('k','a'): return PyUnicode_FromString("Georgian");
		case SC('k','k'): return PyUnicode_FromString("Kazakh");
		case SC('k','l'): return PyUnicode_FromString("Greenlandic");
		case SC('k','m'): return PyUnicode_FromString("Cambodian");
		case SC('k','n'): return PyUnicode_FromString("Kannada");
		case SC('k','o'): return PyUnicode_FromString("Korean");
		case SC('k','s'): return PyUnicode_FromString("Kashmiri");
		case SC('k','u'): return PyUnicode_FromString("Kurdish");
		case SC('k','y'): return PyUnicode_FromString("Kirghiz");
		case SC('l','a'): return PyUnicode_FromString("Latin");
		case SC('l','n'): return PyUnicode_FromString("Lingala");
		case SC('l','o'): return PyUnicode_FromString("Laothian");
		case SC('l','t'): return PyUnicode_FromString("Lithuanian");
		case SC('l','v'): return PyUnicode_FromString("Latvian, Lettish");
		case SC('m','g'): return PyUnicode_FromString("Malagasy");
		case SC('m','i'): return PyUnicode_FromString("Maori");
		case SC('m','k'): return PyUnicode_FromString("Macedonian");
		case SC('m','l'): return PyUnicode_FromString("Malayalam");
		case SC('m','n'): return PyUnicode_FromString("Mongolian");
		case SC('m','o'): return PyUnicode_FromString("Moldavian");
		case SC('m','r'): return PyUnicode_FromString("Marathi");
		case SC('m','s'): return PyUnicode_FromString("Malay");
		case SC('m','t'): return PyUnicode_FromString("Maltese");
		case SC('m','y'): return PyUnicode_FromString("Burmese");
		case SC('n','a'): return PyUnicode_FromString("Nauru");
		case SC('n','e'): return PyUnicode_FromString("Nepali");
		case SC('n','l'): return PyUnicode_FromString("Nederlands");
		case SC('n','o'): return PyUnicode_FromString("Norsk");
		case SC('o','c'): return PyUnicode_FromString("Occitan");
		case SC('o','m'): return PyUnicode_FromString("Oromo");
		case SC('o','r'): return PyUnicode_FromString("Oriya");
		case SC('p','a'): return PyUnicode_FromString("Punjabi");
		case SC('p','l'): return PyUnicode_FromString("Polish");
		case SC('p','s'): return PyUnicode_FromString("Pashto, Pushto");
		case SC('p','t'): return PyUnicode_FromString("Portugues");
		case SC('q','u'): return PyUnicode_FromString("Quechua");
		case SC('r','m'): return PyUnicode_FromString("Rhaeto-Romance");
		case SC('r','n'): return PyUnicode_FromString("Kirundi");
		case SC('r','o'): return PyUnicode_FromString("Romanian");
		case SC('r','u'): return PyUnicode_FromString("Russian");
		case SC('r','w'): return PyUnicode_FromString("Kinyarwanda");
		case SC('s','a'): return PyUnicode_FromString("Sanskrit");
		case SC('s','d'): return PyUnicode_FromString("Sindhi");
		case SC('s','g'): return PyUnicode_FromString("Sangho");
		case SC('s','h'): return PyUnicode_FromString("Serbo-Croatian");
		case SC('s','i'): return PyUnicode_FromString("Sinhalese");
		case SC('s','k'): return PyUnicode_FromString("Slovak");
		case SC('s','l'): return PyUnicode_FromString("Slovenian");
		case SC('s','m'): return PyUnicode_FromString("Samoan");
		case SC('s','n'): return PyUnicode_FromString("Shona");
		case SC('s','o'): return PyUnicode_FromString("Somali");
		case SC('s','q'): return PyUnicode_FromString("Albanian");
		case SC('s','r'): return PyUnicode_FromString("Serbian");
		case SC('s','s'): return PyUnicode_FromString("Siswati");
		case SC('s','t'): return PyUnicode_FromString("Sesotho");
		case SC('s','u'): return PyUnicode_FromString("Sundanese");
		case SC('s','v'): return PyUnicode_FromString("Svenska");
		case SC('s','w'): return PyUnicode_FromString("Swahili");
		case SC('t','a'): return PyUnicode_FromString("Tamil");
		case SC('t','e'): return PyUnicode_FromString("Telugu");
		case SC('t','g'): return PyUnicode_FromString("Tajik");
		case SC('t','h'): return PyUnicode_FromString("Thai");
		case SC('t','i'): return PyUnicode_FromString("Tigrinya");
		case SC('t','k'): return PyUnicode_FromString("Turkmen");
		case SC('t','l'): return PyUnicode_FromString("Tagalog");
		case SC('t','n'): return PyUnicode_FromString("Setswana");
		case SC('t','o'): return PyUnicode_FromString("Tonga");
		case SC('t','r'): return PyUnicode_FromString("Turkish");
		case SC('t','s'): return PyUnicode_FromString("Tsonga");
		case SC('t','t'): return PyUnicode_FromString("Tatar");
		case SC('t','w'): return PyUnicode_FromString("Twi");
		case SC('u','g'): return PyUnicode_FromString("Uighur");
		case SC('u','k'): return PyUnicode_FromString("Ukrainian");
		case SC('u','r'): return PyUnicode_FromString("Urdu");
		case SC('u','z'): return PyUnicode_FromString("Uzbek");
		case SC('v','i'): return PyUnicode_FromString("Vietnamese");
		case SC('v','o'): return PyUnicode_FromString("Volapuk");
		case SC('w','o'): return PyUnicode_FromString("Wolof");
		case SC('x','h'): return PyUnicode_FromString("Xhosa");
		case SC('y','i'): return PyUnicode_FromString("Yiddish");
		case SC('y','o'): return PyUnicode_FromString("Yoruba");
		case SC('z','a'): return PyUnicode_FromString("Zhuang");
		case SC('z','h'): return PyUnicode_FromString("Chinese");
		case SC('z','u'): return PyUnicode_FromString("Zulu");
		case SC('x','x'): return PyUnicode_FromString("Unknown");
	}

	return PyUnicode_FromString("Not Specified");
}
#undef SC

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// PyObject types structs

typedef struct {
	PyObject_HEAD
	PyObject *path;
	dvd_reader_t *dvd;

	PyObject *TitleClass;

	int numifos;
	ifo_handle_t **ifos;

	int numtitles;
} DVD;

typedef struct {
	PyObject_HEAD
	int ifonum;
	int titlenum;

	PyObject *AudioClass;
	PyObject *ChapterClass;
	PyObject *SubpictureClass;

	DVD *dvd;

	ifo_handle_t *ifo;

	int numangles;
	int numaudios;
	int numsubpictures;
	int numchapters;
} Title;

typedef struct {
	PyObject_HEAD
	int audionum;

	Title *title;

	audio_attr_t *audio;
} Audio;

typedef struct {
	PyObject_HEAD
	int chapternum;

	Title *title;

	int startcell;
	int endcell;
	long lenms;
	PyObject *lenfancy;
} Chapter;

typedef struct {
	PyObject_HEAD
	int subpicturenum;

	Title *title;

	subp_attr_t *subpicture;
} Subpicture;

// Predefine them so they can be used below since their full definition references the functions below
static PyTypeObject DvdType;
static PyTypeObject TitleType;
static PyTypeObject AudioType;
static PyTypeObject ChapterType;
static PyTypeObject SubpictureType;

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// Administrative functions for DVD

static PyObject*
DVD_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	DVD *self;

	self = (DVD*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		Py_INCREF(Py_None);
		self->path = Py_None;
		self->dvd = NULL;

		Py_INCREF(Py_None);
		self->TitleClass = Py_None;

		self->numifos = 0;
		self->ifos = NULL;
		self->numtitles = 0;
	}

	return (PyObject *)self;
}

static int
DVD_init(DVD *self, PyObject *args, PyObject *kwds)
{
	PyObject *path=NULL, *titleclass=NULL, *tmp;
	static char *kwlist[] = {"Path", "TitleClass", NULL};

	if (! PyArg_ParseTupleAndKeywords(args,kwds, "OO", kwlist, &path, &titleclass))
	{
		return -1;
	}

	// Path
	tmp = self->path;
	Py_INCREF(path);
	self->path = path;
	Py_CLEAR(tmp);

	// TitleClass
	tmp = self->TitleClass;
	Py_INCREF(titleclass);
	self->TitleClass = titleclass;
	Py_CLEAR(tmp);

	return 0;
}

static void
DVD_dealloc(DVD *self)
{
	Py_CLEAR(self->path);
	Py_CLEAR(self->TitleClass);

	if (self->ifos)
	{
		for (int i=0; i < self->numifos; i++)
		{
			if (self->ifos[i])
			{
				ifoClose(self->ifos[i]);
				self->ifos[i] = NULL;
			}
		}
		free(self->ifos);
	}

	if (self->dvd)
	{
		DVDClose(self->dvd);
	}
	self->dvd = NULL;

	self->numifos = 0;
	self->numtitles = 0;

	Py_TYPE(self)->tp_free((PyObject*)self);
}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// Interface stuff for DVD

static PyObject*
DVD_getPath(DVD *self)
{
	if (self->path == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, "_path");
		return NULL;
	}

	Py_INCREF(self->path);
	return self->path;
}

static int
_DVD_getIsOpen(DVD *self)
{
	return !!self->dvd;
}

static PyObject*
DVD_getIsOpen(DVD *self)
{
	if ( _DVD_getIsOpen(self) )
	{
		Py_INCREF(Py_True);
		return Py_True;
	}
	else
	{
		Py_INCREF(Py_False);
		return Py_False;
	}
}

static PyObject*
DVD_GetVMGID(DVD *self)
{
	if (!_DVD_getIsOpen(self))
	{
		PyErr_SetString(PyExc_AttributeError, "VMGID: disc not open");
		return NULL;
	}

	char buf[13];
	strncpy(buf, self->ifos[0]->vmgi_mat->vmg_identifier, 12);
	buf[12] = '\0';

	return PyUnicode_FromString(buf);
}

static PyObject*
DVD_GetProviderID(DVD *self)
{
	if (!_DVD_getIsOpen(self))
	{
		PyErr_SetString(PyExc_AttributeError, "ProviderID: disc not open");
		return NULL;
	}

	char buf[33];
	strncpy(buf, self->ifos[0]->vmgi_mat->provider_identifier, 32);
	buf[32] = '\0';

	return PyUnicode_FromString(buf);
}

static PyObject*
DVD_GetNumberOfTitles(DVD *self)
{
	if (!_DVD_getIsOpen(self))
	{
		PyErr_SetString(PyExc_AttributeError, "NumberOfTitles: disc not open");
		return NULL;
	}

	return PyLong_FromLong((long)self->numtitles);
}



static PyObject*
DVD_Open(DVD *self)
{
	// Ensure not already open
	if (_DVD_getIsOpen(self))
	{
		PyErr_SetString(PyExc_Exception, "Device is already open, first Close() it to re-open");
		return NULL;
	}

	// Ensure path is present
	if (self->path == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, "_path");
		return NULL;
	}


	// Open the DVD
	char *path = PyUnicode_AsUTF8(self->path);
	if (path == NULL)
	{
		// Not responsible for clearing char*
		return NULL;
	}

	struct stat s;
	if (stat(path, &s))
	{
		PyErr_SetString(PyExc_ValueError, "Device/file not found");
		return NULL;
	}

	// Open the DVD
	self->dvd = DVDOpen(path);
	if (self->dvd == NULL)
	{
		PyErr_SetString(PyExc_Exception, "Could not open device");
		return NULL;
	}

	// Get the root IFO
	ifo_handle_t *zero = ifoOpen(self->dvd, 0);
	if (zero == NULL)
	{
		PyErr_SetString(PyExc_Exception, "Could not open IFO zero");
		goto error;
	}

	// Get number of IFOs and create pointer space for the ifo_handle_t pointers
	self->numifos = zero->vts_atrt->nr_of_vtss;
	self->ifos = (ifo_handle_t**)calloc(self->numifos+1, sizeof(ifo_handle_t*));
	self->ifos[0] = zero;

	// Get all IFOs
	for (int i=1; i <= self->numifos; i++)
	{
		self->ifos[i] = ifoOpen(self->dvd, i);
		if (!self->ifos[i])
		{
			PyErr_Format(PyExc_Exception, "Could not open IFO %d", i);
			goto error;
		}
	}

	// Get total number of titles on the device
	self->numtitles = zero->tt_srpt->nr_of_srpts;

	// return None for success
	Py_INCREF(Py_None);
	return Py_None;

error:

	// If zero was set but not the ifos array, otherwise next block will take care of zero by closing self->ifso[0]
	if (zero && self->ifos == NULL)
	{
		ifoClose(zero);
	}
	zero = NULL;

	if (self->ifos)
	{
		// Close and null each open IFO
		for (int i=0; i < self->numifos; i++)
		{
			if (self->ifos[i])
			{
				ifoClose(self->ifos[i]);
				self->ifos[i] = NULL;
			}
		}
		// Free the array
		free(self->ifos);
	}
	self->ifos = NULL;

	// Close the dvd
	if (self->dvd)
	{
		DVDClose(self->dvd);
	}
	self->dvd = NULL;

	return NULL;
}

static PyObject*
DVD_Close(DVD *self)
{
	// Ensure not closed already, or never was open
	if (!_DVD_getIsOpen(self))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot close it");
		return NULL;
	}

	// NB: leave path set

	// Close ifos
	if (self->ifos)
	{
		for (int i=0; i < self->numifos; i++)
		{
			ifoClose(self->ifos[i]);
			self->ifos[i] = NULL;
		}
		free(self->ifos);
	}
	self->ifos = NULL;

	// Close and null the pointer
	if (self->dvd)
	{
		DVDClose(self->dvd);
	}
	self->dvd = NULL;

	// return None for success
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject*
DVD_GetTitle(DVD *self, PyObject *args)
{
	// Ensure DVD is open before getting titles
	if (!_DVD_getIsOpen(self))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot get titles");
		return NULL;
	}

	int title=0;

	if (! PyArg_ParseTuple(args, "i", &title))
	{
		return NULL;
	}

	// Bounds check
	if (title < 1)
	{
		PyErr_Format(PyExc_ValueError, "Title must be non-negative (%d)", title);
		return NULL;
	}
	if (title > self->numtitles)
	{
		PyErr_Format(PyExc_ValueError, "Title value too large (%d > %d)", title, self->numtitles);
		return NULL;
	}

	// Get IFO number
	int ifonum = self->ifos[0]->tt_srpt->title[title-1].title_set_nr;

	PyObject *a = Py_BuildValue("Oii", self, ifonum, title);
	if (a == NULL)
	{
		return NULL;
	}

	return PyObject_CallObject(self->TitleClass, a);
}




static PyMemberDef DVD_members[] = {
	{"_path", T_OBJECT_EX, offsetof(DVD, path), 0, "Path of DVD device"},
	{NULL}
};

static PyMethodDef DVD_methods[] = {
	{"Open", (PyCFunction)DVD_Open, METH_NOARGS, "Opens the device for reading"},
	{"Close", (PyCFunction)DVD_Close, METH_NOARGS, "Closes the device"},
	{"GetTitle", (PyCFunction)DVD_GetTitle, METH_VARARGS, "Gets Title object for specified non-negative title number"},
	{NULL}
};

static PyGetSetDef DVD_getseters[] = {
	{"IsOpen", (getter)DVD_getIsOpen, NULL, "Gets flag indicating if device is open or not", NULL},
	{"Path", (getter)DVD_getPath, NULL, "Get the path to the DVD device", NULL},
	{"VMGID", (getter)DVD_GetVMGID, NULL, "Gets the VMD ID", NULL},
	{"ProviderID", (getter)DVD_GetProviderID, NULL, "Gets the Provider ID", NULL},
	{"NumberOfTitles", (getter)DVD_GetNumberOfTitles, NULL, "Gets the number of titles", NULL},
	{NULL}
};

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// Administrative functions for Title

static PyObject*
Title_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Title *self;

	self = (Title*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		self->ifonum = 0;
		self->titlenum = 0;

		Py_INCREF(Py_None);Py_INCREF(Py_None);Py_INCREF(Py_None);
		self->AudioClass = Py_None;
		self->ChapterClass = Py_None;
		self->SubpictureClass = Py_None;

		self->dvd = NULL;
		self->ifo = NULL;

		self->numangles = 0;
		self->numaudios = 0;
		self->numsubpictures = 0;
		self->numchapters = 0;
	}

	return (PyObject*)self;
}

static int
Title_init(Title *self, PyObject *args, PyObject *kwds)
{
	PyObject *_dvd=NULL, *audioclass=NULL, *chapterclass=NULL, *subpictureclass=NULL, *tmp;
	int titlenum=0, ifonum=0;
	static char *kwlist[] = {"DVD", "IFONum", "TitleNum", "AudioClass", "ChapterClass", "SubpictureClass", NULL};

	// Parse arguments
	if (! PyArg_ParseTupleAndKeywords(args,kwds, "OiiOOO", kwlist, &_dvd, &ifonum, &titlenum, &audioclass, &chapterclass, &subpictureclass))
	{
		return -1;
	}

	// Ensure DVD is correct type
	if (! PyObject_TypeCheck(_dvd, &DvdType))
	{
		PyErr_SetString(PyExc_TypeError, "DVD incorrect type");
		return -1;
	}
	DVD *dvd = (DVD*)_dvd;

	// Bounds check on ifonum
	if (ifonum < 0)
	{
		PyErr_Format(PyExc_ValueError, "IFO number cannot be negative (%d)", ifonum);
		return -1;
	}
	if (ifonum > dvd->numifos)
	{
		PyErr_Format(PyExc_ValueError, "IFO number is too large (%d > %d)", ifonum, dvd->numifos);
		return -1;
	}
	self->ifonum = ifonum;

	// Bounds check on title
	if (titlenum < 0)
	{
		PyErr_Format(PyExc_ValueError, "Title number cannot be negative (%d)", titlenum);
		return -1;
	}
	if (titlenum > dvd->numtitles)
	{
		PyErr_Format(PyExc_ValueError, "Title number is too large (%d > %d)", titlenum, dvd->numtitles);
		return -1;
	}
	self->titlenum = titlenum;

	// Get ifo_handle_t* for ifonum
	self->ifo = dvd->ifos[ifonum];

	// Assign DVD object
	tmp = (PyObject*)self->dvd;
	Py_INCREF(dvd);
	self->dvd = dvd;
	Py_CLEAR(tmp);

	ifo_handle_t *zero = self->dvd->ifos[0];



	pgcit_t *vts_pgcit = self->ifo->vts_pgcit;
	int vts_ttn = zero->tt_srpt->title[self->titlenum-1].vts_ttn;
	int pgcidx = self->ifo->vts_ptt_srpt->title[vts_ttn - 1].ptt[0].pgcn - 1;
	pgc_t *pgc = vts_pgcit->pgci_srp[ pgcidx ].pgc;

	// Cache these values since the struct constant isn't always correct
	self->numangles = zero->tt_srpt->title[ titlenum-1 ].nr_of_angles;
	self->numchapters = pgc->nr_of_programs;
	self->numaudios = 0;
	self->numsubpictures = 0;

	// Reported doesn't always match wath the program says
	for (int i=0; i < self->ifo->vtsi_mat->nr_of_vts_audio_streams; i++)
	{
		if (pgc->audio_control[i] & 0x8000)
		{
			self->numaudios++;
		}
	}
	for (int i=0; i < self->ifo->vtsi_mat->nr_of_vts_subp_streams; i++)
	{
		if (pgc->subp_control[i] & 0x80000000)
		{
			self->numsubpictures++;
		}
	}

	// AudioClass
	tmp = self->AudioClass;
	Py_INCREF(audioclass);
	self->AudioClass = audioclass;
	Py_CLEAR(tmp);

	// ChapterClass
	tmp = self->ChapterClass;
	Py_INCREF(chapterclass);
	self->ChapterClass = chapterclass;
	Py_CLEAR(tmp);

	// SubpictureClass
	tmp = self->SubpictureClass;
	Py_INCREF(subpictureclass);
	self->SubpictureClass = subpictureclass;
	Py_CLEAR(tmp);

	return 0;
}

static void
Title_dealloc(Title *self)
{
	self->ifonum = 0;
	self->titlenum = 0;

	Py_CLEAR(self->dvd);
	Py_CLEAR(self->AudioClass);
	Py_CLEAR(self->ChapterClass);
	Py_CLEAR(self->SubpictureClass);

	self->ifo = NULL;

	self->numangles = 0;
	self->numaudios = 0;
	self->numsubpictures = 0;
	self->numchapters = 0;

	Py_TYPE(self)->tp_free((PyObject*)self);
}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// Interface stuff for Title

static PyObject*
Title_getDVD(Title *self)
{
	return (PyObject*)self->dvd;
}

static PyObject*
Title_getTitleNum(Title *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	return PyLong_FromLong((long)self->titlenum);
}

static PyObject*
Title_getFrameRate(Title *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	ifo_handle_t *zero = self->dvd->ifos[0];

	pgcit_t *vts_pgcit = self->ifo->vts_pgcit;

	int vts_ttn = zero->tt_srpt->title[self->titlenum-1].vts_ttn;

	int pgcidx = self->ifo->vts_ptt_srpt->title[vts_ttn - 1].ptt[0].pgcn - 1;

	pgc_t *pgc = vts_pgcit->pgci_srp[ pgcidx ].pgc;
	dvd_time_t *t = &pgc->playback_time;

	int f = t->frame_u >> 6;
	if (f == 1)
	{
		return PyUnicode_FromString("25.00");
	}
	else if (f == 3)
	{
		return PyUnicode_FromString("29.97");
	}
	else
	{
		return PyUnicode_FromString("?");
	}
}

static PyObject*
Title_getAspectRatio(Title *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	char w = self->ifo->vtsi_mat->vts_video_attr.display_aspect_ratio;
	switch(w)
	{
		case 0: return PyUnicode_FromString("4:3");
		case 1: return PyUnicode_FromString("16:9");
		case 3: return PyUnicode_FromString("16:9");
		default:
			PyErr_SetString(PyExc_ValueError, "Invalid picture size value");
			return NULL;
	}
}

static PyObject*
Title_getWidth(Title *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	char w = self->ifo->vtsi_mat->vts_video_attr.picture_size;
	switch(w)
	{
		case 0: return PyLong_FromLong(720);
		case 1: return PyLong_FromLong(704);
		case 2: return PyLong_FromLong(352);
		case 3: return PyLong_FromLong(352);
		default:
			PyErr_SetString(PyExc_ValueError, "Invalid picture size value");
			return NULL;
	}
}

static PyObject*
Title_getHeight(Title *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	char h = self->ifo->vtsi_mat->vts_video_attr.video_format;
	switch(h)
	{
		case 0: return PyLong_FromLong(480);
		case 1: return PyLong_FromLong(576);
		case 3: return PyLong_FromLong(576);
		default:
			PyErr_SetString(PyExc_ValueError, "Invalid picture size value");
			return NULL;
	}
}

static PyObject*
Title_getPlaybackTime(Title *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	ifo_handle_t *zero = self->dvd->ifos[0];

	pgcit_t *vts_pgcit = self->ifo->vts_pgcit;

	int vts_ttn = zero->tt_srpt->title[self->titlenum-1].vts_ttn;

	int pgcidx = self->ifo->vts_ptt_srpt->title[vts_ttn - 1].ptt[0].pgcn - 1;

	pgc_t *pgc = vts_pgcit->pgci_srp[ pgcidx ].pgc;

	return PyLong_FromLong( dvdtimetoms( &pgc->playback_time ) );
}

static PyObject*
Title_getPlaybackTimeFancy(Title *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	ifo_handle_t *zero = self->dvd->ifos[0];

	pgcit_t *vts_pgcit = self->ifo->vts_pgcit;

	int vts_ttn = zero->tt_srpt->title[self->titlenum-1].vts_ttn;

	int pgcidx = self->ifo->vts_ptt_srpt->title[vts_ttn - 1].ptt[0].pgcn - 1;

	pgc_t *pgc = vts_pgcit->pgci_srp[ pgcidx ].pgc;

	return dvdtimetofancy( dvdtimetoms( &pgc->playback_time ), (pgc->playback_time.frame_u >> 6) );
}

static PyObject*
Title_getNumberOfAngles(Title *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	return PyLong_FromLong( self->numangles );
}

static PyObject*
Title_getNumberOfAudios(Title *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	return PyLong_FromLong( self->numaudios );
}

static PyObject*
Title_getNumberOfSubpictures(Title *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	return PyLong_FromLong( self->numsubpictures );
}

static PyObject*
Title_getNumberOfChapters(Title *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	return PyLong_FromLong( self->numchapters );
}




static PyObject*
Title_GetAudio(Title *self, PyObject *args)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}

	int audionum;

	if (! PyArg_ParseTuple(args, "i", &audionum))
	{
		return NULL;
	}

	// Bounds check
	if (audionum < 1)
	{
		PyErr_Format(PyExc_ValueError, "Audio track must be positive (%d)", audionum);
		return NULL;
	}
	if (audionum > self->numaudios)
	{
		PyErr_Format(PyExc_ValueError, "Audio track too large (%d > %d)", audionum, self->numaudios);
		return NULL;
	}

	PyObject *a = Py_BuildValue("Oi", self, audionum);
	if (a == NULL)
	{
		return NULL;
	}

	return PyObject_CallObject(self->AudioClass, a);
}

static PyObject*
Title_GetChapter(Title *self, PyObject *args)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	int chapternum;

	if (! PyArg_ParseTuple(args, "i", &chapternum))
	{
		return NULL;
	}

	// Bounds check
	if (chapternum < 1)
	{
		PyErr_Format(PyExc_ValueError, "Chapter must be positive (%d)", chapternum);
		return NULL;
	}
	if (chapternum > self->numchapters)
	{
		PyErr_Format(PyExc_ValueError, "Chapter track too large (%d > %d)", chapternum, self->numchapters);
		return NULL;
	}

	int startcell, endcell, lenms=0;
	PyObject *lenfancy = NULL;


	ifo_handle_t *zero = self->dvd->ifos[0];
	pgcit_t *vts_pgcit = self->ifo->vts_pgcit;
	int vts_ttn = zero->tt_srpt->title[self->titlenum-1].vts_ttn;
	int pgcidx = self->ifo->vts_ptt_srpt->title[vts_ttn - 1].ptt[0].pgcn - 1;
	pgc_t *pgc = vts_pgcit->pgci_srp[ pgcidx ].pgc;

	// Start and end cells are easy
	startcell = pgc->program_map[ chapternum-1 ];

	if (chapternum == self->numchapters)
	{
		endcell = pgc->nr_of_cells;
	}
	else
	{
		endcell = pgc->program_map[ chapternum-1 + 1] - 1;
	}

	// It's possible that the endcell can be calculated to be negative...so just assume zero???
	if (endcell < 0)
	{
		endcell = 0;
	}

	// Length requires iterating over cells
	lenms = 0;
	for (int i=startcell; i <= endcell; i++)
	{
		lenms += dvdtimetoms( &pgc->cell_playback[i-1].playback_time );
	}
	lenfancy = dvdtimetofancy( lenms, (pgc->cell_playback[startcell-1].playback_time.frame_u >> 6) );


	PyObject *a = Py_BuildValue("OiiilO", self, chapternum, startcell, endcell, lenms, lenfancy);
	if (a == NULL)
	{
		return NULL;
	}

	return PyObject_CallObject(self->ChapterClass, a);
}

static PyObject*
Title_GetSubpicture(Title *self, PyObject *args)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	int subpicturenum;

	if (! PyArg_ParseTuple(args, "i", &subpicturenum))
	{
		return NULL;
	}

	// Bounds check
	if (subpicturenum < 1)
	{
		PyErr_Format(PyExc_ValueError, "Subpicture track must be positive (%d)", subpicturenum);
		return NULL;
	}
	if (subpicturenum > self->numsubpictures)
	{
		PyErr_Format(PyExc_ValueError, "Subpicture track too large (%d > %d)", subpicturenum, self->numsubpictures);
		return NULL;
	}

	PyObject *a = Py_BuildValue("Oi", self, subpicturenum);
	if (a == NULL)
	{
		return NULL;
	}

	return PyObject_CallObject(self->SubpictureClass, a);
}




static PyMemberDef Title_members[] = {
	{NULL}
};

static PyMethodDef Title_methods[] = {
	{"GetAudio", (PyCFunction)Title_GetAudio, METH_VARARGS, "Gets the specified audio track of this title"},
	{"GetChapter", (PyCFunction)Title_GetChapter, METH_VARARGS, "Gets the specified chapter of this title"},
	{"GetSubpicture", (PyCFunction)Title_GetSubpicture, METH_VARARGS, "Gets the specified subpicture of this title"},
	{NULL}
};

static PyGetSetDef Title_getseters[] = {
	{"DVD", (getter)Title_getDVD, NULL, "Gets the DVD this title is associated with", NULL},
	{"AspectRatio", (getter)Title_getAspectRatio, NULL, "Gets the aspect ratio", NULL},
	{"FrameRate", (getter)Title_getFrameRate, NULL, "Gets the frame rate", NULL},
	{"PlaybackTime", (getter)Title_getPlaybackTime, NULL, "Gets the playback time in milliseconds", NULL},
	{"PlaybackTimeFancy", (getter)Title_getPlaybackTimeFancy, NULL, "Gets the playback time in fancy HH:MM:SS.FF string format", NULL},
	{"TitleNum", (getter)Title_getTitleNum, NULL, "Gets the number associated with this Title", NULL},
	{"Width", (getter)Title_getWidth, NULL, "Gets the width of the video in pixels", NULL},
	{"Height", (getter)Title_getHeight, NULL, "Gets the height of the video in pixels", NULL},
	{"NumberOfAngles", (getter)Title_getNumberOfAngles, NULL, "Gets the number of angles in this title", NULL},
	{"NumberOfAudios", (getter)Title_getNumberOfAudios, NULL, "Gets the number of audio tracks in this title", NULL},
	{"NumberOfChapters", (getter)Title_getNumberOfChapters, NULL, "Gets the number of chapters in this title", NULL},
	{"NumberOfSubpictures", (getter)Title_getNumberOfSubpictures, NULL, "Gets the number of subpictures in this title", NULL},
	{NULL}
};

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// Administrative functions for Audio

static PyObject*
Audio_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Audio *self;

	self = (Audio*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		self->audionum = 0;
		self->audio = NULL;

		self->title = NULL;
	}

	return (PyObject*)self;
}

static int
Audio_init(Audio *self, PyObject *args, PyObject *kwds)
{
	PyObject *_title=NULL, *tmp;
	int audionum=0;
	static char *kwlist[] = {"Title", "AudioNum", NULL};

	// Parse arguments
	if (! PyArg_ParseTupleAndKeywords(args,kwds, "Oi", kwlist, &_title, &audionum))
	{
		return -1;
	}

	// Ensure Title is correct type
	if (! PyObject_TypeCheck(_title, &TitleType))
	{
		PyErr_SetString(PyExc_TypeError, "Title incorrect type");
		return -1;
	}
	Title *title = (Title*)_title;

	// Ensure device is open to access it
	if (!_DVD_getIsOpen(title->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return -1;
	}



	// Bounds check on audionum
	if (audionum < 0)
	{
		PyErr_Format(PyExc_ValueError, "Audio number cannot be negative (%d)", audionum);
		return -1;
	}
	if (audionum > title->numaudios)
	{
		PyErr_Format(PyExc_ValueError, "Audio number is too large (%d > %d)", audionum, title->numaudios);
		return -1;
	}
	self->audionum = audionum;


	ifo_handle_t *zero = title->dvd->ifos[0];
	pgcit_t *vts_pgcit = title->ifo->vts_pgcit;
	int vts_ttn = zero->tt_srpt->title[title->titlenum-1].vts_ttn;
	int pgcidx = title->ifo->vts_ptt_srpt->title[vts_ttn - 1].ptt[0].pgcn - 1;
	pgc_t *pgc = vts_pgcit->pgci_srp[ pgcidx ].pgc;

	// Get audio_attr_t*
	self->audio = NULL;
	for (int i=0; i < title->ifo->vtsi_mat->nr_of_vts_audio_streams; i++)
	{
		if (pgc->audio_control[i] & 0x8000)
		{
			audionum--;
			if (audionum == 0)
			{
				self->audio = &title->ifo->vtsi_mat->vts_audio_attr[i];
				break;
			}
		}
	}



	// Assign Title object
	tmp = (PyObject*)self->title;
	Py_INCREF(title);
	self->title = title;
	Py_CLEAR(tmp);

	return 0;
}

static void
Audio_dealloc(Audio *self)
{
	self->audionum = 0;

	Py_CLEAR(self->title);
	self->audio = NULL;

	Py_TYPE(self)->tp_free((PyObject*)self);
}


// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// Interface stuff for Audio

static PyObject*
Audio_getTitle(Audio *self)
{
	return (PyObject*)self->title;
}

static PyObject*
Audio_getLangCode(Audio *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->title->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	char a = self->audio->lang_code >> 8;
	char b = self->audio->lang_code & 0xFF;

	// I guess this means "hidden" or something
	if (a == -1 && b == -1)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyUnicode_FromFormat("%c%c", a, b);
}

static PyObject*
Audio_getLanguage(Audio *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->title->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	return LangCodeToName(self->audio->lang_code);
}

static PyObject*
Audio_getFormat(Audio *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->title->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	switch(self->audio->audio_format)
	{
		case 0: return PyUnicode_FromString("AC3");
		case 2: return PyUnicode_FromString("MPEG1");
		case 3: return PyUnicode_FromString("MPEG2");
		case 4: return PyUnicode_FromString("LPCM");
		case 5: return PyUnicode_FromString("SDDS");
		case 6: return PyUnicode_FromString("DTS");
	}

	return PyUnicode_FromString("?");
}

static PyObject*
Audio_getSamplingRate(Audio *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->title->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	// Apparently it's either 48kHz or 48kHz
	return PyUnicode_FromString("48000");
}





static PyMemberDef Audio_members[] = {
	{NULL}
};

static PyMethodDef Audio_methods[] = {
	{NULL}
};

static PyGetSetDef Audio_getseters[] = {
	{"Title", (getter)Audio_getTitle, NULL, "Gets the title this track is associated with", NULL},
	{"Format", (getter)Audio_getFormat, NULL, "Gets the format", NULL},
	{"LangCode", (getter)Audio_getLangCode, NULL, "Gets the language code", NULL},
	{"Language", (getter)Audio_getLanguage, NULL, "Gets the language", NULL},
	{"SamplingRate", (getter)Audio_getSamplingRate, NULL, "Gets the sampling rate", NULL},
	{NULL}
};

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// Administrative functions for Chapter

static PyObject*
Chapter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Chapter *self;

	self = (Chapter*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		self->chapternum = 0;

		self->title = NULL;

		self->startcell = 0;
		self->endcell = 0;

		self->lenms = 0;

		Py_INCREF(Py_None);
		self->lenfancy = Py_None;
	}

	return (PyObject*)self;
}

static int
Chapter_init(Chapter *self, PyObject *args, PyObject *kwds)
{
	PyObject *_title=NULL, *tmp;
	int chapternum=0, start=0, end=0;
	long lenms=0;
	PyObject *lenfancy=NULL;
	static char *kwlist[] = {"Title", "ChapterNum", "StartCell", "EndCell", "LenMS", "LenFancy", NULL};

	// Parse arguments
	if (! PyArg_ParseTupleAndKeywords(args,kwds, "OiiilO", kwlist, &_title, &chapternum, &start, &end, &lenms, &lenfancy))
	{
		return -1;
	}

	// Ensure Title is correct type
	if (! PyObject_TypeCheck(_title, &TitleType))
	{
		PyErr_SetString(PyExc_TypeError, "Title incorrect type");
		return -1;
	}
	Title *title = (Title*)_title;

	// Ensure device is open to access it
	if (!_DVD_getIsOpen(title->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return -1;
	}



	// Bounds check on chapternum
	if (chapternum < 0)
	{
		PyErr_Format(PyExc_ValueError, "Chapter number cannot be negative (%d)", chapternum);
		return -1;
	}
	if (chapternum > title->numchapters)
	{
		PyErr_Format(PyExc_ValueError, "Chapter number is too large (%d > %d)", chapternum, title->numchapters);
		return -1;
	}
	self->chapternum = chapternum;

	// Bounds check on start
	if (start < 0)
	{
		PyErr_Format(PyExc_ValueError, "Start cell number cannot be negative (%d)", start);
		return -1;
	}
	self->startcell = start;

	// Bounds check on end
	if (end < 0)
	{
		PyErr_Format(PyExc_ValueError, "End cell number cannot be negative (%d)", end);
		return -1;
	}
	self->endcell = end;

	self->lenms = lenms;


	// Assign fancy length
	tmp = (PyObject*)self->lenfancy;
	Py_INCREF(lenfancy);
	self->lenfancy = lenfancy;
	Py_CLEAR(tmp);


	// Assign Title object
	tmp = (PyObject*)self->title;
	Py_INCREF(title);
	self->title = title;
	Py_CLEAR(tmp);

	return 0;
}

static void
Chapter_dealloc(Chapter *self)
{
	self->chapternum = 0;
	self->startcell = 0;
	self->endcell = 0;
	self->lenms = 0;

	Py_CLEAR(self->title);
	Py_CLEAR(self->lenfancy);

	Py_TYPE(self)->tp_free((PyObject*)self);
}


// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// Interface stuff for Chapter

static PyObject*
Chapter_getTitle(Audio *self)
{
	return (PyObject*)self->title;
}

static PyObject*
Chapter_getChapterNum(Chapter *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->title->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	return PyLong_FromLong(self->chapternum);
}

static PyObject*
Chapter_getStartCell(Chapter *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->title->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	return PyLong_FromLong(self->startcell);
}

static PyObject*
Chapter_getEndCell(Chapter *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->title->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	return PyLong_FromLong(self->endcell);
}

static PyObject*
Chapter_getLength(Chapter *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->title->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	return PyLong_FromLong(self->lenms);
}

static PyObject*
Chapter_getLengthFancy(Chapter *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->title->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	Py_INCREF(self->lenfancy);
	return self->lenfancy;
}





static PyMemberDef Chapter_members[] = {
	{NULL}
};

static PyMethodDef Chapter_methods[] = {
	{NULL}
};

static PyGetSetDef Chapter_getseters[] = {
	{"Title", (getter)Chapter_getTitle, NULL, "Gets the title this chapter is associated with", NULL},
	{"ChapterNum", (getter)Chapter_getChapterNum, NULL, "Gets the chapter number", NULL},
	{"StartCell", (getter)Chapter_getStartCell, NULL, "Gets the starting cell for this chapter", NULL},
	{"EndCell", (getter)Chapter_getEndCell, NULL, "Gets the ending cell for this chapter", NULL},
	{"Length", (getter)Chapter_getLength, NULL, "Gets the length of the chapter in milliseconds", NULL},
	{"LengthFancy", (getter)Chapter_getLengthFancy, NULL, "Gets the length of the chapter in HH:MM:SS:FF string format", NULL},
	{NULL}
};

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// Administrative functions for Subpicture

static PyObject*
Subpicture_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Subpicture *self;

	self = (Subpicture*)type->tp_alloc(type, 0);
	if (self != NULL)
	{
		self->subpicturenum = 0;

		self->title = NULL;

		self->subpicture = NULL;
	}

	return (PyObject*)self;
}

static int
Subpicture_init(Subpicture *self, PyObject *args, PyObject *kwds)
{
	PyObject *_title=NULL, *tmp;
	int subpicturenum=0;
	static char *kwlist[] = {"Title", "SubpictureNum", NULL};

	// Parse arguments
	if (! PyArg_ParseTupleAndKeywords(args,kwds, "Oi", kwlist, &_title, &subpicturenum))
	{
		return -1;
	}

	// Ensure Title is correct type
	if (! PyObject_TypeCheck(_title, &TitleType))
	{
		PyErr_SetString(PyExc_TypeError, "Title incorrect type");
		return -1;
	}
	Title *title = (Title*)_title;

	// Ensure device is open to access it
	if (!_DVD_getIsOpen(title->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return -1;
	}



	// Bounds check on subpicturenum
	if (subpicturenum < 0)
	{
		PyErr_Format(PyExc_ValueError, "Subpicture number cannot be negative (%d)", subpicturenum);
		return -1;
	}
	if (subpicturenum > title->numsubpictures)
	{
		PyErr_Format(PyExc_ValueError, "Subpicture number is too large (%d > %d)", subpicturenum, title->numsubpictures);
		return -1;
	}
	self->subpicturenum = subpicturenum;


	ifo_handle_t *zero = title->dvd->ifos[0];
	pgcit_t *vts_pgcit = title->ifo->vts_pgcit;
	int vts_ttn = zero->tt_srpt->title[title->titlenum-1].vts_ttn;
	int pgcidx = title->ifo->vts_ptt_srpt->title[vts_ttn - 1].ptt[0].pgcn - 1;
	pgc_t *pgc = vts_pgcit->pgci_srp[ pgcidx ].pgc;

	// Find subpicture
	int found = 0;
	for (int i=0; i < title->ifo->vtsi_mat->nr_of_vts_subp_streams; i++)
	{
		if (pgc->subp_control[i] & 0x80000000)
		{
			found++;
			if (found == subpicturenum)
			{
				self->subpicture = title->ifo->vtsi_mat->vts_subp_attr;
				break;
			}
		}
	}
	if (self->subpicture == NULL)
	{
		PyErr_Format(PyExc_ValueError, "Subpicture number too large (%d > %d)", found, subpicturenum);
		return -1;
	}

	// Assign Title object
	tmp = (PyObject*)self->title;
	Py_INCREF(title);
	self->title = title;
	Py_CLEAR(tmp);

	return 0;
}

static void
Subpicture_dealloc(Subpicture *self)
{
	self->subpicturenum = 0;

	Py_CLEAR(self->title);

	self->subpicture = NULL;

	Py_TYPE(self)->tp_free((PyObject*)self);
}


// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// Interface stuff for Subpicture

static PyObject*
Subpicture_getTitle(Audio *self)
{
	return (PyObject*)self->title;
}

static PyObject*
Subpicture_getLangCode(Subpicture *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->title->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	char a = self->subpicture->lang_code >> 8;
	char b = self->subpicture->lang_code & 0xFF;

	// I guess this means "hidden" or something
	if (a == -1 && b == -1)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyUnicode_FromFormat("%c%c", a, b);
}

static PyObject*
Subpicture_getLanguage(Subpicture *self)
{
	// Ensure device is open to access it
	if (!_DVD_getIsOpen(self->title->dvd))
	{
		PyErr_SetString(PyExc_Exception, "Device not open, cannot read from it");
		return NULL;
	}


	return LangCodeToName(self->subpicture->lang_code);
}




static PyMemberDef Subpicture_members[] = {
	{NULL}
};

static PyMethodDef Subpicture_methods[] = {
	{NULL}
};

static PyGetSetDef Subpicture_getseters[] = {
	{"Title", (getter)Subpicture_getTitle, NULL, "Gets the title this subpicture is associated with", NULL},
	{"LangCode", (getter)Subpicture_getLangCode, NULL, "Gets the language code", NULL},
	{"Language", (getter)Subpicture_getLanguage, NULL, "Gets the language", NULL},
	{NULL}
};

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// Fully define PyObject types now

static PyTypeObject DvdType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_dvdread.DVD",            /* tp_name */
	sizeof(DVD),               /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)DVD_dealloc,   /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_reserved */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	0,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,               /* tp_flags */
	"Represents dvd_reader_t* from libdvdread",           /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	DVD_methods,               /* tp_methods */
	DVD_members,               /* tp_members */
	DVD_getseters,             /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)DVD_init,        /* tp_init */
	0,                         /* tp_alloc */
	DVD_new,                   /* tp_new */
};

static PyTypeObject TitleType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_dvdread.Title",          /* tp_name */
	sizeof(Title),             /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)Title_dealloc, /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_reserved */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	0,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,               /* tp_flags */
	"Represents a DVD title from libdvdread",             /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	Title_methods,             /* tp_methods */
	Title_members,             /* tp_members */
	Title_getseters,           /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)Title_init,      /* tp_init */
	0,                         /* tp_alloc */
	Title_new,                 /* tp_new */
};

static PyTypeObject AudioType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_dvdread.Audio",          /* tp_name */
	sizeof(Audio),             /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)Audio_dealloc, /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_reserved */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	0,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,               /* tp_flags */
	"Represents a DVD audio track from libdvdread",             /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	Audio_methods,             /* tp_methods */
	Audio_members,             /* tp_members */
	Audio_getseters,           /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)Audio_init,      /* tp_init */
	0,                         /* tp_alloc */
	Audio_new,                 /* tp_new */
};

static PyTypeObject ChapterType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_dvdread.Chapter",        /* tp_name */
	sizeof(Chapter),           /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)Chapter_dealloc, /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_reserved */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	0,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,               /* tp_flags */
	"Represents a DVD chapter from libdvdread",           /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	Chapter_methods,           /* tp_methods */
	Chapter_members,           /* tp_members */
	Chapter_getseters,         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)Chapter_init,    /* tp_init */
	0,                         /* tp_alloc */
	Chapter_new,               /* tp_new */
};

static PyTypeObject SubpictureType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_dvdread.Subpicture",     /* tp_name */
	sizeof(Subpicture),        /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)Subpicture_dealloc, /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_reserved */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	0,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,                                /* tp_flags */
	"Represents a DVD subpicture (aka subtitle) from libdvdread",          /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	Subpicture_methods,        /* tp_methods */
	Subpicture_members,        /* tp_members */
	Subpicture_getseters,      /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)Subpicture_init, /* tp_init */
	0,                         /* tp_alloc */
	Subpicture_new,            /* tp_new */
};

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// Define the module

static PyMethodDef DVDReadModuleMethods[] = {
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef DvdReadmodule = {
	PyModuleDef_HEAD_INIT,
	"_dvdread",
	"Python wrapper for libdvdread4", // Doc
	-1, // state in global vars
	DVDReadModuleMethods
};

// Python expects a function named "PyInit_%s" % ModuleName, and since the module name is "_dvdread" there are two underscores
PyMODINIT_FUNC
PyInit__dvdread(void)
{
	// Ready the types
	if (PyType_Ready(&DvdType) < 0) { return NULL; }
	if (PyType_Ready(&TitleType) < 0) { return NULL; }
	if (PyType_Ready(&AudioType) < 0) { return NULL; }
	if (PyType_Ready(&ChapterType) < 0) { return NULL; }
	if (PyType_Ready(&SubpictureType) < 0) { return NULL; }

	// Create the module defined in the struct above
	PyObject *m = PyModule_Create(&DvdReadmodule);
	if (m == NULL)
	{
		return NULL;
	}

	// Not sure of a better way to do this, but form a string containing the version
	char v[32];
	sprintf(v, "%d.%d", MAJOR_VERSION, MINOR_VERSION);

	// Add the types to the module
	Py_INCREF(&DvdType);
	Py_INCREF(&TitleType);
	Py_INCREF(&AudioType);
	Py_INCREF(&ChapterType);
	Py_INCREF(&SubpictureType);
	PyModule_AddObject(m, "DVD", (PyObject*)&DvdType);
	PyModule_AddObject(m, "Title", (PyObject*)&TitleType);
	PyModule_AddObject(m, "Audio", (PyObject*)&AudioType);
	PyModule_AddObject(m, "Chapter", (PyObject*)&ChapterType);
	PyModule_AddObject(m, "Subpicture", (PyObject*)&SubpictureType);
	// Add the version as a string to the version
	PyModule_AddStringConstant(m, "Version", v);

	// Return the module object
	return m;
}

