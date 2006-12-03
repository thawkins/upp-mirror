#include "CtrlLib.h"

#define LLOG(x) // LOG(x)

CH_INT(ScrollBarSize, FrameButtonWidth());
CH_INT(ScrollBarArrowSize, ScrollBarSize());
CH_INT(ScrollBarThumbMin, 1);

CH_LOOKS(ScrollBarVertUpper, 4, CtrlsImgLook(CtrlsImg::I_SBVU));
CH_LOOKS(ScrollBarVertThumb, 4, CtrlsImgLook(CtrlsImg::I_SBVT, CtrlsImg::SBVI()));
CH_LOOKS(ScrollBarVertLower, 4, CtrlsImgLook(CtrlsImg::I_SBVL));
CH_LOOKS(ScrollBarHorzUpper, 4, CtrlsImgLook(CtrlsImg::I_SBHU));
CH_LOOKS(ScrollBarHorzThumb, 4, CtrlsImgLook(CtrlsImg::I_SBHT, CtrlsImg::SBHI()));
CH_LOOKS(ScrollBarHorzLower, 4, CtrlsImgLook(CtrlsImg::I_SBHL));
CH_LOOKS(ScrollBarUp, 4, CtrlsImgLook(CtrlsImg::I_SB, CtrlsImg::UA(), ButtonMonoColor));
CH_LOOKS(ScrollBarDown, 4, CtrlsImgLook(CtrlsImg::I_SB, CtrlsImg::DA(), ButtonMonoColor));
CH_LOOKS(ScrollBarLeft, 4, CtrlsImgLook(CtrlsImg::I_SB, CtrlsImg::LA(), ButtonMonoColor));
CH_LOOKS(ScrollBarRight, 4, CtrlsImgLook(CtrlsImg::I_SB, CtrlsImg::RA(), ButtonMonoColor));

CH_INT(ScrollBarTrough, 0);
CH_INT(ScrollBarOverThumb, 0);

int& Slider::HV(int& h, int& v) const
{
	return IsHorz() ? h : v;
}

int Slider::GetHV(int h, int v) const {
	return IsHorz() ? h : v;
}

Rect Slider::GetPartRect(int p) const {
	Size sz = GetSize();
	Rect h(0, 0, sz.cx, sz.cy);
	int sbo = ScrollBarOverThumb();
	switch(p) {
	case 0:
		HV(h.right, h.bottom) = thumbpos - sbo + thumbsize / 2;
		break;
	case 1:
		HV(h.left, h.top) = thumbpos + thumbsize / 2 + sbo;
		break;
	case 2:
		HV(h.left, h.top) = thumbpos - sbo;
		HV(h.right, h.bottom) = thumbpos + thumbsize + sbo;
		break;
	}
	return h;
}

int  Slider::GetMousePart()
{
	int q = -1;
	for(int i = 2; i >= 0; i--)
		if(HasMouseIn(GetPartRect(i))) {
			q = i;
			break;
		}
	return q;
}

int  Slider::GetRange() const {
	return GetHV(GetSize());
}

void Slider::Bounds() {
	int maxsize = GetRange();
	if(thumbsize > maxsize)
		thumbsize = maxsize;
	if(thumbpos + thumbsize > maxsize)
		thumbpos = maxsize - thumbsize;
	if(thumbpos < 0)
		thumbpos = 0;
}

bool Slider::Set(int _thumbpos, int _thumbsize) {
	int ts = thumbsize;
	int tp = thumbpos;
	thumbsize = _thumbsize;
	thumbpos = _thumbpos;
	Bounds();
	if(thumbsize != ts || thumbpos != tp) {
		Refresh();
		return true;
	}
	return false;
}

void Slider::Drag(Point p) {
	if(Set(max(0, IsHorz() ? p.x - delta : p.y - delta)) && track)
		Action();
}

void Slider::Layout() {
	Ctrl::Layout();
	Bounds();
}

