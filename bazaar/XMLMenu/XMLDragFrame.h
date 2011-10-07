#ifndef _XMLDragFrame_h_
#define _XMLDragFrame_h_

#include <CtrlLib/CtrlLib.h>

NAMESPACE_UPP

class XMLDragFrame : public CtrlFrame
{
	private:
	
		Ptr<Ctrl> parent;
		
		int align;
	
	protected:
	
		// frame painting
		virtual void FramePaint(Draw& w, const Rect& r);
		
		// frame insertion/removing handlers
		virtual void FrameAdd(Ctrl &_parent);
		virtual void FrameRemove(void);

	public:
	
		// frame layout functions
		virtual void FrameLayout(Rect &r);
		virtual void FrameAddSize(Size &s);
	
		XMLDragFrame();
		~XMLDragFrame();
		
		XMLDragFrame &Align(int _align);
};

END_UPP_NAMESPACE

#endif
