#ifndef _Controls4U_Demo_Controls4U_Demo_h
#define _Controls4U_Demo_Controls4U_Demo_h

#include <CtrlLib/CtrlLib.h>

using namespace Upp;

#include "Controls4U/Controls4U.h"
#include "Functions4U/Functions4U.h"

#define LAYOUTFILE <Controls4U_Demo/Controls4U_Demo.lay>
#include <CtrlCore/lay.h>

class EditFileFolder_Demo : public WithEditFileFolder<StaticRect> {
public:
	typedef EditFileFolder_Demo CLASSNAME;
	EditFileFolder_Demo();
	void OnNewFile();
	void ChangeProperties();
};

class StaticCtrls_Demo : public WithStaticCtrls<StaticRect> {
public:
	typedef StaticCtrls_Demo CLASSNAME;
	StaticCtrls_Demo();
};

class StaticCtrlsTest_Demo : public WithStaticCtrlsTest<StaticRect> {
public:
	typedef StaticCtrlsTest_Demo CLASSNAME;
	StaticCtrlsTest_Demo();
};

class StaticClock_Demo : public WithStaticClock<StaticRect> {
public:
	typedef StaticClock_Demo CLASSNAME;
	StaticClock_Demo();
	void Update();
	void ChangeProperties();	
};

class Meter_Demo : public WithMeter<StaticRect> {
public:
	typedef Meter_Demo CLASSNAME;
	Meter_Demo();
	void ChangeValue();
	void ChangeProperties();
};

class Controls4U_Demo : public WithMain<TopWindow> {
public:
	typedef Controls4U_Demo CLASSNAME;
	Controls4U_Demo();
	
	EditFileFolder_Demo editFileFolder_Demo;
	StaticCtrls_Demo staticCtrls_Demo;
	StaticCtrlsTest_Demo staticCtrlsTest_Demo;
	StaticClock_Demo staticClock_Demo;
	Meter_Demo meter_Demo;
	
	int timerOn;
	void Timer();
};

#endif

