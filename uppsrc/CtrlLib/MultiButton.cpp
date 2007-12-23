#include "CtrlLib.h"

NAMESPACE_UPP

CH_STYLE(MultiButton, Style, StyleDefault)
{
	for(int i = 0; i < 4; i++) {
		simple[i] = trivial[i] = left[i] = right[i] = lmiddle[i] = rmiddle[i]
			= Button::StyleEdge().look[i];
		monocolor[i] = Button::StyleEdge().monocolor[i];
		edge[i] = EditFieldEdge();
	}
	activeedge = false;
	trivialborder = 1;
	border = 1;
	pressoffset = Button::StyleEdge().pressoffset;
	sep1 = SColorHighlight();
	sep2 = SColorLight();
	sepm = 2;
	stdwidth = FrameButtonWidth();
	trivialsep = false;
	margin = Rect(2, 2, 2, 2);
	usetrivial = false;
	overpaint = 0;
}

CH_STYLE(MultiButton, Style, StyleFrame)
{
	Assign(MultiButton::StyleDefault());
}

MultiButton::SubButton::SubButton()
{
	cx = Null;
	left = false;
	monoimg = false;
	enabled = true;
	main = false;
}

void MultiButton::SubButton::Refresh()
{
	owner->Refresh();
	if(owner->Frame() && owner->GetParent())
		owner->GetParent()->RefreshLayout();
}

MultiButton::SubButton& MultiButton::SubButton::SetImage(const Image& m)
{
	img = m;
	monoimg = false;
	Refresh();
	return *this;
}

MultiButton::SubButton& MultiButton::SubButton::SetMonoImage(const Image& m)
{
	img = m;
	monoimg = true;
	Refresh();
	return *this;
}

MultiButton::SubButton& MultiButton::SubButton::SetStdImage()
{
	img = Null;
	monoimg = true;
	Refresh();
	return *this;
}

MultiButton::SubButton& MultiButton::SubButton::Left(bool b)
{
	left = b;
	Refresh();
	return *this;
}

MultiButton::SubButton& MultiButton::SubButton::Width(int w)
{
	cx = w;
	Refresh();
	return *this;
}

MultiButton::SubButton& MultiButton::SubButton::Enable(bool b)
{
	enabled = b;
	Refresh();
	return *this;
}

MultiButton::SubButton& MultiButton::AddButton()
{
	SubButton& b = button.Add();
	b.owner = this;
	return b;
}

MultiButton::SubButton& MultiButton::InsertButton(int i)
{
	SubButton& b = button.Insert(i);
	b.owner = this;
	return b;
}

MultiButton::SubButton& MultiButton::SubButton::Main(bool b)
{
	if(b)
		for(int i = 0; i < owner->button.GetCount(); i++)
			owner->button[i].main = false;
	main = b;
	return *this;
}

Size MultiButton::GetMinSize() const
{
	return Size(4, StdFont().Info().GetHeight() + 6);
}

void MultiButton::GotFocus()
{
	Refresh();
}

void MultiButton::LostFocus()
{
	Refresh();
}

static CtrlFrame& sNullFrame()
{
	static NullFrameClass f;
	return f;
}

int MultiButton::OverPaint() const
{
	return style->overpaint;
}

bool MultiButton::Frame()
{
	return false;
}

bool MultiButton::ComplexFrame()
{
	return Frame() ? GetParent() && &GetParent()->GetFrame() != &sNullFrame()
	                 : &GetFrame() != &sNullFrame();
}

bool MultiButton::Metrics(int& border, int& lx, int &rx, const Rect& r)
{
	border = IsTrivial() ? style->trivialborder : style->border;
	lx = r.left;
	rx = r.right;
	if(ComplexFrame()) {
		border = 0;
		return false;
	}
	if(!IsNull(style->edge[0])) {
		lx += border;
		rx -= border;
		return true;
	}
	border = 0;
	return false;
}

bool MultiButton::Metrics(int& border, int& lx, int &rx)
{
	return Metrics(border, lx, rx, GetSize());
}

