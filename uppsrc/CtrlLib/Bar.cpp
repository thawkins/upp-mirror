#include "CtrlLib.h"

#define LLOG(x) // LOG(x)

void BarPane::Paint(Draw& w)
{
	Size sz = GetSize();
	w.DrawRect(sz, menu ? (IsXPStyle() ? SColorMenu : SLtGray) : SColorFace);
	for(int i = 0; i < breakpos.GetCount(); i++)
		if(horz) {
			w.DrawRect(0, breakpos[i], sz.cx, 1, SGray);
			w.DrawRect(0, breakpos[i] + 1, sz.cx, 1, SWhite);
		}
		else {
			w.DrawRect(breakpos[i], 0, 1, sz.cy, SGray);
			w.DrawRect(breakpos[i] + 1, 0, 1, sz.cy, SWhite);
		}
}

Size BarPane::Repos(bool _horz, int maxsize)
{
	horz = _horz;
	maxsize -= 2 * (horz ? hmargin : vmargin);
	int bmargin = horz ? vmargin : hmargin;
	breakpos.Clear();
	Size asz(0, 0);
	VeHo(asz) = bmargin;
	Array<Item>::Iterator q = item.Begin();
	while(q < item.End()) {
		Size psz(0, 0);
		Array<Item>::Iterator f = q;
		while(q < item.End()) {
			if(q->ctrl == NULL && IsNull(q->gapsize)) {
				q++;
				break;
			}
			int gapsize = q->gapsize;
			if(gapsize == INT_MAX && q->ctrl == NULL) {
				if(maxsize == INT_MAX) {
					q++;
					break;
				}
				gapsize = 0;
				Array<Item>::Iterator w = q + 1;
				while(w < item.End()) {
					if(!w->ctrl) break;
					Size sz = w->ctrl->GetMinSize();
					if(HoVe(psz) + gapsize + HoVe(sz) > maxsize) break;
					gapsize += HoVe(sz);
					w++;
				}
				gapsize = maxsize - gapsize - HoVe(psz);
			}
			Size sz = q->ctrl ? q->ctrl->GetMinSize()
			                  : Size(horz ? gapsize : 0, horz ? 0 : gapsize);
			if(HoVe(sz) == INT_MAX)
				HoVe(sz) = maxsize - HoVe(psz);
			if(HoVe(psz) + HoVe(sz) > maxsize && HoVe(psz)) {
				while(q < item.End() && q->ctrl == NULL && !IsNull(q->gapsize))
					q++;
				break;
			}
			VeHo(psz) = max(VeHo(psz), VeHo(sz));
			if(q->ctrl)
				if(horz)
					q->ctrl->LeftPos(psz.cx + hmargin, sz.cx);
				else
					q->ctrl->TopPos(psz.cy + vmargin, sz.cy);
			HoVe(psz) += HoVe(sz);
			q++;
		}
		int& bp = VeHo(asz);
		if(bp > bmargin) {
			breakpos.Add(bp + bmargin);
			bp += 2 + 2 * bmargin;
		}
		while(f != q) {
			if(f->ctrl)
				if(horz)
					f->ctrl->TopPos(bp, psz.cy);
				else
					f->ctrl->LeftPos(bp, psz.cx);
			f++;
		}
		bp += VeHo(psz);
		HoVe(asz) = max(HoVe(asz), HoVe(psz));
	}
	VeHo(asz) += bmargin;
	HoVe(asz) += horz ? 2 * hmargin : 2 * vmargin;
	LLOG("Repos " << Name() << " " << asz);
	return asz;
}

