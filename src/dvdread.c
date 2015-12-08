#include "dvdread.h"

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------

typedef struct {
	PyObject_HEAD
	PyObject *path;
	dvd_reader_t *dvd;

	int numifos;
	ifo_handle_t **ifos;

	int numtitles;
} DVD;

typedef struct {
	PyObject_HEAD
	int ifonum;
	int titlenum;

	DVD *dvd;

	ifo_handle_t *ifo;
} Title;

static PyTypeObject DvdType;
static PyTypeObject TitleType;

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
		self->numifos = 0;
		self->ifos = NULL;
		self->numifos = 0;
	}

	return (PyObject *)self;
}

static int
DVD_init(DVD *self, PyObject *args, PyObject *kwds)
{
	PyObject *path=NULL, *tmp;
	static char *kwlist[] = {"path", NULL};

	if (! PyArg_ParseTupleAndKeywords(args,kwds, "O", kwlist, &path))
	{
		return -1;
	}

	if (path)
	{
		tmp = self->path;
		Py_INCREF(path);
		self->path = path;
		Py_XDECREF(tmp);
	}

	return 0;
}

static void
DVD_dealloc(DVD *self)
{
	Py_XDECREF(self->path);

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

	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject*
DVD___enter__(DVD *self)
{
	// Nothing to set up
	return (PyObject*)self;
}

static PyObject*
DVD___exit__(DVD *self, PyObject *args)
{
	PyObject *type=NULL, *value=NULL, *traceback=NULL;

	if (PyArg_ParseTuple(args, "OOO", &type, &value, &traceback))
	{
		// Nothing to do
	}

	if (self->dvd)
	{
		DVDClose(self->dvd);
		self->dvd = NULL;
	}

	Py_INCREF(Py_False);
	return Py_False;
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

	return self->path;
}

static PyObject*
DVD_getIsOpen(DVD *self)
{
	if (self->dvd)
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
	char buf[13];
	strncpy(buf, self->ifos[0]->vmgi_mat->vmg_identifier, 12);
	buf[12] = '\0';

	return PyUnicode_FromString(buf);
}

static PyObject*
DVD_GetProviderID(DVD *self)
{
	char buf[33];
	strncpy(buf, self->ifos[0]->vmgi_mat->provider_identifier, 32);
	buf[32] = '\0';

	return PyUnicode_FromString(buf);
}

static PyObject*
DVD_GetNumberOfTitles(DVD *self)
{
	return PyLong_FromLong((long)self->numtitles);
}



static PyObject*
DVD_Open(DVD *self)
{
	// Ensure not already open
	if (self->dvd)
	{
		PyErr_SetString(PyExc_Exception, "Device is already open, first Close() it to re-open");
		return NULL;
	}


	// Open the DVD
	char *path = PyUnicode_AsUTF8(self->path);
	if (path == NULL)
	{
		// Not responsible for clearing char*
		return NULL;
	}

	// Open the DVD
	dvd_reader_t *dvd = DVDOpen(path);
	if (dvd == NULL)
	{
		PyErr_SetString(PyExc_Exception, "Could not open device");
		return NULL;
	}
	self->dvd = dvd;

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
			PyErr_SetString(PyExc_Exception, "Could not open IFO");
			goto error;
		}
	}

	// Get total number of titles on the device
	self->numtitles = zero->tt_srpt->nr_of_srpts;

	// return None for success
	Py_INCREF(Py_None);
	return Py_None;

error:
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

	return NULL;
}

static PyObject*
DVD_Close(DVD *self)
{
	// Ensure not closed already, or never was open
	if (self->dvd == NULL)
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
		self->ifos = NULL;
	}

	// Close and null the pointer
	if (self->dvd)
	{
		DVDClose(self->dvd);
		self->dvd = NULL;
	}

	// return None for success
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject*
DVD_GetTitle(DVD *self, PyObject *args)
{
	int title=0;

	if (! PyArg_ParseTuple(args, "i", &title))
	{
		return NULL;
	}

	// Bounds check
	if (title < 1)
	{
		PyErr_SetString(PyExc_ValueError, "Title must be non-negative");
		return NULL;
	}
	if (title > self->numtitles)
	{
		PyErr_SetString(PyExc_ValueError, "Title value too large");
		return NULL;
	}

	// Get IFO number
	int ifonum = self->ifos[0]->tt_srpt->title[title-1].title_set_nr;

	PyObject *a = Py_BuildValue("Oii", self, ifonum, title);
	if (a == NULL)
	{
		return NULL;
	}

	PyObject *t = PyObject_CallObject((PyObject*)&TitleType, a);
	Py_XDECREF(a);
	if (t == NULL)
	{
		return NULL;
	}

	return t;
}




static PyMemberDef DVD_members[] = {
	{"_path", T_OBJECT_EX, offsetof(DVD, path), 0, "Path of DVD device"},
	{NULL}
};

static PyMethodDef DVD_methods[] = {
	{"Open", (PyCFunction)DVD_Open, METH_NOARGS, "Opens the device for reading"},
	{"Close", (PyCFunction)DVD_Close, METH_NOARGS, "Closes the device"},
	{"GetTitle", (PyCFunction)DVD_GetTitle, METH_VARARGS, "Gets Title object for specified non-negative title number"},
	{"__enter__", (PyCFunction)DVD___enter__, METH_NOARGS, "Enters a context using the with keyword"},
	{"__exit__", (PyCFunction)DVD___exit__, METH_VARARGS, "Exits a context created with the with keyword"},
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

		self->dvd = NULL;
		self->ifo = NULL;
	}

	return (PyObject*)self;
}

