#ifndef _Browser_Browser_h
#define _Browser_Browser_h

#include <CtrlLib/CtrlLib.h>
#include <CppBase/CppBase.h>
#include <ide/Common/Common.h>

#define LAYOUTFILE <ide/Browser/Browser.lay>
#include <CtrlCore/lay.h>

#define IMAGECLASS BrowserImg
#define IMAGEFILE <ide/Browser/Browser.iml>
#include <Draw/iml_header.h>

enum { WITHBODY = 33 };

struct CppNestingInfo {
	String nesting;
	int    namespacel;
};

struct CppItemInfo : CppSimpleItem, CppNestingInfo {
	String key;
	String name;
	bool   over;
	bool   overed;
	int    inherited;
	String fn;
	int    line;
	int    typei;
	int    nesti;
	
	CppItemInfo() { over = overed = virt = false; inherited = line = namespacel = 0; }
};

enum {
	ITEM_TEXT,
	ITEM_NAME,
	ITEM_CPP,
	ITEM_PNAME,
	ITEM_TNAME,
	ITEM_NUMBER,
	ITEM_SIGN
};

struct ItemTextPart : Moveable<ItemTextPart> {
	int pos;
	int len;
	int type;
};

Vector<ItemTextPart> ParseItemNatural(const CppItemInfo& m, const char *natural);
Vector<ItemTextPart> ParseItemNatural(const CppItemInfo& m);

bool SplitNestKey(const String& s, String& nest, String& key);

struct BrowserFileInfo {
	Time     time;
	String   package;
	String   file;
	
	BrowserFileInfo() { time = Null; }
};

ArrayMap<String, BrowserFileInfo>& FileSet();

int GetItemHeight(const CppSimpleItem& m, int cx);

struct BrowserQuery {
	String name;
	String package;
	String file;
	bool   a_private, a_protected, a_public;
	bool   code, data, type, macro;
	bool   documented, notdocumented;
	
	void Clear();
	void Serialize(Stream& s);
	
	BrowserQuery() { Clear(); }
};

struct QueryDlg : public WithQueryLayout<TopWindow> {
	typedef QueryDlg CLASSNAME;
	
	void Serialize(Stream& s);
	void EnterPackage();
	void Clear();
	
	int Perform(BrowserQuery& q);
	
	QueryDlg();
};

struct CppItemInfoDisplay : public Display
{
	String hkey;
	int    htopic;

	int DoPaint(Draw& w, const Rect& r, const Value& q,
		        Color _ink, Color paper, dword style) const;
	virtual void Paint(Draw& w, const Rect& r, const Value& q,
		               Color _ink, Color paper, dword style) const;
	virtual Size GetStdSize(const Value& q) const;
};

struct CppNestingInfoDisplay : public Display
{
	virtual void Paint(Draw& w, const Rect& r, const Value& q,
		               Color _ink, Color paper, dword style) const;
};

class ItemList : public ColumnList {
	CppItemInfoDisplay display;
	
	friend struct ItemDisplay;

	int    GetTopic(Point p, String& key);
	String Item(int i);

public:
	bool active_topics;
	
	void Clear();

	ItemList();
};

class Browser : public StaticRect {
public:
	void Serialize(Stream& s);
	void SerializeWspc(Stream& s);

	ArrayCtrl              nesting;
	ItemList               item;
	Splitter               split;
	BrowserQuery           query;
	QueryDlg               querydlg;
	Callback2<String, int> WhenPos;
	int                    pos;
	Callback               WhenItem;
	Callback               WhenItemDblClk;
	Callback1<String>      WhenShowTopic;
	bool                   clickpos;
	bool                   show_inherited;

	void     LoadNest(const String& nest, ArrayMap<String, CppItemInfo>& item, int inherited);
	bool     IsCurrentItem();
	CppItem& CurrentItem();
	void     Reload();
	void     EnterNesting();
	void     EnterItem();
	void     ItemClick();
	void     ItemDblClk();
	void     GotoItem();
	void     GotoPos(int n);
	void     ItemMenu(Bar& bar);
	void     QueryNest();
	bool     FindSet(const String& knesting, const String& kitem, int nestingsc = 0, int itemsc = 0);
	bool     FindSet(const String& item);
	
	String      GetItem(int i);
	CppItemInfo GetItemInfo(int i);

	String      GetItem()               { return GetItem(item.GetCursor()); }
	CppItemInfo GetItemInfo()           { return GetItemInfo(item.GetCursor()); }

	bool     DoQuery();
	void     DoDoQuery()                { DoQuery(); }
	void     QueryWord(const String& w);
	void     SetId(const String& id, const Vector<String>& nest);
	
	void     ShowTopic(String w);
	void     ShowHelp();

	typedef Browser CLASSNAME;
	Browser();
	~Browser();
};

Vector<String> IgnoreList();
CppBase&       BrowserBase();
void           StartBrowserBase();
void           BrowserBaseScan(Stream& s, const String& fn);
void           ClearBrowserBase();
void           RescanBrowserBase();
void           SyncBrowserBase();
void           SaveBrowserBase();
void           Register(Browser *b);
void           UnRegister(Browser *b);
bool           ExistsBrowserItem(const String& item);
void           ReQualifyBrowserBase();

void           BrowserBaseScanLay(const String& fn);
void           ScanLayFile(const char *fn);


struct RefInfo : Moveable<RefInfo> {
	String title;
	String link;
};

void   SyncRefs();
void   SyncRefsFile(const String& path, const String& link);
const  Vector<RefInfo>& GetRefInfo(const String& ref);
String GetTopicTitle(const String& link);


#endif