Size BarPane::GetPaneSize(bool horz, int maxsize) const
{
	maxsize -= 2 * (horz ? hmargin : vmargin);
	Size asz(0, 0);
	int bmargin = horz ? vmargin : hmargin;
	VeHo(horz, asz) = bmargin;
	Array<Item>::ConstIterator q = item.Begin();
	while(q < item.End()) {
		Size psz(0, 0);
		while(q < item.End()) {
			if(q->ctrl == NULL && IsNull(q->gapsize)) {
				q++;
				break;
			}
			int gapsize = q->gapsize;
			if(gapsize == INT_MAX && q->ctrl == NULL) {
				if(maxsize == INT_MAX) {
					q++;
					break;
				}
				gapsize = 0;
				Array<Item>::ConstIterator w = q + 1;
				while(w < item.End()) {
					if(!w->ctrl) break;
					Size sz = w->ctrl->GetMinSize();
					if(HoVe(horz, psz) + gapsize + HoVe(horz, sz) > maxsize) break;
					gapsize += HoVe(horz, sz);
					w++;
				}
				gapsize = maxsize - gapsize - HoVe(horz, psz);
			}
			Size sz = q->ctrl ? q->ctrl->GetMinSize()
			                  : Size(horz ? gapsize : 0, horz ? 0 : gapsize);
			if(HoVe(horz, psz) + HoVe(horz, sz) >= maxsize && HoVe(horz, psz)) {
				while(q < item.End() && q->ctrl == NULL && !IsNull(q->gapsize))
					q++;
				break;
			}
			VeHo(horz, psz) = max(VeHo(horz, psz), VeHo(horz, sz));
			HoVe(horz, psz) += HoVe(horz, sz);
			q++;
		}
		int& bp = VeHo(horz, asz);
		if(bp > bmargin)
			bp += 2 + 2 * bmargin;
		bp += VeHo(horz, psz);
		HoVe(horz, asz) = max(HoVe(horz, asz), HoVe(horz, psz));
	}
	VeHo(horz, asz) += bmargin;
	HoVe(horz, asz) += horz ? 2 * hmargin : 2 * vmargin;
	LLOG("Size " << Name() << " " << asz);
	return asz + Size(2 * hmargin, 2 * vmargin);
}

void BarPane::IClear()
{
	breakpos.Clear();
	item.Clear();
}

void BarPane::Clear()
{
	while(GetFirstChild())
		RemoveChild(GetFirstChild());
	IClear();
}

void BarPane::Add(Ctrl *ctrl, int gapsize)
{
	Item& m = item.Add();
	m.ctrl = ctrl;
	m.gapsize = gapsize;
	if(ctrl)
		AddChild(ctrl);
}

void BarPane::MouseMove(Point p, dword)
{
	Ctrl *q = Ctrl::GetVisibleChild(this, p, false);
	if(q != phelpctrl) {
		phelpctrl = q;
		BarCtrl::SendHelpLine(q);
	}
}

BarPane::BarPane()
{
	NoWantFocus();
	phelpctrl = NULL;
	hmargin = vmargin = 0;
	menu = false;
}

BarPane::~BarPane() {}

Bar::Item::Item() {}
Bar::Item::~Item() {}

Bar::Item& Bar::Item::Text(const char *text)        { return *this; }
Bar::Item& Bar::Item::Key(dword key)                { return *this; }
Bar::Item& Bar::Item::Repeat(bool b)                { return *this; }
Bar::Item& Bar::Item::Image(const ::Image& img)     { return *this; }
Bar::Item& Bar::Item::Enable(bool _enable)          { return *this; }
Bar::Item& Bar::Item::Tip(const char *tip)          { return *this; }
Bar::Item& Bar::Item::Help(const char *help)        { return *this; }
Bar::Item& Bar::Item::Topic(const char *topic)      { return *this; }
Bar::Item& Bar::Item::Description(const char *desc) { return *this; }
Bar::Item& Bar::Item::Check(bool check)             { return *this; }
Bar::Item& Bar::Item::Radio(bool check)             { return *this; }
void       Bar::Item::FinalSync() {}

Bar::Item& Bar::Item::Key(KeyInfo& (*key)()) {
	KeyInfo& k = (*key)();
	return Key(k.key[0]).Key(k.key[1]).Key(k.key[2]).Key(k.key[3]);
}

Bar::Bar() {}
Bar::~Bar() {}

Bar::Item& Bar::NilItem()
{
	static Item m;
	return m;
}

void Bar::AddNC(Ctrl& ctrl)
{
	AddCtrl(&ctrl, (int)Null);
}

Bar::Item&  Bar::Add(bool enable, const char *text, const ::Image& image, Callback cb)
{
	return AddItem(cb).Text(text).Image(image).Enable(enable);
}

