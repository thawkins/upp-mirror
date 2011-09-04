#include "CtrlCore.h"

#ifdef GUI_X11

NAMESPACE_UPP

#define LLOG(x)  //  DLOG(x)

void TopWindow::SyncSizeHints()
{
	GuiLock __; 
	Size min = GetMinSize();
	Size max = GetMaxSize();
	if(!sizeable)
		min = max = GetRect().Size();
	Window w = GetWindow();
	if(w && (min != xminsize || max != xmaxsize)) {
		xminsize = min;
		xmaxsize = max;
		size_hints->min_width = min.cx;
		size_hints->min_height = min.cy;
		size_hints->max_width = max.cx;
		size_hints->max_height = max.cy;
		size_hints->flags = PMinSize|PMaxSize;
		XSetWMNormalHints(Xdisplay, w, size_hints);
	}
}

void TopWindow::EndIgnoreTakeFocus()
{
	GuiLock __; 
	ignoretakefocus = false;
}

void TopWindow::EventProc(XWindow& w, XEvent *event)
{
	GuiLock __; 
	Ptr<Ctrl> this_ = this;
	if(event->type == ClientMessage) {
		if(event->xclient.format == 32 && event->xclient.message_type)
			if(event->xclient.message_type == XAtom("WM_PROTOCOLS")) {
				Atom a = event->xclient.data.l[0];
				if(a == XAtom("WM_DELETE_WINDOW") && IsEnabled()) {
					LLOG("DELETE_WINDOW " << Name());
					WhenClose();
					return;
				}
				if(a == XAtom("WM_TAKE_FOCUS")) {
					LLOG("TAKE_FOCUS serial: " << event->xclient.serial);
					Xeventtime = event->xclient.data.l[1];
					TakeFocus();
					return;
				}
				if(a == XAtom("_NET_WM_PING")) {
					XEvent ev = *event;
					ev.xclient.window = Xroot;
					XSendEvent(Xdisplay, Xroot, 0, SubstructureRedirectMask|SubstructureNotifyMask, &ev);
					return;
				}
				LLOG("Unknown WM_PROTOCOLS: " << XAtomName(a));
			}
	}
	else
	if(event->type == PropertyNotify && event->xproperty.atom == XAtom("_NET_WM_STATE")) {
		LLOG("_NET_WM_STATE notify");
		Vector<int> p = GetPropertyInts(GetWindow(), XAtom("_NET_WM_STATE"));
		if(FindIndex(p, XAtom("_NET_WM_STATE_HIDDEN")) >= 0) {
			state = MINIMIZED;
			LLOG("MINIMIZED");
		}
		else
		if(FindIndex(p, XAtom("_NET_WM_STATE_MAXIMIZED_HORZ")) >= 0 &&
		   FindIndex(p, XAtom("_NET_WM_STATE_MAXIMIZED_VERT")) >= 0) {
			state = MAXIMIZED;
			LLOG("MAXIMIZED");
		}
		else {
			state = OVERLAPPED;
			LLOG("OVERLAPPED");
		}
	}
	if(this_) Ctrl::EventProc(w, event);
	if(this_) SyncSizeHints();
}

void TopWindow::DefSyncTitle()
{
	GuiLock __; 
	if(title2 == title)
		return;
	title2 = title;
	if(IsOpen() && GetWindow()) {
		Window w = GetWindow();
		XStoreName(Xdisplay, w, title.ToString());
		XSetIconName(Xdisplay, w, title.ToString());
		String utf8title = FromUnicode(title, CHARSET_UTF8);
		XChangeProperty(Xdisplay, w, XAtom("_NET_WM_NAME"), XAtom("UTF8_STRING"),
		                8, PropModeReplace,
		                (const unsigned char *)~utf8title, utf8title.GetLength());
		XChangeProperty(Xdisplay, w, XAtom("_NET_WM_ICON_NAME"), XAtom("UTF8_STRING"),
		                8, PropModeReplace,
		                (const unsigned char *)~utf8title, utf8title.GetLength());
	}
}

void TopWindow::SyncTitle0()
{
	GuiLock __; 
	LLOG("SyncTitle: " << title);
	KillTimeCallback(TIMEID_DEFSYNCTITLE);
	SetTimeCallback(0, THISBACK(DefSyncTitle), TIMEID_DEFSYNCTITLE);
	LLOG("*SyncTitle: " << title);
}