int MultiButton::FindButton(int px)
{
	if(IsReadOnly())
		return Null;
	if(IsTrivial() && !Frame())
		return button[0].enabled ? 0 : Null;
	int border, lx, rx;
	Metrics(border, lx, rx);
	for(int i = 0; i < button.GetCount(); i++) {
		SubButton& b = button[i];
		int bcx = Nvl(b.cx, style->stdwidth);
		if(b.left) {
			lx += bcx;
			if(px < lx)
				return b.enabled ? i : Null;
		}
		else {
			rx -= bcx;
			if(px >= rx)
				return b.enabled ? i : Null;
		}
	}
	if(WhenPush || WhenClick)
		return MAIN;
	if(display)
		for(int i = 0; i < button.GetCount(); i++)
			if(button[i].main)
				return i;
	return Null;
}

void MultiButton::GetPos(int ii, int& x, int& cx)
{
	int border, lx, rx;
	Metrics(border, lx, rx);
	for(int i = 0; i <= ii; i++) {
		SubButton& b = button[i];
		cx = Nvl(b.cx, style->stdwidth);
		if(b.left) {
			x = lx;
			lx += cx;
		}
		else {
			rx -= cx;
			x = rx;
		}
	}
}

int MultiButton::ChState(int i)
{
	if(i == MAIN && Frame() && style->activeedge) {
		int q = 0;
		Ctrl *p = GetParent();
		if(p)
			q = !p->IsEnabled() || !IsEnabled() || i >= 0 && !button[i].enabled ? CTRL_DISABLED
			    : p->HasFocus() || push ? CTRL_PRESSED
			    : p->HasMouse() || hl >= 0 ? CTRL_HOT
			    : CTRL_NORMAL;
		return q;
	}
	if(IsTrivial() && !Frame())
		i = 0;
	if(!IsEnabled() || i >= 0 && !button[i].enabled)
		return CTRL_DISABLED;
	return hl == i ? push ? CTRL_PRESSED
	                      : CTRL_HOT
	               : CTRL_NORMAL;
}

void MultiButton::Lay(Rect& r)
{
	int border, lx, rx;
	bool frm = Metrics(border, lx, rx);
	bool left = false;
	bool right = false;
	for(int i = 0; i < button.GetCount(); i++) {
		const SubButton& b = button[i];
		int cx = Nvl(b.cx, style->stdwidth);
		int x;
		if(b.left) {
			x = lx;
			lx += cx;
		}
		else {
			rx -= cx;
			x = rx;
		}
		(b.left ? left : right) = true;
	}
	if(ComplexFrame()) {
		r.right = r.left + rx;
		r.left += lx;
	}
	else
	if(frm) {
		Rect m = style->margin;
		r = Rect(r.left + max(lx, m.left), r.top + m.top, min(rx, r.right - m.right), r.bottom - m.bottom);
	}
	else {
		Rect m = style->margin;
		r = Rect(r.left + max(lx, m.left), r.top + m.top, min(rx, r.right - m.right), r.bottom - m.bottom);
		if(!IsTrivial() || style->trivialsep) {
			if(left)
				r.left++;
			if(right)
				r.right--;
		}
	}
	if(!IsNull(valuecy)) {
		r.top += (r.GetHeight() - valuecy) / 2;
		r.bottom = r.top + valuecy;
	}
}