void Slider::Paint(Draw& w) {
	Size sz = GetSize();
	light = GetMousePart();
	int p = push;
	if(!HasCapture())
		p = -1;
	static Value (*hl[])(int) = { ScrollBarHorzUpper, ScrollBarHorzLower, ScrollBarHorzThumb };
	static Value (*vl[])(int) = { ScrollBarVertUpper, ScrollBarVertLower, ScrollBarVertThumb };

	Value (**l)(int) = IsHorz() ? hl : vl;

	if(IsShowEnabled()) {
		for(int i = 0; i < 3; i++) {
			Rect pr = GetPartRect(i);
			if(i != 2) {
				w.Clip(pr);
				pr = sz;
				if(ScrollBarTrough()) {
					HV(pr.left, pr.top) -= ScrollBarArrowSize();
					HV(pr.right, pr.bottom) += ScrollBarArrowSize();
				}
			}
			ChPaint(w, pr,
			        (*l[i])(p == i ? CTRL_PRESSED : light == i ? CTRL_HOT : CTRL_NORMAL));
			if(i != 2)
				w.End();
		}
	}
	else
		if(ScrollBarTrough()) {
			Rect r = sz;
			HV(r.left, r.top) -= ScrollBarArrowSize();
			HV(r.right, r.bottom) += ScrollBarArrowSize();
			ChPaint(w, r, (*l[0])(CTRL_DISABLED));
		}
		else
		if(IsHorz()) {
			ChPaint(w, 0, 0, sz.cx / 2, sz.cy, (*l[0])(CTRL_DISABLED));
			ChPaint(w, sz.cx / 2, 0, sz.cx - sz.cx / 2, sz.cy, (*l[1])(CTRL_DISABLED));
		}
		else {
			ChPaint(w, 0, 0, sz.cx, sz.cy / 2, (*l[0])(CTRL_DISABLED));
			ChPaint(w, 0, sz.cy / 2, sz.cx, sz.cy - sz.cy / 2, (*l[1])(CTRL_DISABLED));
		}
}

void Slider::LeftDown(Point p, dword) {
	push = GetMousePart();
	if(push == 2)
		delta = GetHV(p.x, p.y) - thumbpos;
	else {
		if(jump) {
			delta = thumbsize / 2;
			Drag(p);
		}
		else
			if(push == 0)
				WhenPrev();
			else
				WhenNext();
	}
	SetCapture();
	Refresh();
	WhenLeftClick();
}

void Slider::MouseMove(Point p, dword) {
	if(HasCapture() && push == 2) {
		int opos = thumbpos;
		Drag(p);
	}
	else
	if(light != GetMousePart())
		Refresh();
}

void Slider::MouseEnter(Point p, dword)
{
	Refresh();
}

void Slider::MouseLeave()
{
	Refresh();
}

void Slider::LeftUp(Point p, dword) {
	ReleaseCapture();
	if(!track)
		Action();
	Refresh();
	push = -1;
}

void Slider::LeftRepeat(Point p, dword) {
	if(jump || push < 0 || push == 2) return;
	if(push == 0)
		WhenPrev();
	else
		WhenNext();
	Refresh();
}

void Slider::CancelMode() {
	push = light = -1;
}

Slider::Slider() {
	thumbsize = 8;
	thumbpos = 0;
	track = true;
	jump = false;
	edgestyle = false;
	horz = false;
	NoWantFocus();
	push = light = -1;
	Transparent();
}

void ScrollBar::Paint(Draw& w)
{
	Size sz = GetSize();
	if(IsHorz()) {
		ChPaint(w, 0, 0, sz.cx / 2, sz.cy, ScrollBarHorzUpper(0));
		ChPaint(w, sz.cx / 2, 0, sz.cx - sz.cx / 2, sz.cy, ScrollBarHorzLower(0));
	}
	else {
		ChPaint(w, 0, 0, sz.cx, sz.cy / 2, ScrollBarVertUpper(0));
		ChPaint(w, 0, sz.cy / 2, sz.cx, sz.cy - sz.cy / 2, ScrollBarVertLower(0));
	}
}