Bar::Item&  Bar::Add(bool enable, KeyInfo& (*key)(), const ::Image& image, Callback callback)
{
	const char *text = (*key)().name;
	if(*text == '\3')
		text = t_GetLngString(text + 1);
	return Add(enable, text, image, callback).Key(key);
}

Bar::Item&  Bar::Add(const char *text, const ::Image& image, Callback callback)
{
	return Add(true, text, image, callback);
}

Bar::Item&  Bar::Add(KeyInfo& (*key)(), const ::Image& image, Callback callback)
{
	return Add(true, key, image, callback);
}

Bar::Item&  Bar::Add(bool enable, const char *text, Callback callback)
{ return IsMenuBar() ? Add(enable, text, Image(), callback) : NilItem(); }

Bar::Item&  Bar::Add(const char *text, Callback callback)
{ return IsMenuBar() ? Add(true, text, Image(), callback) : NilItem(); }

Bar::Item&  Bar::Add(bool enable, KeyInfo& (*key)(), Callback callback)
{ return IsMenuBar() ? Add(enable, key, Image(), callback) : NilItem(); }

Bar::Item&  Bar::Add(KeyInfo& (*key)(), Callback callback)
{ return IsMenuBar() ? Add(true, key, Image(), callback) : NilItem(); }

void Bar::Break()
{
	AddCtrl(NULL, (int)Null);
}

void Bar::Gap(int size)
{
	AddCtrl(NULL, size);
}

void   Bar::MenuSeparator()              { if(IsMenuBar()) Separator(); }
void   Bar::MenuBreak()                  { if(IsMenuBar()) Break(); }
void   Bar::MenuGap(int size)            { if(IsMenuBar()) Gap(size); }
void   Bar::AddMenu(Ctrl& ctrl)          { if(IsMenuBar()) Add(ctrl); }
void   Bar::AddMenu(Ctrl& ctrl, Size sz) { if(IsMenuBar()) Add(ctrl, sz); }

Bar::Item&  Bar::AddMenu(bool enable, KeyInfo& (*key)(), const ::Image& image, Callback callback)
{ return IsMenuBar() ? Add(enable, key, image, callback) : NilItem(); }

Bar::Item&  Bar::AddMenu(KeyInfo& (*key)(), const ::Image& image, Callback callback)
{ return IsMenuBar() ? Add(key, image, callback) : NilItem(); }

Bar::Item&  Bar::AddMenu(bool enable, const char *text, const ::Image& image, Callback callback)
{ return IsMenuBar() ? Add(enable, text, image, callback) : NilItem(); }

Bar::Item&  Bar::AddMenu(const char *text, const ::Image& image, Callback callback)
{ return IsMenuBar() ? Add(text, image, callback) : NilItem(); }

Bar::Item& Bar::Add(bool enable, const char *text, Callback1<Bar&> proc)
{ return AddSubMenu(proc).Text(text).Enable(enable); }

Bar::Item& Bar::Add(const char *text, Callback1<Bar&> proc)
{ return Add(true, text, proc); }

void   Bar::ToolSeparator()                { if(IsToolBar()) Separator(); }
void   Bar::ToolBreak()                    { if(IsToolBar()) Break(); }
void   Bar::ToolGap(int size)              { if(IsToolBar()) Gap(size); }

void   Bar::AddTool(Ctrl& ctrl)            { if(IsToolBar()) Add(ctrl); }
void   Bar::AddTool(Ctrl& ctrl, Size sz)   { if(IsToolBar()) Add(ctrl, sz); }

void   Bar::AddKey(dword key, Callback cb) {}

void Bar::AddKey(KeyInfo& (*key)(), Callback cb)
{
	KeyInfo& k = (*key)();
	AddKey(k.key[0], cb);
	AddKey(k.key[1], cb);
	AddKey(k.key[2], cb);
	AddKey(k.key[3], cb);
}

Bar::Item&  Bar::Add(bool enable, const ::Image& image, Callback cb)
{ return IsToolBar() ? AddItem(cb).Image(image).Enable(enable) : NilItem(); }

Bar::Item&  Bar::Add(const ::Image& image, Callback cb)
{ return IsToolBar() ? Add(true, image, cb) : NilItem(); }

