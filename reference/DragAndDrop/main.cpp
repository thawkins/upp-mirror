#include "CtrlLib/CtrlLib.h"

using namespace Upp;

const char *MyDnDName = "MyReferenceDragAndDropExample";

struct MyApp : TopWindow {
	TreeCtrl   tree;

	typedef MyApp CLASSNAME;

	void DropInsert(int parent, int ii, PasteClip& d)
	{
		tree.AdjustAction(parent, d);
		if(d.Accept(MyDnDName)) {
			tree.SetCursor(tree.Insert(parent, ii, Image(), ~d));
			tree.SetFocus();
		}
	}

	void Drag()
	{
		if(!tree.IsCursor())
			return;
		int id = tree.GetCursor();
		String text = tree.Get();
		Size isz = GetTextSize(text.ToWString(), StdFont());
		ImageDraw iw(isz);
		iw.DrawRect(isz, White);
		iw.DrawText(0, 0, text);
		
		VectorMap<String, ClipData> clip;
		clip.Add(MyDnDName, text);

		if(DoDragAndDrop(clip, iw) == DND_MOVE)
			tree.Remove(id);
	}

	MyApp() {
		Add(tree.SizePos());
		Vector<int> parent, parent2;
		parent.Add(0);
		tree.SetRoot(Image(), "The Tree");
		for(int i = 1; i < 10000; i++) {
			parent.Add(tree.Add(parent[rand() % parent.GetCount()], Image(),
			           FormatIntRoman(i, true)));
			if((rand() & 3) == 0)
				tree.Open(parent.Top());
		}
		tree.Open(0);
		tree.WhenDropInsert = THISBACK(DropInsert);
		tree.WhenDrag = THISBACK(Drag);
		Sizeable();
	}
};

GUI_APP_MAIN
{
	MyApp().Run();
}