void WmState(Window w, bool set, Atom a1, Atom a2 = 0)
{
	XEvent e;
	memset(&e, 0, sizeof(e));
	e.xclient.type = ClientMessage;
	e.xclient.message_type = XAtom("_NET_WM_STATE");
	e.xclient.display = Xdisplay;
	e.xclient.window = w;
	e.xclient.format = 32;
	e.xclient.data.l[0] = set;
	e.xclient.data.l[1] = a1;
	e.xclient.data.l[2] = a2;
	XSendEvent(Xdisplay, Xroot, false, SubstructureNotifyMask | SubstructureRedirectMask, &e);
}

void TopWindow::SyncState0()
{
	GuiLock __; 
	LLOG("SyncState");
	SyncCaption();
	if(IsOpen() && GetWindow()) {		
		Window w = GetWindow();
		WmState(w, topmost, XAtom("_NET_WM_STATE_ABOVE"));
		WmState(w, state == MAXIMIZED, XAtom("_NET_WM_STATE_MAXIMIZED_HORZ"), XAtom("_NET_WM_STATE_MAXIMIZED_VERT"));
		if(state == MINIMIZED)
			XIconifyWindow(Xdisplay, GetWindow(), Xscreenno);
		else
			XMapWindow(Xdisplay, GetWindow());

		WmState(w, state == MINIMIZED, XAtom("_NET_WM_STATE_HIDDEN"));
		WmState(w, fullscreen, XAtom("_NET_WM_STATE_FULLSCREEN"));
	}
}

void TopWindow::SyncState()
{
	ICall(THISBACK(SyncState0));
}

void TopWindow::SyncCaption0()
{
	GuiLock __; 
	LLOG("SyncCaption");
	SyncTitle();
	if(IsOpen() && GetWindow()) {
		unsigned long wina[6];
		memset(wina, 0, sizeof(wina));
		int n = 0;
		Window w = GetWindow();
		if(tool)
			wina[n++] = XAtom("_NET_WM_WINDOW_TYPE_UTILITY");
		if(GetOwner())
			wina[n++] = XAtom("_NET_WM_WINDOW_TYPE_DIALOG");
		wina[n++] = XAtom("_NET_WM_WINDOW_TYPE_NORMAL");
		XChangeProperty(Xdisplay, GetWindow(), XAtom("_NET_WM_WINDOW_TYPE"), XAtom("ATOM"), 32,
		                PropModeReplace, (const unsigned char *)wina, n);

//		n = 0;
//		XChangeProperty(Xdisplay, GetWindow(), XAtom("_NET_WM_STATE"), XAtom("ATOM"), 32,
//		                PropModeReplace, (const unsigned char *)wina, n);
		wm_hints->flags = InputHint|WindowGroupHint|StateHint;
		if(urgent) {
			if(IsForeground()) urgent = false;
			wm_hints->flags |= XUrgencyHint;
			WmState(w, urgent, XAtom("_NET_WM_STATE_DEMANDS_ATTENTION"));
		}
		wm_hints->initial_state = NormalState;
		wm_hints->input = XTrue;
		Ctrl *owner = GetOwner();
		wm_hints->window_group = owner ? owner->GetWindow() : w;
		if(!icon.IsEmpty()) {
			Size isz = icon.GetSize();
			int len = 2 + isz.cx * isz.cy;
			Buffer<unsigned long> data(len);
			unsigned long *t = data;
			*t++ = isz.cx;
			*t++ = isz.cy;
			for(int y = 0; y < isz.cy; y++) {
				const RGBA *q = icon[y];
				for(int x = isz.cx; x--;) {
					*t++ = ((dword)q->a << 24) |
					       (dword)q->b | ((dword)q->g << 8) | ((dword)q->r << 16);
					q++;
				}
			}
			XChangeProperty(Xdisplay, w, XAtom("_NET_WM_ICON"), XA_CARDINAL, 32, PropModeReplace,
			                (const unsigned char *)~data, len);
		}
		XSetWMHints(Xdisplay, w, wm_hints);
	}
}

