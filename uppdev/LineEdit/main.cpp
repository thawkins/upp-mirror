#include <CtrlLib/CtrlLib.h>

using namespace Upp;

GUI_APP_MAIN
{
	LineEdit edit;
	edit.ShowSpaces();
	edit <<= LoadFile("C:/Users/cxl/Downloads/Nos.log");
	TopWindow win;
//	win.SetRect(100, 100, 100, 100);
	win.Sizeable();
	win.Add(edit.HSizePos(20, 20).VSizePos(20, 20));
	win.Open();
	edit.ShowLineEndings();
	edit.ShowCurrentLine(LtCyan());
	Ctrl::EventLoop();
//	SaveFile(ConfigFile("data.txt"), ~edit);
}

// 156 0x9c ""