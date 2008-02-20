#include "DockCtrl.h"

AutoHideBar& AutoHideBar::Attach(DockableCtrl& ctrl)
{
	TabInterface::Add(ctrl.SetOwnerBar(this));
	ShowBar();
	active = -1;
	return *this;
}

void AutoHideBar::Detach(DockableCtrl& ctrl)
{
	int n = TabInterface::Find(ctrl);
	if(n == -1) return;
	if(tabs.GetCount() == 1) 
	{
		CloseAll();
		HideBar();
	}
	else Close(n);
	ctrl.SetOwnerBar(NULL);
	HideWindow();
}

void AutoHideBar::ShowWindow()
{
	DockWindow* hiddenwindow = reinterpret_cast<DockWindow*>(tabs[active].dock);
	if(!hiddenwindow->IsOpen())
	{
		Size s  = hiddenwindow->SizeHint();
		Rect r  = GetScreenRect();
		Rect rr = r;	
		switch(layout)
		{
			case LAYOUT_LEFT:
				rr.left	= r.right;
				s.cy = r.GetHeight(); 
				break;
			case LAYOUT_TOP:
				rr.top = r.bottom;
				s.cx = r.GetWidth();
				break;
			case LAYOUT_RIGHT:
				rr.right = r.left;
				s.cy = r.GetHeight();
				break;
			case LAYOUT_BOTTOM:
				rr.bottom = r.top;
				s.cx = r.GetWidth();
				break;			
				
		}
		popup.SetRect(rr);
		popup.Add(hiddenwindow->SizePos());
		popup.PopUp(GetParent(), false, true, false, false);
		ctrl = hiddenwindow;
		Ctrl::ProcessEvents();
		AdjustSize(rr, s);
		Animate(popup, rr, GUIEFFECT_SLIDE);
	}
}

void AutoHideBar::HideWindow()
{
	if(popup.IsOpen()) 
		popup.Close();
	if(ctrl)
	{	
		if(ctrl->IsChild()) 
			ctrl->Remove();
		ctrl = NULL;
	}
	active = -1;
	highlight = -1;
	Refresh();
}

void AutoHideBar::AdjustSize(Rect &r, const Size &sz)
{
	switch (layout) 
	{
	 	case LAYOUT_LEFT: 
	 		r.right += sz.cx;
	 		break;
	 	case LAYOUT_TOP: 
	 		r.bottom += sz.cy;
	 		break;
	 	case LAYOUT_RIGHT:
	 		r.left -= sz.cx;
	 		break;
	 	case LAYOUT_BOTTOM:
	 		r.top -= sz.cy;
	 		break;
	};		
}

void AutoHideBar::OnHighlight()
{
	DockableCtrl* hiddenwindow = NULL;
	
	if(highlight >= 0)
		hiddenwindow = tabs[highlight].dock;
	if(!hiddenwindow || hiddenwindow == ctrl)
		return;
	else if(ctrl)
	{
		if(hiddenwindow)
		{
			if(popup.IsOpen())
				popup.Close();
			ctrl->Remove();
			ctrl = NULL;
		}
		else HideWindow();
	}
	if(hiddenwindow)
	{
		TabInterface::SetActiveTab(highlight);
		ShowWindow();
	}
}

void AutoHideBar::OnClose(int id, DockableCtrl& ctrl)
{
	ctrl.Shut();
}

void AutoHideBar::MouseMove(Point p, dword keysflags)
{
	TabInterface::MouseMove(p, keysflags);
}

void AutoHideBar::MouseEnter(Point p, dword keyflags)
{
	if (ctrl && ctrl->IsOpen()) KillTimeCallback(2);
	TabInterface::MouseEnter(p, keyflags);
}

void AutoHideBar::MouseLeave()
{
	if (ctrl && ctrl->IsOpen())
		KillSetTimeCallback(1000, THISBACK(HideWindow), 2);
	TabInterface::MouseLeave();
}

void AutoHideBar::FrameAdd(Ctrl& parent)
{
	parent.Add(*this);
}

void AutoHideBar::FrameRemove()
{
	this->Ctrl::Remove();
}

void AutoHideBar::ShowBar()
{
	if(!size) SetSize(GetHeight() + 2);
}

void AutoHideBar::HideBar()
{
	if(size) SetSize(0);
}

AutoHideBar::AutoHideBar()
{
	ctrl = NULL;
	
	SetStyle(DockCtrlChStyle::StyleDefault());
	HasScrollBar(false);
	HasButtons(false);
	HasIcons(true);
	SetSize(0);
	
	TabInterface::WhenHighlight = THISBACK(OnHighlight);
	TabInterface::WhenClose		= THISBACK(OnClose);
	
	popup.WhenEnter = THISBACK2(MouseEnter, Point(0, 0), 0);
	popup.WhenLeave = THISBACK(MouseLeave);
}

AutoHideBar::~AutoHideBar()
{
	ctrl = NULL;
}

void AutoHideBar::PopupWindow::ChildMouseEvent(Ctrl *child, int event, Point p, int zdelta, dword keyflags)
{
	if(event == MOUSELEAVE)
		WhenLeave();
	else if(event == MOUSEENTER) 
		WhenEnter();
	Ctrl::ChildMouseEvent(child, event, p, zdelta, keyflags);
}