bool  ScrollBar::Set(int apagepos) {
	int op = pagepos;
	pagepos = apagepos;
	if(pagepos > totalsize - pagesize) pagepos = totalsize - pagesize;
	if(pagepos < 0) pagepos = 0;
	int slsize = slider.GetRange();
	int mint = max(minthumb, ScrollBarThumbMin());
	if(totalsize <= 0)
		slider.Set(0, slsize);
	else {
		double thumbsize = slsize * pagesize / (double) totalsize;
		double rest = slsize * pagesize - thumbsize * totalsize;
		double ts, ps;
		if(thumbsize >= slsize || thumbsize < 0) {
			ts = slsize;
			ps = 0;
		}
		else
		if(thumbsize <= mint) {
			ps = ((slsize - mint) * (double)pagepos + rest) / (double) (totalsize - pagesize);
			ts = mint;
		}
		else {
			ps = (slsize * (double)pagepos + rest) / (double) totalsize;
			ts = thumbsize;
		}
		slider.Set(ffloor(ps), fceil(ts));
    }
	if(pagepos != op) {
		Refresh();
		WhenScroll();
		return true;
	}
	return false;
}

void ScrollBar::Set(int _pagepos, int _pagesize, int _totalsize) {
	pagesize = _pagesize;
	totalsize = _totalsize;
	bool a = totalsize > pagesize && pagesize > 0;
	if(autohide && a != IsShown()) {
		Show(a);
		WhenVisibility();
	}
	if(autodisable) {
		prev.Enable(a);
		next.Enable(a);
		slider.Enable(a);
	}
	Set(_pagepos);
}

void ScrollBar::SetPage(int _pagesize) {
	Set(pagepos, _pagesize, totalsize);
}

void ScrollBar::SetTotal(int _totalsize) {
	Set(pagepos, pagesize, _totalsize);
}

void ScrollBar::Position() {
	int thumbpos = slider.GetPos();
	int thumbsize = slider.GetThumbSize();
	int slsize = slider.GetRange();
	int mint = max(minthumb, ScrollBarThumbMin());
	if(slsize < mint || totalsize <= pagesize)
		pagepos = 0;
	else
	if(thumbpos == slsize - thumbsize)
		pagepos = totalsize - pagesize;
	else
	if(thumbsize == mint)
		pagepos = iscale(thumbpos, (totalsize - pagesize), (slsize - mint));
	else
		pagepos = iscale(thumbpos, totalsize, slsize);
	Action();
	WhenScroll();
}

void ScrollBar::Uset(int a) {
	if(Set(a))
		Action();
}

void ScrollBar::PrevLine() {
	Uset(pagepos - linesize);
}

void ScrollBar::NextLine() {
	Uset(pagepos + linesize);
}

void ScrollBar::PrevPage() {
	Uset(pagepos - max(pagesize - linesize, 1));
}

void ScrollBar::NextPage() {
	Uset(pagepos + max(pagesize - linesize, 1));
}

void ScrollBar::Wheel(int zdelta, int lines) {
	Uset(pagepos - lines * linesize * zdelta / 120);
}

bool ScrollBar::VertKey(dword key, bool homeend) {
	if(!IsVisible() || !IsEnabled() || GetRect().IsEmpty())
		return false;
	switch(key) {
	case K_PAGEUP:
		PrevPage();
		break;
	case K_PAGEDOWN:
		NextPage();
		break;
	case K_UP:
		PrevLine();
		break;
	case K_DOWN:
		NextLine();
		break;
	case K_HOME:
		if(!homeend) break;
	case K_CTRL_HOME:
	case K_CTRL_PAGEUP:
		Begin();
		break;
	case K_END:
		if(!homeend) break;
	case K_CTRL_END:
	case K_CTRL_PAGEDOWN:
		End();
		break;
	default:
		return false;
	}
	return true;
}

void ScrollBar::Begin()
{
	Uset(0);
}

void ScrollBar::End()
{
	Uset(max(0, totalsize - pagesize));
}

bool ScrollBar::HorzKey(dword key) {
	if(!IsVisible() || !IsEnabled() || GetRect().IsEmpty())
		return false;
	switch(key) {
	case K_CTRL_LEFT:
		PrevPage();
		break;
	case K_CTRL_RIGHT:
		NextPage();
		break;
	case K_LEFT:
		PrevLine();
		break;
	case K_RIGHT:
		NextLine();
		break;
	case K_HOME:
		Begin();
		break;
	case K_END:
		End();
		break;
	default:
		return false;
	}
	return true;
}