class Bar::ScanKeys : public Bar {
	struct KeyItem : public Bar::Item {
		bool          enabled;
		Vector<dword> key;
		Callback      action;

		virtual Item& Text(const char *)             { return *this; }
		virtual Item& Check(bool check)              { return *this; }
		virtual Item& Radio(bool check)              { return *this; }
		virtual Item& Key(dword _key)                { if(_key) key.Add(_key); return *this; }
		virtual Item& Image(const ::Image& img)      { return *this;  }
		virtual Item& Enable(bool _enable = true)    { enabled = _enable; return *this; }
		virtual Item& Tip(const char *tip)           { return *this; }
		virtual Item& Help(const char *help)         { return *this; }
		virtual Item& Topic(const char *help)        { return *this; }
		virtual Item& Description(const char *help)  { return *this; }
	};

	Array<KeyItem> key;
	KeyItem        nilitem;
	int            level;

protected:
	virtual Item&  AddItem(Callback cb);
	virtual Item&  AddSubMenu(Callback1<Bar&> proc);
	virtual void   AddCtrl(Ctrl *ctrl, int gapsize) {};
	virtual void   AddCtrl(Ctrl *ctrl, Size sz) {};

public:
	virtual bool   IsMenuBar() const                 { return true; }
	virtual bool   IsToolBar() const                 { return true; }
	virtual bool   IsScanKeys() const                { return true; }

	virtual bool   IsEmpty() const                   { return key.IsEmpty(); }
	virtual void   Separator()                       {}
	virtual void   AddKey(dword key, Callback cb);

	bool Do(dword key);

	ScanKeys()                                       { level = 0; }
};

Bar::Item& Bar::ScanKeys::AddItem(Callback cb)
{
	KeyItem& m = key.Add();
	m.action = cb;
	m.enabled = true;
	return m;
}

Bar::Item&  Bar::ScanKeys::AddSubMenu(Callback1<Bar&> proc)
{
	level++;
	if(level > 5)
		return NilItem();;
	proc(*this);
	level--;
	return key.GetCount() ? key.Top() : nilitem;
}

void   Bar::ScanKeys::AddKey(dword _key, Callback cb)
{
	if(_key) {
		KeyItem& m = key.Add();
		m.action = cb;
		m.enabled = true;
		m.key.Add(_key);
	}
}

bool   Bar::ScanKeys::Do(dword k)
{
	for(int i = 0; i < key.GetCount(); i++) {
		KeyItem& m = key[i];
		if(m.enabled && FindIndex(m.key, k) >= 0) {
			m.action();
			return true;
		}
	}
	return false;
}

bool   Bar::Scan(Callback1<Bar&> proc, dword key)
{
	ScanKeys keys;
	proc(keys);
	return keys.Do(key);
}

bool BarCtrl::IsEmpty() const
{
	return pane.IsEmpty();
}

void BarCtrl::Separator()
{
	AddNC(separator.DoIndex(sii++).Margin(smargin).SetSize(ssize));
}

void BarCtrl::AddCtrl(Ctrl *ctrl, Size sz)
{
	SizeCtrl& m = sizer.DoIndex(zii++);
	LogPos p = ctrl->GetPos();
	if(p.x.IsEmpty())
		if(sz.cx == INT_MAX)
			ctrl->HSizePos();
		else
			ctrl->HCenterPos(sz.cx ? sz.cx : MINSIZE);
	if(p.y.IsEmpty())
		if(sz.cy == INT_MAX)
			ctrl->VSizePos();
		else
			ctrl->VCenterPos(sz.cy ? sz.cy : MINSIZE);
	LLOG("AddCtrl Ctrl::GetSize: " << ctrl->GetSize());
	m.SetSize(sz);
	m.AddChild(ctrl);
	AddNC(m);
	LLOG("AddCtrl2 Ctrl::GetSize: " << ctrl->GetSize());
}

Size BarCtrl::SizeCtrl::GetMinSize() const
{
	Size sz = size;
	Size chsz = Size(8, 8);
	if(GetFirstChild())
		chsz = GetFirstChild()->GetStdSize();
	sz.cx = sz.cx <= 0 ? chsz.cx : sz.cx;
	sz.cy = sz.cy <= 0 ? chsz.cy : sz.cy;
	return sz;
}