static int
Title_init(Title *self, PyObject *args, PyObject *kwds)
{
	PyObject *_dvd=NULL, *tmp;
	int titlenum=0, ifonum=0;
	static char *kwlist[] = {"DVD", "IFONum", "TitleNum", NULL};

	// Parse arguments
	if (! PyArg_ParseTupleAndKeywords(args,kwds, "Oii", kwlist, &_dvd, &ifonum, &titlenum))
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
		PyErr_SetString(PyExc_ValueError, "IFO number cannot be negative");
		return -1;
	}
	if (ifonum > dvd->numifos)
	{
		PyErr_SetString(PyExc_ValueError, "IFO number is too large");
		return -1;
	}
	self->ifonum = ifonum;

	// Bounds check on title
	if (titlenum < 0)
	{
		PyErr_SetString(PyExc_ValueError, "Title number cannot be negative");
		return -1;
	}
	if (titlenum > dvd->numtitles)
	{
		PyErr_SetString(PyExc_ValueError, "Title number is too large");
		return -1;
	}
	self->titlenum = titlenum;

	// Get ifo_handle_t* for ifonum
	self->ifo = dvd->ifos[ifonum];

	// Assign DVD object
	tmp = (PyObject*)self->dvd;
	Py_INCREF(dvd);
	self->dvd = (DVD*)dvd;
	Py_XDECREF(tmp);

	return 0;
}

static void
Title_dealloc(Title *self)
{
	Py_XDECREF(self->dvd);
	self->ifo = NULL;

	Py_TYPE(self)->tp_free((PyObject*)self);
}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// Interface stuff for Title

static PyObject*
Title_getTitleNum(Title *self)
{
	return PyLong_FromLong((long)self->titlenum);
}

static PyObject*
Title_getFrameRate(Title *self)
{
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
Title_getPlaybackTime(Title *self)
{
	ifo_handle_t *zero = self->dvd->ifos[0];

	pgcit_t *vts_pgcit = self->ifo->vts_pgcit;

	int vts_ttn = zero->tt_srpt->title[self->titlenum-1].vts_ttn;

	int pgcidx = self->ifo->vts_ptt_srpt->title[vts_ttn - 1].ptt[0].pgcn - 1;

	pgc_t *pgc = vts_pgcit->pgci_srp[ pgcidx ].pgc;
	dvd_time_t *t = &pgc->playback_time;

	// Time is in 8421 BCD of HH:MM:SS.FF
	// with upper two bits of FF indicating frame rate (1 = 25; 3 = 29.97; 0 & 2 reserved)
	// and lower 6 bits indicated BCD frame #

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

	return PyLong_FromLong(ms);
}

static PyObject*
Title_getPlaybackTimeFancy(Title *self)
{
	ifo_handle_t *zero = self->dvd->ifos[0];

	pgcit_t *vts_pgcit = self->ifo->vts_pgcit;

	int vts_ttn = zero->tt_srpt->title[self->titlenum-1].vts_ttn;

	int pgcidx = self->ifo->vts_ptt_srpt->title[vts_ttn - 1].ptt[0].pgcn - 1;

	pgc_t *pgc = vts_pgcit->pgci_srp[ pgcidx ].pgc;
	dvd_time_t *t = &pgc->playback_time;

	// Time is in 8421 BCD of HH:MM:SS.FF
	// with upper two bits of FF indicating frame rate (1 = 25; 3 = 29.97; 0 & 2 reserved)
	// and lower 6 bits indicated BCD frame #

	long hh,h,mm,m,ss,s,ff,f;

	hh = (t->hour & 0xF0) >> 4;
	 h = (t->hour & 0x0F);

	mm = (t->minute & 0xF0) >> 4;
	 m = (t->minute & 0x0F);

	ss = (t->second & 0xF0) >> 4;
	 s = (t->second & 0x0F);

	ff = (t->frame_u & 0x30) >> 4;
	 f = (t->frame_u & 0x0F);

	// Format as string
	return PyUnicode_FromFormat("%d%d:%d%d:%d%d.%d%d", hh,h, mm,m, ss,s, ff,f);
}





static PyMemberDef Title_members[] = {
	{NULL}
};

static PyMethodDef Title_methods[] = {
	{NULL}
};

static PyGetSetDef Title_getseters[] = {
	{"FrameRate", (getter)Title_getFrameRate, NULL, "Gets the frame rate", NULL},
	{"PlaybackTime", (getter)Title_getPlaybackTime, NULL, "Gets the playback time in milliseconds", NULL},
	{"PlaybackTimeFancy", (getter)Title_getPlaybackTimeFancy, NULL, "Gets the playback time in fancy HH:MM:SS.FF string format", NULL},
	{"TitleNum", (getter)Title_getTitleNum, NULL, "Gets the number associated with this Title", NULL},
	{NULL}
};

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------


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

PyMODINIT_FUNC
PyInit__dvdread(void)
{
	if (PyType_Ready(&DvdType) < 0) { return NULL; }
	if (PyType_Ready(&TitleType) < 0) { return NULL; }

	PyObject *m = PyModule_Create(&DvdReadmodule);
	if (m == NULL)
	{
		return NULL;
	}

	char v[32];
	sprintf(v, "%d.%d", MAJOR_VERSION, MINOR_VERSION);

	Py_INCREF(&DvdType);
	PyModule_AddObject(m, "DVD", (PyObject*)&DvdType);
	PyModule_AddObject(m, "Title", (PyObject*)&TitleType);
	PyModule_AddStringConstant(m, "Version", v);

	return m;
}