void ScrollBar::Layout() {
	bool horz = IsHorz();
	Size sz = GetSize();
	if(IsHorz()) {
		prev.Style(ScrollBarLeft);
		next.Style(ScrollBarRight);
		int cc = sz.cx > 3 * sz.cy ? sz.cy : 0;
		prev.SetRect(0, 0, cc, sz.cy);
		slider.SetRect(cc, 0, sz.cx - 2 * cc, sz.cy);
		next.SetRect(sz.cx - cc, 0, cc, sz.cy);
	}
	else {
		prev.Style(ScrollBarUp);
		next.Style(ScrollBarDown);
		int cc = sz.cy > 3 * sz.cx ? sz.cx : 0;
		prev.SetRect(0, 0, sz.cx, cc);
		slider.SetRect(0, cc, sz.cx, sz.cy - 2 * cc);
		next.SetRect(0, sz.cy - cc, sz.cx, cc);
	}
	Set(pagepos);
}

bool ScrollBar::ScrollInto(int pos, int _linesize) {
	int new_pos = pagepos;
	if(pos > new_pos + pagesize - _linesize)
		new_pos = pos - pagesize + _linesize;
	if(pos < new_pos)
		new_pos = pos;
	return Set(new_pos);
}

Size ScrollBar::GetStdSize() const {
	int a = HeaderCtrl::GetStdHeight();
	return Size(a, a);
}

void ScrollBar::FrameLayout(Rect& r)
{
	(IsHorz() ? LayoutFrameBottom : LayoutFrameRight)(r, this, ScrollBarSize());
}

void ScrollBar::FrameAddSize(Size& sz)
{
	(IsHorz() ? sz.cx : sz.cy) += ScrollBarSize();
}

Size ScrollBar::GetViewSize() const {
	if(IsChild() && InFrame()) {
		Size sz = GetParent()->GetSize();
		if(IsShown())
			(IsVert() ? sz.cx : sz.cy) += ScrollBarSize();
		return sz;
	}
	return Size(0, 0);
}

Size ScrollBar::GetReducedViewSize() const {
	if(IsChild() && InFrame()) {
		Size sz = GetParent()->GetSize();
		if(!IsShown())
			(IsVert() ? sz.cx : sz.cy) -= ScrollBarSize();
		return sz;
	}
	return Size(0, 0);
}

ScrollBar& ScrollBar::AutoHide(bool b) {
	autohide = b;
	if(b)
		SetTotal(totalsize);
	else
		Show();
	WhenVisibility();
	return *this;
}

ScrollBar& ScrollBar::AutoDisable(bool b) {
	autodisable = b;
	if(!b) {
		prev.Enable();
		next.Enable();
		slider.Enable();
	}
	return *this;
}

ScrollBar::ScrollBar() {
	minthumb = 16;
	pagepos = pagesize = totalsize = 0;
	linesize = 1;
	autohide = false;
	autodisable = true;
	Add(slider);
	Add(prev);
	Add(next);
	prev.ScrollStyle().NoWantFocus().Transparent();
	next.ScrollStyle().NoWantFocus().Transparent();
	prev.WhenPush = prev.WhenRepeat = callback(this, &ScrollBar::PrevLine);
	prev.WhenPush << Proxy(WhenLeftClick);
	next.WhenPush = next.WhenRepeat = callback(this, &ScrollBar::NextLine);
	next.WhenPush << Proxy(WhenLeftClick);
	slider.WhenPrev = callback(this, &ScrollBar::PrevPage);
	slider.WhenNext = callback(this, &ScrollBar::NextPage);
	slider.WhenAction = callback(this, &ScrollBar::Position);
	slider.WhenLeftClick = Proxy(WhenLeftClick);
	slider.EdgeStyle();
}

ScrollBar::~ScrollBar() {}

