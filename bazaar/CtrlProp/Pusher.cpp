#include "CtrlPropCommon.h"
#include <CtrlLib/CtrlLib.h>

bool PropSetLabel(const Value& v, Pusher& o) { if(!IsString(v)) return false; o.SetLabel(String(v)); return true; }
bool PropGetLabel(Value& v, const Pusher& o) { v = o.GetLabel(); return true; }
bool PropSetClickFocus(const Value& v, Pusher& o) { if(!IsNumber(v)) return false; o.ClickFocus(v); return true; }
bool PropGetClickFocus(Value& v, const Pusher& o) { v = o.IsClickFocus(); return true; }
bool PropGetVisualState(Value& v, const Pusher& o) { v = o.GetVisualState(); return true; }

PROPERTIES(Pusher, Ctrl)
PROPERTY("label", PropSetLabel, PropGetLabel)
PROPERTY("clickfocus", PropSetClickFocus, PropGetClickFocus)
//PROPERTY_GET("visualstate", PropGetVisualState)
END_PROPERTIES

PROPS(Ctrl, Pusher, Ctrl)


