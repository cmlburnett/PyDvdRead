
from xml.sax.saxutils import escape as XMLescape
from xml.sax.saxutils import unescape as XMLunescape
from xml.dom.minidom import parseString as MDparseString

class _node:
	_name = None
	_attrs = None
	_parent = None

	def __init__(self, name, **attrs):
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
	_text = None

	def __init__(self, name, text, **attrs):
		_node.__init__(self, name, **attrs)
		self._text = text

	@property
	def Text(self):
		return self._text
	@Text.setter
	def setText(self, v):
		self._text = v

	@property
	def InnerXML(self):
		"""
		A text node just has inner XML.
		"""
		return XMLescape(str(self.Text))

class node(_node):
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