void TopWindow::CenterRect(Ctrl *owner)
{
	GuiLock __;
	SetupRect();
	if(owner && center == 1 || center == 2) {
		Size sz = GetRect().Size();
		Rect r, wr;
		wr = Ctrl::GetWorkArea();
		GuiLock __;
		Rect fm = windowFrameMargin;
		if((fm.left|fm.right|fm.top|fm.bottom) == 0)
			fm = Rect(8, 32, 8, 8);
		if(center == 1)
			r = owner->GetRect();
		else
			r = wr;
		Point p = r.CenterPos(sz);
		r = RectC(p.x, p.y, sz.cx, sz.cy);
		wr.left += fm.left;
		wr.right -= fm.right;
		wr.top += fm.top;
		wr.bottom -= fm.bottom;
		if(r.top < wr.top) {
			r.bottom += wr.top - r.top;
			r.top = wr.top;
		}
		if(r.bottom > wr.bottom)
			r.bottom = wr.bottom;
		minsize.cx = min(minsize.cx, r.GetWidth());
		minsize.cy = min(minsize.cy, r.GetHeight());
		SetRect(r);
	}
}

void TopWindow::Open(Ctrl *owner)
{
	GuiLock __; 
	if(dokeys && (!GUI_AKD_Conservative() || GetAccessKeysDeep() <= 1))
		DistributeAccessKeys();
	UsrLogT(3, "OPEN " + Desc(this));
	LLOG("OPEN " << Name() << " owner: " << UPP::Name(owner));
	IgnoreMouseUp();
	bool weplace = owner && center == 1 || center == 2 || !GetRect().IsEmpty();
	if(fullscreen)
		SetRect(0, 0, Xwidth, Xheight);
	else
		CenterRect(owner);
	LLOG("Open NextRequest " << NextRequest(Xdisplay));
	Create(owner, false, false);
	xminsize.cx = xmaxsize.cx = Null;
	title2.Clear();
	if(!weplace) {
		LLOG("SyncCaption");
		SyncCaption0();
	}
	LLOG("SyncSizeHints");
	size_hints->flags = 0;
	SyncSizeHints();
	Rect r = GetRect();
	size_hints->x = r.left;
	size_hints->y = r.top;
	size_hints->width = r.Width();
	size_hints->height = r.Height();
	size_hints->win_gravity = StaticGravity;
	size_hints->flags |= PPosition|PSize|PWinGravity;
	if(owner) {
		ASSERT(owner->IsOpen());
		LLOG("XSetTransientForHint");
		XSetTransientForHint(Xdisplay, GetWindow(), owner->GetWindow());
	}
	LLOG("XSetWMNormalHints");
	XSetWMNormalHints(Xdisplay, GetWindow(), size_hints);
	Atom protocols[3];
	protocols[0] = XAtom("WM_DELETE_WINDOW");
	protocols[1] = XAtom("WM_TAKE_FOCUS");
	protocols[2] = XAtom("_NET_WM_PING");
	LLOG("XSetWMProtocols");
	XSetWMProtocols(Xdisplay, GetWindow(), protocols, 3);
	String x = GetExeTitle().ToString();
	const char *progname = ~x;
	class_hint->res_name = (char *)progname;
	class_hint->res_class = (char *)progname;
	XSetClassHint(Xdisplay, GetWindow(), class_hint);
	LLOG("WndShow(" << visible << ")");
	WndShow(visible);
	if(visible) {
		XEvent e;
		LLOG("XWindowEvent");
		XWindowEvent(Xdisplay, top->window, VisibilityChangeMask, &e);
		ignoretakefocus = true;
		SetTimeCallback(500, THISBACK(EndIgnoreTakeFocus));
		LLOG("SetWndFocus");
		SetWndFocus();
		for(int i = 0; i < 50; i++) {
			// X11 tries to move our window, so ignore the first set of ConfigureNotify
			// and move the window into position after FocusIn - but not if we want WM to
			// place the window
			if(weplace)
				while(XCheckTypedWindowEvent(Xdisplay, top->window, ConfigureNotify, &e)) {
					if(e.xconfigure.window != top->window)
						ProcessEvent(&e);
				}	
			if(XCheckTypedWindowEvent(Xdisplay, top->window, FocusIn, &e)) {
				ProcessEvent(&e);
				if(e.xfocus.window == top->window)
					break;
			}
			Sleep(10);
		}
	}
	if(weplace) {
		WndSetPos0(GetRect());
		LLOG("SyncCaption");
		SyncCaption0();
	}
	LLOG(">Open NextRequest " << NextRequest(Xdisplay));
	LLOG(">OPENED " << Name());
	PlaceFocus();
	StateH(OPEN);
	Vector<int> fe = GetPropertyInts(top->window, XAtom("_NET_FRAME_EXTENTS"));
	if(fe.GetCount() >= 4 &&
	   fe[0] >= 0 && fe[0] <= 16 && fe[1] >= 0 && fe[1] <= 16 && //fluxbox returns wrong numbers - quick&dirty workaround
	   fe[2] >= 0 && fe[2] <= 64 && fe[3] >= 0 && fe[3] <= 48)
	{
		GuiLock __;
		windowFrameMargin.left = max(windowFrameMargin.left, fe[0]);
		windowFrameMargin.right = max(windowFrameMargin.right, fe[1]);
		windowFrameMargin.top = max(windowFrameMargin.top, fe[2]);
		windowFrameMargin.bottom = max(windowFrameMargin.bottom, fe[3]);
	}
	if(IsOpen() && top)
		top->owner = owner;

	long curr_pid = getpid();
	XChangeProperty(Xdisplay, GetWindow(), XAtom("_NET_WM_PID"), XA_CARDINAL, 32,
	                PropModeReplace, (byte *) &curr_pid, 1);

	int version = 5;
	XChangeProperty(Xdisplay, GetWindow(), XAtom("XdndAware"), XA_ATOM, 32,
					0, (byte *)&version, 1);
	SyncState0(); 
	FixIcons();
}

