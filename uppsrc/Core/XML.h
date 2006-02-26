#include <Core/Core.h>

String DeXml(const char *s, byte charset = CHARSET_DEFAULT);
String XmlPI(const char *text);
String XmlHeader(const char *encoding = "UTF-8", const char *version = "1.0", const char *standalone = "yes");
String XmlDecl(const char *text);
String XmlDocType(const char *text);
String XmlDoc(const char *name, const char *xmlbody);
String XmlComment(const char *text);


class XmlTag : Moveable<XmlTag> {
	String tag;
	String end;

public:
	XmlTag& Tag(const char *s);

	String  operator()();
	String  operator()(const char *text);
	String  operator()(const String& text)                        { return operator()(~text); }
	String  Text(const char *s, byte charset = CHARSET_DEFAULT);
	String  Text(const String& s, byte charset = CHARSET_DEFAULT) { return Text(~s, charset); }

	XmlTag& operator()(const char *attr, const char *val);
	XmlTag& operator()(const char *attr, int q);
	XmlTag& operator()(const char *attr, double q);

	XmlTag() {}
	XmlTag(const char *tag)                                       { Tag(tag); }
};

enum { XML_DOC, XML_TAG, XML_END, XML_TEXT, XML_DECL, XML_PI, XML_COMMENT, XML_EOF };

struct XmlError : public Exc
{
	XmlError(const char *s) : Exc(s) {}
};

class XmlParser {
	const char               *begin;
	const char               *term;
	VectorMap<String, String> attr;
	Vector<String>            stack;

	int                       type;
	VectorMap<String, String> nattr;
	String                    text;
	bool                      empty_tag;
	
	int                       line;
	
	void                      Ent(StringBuffer& out);
	void                      Next();

public:
	void   SkipWhites();
	
	bool   IsEof();
	
	bool   IsTag();
	String ReadTag();
	bool   Tag(const char *tag);
	void   PassTag(const char *tag);
	bool   IsEnd();
	bool   End();
	void   PassEnd();
	bool   TagE(const char *tag);
	void   PassTagE(const char *tag);
	
	int    GetAttrCount() const                               { return attr.GetCount(); }
	String GetAttr(int i) const                               { return attr.GetKey(i); }
	String operator[](int i) const                            { return attr[i]; }
	String operator[](const char *id) const                   { return attr.Get(id, Null); }
	int    Int(const char *id, int def = Null) const;
	double Double(const char *id, double def = Null) const;
	
	bool   IsText();
	String ReadRawText();
	String ReadText();
	
	bool   IsDecl();
	String ReadDecl();
	
	bool   IsPI();
	String ReadPI();
	
	bool   IsComment();
	String ReadComment();
	
	void   Skip();
	
	VectorMap<String, String> PickAttrs() pick_               { return attr; }
	
	int    GetLine() const                                    { return line; }
	int    GetColumn() const;

	XmlParser(const char *s);
};

class XmlNode {
	int                       type;
	String                    text;
	VectorMap<String, String> attr;
	Array<XmlNode>            node;

public:
	int            GetType() const                            { return type; }
	String         GetText() const                            { return text; }
	String         GetTag() const                             { return text; }
	bool           IsTag() const                              { return type == XML_TAG; }
	bool           IsTag(const char *tag) const               { return IsTag() && text == tag; }
	bool           IsText() const                             { return type == XML_TEXT; }

	void           Clear()                                    { text.Clear(); attr.Clear(); node.Clear(); type = -1; }
	void           CreateTag(const char *tag)                 { type = XML_TAG; text = tag; }
	void           CreateText(const String& txt)              { type = XML_TEXT; text = txt; }
	void           CreatePI(const String& pi)                 { type = XML_PI; text = pi; }
	void           CreateDecl(const String& decl)             { type = XML_DECL; text = decl; }
	void           CreateComment(const String& comment)       { type = XML_COMMENT; text = comment; }
	void           CreateDocument()                           { Clear(); }
	bool           IsEmpty() const                            { return type == XML_DOC && node.GetCount() == 0; }
	operator bool() const                                     { return !IsEmpty(); }

	int            GetCount() const                           { return node.GetCount(); }
	XmlNode&       At(int i)                                  { return node.At(i); }
	const XmlNode& Node(int i) const                          { return node[i]; }
	const XmlNode& operator[](int i) const                    { return node[i]; }
	const XmlNode& operator[](const char *tag) const; // throws XmlError
	XmlNode&       Add()                                      { return node.Add(); }
	int            FindTag(const char *tag) const;
	XmlNode&       Add(const char *tag);
	XmlNode&       GetAdd(const char *tag);
	XmlNode&       operator()(const char *tag)                { return GetAdd(tag); }
	void           Remove(const char *tag);

	int            GetAttrCount() const                       { return attr.GetCount(); }
	String         AttrId(int i) const                        { return attr.GetKey(i); }
	String         Attr(int i) const                          { return attr[i]; }
	String         Attr(const char *id) const                 { return attr.Get(id, Null); }
	XmlNode&       SetAttr(const char *id, const String& val);
	int            AttrInt(const char *id, int def = Null) const;
	XmlNode&       SetAttr(const char *id, int val);	

	void           SetAttrsPick(pick_ VectorMap<String, String>& a) { attr = a; }

	XmlNode()      { type = XML_DOC; }
};

enum {
	XML_IGNORE_DECLS    = 0x01,
	XML_IGNORE_PIS      = 0x02,
	XML_IGNORE_COMMENTS = 0x04,
};

XmlNode ParseXML(XmlParser& p, dword style = XML_IGNORE_DECLS|XML_IGNORE_PIS|XML_IGNORE_COMMENTS);
XmlNode ParseXML(const char *s, dword style = XML_IGNORE_DECLS|XML_IGNORE_PIS|XML_IGNORE_COMMENTS);

enum {
	XML_HEADER  = 0x01,
	XML_DOCTYPE = 0x02,
};

String  AsXML(const XmlNode& n, dword style = XML_HEADER|XML_DOCTYPE);