void BarCtrl::Clear()
{
	separator.Clear();
	sizer.Clear();
	pane.Clear();
}

void BarCtrl::IClear()
{
	sii = zii = 0;
	pane.IClear();
}

void BarCtrl::IFinish()
{
	separator.SetCount(sii);
	sizer.SetCount(zii);
}

BarCtrl *BarCtrl::GetBarCtrlParent(Ctrl *q)
{
	while(q) {
		BarCtrl *w = dynamic_cast<BarCtrl *>(q);
		if(w) return w;
		q = q->GetParent();
	}
	return NULL;
}

void BarCtrl::SendHelpLine(Ctrl *q)
{
	String s = q->GetHelpLine();
	if(!::IsEmpty(s)) {
		BarCtrl *b = GetBarCtrlParent(q);
		if(b) b->WhenHelp(s);
	}
}

void BarCtrl::ClearHelpLine(Ctrl *q)
{
	BarCtrl *b = GetBarCtrlParent(q);
	if(b) b->WhenHelp(Null);
}

void BarCtrl::FrameLayout(Rect& r)
{
	switch(align) {
	case BAR_LEFT:
		LayoutFrameLeft(r, this, pane.Repos(false, r.Height()).cx);
		break;
	case BAR_RIGHT:
		LayoutFrameRight(r, this, pane.Repos(false, r.Height()).cx);
		break;
	case BAR_BOTTOM:
		LayoutFrameBottom(r, this, pane.Repos(true, r.Width()).cy);
		break;
	default:
		LayoutFrameTop(r, this, pane.Repos(true, r.Width()).cy);
		break;
	}
}

void BarCtrl::FrameAddSize(Size& sz)
{
	if(align == BAR_LEFT || align == BAR_RIGHT)
		sz.cx += pane.GetPaneSize(true, sz.cy).cx;
	else
		sz.cy += pane.GetPaneSize(true, sz.cx).cy;
}

void BarCtrl::FrameAdd(Ctrl& parent)
{
	parent.Add(*this);
}

void BarCtrl::FrameRemove()
{
	Remove();
}

void BarCtrl::AddCtrl(Ctrl *ctrl, int gapsize)
{
	pane.Add(ctrl, gapsize);
}

BarCtrl& BarCtrl::Align(int al) {
	align = al;
	if(InFrame() && GetParent())
		RefreshParentLayout();
	return *this;
}

void BarCtrl::SyncBar()
{
	if(IsChild() && InFrame())
		RefreshParentLayout();
	else
		Layout();
}

void BarCtrl::Layout()
{
	LLOG("BarCtrl::Layout");
	if(IsChild())
		if(GetAlign() == BAR_LEFT || GetAlign() == BAR_RIGHT)
			pane.Repos(false, InFrame() ? GetSize().cy : INT_MAX);
		else
			pane.Repos(true, InFrame() ? GetSize().cx : INT_MAX);
}


BarCtrl::BarCtrl() {
	align = BAR_TOP;
	Ctrl::Add(pane.SizePos());
	smargin = 2;
	ssize = 8;
	sii = zii = 0;
	NoWantFocus();
}

BarCtrl::~BarCtrl() {}

void LRUList::Serialize(Stream& stream)
{
	StreamContainer(stream, lru);
}

void LRUList::Select(int i)
{
	WhenSelect(lru[i]);
}

void LRUList::operator()(Bar& bar)
{
	if(bar.IsMenuBar() && !lru.IsEmpty()) {
		bar.Separator();
		char n = '1';
		for(int i = 0; i < lru.GetCount(); i++) {
			bar.Add(String("&") + n + ' ' + GetFileName(lru[i]), THISBACK1(Select, i));
			n = n == '9' ? 'A' : n + 1;
		}
	}
}

void LRUList::NewEntry(const String& path)
{
	LruAdd(lru, path, limit);
}

void LRUList::RemoveEntry(const String& path)
{
	int q = FindIndex(lru, path);
	if(q >= 0)
		lru.Remove(q);
}