void MultiButton::Paint(Draw& w)
{
	Size sz = GetSize();
	int border, lx, rx;
	bool frm = Metrics(border, lx, rx);
	int mst = ChState(MAIN);
	if(frm)
		ChPaint(w, sz, style->edge[style->activeedge ? mst : 0]);
	bool left = false;
	bool right = false;
	for(int i = 0; i < button.GetCount(); i++) {
		SubButton& b = button[i];
		int st = ChState(i);
		int cx = Nvl(b.cx, style->stdwidth);
		int x;
		if(b.left) {
			x = lx;
			lx += cx;
		}
		else {
			rx -= cx;
			x = rx;
		}
		bool paintimg = true;
		Value v = b.left ? left ? style->lmiddle[st] : style->left[st]
		                 : right ? style->rmiddle[st] : style->right[st];
		if(ComplexFrame())
			ChPaint(w, x, border, cx, sz.cy - 2 * border, style->simple[st]);
		else
		if(frm) {
			if(IsTrivial() && style->usetrivial)
				paintimg = false;
			ChPaint(w, x, border, cx, sz.cy - 2 * border,
			        paintimg ? v : style->trivial[st]);
		}
		else {
			w.Clip(x, 0, cx, sz.cy);
			ChPaint(w, sz, style->look[Frame() ? mst : st/* == CTRL_DISABLED ? CTRL_NORMAL : st*/]);
			if(IsNull(v) || !Frame()) {
				if((!IsTrivial() || style->trivialsep) && IsEnabled()) {
					if(b.left) {
						if(left)
							ChPaint(w, x, style->sepm, 1, sz.cy - 2 * style->sepm, style->sep1);
						ChPaint(w, x + cx - 1, style->sepm, 1, sz.cy - 2 * style->sepm, style->sep2);
					}
					else {
						ChPaint(w, x, style->sepm, 1, sz.cy - 2 * style->sepm, style->sep1);
						if(right)
							ChPaint(w, x + cx - 1, style->sepm, 1, sz.cy - 2 * style->sepm, style->sep2);
					}
				}
			}
			else
				ChPaint(w, x, 0, cx, sz.cy, v);
			w.End();
		}
		if(paintimg) {
			Image m = Nvl(b.img, CtrlsImg::DA());
			Size isz = m.GetSize();
			Point p = (st == CTRL_PRESSED) * style->pressoffset;
			p.x += x + (cx - isz.cx) / 2;
			p.y += (sz.cy - isz.cy) / 2;
			if(b.monoimg || IsNull(b.img))
				w.DrawImage(p.x, p.y, m, style->monocolor[st]);
			else
				w.DrawImage(p.x, p.y, m);
		}
		(b.left ? left : right) = true;
	}
	Rect r, cr;
	Color text = SColorText();
	Color paper = Null;
	if(ComplexFrame()) {
		cr = GetSize();
		cr.left = lx;
		cr.right = rx;
		r = cr;
		paper = HasFocus() ? SColorHighlight() : SColorPaper();
		if(HasFocus())
			text = SColorHighlightText();
	}
	else
	if(frm) {
		Rect m = style->margin;
		r = Rect(max(lx, m.left), m.top, min(rx, sz.cx - m.right), sz.cy - m.bottom);
		Color paper;
		if(mst == CTRL_HOT && !IsTrivial())
			paper = Blend(SColorHighlight, SColorPaper, 235);
		else
		if(mst == CTRL_PRESSED && !IsTrivial())
			paper = Blend(SColorHighlight, SColorFace, 235);
		else
		if(HasFocus()) {
			paper = SColorHighlight();
			text = SColorHighlightText();
		}
		else
			paper = SColorPaper();
		w.DrawRect(r, paper);
		cr = r;
	}
	else {
		w.Clip(lx, 0, rx - lx, sz.cy);
		ChPaint(w, sz, style->look[mst]);
		Rect m = style->margin;
		r = Rect(max(lx, m.left), m.top, min(rx, sz.cx - m.right), sz.cy - m.bottom);
		if(!IsTrivial() || style->trivialsep) {
			if(left) {
				r.left++;
				if(IsEnabled())
					ChPaint(w, lx, style->sepm, 1, sz.cy - 2 * style->sepm, style->sep1);
			}
			if(right) {
				if(IsEnabled())
					ChPaint(w, rx - 1, style->sepm, 1, sz.cy - 2 * style->sepm, style->sep2);
				r.right--;
			}
		}
		w.End();
		cr = r;
	}
	cr.left++;
	if(!IsNull(valuecy)) {
		cr.top += (cr.GetHeight() - valuecy) / 2;
		cr.bottom = cr.top + valuecy;
	}
	Value v = convert->Format(value);
	bool f = HasFocus() && !push && frm;
	if(cr.left < cr.right && display) {
		w.Clip(cr);
		display->Paint(w, cr, v,
		               IsShowEnabled() ? text : SColorDisabled,
		               paper, f ? Display::CURSOR : Display::FOCUS|Display::CURSOR);
		w.End();
	}
	if(!frm && HasFocus())
		DrawFocus(w, r);
}

void MultiButton::MouseMove(Point p, dword flags)
{
	int h = FindButton(p.x);
	if(hl != h) {
		hl = h;
		Ctrl::Tip(hl >= 0 ? Nvl(button[hl].tip, tip) : tip);
		Refresh();
	}
	bool b = flags & K_MOUSELEFT;
	if(b != push) {
		push = b;
		Refresh();
	}
}

