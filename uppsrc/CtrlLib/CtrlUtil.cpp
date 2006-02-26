#include "CtrlLib.h"

#define LLOG(x) // RLOG(x)

#define IMAGECLASS CtrlImg
#define IMAGEFILE  <CtrlLib/Ctrl.iml>
#include <Draw/iml_source.h>

void Animate(Ctrl& c, const Rect& target, int type)
{
	if(type < 0)
		if(Ctrl::IsFlag(Ctrl::EFFECT_FADE))
			type = Ctrl::EFFECT_FADE;
		else
		if(Ctrl::IsFlag(Ctrl::EFFECT_SLIDE))
			type = Ctrl::EFFECT_SLIDE;
	Rect r0 = c.GetRect();
	dword time0 = GetTickCount();
	for(;;) {
		dword t = GetTickCount() - time0;
		if(t > 200)
			break;
		if(type == Ctrl::EFFECT_SLIDE) {
			int q = 25 * t / 200;
			q *= q;
			Rect r = r0;
			if(r.left > target.left)
				r.left = max(r.left - q, target.left);
			if(r.top > target.top)
				r.top = max(r.top - q, target.top);
			if(r.right < target.right)
				r.right = min(r.right + q, target.right);
			if(r.bottom < target.bottom)
				r.bottom = min(r.bottom + q, target.bottom);
			if(r == target)
				break;
			c.SetRect(r);
		}
		else
		if(type == Ctrl::EFFECT_FADE)
			c.SetAlpha((byte)(255 * t / 200));
		else
			break;
		c.Sync();
		Sleep(0);
	}
	c.SetRect(target);
	c.SetAlpha(255);
}

void Animate(Ctrl& c, int x, int y, int cx, int cy, int type)
{
	Animate(c, RectC(x, y, cx, cy), type);
}

const Image& GetIBeamCursor() {
	return CtrlImg::ibeam0();
}

bool CtrlLibDisplayError(const Value& e) {
	if(!e.IsError())
		return false;
	String s = ValueTo<String>(e);
	if(s.IsEmpty())
		s = t_("Invalid data.");
	Exclamation(s);
	return true;
}

INITBLOCK
{
	DisplayErrorFn() = &CtrlLibDisplayError;
}
/*
String SaveCtrlLayout(Ctrl::LogPos p, const String& classname, const String& variable,
					  const String& label, const String& help) {
	String out;
	if(classname.IsEmpty())
		out << "\tUNTYPED(";
	else
		out << "\tITEM(" << classname << ", ";
	out << variable << ", ";
	switch(p.x.GetAlign()) {
	case Ctrl::LEFT:   out << Format("LeftPos(%d, %d).", p.x.GetA(), p.x.GetB()); break;
	case Ctrl::RIGHT:  out << Format("RightPos(%d, %d).", p.x.GetA(), p.x.GetB()); break;
	case Ctrl::SIZE:   out << Format("HSizePos(%d, %d).", p.x.GetA(), p.x.GetB()); break;
	case Ctrl::CENTER: out << Format("HCenterPos(%d, %d).", p.x.GetB(), p.x.GetA()); break;
	}
	switch(p.y.GetAlign()) {
	case Ctrl::TOP:    out << Format("TopPos(%d, %d)", p.y.GetA(), p.y.GetB()); break;
	case Ctrl::BOTTOM: out << Format("BottomPos(%d, %d)", p.y.GetA(), p.y.GetB()); break;
	case Ctrl::SIZE:   out << Format("VSizePos(%d, %d)", p.y.GetA(), p.y.GetB()); break;
	case Ctrl::CENTER: out << Format("VCenterPos(%d, %d)", p.y.GetB(), p.y.GetA()); break;
	}
	if(!label.IsEmpty()) {
		out << ".SetLabel(\"";
		for(const char *s = label; *s; s++)
			if(*s == '\n')
				out.Cat("\\n");
			else
				out.Cat(*s);
		out << "\")";
	}
	if(!help.IsEmpty()) {
		out << ".HelpC(\"";
		for(const char *s = help; *s; s++)
			if(*s == '\n')
				out.Cat("\\n");
			else
				out.Cat(*s);
		out << "\")";
	}
	out << ")\n";
	return out;
}
*/
void Show2(Ctrl& ctrl1, Ctrl& ctrl2, bool show) {
	ctrl1.Show(show);
	ctrl2.Show(show);
}

void Hide2(Ctrl& ctrl1, Ctrl& ctrl2) {
	Show2(ctrl1, ctrl2, false);
}

void DelayCallback::Invoke() {
	KillTimeCallback(this);
	SetTimeCallback(delay, target, this);
}

bool EditText(String& s, const char *title, const char *label, int (*f)(int), int maxlen)
{
	WithEditStringLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, title);
	dlg.lbl = label;
	dlg.text = s.ToWString();
	dlg.text.SetFilter(f);
	if(maxlen) dlg.text.MaxLen(maxlen);
	if(dlg.Execute() == IDOK) {
		s = dlg.text;
		return true;
	}
	return false;
}

bool EditText(String& s, const char *title, const char *label, int maxlen)
{
	return EditText(s, title, label, CharFilterUnicode, maxlen);
}

bool EditText(WString& s, const char *title, const char *label, int (*f)(int), int maxlen)
{
	WithEditStringLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, title);
	dlg.lbl = label;
	dlg.text = s;
	dlg.text.SetFilter(f);
	if(maxlen) dlg.text.MaxLen(maxlen);
	if(dlg.Execute() == IDOK) {
		s = dlg.text.GetText();
		return true;
	}
	return false;
}

bool EditText(WString& s, const char *title, const char *label, int maxlen)
{
	return EditText(s, title, label, CharFilterUnicode, maxlen);
}

Callback CtrlRetriever::operator<<=(Callback cb)
{
	for(int i = 0; i < item.GetCount(); i++) {
		CtrlItem0 *m = dynamic_cast<CtrlItem0 *>(&item[i]);
		if(m)
			m->ctrl->WhenAction = cb;
	}
	return cb;
}

void CtrlRetriever::Retrieve()
{
	for(int i = 0; i < item.GetCount(); i++)
		item[i].Retrieve();
}