Image SizeGrip::CursorImage(Point p, dword)
{
#ifdef PLATFORM_X11
	if(_NET_Supported().Find(XAtom("_NET_WM_MOVERESIZE")) >= 0) {
#endif
		TopWindow *q = dynamic_cast<TopWindow *>(GetTopCtrl());
		if(q && !q->IsMaximized() && q->IsSizeable())
			return Image::SizeBottomRight();
#ifdef PLATFORM_X11
	}
#endif
	return Image::Arrow();
}

CH_IMAGE(SizeGripImg, CtrlsImg::SizeGrip());

void SizeGrip::Paint(Draw& w)
{
    Size sz = GetSize();
    if(InFrame())
        w.DrawRect(sz, SColorFace);
#ifdef PLATFORM_X11
    if(_NET_Supported().Find(XAtom("_NET_WM_MOVERESIZE")) >= 0)
    {
#endif
		TopWindow *q = dynamic_cast<TopWindow *>(GetTopCtrl());
		if(q && !q->IsMaximized() && q->IsSizeable()) {
			Size isz = CtrlsImg::SizeGrip().GetSize();
			w.DrawImage(sz.cx - isz.cx, sz.cy - isz.cy, CtrlsImg::SizeGrip());
		}
#ifdef PLATFORM_X11
    }
#endif
}

SizeGrip::SizeGrip()
{
	Transparent();
	RightPos(0, 12).BottomPos(0, 12);
	NoWantFocus();
	Width(14);
}

SizeGrip::~SizeGrip() {}

void SizeGrip::LeftDown(Point p, dword flags)
{
	TopWindow *q = dynamic_cast<TopWindow *>(GetTopCtrl());
	if(!q || q->IsMaximized() || !q->IsSizeable()) return;
#ifdef PLATFORM_WIN32
	HWND hwnd = q->GetHWND();
	if(hwnd)
		::SendMessage(hwnd, WM_SYSCOMMAND, 0xf008, MAKELONG(p.x, p.y));
#endif
#ifdef PLATFORM_X11
	if(_NET_Supported().Find(XAtom("_NET_WM_MOVERESIZE")) >= 0) {
		XClientMessageEvent m;
		m.type = ClientMessage;
		m.window = q->GetWindow();
		m.message_type = XAtom("_NET_WM_MOVERESIZE");
	    m.format = 32;
	    p = GetMousePos();
		m.data.l[0] = p.x;
		m.data.l[1] = p.y;
		m.data.l[2] = 4;
		XSendEvent(Xdisplay, Xroot, 0, (SubstructureNotifyMask|SubstructureRedirectMask),
		           (XEvent*)&m);
	}
#endif
}

void ScrollBars::Scroll() {
	WhenScroll();
}

bool ScrollBars::Key(dword key) {
	return x.HorzKey(key) || y.VertKey(key);
}

void ScrollBars::Set(Point pos, Size page, Size total) {
	x.Set(pos.x, page.cx, total.cx);
	y.Set(pos.y, page.cy, total.cy);
}

bool ScrollBars::Set(int _x, int _y) {
	bool b = x.Set(_x) | y.Set(_y);
	return b;
}

bool ScrollBars::Set(Point pos) {
	return Set(pos.x, pos.y);
}

void ScrollBars::SetPage(int cx, int cy) {
	x.SetPage(cx);
	y.SetPage(cy);
}

void ScrollBars::SetPage(Size page) {
	SetPage(page.cx, page.cy);
}

void ScrollBars::SetTotal(int cx, int cy) {
	x.SetTotal(cx);
	y.SetTotal(cy);
}

void ScrollBars::SetTotal(Size total) {
	SetTotal(total.cx, total.cy);
}

bool ScrollBars::ScrollInto(Point pos, Size linesize) {
	return x.ScrollInto(pos.x, linesize.cx) | y.ScrollInto(pos.y, linesize.cy);
}

bool ScrollBars::ScrollInto(Point pos) {
	return x.ScrollInto(pos.x) | y.ScrollInto(pos.y);
}

ScrollBars& ScrollBars::SetLine(int linex, int liney) {
	x.SetLine(linex);
	y.SetLine(liney);
	return *this;
}