void TopWindow::Open()
{
	GuiLock __; 
	Open(GetActiveWindow());
}

void TopWindow::OpenMain()
{
	GuiLock __; 
	Open(NULL);
}

void TopWindow::Minimize(bool)
{
	GuiLock __; 
	state = MINIMIZED;
	SyncState();
}

void TopWindow::Maximize(bool effect)
{
	GuiLock __; 
	state = MAXIMIZED;
	SyncState();
}

void TopWindow::Overlap(bool effect)
{
	GuiLock __; 
	state = OVERLAPPED;
	SyncState();
}

TopWindow& TopWindow::FullScreen(bool b)
{
	GuiLock __; 
	fullscreen = b;
	SyncState();
	return *this;
}

TopWindow& TopWindow::TopMost(bool b, bool)
{
	GuiLock __; 
	topmost = b;
	SyncState();
	return *this;
}

bool TopWindow::IsTopMost() const
{
	GuiLock __; 
	return topmost;
}

void TopWindow::GuiPlatformConstruct()
{
	size_hints = XAllocSizeHints();
	wm_hints = XAllocWMHints();
	class_hint = XAllocClassHint();
	topmost = false;
}

void TopWindow::GuiPlatformDestruct()
{
	XFree(size_hints);
	XFree(wm_hints);
	XFree(class_hint);
}

void TopWindow::SerializePlacement(Stream& s, bool reminimize)
{
	GuiLock __;
	int version = 0;
	s / version;
	Rect rect = GetRect();
	s % overlapped % rect;
	bool mn = state == MINIMIZED;
	bool mx = state == MAXIMIZED;
	s.Pack(mn, mx);
	LLOG("TopWindow::SerializePlacement / " << (s.IsStoring() ? "write" : "read"));
	LLOG("minimized = " << mn << ", maximized = " << mx);
	LLOG("rect = " << rect << ", overlapped = " << overlapped);
	if(s.IsLoading()) {
		if(mn) rect = overlapped;
		Rect limit = GetWorkArea();
		Rect fm = windowFrameMargin;
		if((fm.left|fm.right|fm.top|fm.bottom) == 0)
			fm = Rect(8, 32, 8, 8);
		limit.left += fm.left;
		limit.right -= fm.right;
		limit.top += fm.top;
		limit.bottom -= fm.bottom;
		Size sz = min(rect.Size(), limit.Size());
		rect = RectC(
			minmax(rect.left, limit.left, limit.right - sz.cx),
			minmax(rect.top,  limit.top,  limit.bottom - sz.cy),
			sz.cx, sz.cy);
		state = OVERLAPPED;
		if(mn && reminimize)
			state = MINIMIZED;
		if(mx)
			state = MAXIMIZED;
		if(state == OVERLAPPED)
			SetRect(rect);
		if(IsOpen()) {
			if(state == MINIMIZED)
				Minimize(false);
			if(state == MAXIMIZED)
				Maximize(false);
		}
	}
}

END_UPP_NAMESPACE

#endif