void MultiButton::LeftDown(Point p, dword flags)
{
	push = true;
	Refresh();
	if(IsNull(hl))
		pushrect = Null;
	else {
		if(hl == MAIN)
			pushrect = GetScreenRect();
		else {
			int x, cx;
			int border = IsTrivial() ? style->trivialborder : style->border;
			GetPos(hl, x, cx);
			pushrect = RectC(x, border, cx, GetSize().cy - 2 * border)
			           .Offseted(GetScreenView().TopLeft());
		}
		Sync();
		if(hl >= 0)
			button[hl].WhenPush();
		else
			WhenPush();
	}
}

void MultiButton::LeftUp(Point p, dword flags)
{
	push = false;
	Refresh();
	Sync();
	if(!IsNull(hl))
		if(hl >= 0)
			button[hl].WhenClick();
		else
			WhenClick();
}

void MultiButton::MouseLeave()
{
	hl = Null;
	Refresh();
}

void MultiButton::CancelMode()
{
	hl = Null;
	push = false;
	Refresh();
}

bool MultiButton::IsTrivial() const
{
	return button.GetCount() == 1 && IsNull(button[0].img) && !WhenPush && !WhenClick;
}

MultiButton::SubButton& MultiButton::MainButton()
{
	for(int i = 0; i < button.GetCount(); i++)
		if(button[i].main)
			return button[i];
	NEVER();
	return button[0];
}

void MultiButton::Reset()
{
	button.Clear();
}

MultiButton& MultiButton::SetDisplay(const Display& d)
{
	if(display != &d) {
		display = &d;
		Refresh();
	}
	return *this;
}

MultiButton& MultiButton::NoDisplay()
{
	if(display) {
		display = NULL;
		Refresh();
	}
	return *this;
}

MultiButton& MultiButton::SetConvert(const Convert& c)
{
	if(convert != &c) {
		convert = &c;
		Refresh();
	}
	return *this;
}

MultiButton& MultiButton::Set(const Value& v)
{
	if(value != v) {
		value = v;
		Refresh();
	}
	return *this;
}

MultiButton& MultiButton::SetValueCy(int cy)
{
	if(cy != valuecy) {
		valuecy = cy;
		Refresh();
		if(Frame() && GetParent())
			GetParent()->RefreshLayout();
	}
	return *this;
}

void MultiButton::SetData(const Value& v)
{
	if(value != v) {
		value = v;
		UpdateRefresh();
	}
}

Value MultiButton::GetData() const
{
	return value;
}

void MultiButton::PseudoPush(int bi)
{
	hl = bi;
	push = true;
	button[bi].WhenPush();
	Sync();
	Sleep(50);
	button[bi].WhenClick();
	hl = Null;
	push = false;
	Sync();
}

void MultiButton::PseudoPush()
{
	hl = MAIN;
	WhenPush();
	Sync();
	Sleep(50);
	WhenClick();
	hl = Null;
	push = false;
	Sync();
}

MultiButton::MultiButton()
{
	Transparent();
	SetStyle(StyleDefault());
	display = &StdDisplay();
	convert = &NoConvert();
	valuecy = Null;
	push = false;
	SetFrame(sNullFrame());
}

void MultiButtonFrame::FrameAdd(Ctrl& parent)
{
	parent.Add(*this);
	NoWantFocus();
}

void MultiButtonFrame::FrameRemove()
{
	Remove();
	SetWantFocus();
}

void MultiButtonFrame::FrameLayout(Rect& r)
{
	SetFrameRect(r);
	Lay(r);
}

void MultiButtonFrame::FrameAddSize(Size& sz)
{
	Rect r(0, 0, 100, 100);
	Lay(r);
	sz.cx += r.left + 100 - r.right;
	sz.cy += r.top + 100 - r.bottom;
}

bool MultiButtonFrame::Frame()
{
	return true;
}

void MultiButtonFrame::AddTo(Ctrl& w)
{
	w.SetFrame(sNullFrame());
	w.AddFrame(*this);
}

END_UPP_NAMESPACE