ScrollBars& ScrollBars::Track(bool b) {
	x.Track(b);
	y.Track(b);
	return *this;
}

ScrollBars& ScrollBars::Jump(bool b) {
	x.Jump(b);
	y.Jump(b);
	return *this;
}

ScrollBars& ScrollBars::AutoHide(bool b) {
	x.AutoHide(b);
	y.AutoHide(b);
	return *this;
}

ScrollBars& ScrollBars::AutoDisable(bool b) {
	x.AutoDisable(b);
	y.AutoDisable(b);
	return *this;
}

ScrollBars& ScrollBars::Box(Ctrl& _box) {
	box->Remove();
	box = &_box;
	if(x.GetParent() && x.GetParent() != box->GetParent())
		x.GetParent()->Add(*box);
	return *this;
}

void ScrollBars::FrameAdd(Ctrl& p) {
	p.Add(x);
	p.Add(y);
	if(box->GetParent() != &p)
		p.Add(*box);
}

void ScrollBars::FrameRemove() {
	x.Remove();
	y.Remove();
	box->Remove();
}

void ScrollBars::FramePaint(Draw& w, const Rect& r) {
	if(x.IsShown() && y.IsShown() && !box) {
		int h = ScrollBarSize();
		w.DrawRect(r.right - h, r.bottom - h, h, h, SColorFace);
	}
}

void ScrollBars::FrameLayout(Rect& r) {
	int h = ScrollBarSize();
	int by = 0;
	int bx = x.IsShown() && y.IsShown() ? h : 0;
	if(box_type == 1)
		by = bx;
	if(box_type == 2)
		by = h;
	int dx = x.IsShown() * h;
	int dy = y.IsShown() * h;
	y.SetFrameRect(r.right - dy, r.top, dy, r.Height() - by);
	x.SetFrameRect(r.left, r.bottom - dx, r.Width() - bx, dx);
	if(box)
		box->SetFrameRect(r.right - by, r.bottom - by, by, by);
	r.right -= dy;
	r.bottom -= dx;
}

void ScrollBars::FrameAddSize(Size& sz) {
	int h = ScrollBarSize();
	sz.cy += x.IsShown() * h;
	sz.cx += y.IsShown() * h;
}

Size ScrollBars::GetViewSize() const {
	return Size(y.GetViewSize().cx, x.GetViewSize().cy);
}

Size ScrollBars::GetReducedViewSize() const {
	return Size(y.GetReducedViewSize().cx, x.GetReducedViewSize().cy);
}

ScrollBars& ScrollBars::NormalBox()
{
	box_type = 1;
	y.RefreshParentLayout();
	return *this;
}

ScrollBars& ScrollBars::NoBox()
{
	box_type = 0;
	y.RefreshParentLayout();
	return *this;
}

ScrollBars& ScrollBars::FixedBox()
{
	box_type = 2;
	y.RefreshParentLayout();
	return *this;
}

ScrollBars::ScrollBars() {
	box = &the_box;
	x.WhenScroll = y.WhenScroll = callback(this, &ScrollBars::Scroll);
	x.WhenLeftClick = y.WhenLeftClick = Proxy(WhenLeftClick);
	x.AutoHide();
	y.AutoHide();
	box_type = 1;
}

ScrollBars::~ScrollBars() {}

void Scroller::Scroll(Ctrl& p, const Rect& rc, Point newpos, Size cellsize)
{
	if(!IsNull(psb)) {
		Point d = psb - newpos;
		p.ScrollView(rc, d.x * cellsize.cx, d.y * cellsize.cy);
	}
	else
		p.Refresh();
	psb = newpos;
}

void Scroller::Scroll(Ctrl& p, const Rect& rc, int newposy, int linesize)
{
	Scroll(p, rc, Point(0, newposy), Size(0, linesize));
}

void Scroller::Scroll(Ctrl& p, Point newpos)
{
	Scroll(p, p.GetSize(), newpos);
}

void Scroller::Scroll(Ctrl& p, int newposy)
{
	Scroll(p, p.GetSize(), newposy);
}
