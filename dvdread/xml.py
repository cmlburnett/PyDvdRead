"""
Simple and crude XML DOM implementation. It does not read XML data, only produces XML text from the node objects provided here. Attributes are supported; namespaces are not. Finally, it is possible to get a "pretty" output using the OuterXMLPretty getter property.

_node is a base class with most of the functionality, with node being the "general" node type and tnode for text nodes. The intent is that these nodes have either node children or one tnode child. This keeps things simpler. Also, unlike Real DOM Implementations, the text node is a named node with text underneath it. The

Example use:
	root = node('dvd', numtitles=d.NumberOfTitles, parser="pydvdread %s"%Version)
	root.AddChild( tnode('device', d.Path) )
	root.AddChild( tnode('name', d.GetName(), fancy=d.GetNameTitleCase()) )
	root.AddChild( tnode('vmg_id', d.VMGID) )
	root.AddChild( tnode('provider_id', d.ProviderID) )
	titles = root.AddChild( node('titles') )

Attributes are passed as named keywords to node() and tnode(), and can be accessed by using [...] on node nbjects.
node.AddChild() returns the supplied node as with @titles above.
The OuterXMLPretty for the above produces:

	<?xml version="1.0" ?>
	<dvd numtitles="7" parser="pydvdread 1.0">
		<device>/dev/sr0</device>
		<name fancy="Blood Diamond">BLOOD_DIAMOND</name>
		<vmg_id>DVDVIDEO-VMG</vmg_id>
		<provider_id>WARNER_HOME_VIDEO</provider_id>
		<titles/>
	</dvd>

Hopefully the rest is self-explanatory.
"""

from xml.sax.saxutils import escape as XMLescape
from xml.sax.saxutils import unescape as XMLunescape
from xml.dom.minidom import parseString as MDparseString

class _node:
	"""
	Base class that handles the tag name and attributes.
	"""

	_name = None
	_attrs = None
	_parent = None

	def __init__(self, name, **attrs):
		"""
		Basic node initialization with the node name (i.e., TagName) and attributes as keywords.
		"""

		self._name = name
		self._attrs = {}
		self._attrs.update(attrs)
		self._parent = None

	def __getitem__(self, k):
		"""
		Gets an attribute.
		"""
		return self._attrs[k]

	def __setitem__(self, k, v):
		"""
		Sets an attribute.
		"""
		self._attrs[k] = v

	@property
	def P(self):
		"""
		Gets this node's parent, or None if it doesn't have one.
		"""
		return self._parent

	@property
	def Name(self):
		"""
		Gets the tag name.
		"""
		return self._name

	def __repr__(self):
		return str(self)

	def __str__(self):
		"""
		Gets just the tag name and id() value. No inner tags or attributes are included.
		"""
		return "<%s at %x>" % (self.Name, id(self))

	@property
	def OuterXMLPretty(self):
		"""
		Same as OuterXML but runs through a pretty parser that uses a single tab for indent and newline per line.
		Similar to xmllint -format.
		"""

		return MDparseString(self.OuterXML).toprettyxml()

	@property
	def OuterXML(self):
		"""
		Wraps InnerXML with this tag name.
		If InnerXML is empty, then the empty tag (e.g., "<foo/>") is returned instead.
		"""
		txt = ""

		ret = self.InnerXML

		attrs = list(self._attrs.items())
		attrs.sort()
		attrstr = " ".join(['%s="%s"' % z for z in attrs])

		if len(attrstr):
			if len(ret):	return "<%s %s>%s</%s>" % (self.Name, attrstr, ret, self.Name)
			else:			return "<%s %s/>" % (self.Name, attrstr)
		else:
			if len(ret):	return "<%s>%s</%s>" % (self.Name, ret, self.Name)
			else:			return "<%s/>" % self.Name

		return txt

	@property
	def InnerXML(self):
		"""
		Requires subclasses to implement this since it varies with the node type.
		"""
		raise NotImplementedError()

class tnode(_node):
	"""
	A text node.
	Unlike Real DOM, this node is named and its only children is a single blurb of text.
	"""

	_text = None

	def __init__(self, name, text, **attrs):
		_node.__init__(self, name, **attrs)
		self._text = text

	@property
	def Text(self):
		"""
		Gets the text under this node.
		"""
		return self._text
	@Text.setter
	def setText(self, v):
		"""
		Sets the text under this node.
		"""
		self._text = v

	@property
	def InnerXML(self):
		"""
		A text node just has inner XML.
		"""
		return XMLescape(str(self.Text))

class node(_node):
	"""
	A regular node.
	This node can have children: other node() objects or tnode() objects.
	Typically, call AddChild(...) and use the return value to get the child node.
	"""

	_children = None

	def __init__(self, name, **attrs):
		_node.__init__(self, name, **attrs)
		self._children = []

	@property
	def C(self):
		"""
		Get children list as a tuple.
		"""
		return tuple(self._children)

	def AddChild(self, c):
		"""
		Adds @c as a child to this node, and sets the parent to this node.
		"""
		self._children.append(c)
		c._parent = self

		return c

	def RemoveChild(self, c):
		"""
		Removes the child @c from this node's children, and clears @c's parent pointer.
		"""
		self._children.remove(c)
		c._parent = None

	@property
	def InnerXML(self):
		"""
		A node has no text, only child nodes.
		"""
		ret = [x.OuterXML for x in self.C]
		return "".join(ret)

