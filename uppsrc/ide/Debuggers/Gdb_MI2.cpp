#include "Debuggers.h"
#include <ide/ide.h>

#include "TypeSimplify.h"

#define LDUMP(x) // DDUMP(x)
#define LLOG(x)  // DLOG(x)

void WatchEdit::HighlightLine(int line, Vector<Highlight>& h, int pos)
{
	Color cEven = Blend(SColorInfo, White, 220);
	Color cOdd = Blend(SColorInfo, White, 128);
	for(int i = 0; i < h.GetCount(); i++)
		h[i].paper = (line % 2 ? cOdd : cEven);
}

// fill a pane with data from a couple of arrays without erasing it first
// (avoid re-painting and resetting scroll if not needed)
void Gdb_MI2::FillPane(ArrayCtrl &pane, Index<String> const &nam, Vector<String> const &val)
{
	int oldCount = pane.GetCount();
	int newCount = nam.GetCount();
	if(newCount < oldCount)
		for(int i = oldCount - 1; i >= newCount; i--)
			pane.Remove(i);
	for(int i = 0; i < min(oldCount, newCount); i++)
	{
		pane.Set(i, 0, nam[i]);
		pane.Set(i, 1, val[i]);
	}
	for(int i = oldCount; i < newCount; i++)
		pane.Add(nam[i], val[i]);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											PUBLIC IDE INTERFACE
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Gdb_MI2::DebugBar(Bar& bar)
{
	bar.Add("Stop debugging", DbgImg::StopDebug(), THISBACK(Stop)).Key(K_SHIFT_F5);
	bar.Separator();
#ifdef PLATFORM_POSIX
	bar.Add(!stopped, "Asynchronous break", THISBACK(AsyncBrk));
#endif
	bool b = !IdeIsDebugLock();
	bar.Add(b, "Step into", DbgImg::StepInto(), THISBACK1(Step, disas.HasFocus() ? "exec-step-instruction" : "exec-step")).Key(K_F11);
	bar.Add(b, "Step over", DbgImg::StepOver(), THISBACK1(Step, disas.HasFocus() ? "exec-next-instruction" : "exec-next")).Key(K_F10);
	bar.Add(b, "Step out", DbgImg::StepOut(), THISBACK1(Step, "exec-finish")).Key(K_SHIFT_F11);
	bar.Add(b, "Run to", DbgImg::RunTo(), THISBACK(doRunTo)).Key(K_CTRL_F10);
	bar.Add(b, "Run", DbgImg::Run(), THISBACK(Run)).Key(K_F5);
	bar.MenuSeparator();
	bar.Add(b, "Quick watch", THISBACK(QuickWatch)).Key(K_CTRL_Q);
	bar.MenuSeparator();
	bar.Add(b, "Copy backtrace", THISBACK(CopyStack));
	bar.Add(b, "Copy dissassembly", THISBACK(CopyDisas));
}

static int CharFilterReSlash(int c)
{
	return c == '\\' ? '/' : c;
}

bool Gdb_MI2::SetBreakpoint(const String& filename, int line, const String& bp)
{
	String file = Filter(host->GetHostPath(NormalizePath(filename)), CharFilterReSlash);
	
	// gets all breakpoints
	MIValue bps = GetBreakpoints();
	
	// line should start from 1...
	line++;
	
	// check wether we've got already a breakpoint here
	// and remove it
	MIValue brk = bps.FindBreakpoint(file, line);
	if(!brk.IsEmpty())
		if(!MICmd(Format("break-delete %s", brk["number"].Get())))
		{
			Exclamation(t_("Couldn't remove breakpoint"));
			return false;
		}
	
	if(bp.IsEmpty())
		return true;
	else if(bp[0] == 0xe)
		return MICmd(Format("break-insert %s:%d", file, line));
	else
		return MICmd(Format("break-insert -c \"%s\" %s:%d", bp, file, line));
}

bool Gdb_MI2::RunTo()
{
	String bi;
	bool df = disas.HasFocus();
	bool res;
	// sets a temporary breakpoint on cursor location
	// it'll be cleared automatically on first stop
	if(df)
	{
		if(!disas.GetCursor())
			return false;
		res = TryBreak(disas.GetCursor(), true);
	}
	else
		res = TryBreak(IdeGetFileName(), IdeGetFileLine() + 1, true);

	if(!res)
	{
		Exclamation(t_("No code at chosen location !"));
		return false;
	}

	Run();
	if(df)
		disas.SetFocus();
	
	return true;
}

void Gdb_MI2::Run()
{
	MIValue val;
	if(firstRun)
		// GDB up to 7.1 has a bug that maps -exec-run ro run, not to run&
		// making so async mode useless; we use the console run& command instead
// 2012-07-08 update : interrupting GDB in async mode without having
// non-stop mode enabled crashes X...don't know if it's a GDB bug or Theide one.
// anyways, by now we give up with async mode and remove 'Asynchronous break' function
		val = MICmd("exec-run");
//		val = MICmd("interpreter-exec console run&");
	else
		val = MICmd("exec-continue --all");
	
	int i = 50;
	while(!started && --i)
	{
		GuiSleep(20);
		Ctrl::ProcessEvents();
		ReadGdb(false);
	}
	if(!started)
	{
		Exclamation(t_("Failed to start application"));
		return;
	}
	Lock();
	while(dbg && !stopped)
	{
		GuiSleep(20);
		Ctrl::ProcessEvents();
		ReadGdb(false);
	}
	Unlock();

	if(stopped)
		CheckStopReason();
		
	started = stopped = false;
	firstRun = false;
	IdeActivateBottom();
}

#ifdef PLATFORM_POSIX
// sends a ctrl-c to debugger, returns true on success, false otherwise
bool Gdb_MI2::InterruptDebugger(void)
{
	int killed = 0;
	for(int iProc = 0; iProc < processes.GetCount(); iProc++)
		if(kill(processes[iProc], SIGINT) == 0)
			killed++;
	return killed;
}
#endif

#ifdef PLATFORM_POSIX
void Gdb_MI2::AsyncBrk()
{
	// data must be refreshed
	dataSynced = false;
	
	// send interrupt command to debugger
	if(!InterruptDebugger())
		return;
	
	// if successful, wait for some time for 'stopped' flag to become true
	for(int iMsec = 0; iMsec < 1000; iMsec += 20)
	{
		ReadGdb(false);
		Sleep(20);
		Ctrl::ProcessEvents();
		if(stopped)
			break;
	}
}
#endif

void Gdb_MI2::Stop()
{
	stopped = true;
	if(dbg && dbg->IsRunning())
		dbg->Kill();
}

bool Gdb_MI2::IsFinished()
{
	return !dbg->IsRunning() && !IdeIsDebugLock();
}

bool Gdb_MI2::Tip(const String& exp, CodeEditor::MouseTip& mt)
{
	// quick exit
	if(exp.IsEmpty() || !dbg)
		return false;

	int iVal;
	String val;

	// first look into locals....
	iVal = localExpressions.Find(exp);
	if(iVal >= 0)
		val = localValues[iVal];
	// if not fount, look into 'this'
	else
	{
		iVal = thisShortExpressions.Find(exp);
		if(iVal >= 0)
			val = thisValues[iVal];
		else
			return false;
	}

	mt.display = &StdDisplay();
	mt.value = exp + "=" + val;
	mt.sz = mt.display->GetStdSize(String(mt.value) + "X");
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											CONSTRUCTOR / DESTRUCTOR
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Gdb_MI2::Periodic()
{
	if(TTYQuit())
		Stop();
}

Gdb_MI2::Gdb_MI2()
{
	CtrlLayout(regs);
	regs.Height(regs.GetLayoutSize().cy);
	AddReg(RPREFIX "ax", &regs.rax);
	AddReg(RPREFIX "bx", &regs.rbx);
	AddReg(RPREFIX "cx", &regs.rcx);
	AddReg(RPREFIX "dx", &regs.rdx);
	AddReg(RPREFIX "si", &regs.rsi);
	AddReg(RPREFIX "di", &regs.rdi);
	AddReg(RPREFIX "bp", &regs.rbp);
	AddReg(RPREFIX "sp", &regs.rsp);
#ifdef CPU_64
	AddReg("r8", &regs.r8);
	AddReg("r9", &regs.r9);
	AddReg("r10", &regs.r10);
	AddReg("r11", &regs.r11);
	AddReg("r12", &regs.r12);
	AddReg("r13", &regs.r13);
	AddReg("r14", &regs.r14);
	AddReg("r15", &regs.r15);
#endif
	regs.Color(SColorLtFace);
	regs.AddFrame(TopSeparatorFrame());
	regs.AddFrame(RightSeparatorFrame());

	autos.NoHeader();
	autos.AddColumn("", 1);
	autos.AddColumn("", 10);
	autos.WhenLeftDouble = THISBACK1(onExploreExpr, &autos);
	autos.EvenRowColor();
	autos.OddRowColor();

	locals.NoHeader();
	locals.AddColumn("", 1);
	locals.AddColumn("", 10);
	locals.WhenLeftDouble = THISBACK1(onExploreExpr, &locals);
	locals.EvenRowColor();
	locals.OddRowColor();

	members.NoHeader();
	members.AddColumn("", 3);
	members.AddColumn("", 10);
	members.WhenLeftDouble = THISBACK1(onExploreExpr, &members);
	members.EvenRowColor();
	members.OddRowColor();

	watches.NoHeader();
	watches.AddColumn("", 1).Edit(watchedit);
	watches.AddColumn("", 10);
	watches.Inserting().Removing();
	watches.WhenLeftDouble = THISBACK1(onExploreExpr, &watches);
	watches.EvenRowColor();
	watches.OddRowColor();
	watches.WhenUpdateRow = THISBACK1(SyncWatches, MIValue());

	int c = EditField::GetStdHeight();
	explorer.AddColumn("", 1);
	explorer.AddColumn("", 6) /*.SetDisplay(Single<VisualDisplay>()) */;
	explorerPane.Add(explorerBackBtn.LeftPos(0, c).TopPos(0, c));
	explorerPane.Add(explorerForwardBtn.LeftPos(c + 2, c).TopPos(0, c));
	explorerPane.Add(explorerExprEdit.HSizePos(2 * c + 4).TopPos(0, c));
	explorerPane.Add(explorer.HSizePos().VSizePos(EditField::GetStdHeight(), 0));
	explorer.NoHeader();
	explorer.WhenLeftDouble = THISBACK(onExplorerChild);
	explorer.WhenBar = THISBACK(ExplorerMenu);

	explorerBackBtn.SetImage(DbgImg::ExplorerBack());
	explorerBackBtn <<= THISBACK(onExplorerBack);
	explorerForwardBtn.SetImage(DbgImg::ExplorerFw());
	explorerForwardBtn <<= THISBACK(onExplorerForward);
	explorerBackBtn.Disable();
	explorerForwardBtn.Disable();
	explorerHistoryPos = -1;
	
	Add(tab.SizePos());
	tab.Add(autos.SizePos(), "Autos");
	tab.Add(locals.SizePos(), t_("Locals"));
	tab.Add(members.SizePos(), t_("This"));
	tab.Add(watches.SizePos(), t_("Watches"));
	tab.Add(explorerPane.SizePos(), t_("Explorer"));
	
	Add(threadSelector.LeftPos(FindTabsRight() + 10, StdFont().GetWidth('X') * 10).TopPos(2, EditField::GetStdHeight()));
	Add(frame.HSizePos(threadSelector.GetRect().right + 10, 0).TopPos(2, EditField::GetStdHeight()));
	frame.Ctrl::Add(dlock.SizePos());
	dlock = "  Running..";
	dlock.SetFrame(BlackFrame());
	dlock.SetInk(Red);
	dlock.NoTransparent();
	dlock.Hide();

	CtrlLayout(quickwatch, "Quick watch");
	quickwatch.close.Cancel() <<= quickwatch.Breaker(IDCANCEL);
	quickwatch.evaluate.Ok() <<= quickwatch.Acceptor(IDOK);
	quickwatch.WhenClose = quickwatch.Breaker(IDCANCEL);
	quickwatch.value.SetReadOnly();
	quickwatch.value.SetFont(Courier(12));
	quickwatch.value.SetColor(TextCtrl::PAPER_READONLY, White);
	quickwatch.Sizeable().Zoomable();
	quickwatch.NoCenter();
	quickwatch.SetRect(0, 150, 300, 400);
	quickwatch.Icon(DbgImg::QuickWatch());

	Transparent();

	started = false;
	stopped = false;
	
	periodic.Set(-50, THISBACK(Periodic));
	dataSynced = false;
	
#ifdef flagMT
	// no running debug threads
	runningThreads = 0;
	stopThreads = false;
#endif

}

Gdb_MI2::~Gdb_MI2()
{
	IdeRemoveBottom(*this);
	IdeRemoveRight(disas);
	KillDebugTTY();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											PRIVATE FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

// find free space at right of tabs (we should probably add something to TabCtrl for that..)
int Gdb_MI2::FindTabsRight(void)
{
	if(!tab.GetCount())
		return 0;
	
	int lastTab = tab.GetCount() - 1;
	int i1 = 0, i2 = 10000;
	
	// bisect up it finds a point on last tab...
	int iTab = -1;
	int i = 0;
	while(iTab != lastTab)
	{
		i = (i1 + i2) / 2;
		iTab = tab.GetTab(Point(i, 0));
		if(iTab < 0)
			i2 = i;
		else
			i1 = i;
	}
	
	// now scan for righ tab edge
	i1 = i; i2 = i + 10000;
	while(abs(i1 - i2) > 2)
	{
		i = (i1 + i2) / 2;
		iTab = tab.GetTab(Point(i, 0));
		if(iTab == -1)
			i2 = i;
		else
			i1 = i;
	}
	return i;
}

// lock/unlock debugger controls
void Gdb_MI2::Lock()
{
	IdeDebugLock();
	watches.Disable();
	locals.Disable();
	frame.Disable();
	threadSelector.Disable();
	dlock.Show();
}

void Gdb_MI2::Unlock()
{
	if(IdeDebugUnLock())
	{
		watches.Enable();
		locals.Enable();
		frame.Enable();
		threadSelector.Enable();
		dlock.Hide();
	}
}

// read debugger output analyzing command responses and async output
// things are quite tricky because debugger output seems to be
// slow and we have almost no terminator to stop on -- (gdb) is not
// so reliable as it can happen (strangely) in middle of nothing
MIValue Gdb_MI2::ParseGdb(String const &output, bool wait)
{
	MIValue res;
	
	LDUMP(output);

	// parse result data
	StringStream ss(output);
	int iSubstr;
	while(!ss.IsEof())
	{
		String s = TrimBoth(ss.GetLine());
		
		// check 'running' and 'stopped' async output
		if(s.StartsWith("*running"))
		{
			started = true;
			stopReason.Clear();
			continue;
		}
// 2012-07-08 -- In some cases, GDB output get mixed with app one
//               so we shall look for 'stop' record in all string,
//               not just at beginning as it was before
// 				 not a wanderful way, as debugger could be tricked by some text...
/*
		else if(s.StartsWith("*stopped"))
		{
			stopped = true;
			s = '{' + s.Mid(9) + '}';
			stopReason = MIValue(s);
			continue;
		}
*/
		else if( (iSubstr = s.Find("*stopped,reason=")) >= 0)
		{
			stopped = true;
			s = '{' + s.Mid(iSubstr + 9) + '}';
			stopReason = MIValue(s);
			continue;
		}
		
		// catch process start/stop and store/remove pids
		else if(s.StartsWith("=thread-group-started,id="))
		{
			String id, pid;
			int i = s.Find("id=");
			if(i < 0)
				continue;
			i += 4;
			while(s[i] && s[i] != '"')
				id.Cat(s[i++]);
			i = s.Find("pid=");
			if(i < 0)
				continue;
			i += 5;
			while(s[i] && s[i] != '"')
				pid.Cat(s[i++]);
			
			processes.Add(id, atoi(pid));
		}
		
		else if(s.StartsWith("=thread-group-exited,id="))
		{
			String id;
			int i = s.Find("id=");
			if(i < 0)
				continue;
			i += 4;
			while(s[i] && s[i] != '"')
				id.Cat(s[i++]);
			i = processes.Find(id);
			if(i >= 0)
				processes.Remove(i);
		}
		
		// skip asynchronous responses
		// in future, we could be gather/use them
		if(s[0] == '*'|| s[0] == '=')
			continue;
		
		// here handling of command responses
		// we're not interested either, as we use MI interface
		if(s[0] == '~')
			continue;
		
		// here handling of target output
		// well, for now discard this one too, but it should go on console output
		if(s[0] == '~')
			continue;
	
		// here handling of gdb log/debug message
		// not interesting here
		if(s[0] == '&')
			continue;
		
		// now we SHALL have something starting with any of
		// // "^done", "^running", "^connected", "^error" or "^exit" records
		if(s.StartsWith("^done") || s.StartsWith("^running"))
		{
			// here we've got succesful command output in list form, if any
			// shall skip the comma; following can be a serie of pairs,
			// or directly an array of maps in form of :
			// [{key="value",key="value",...},{key="value"...}...]
			
			int i = 5; // just skip shortest, ^done
			while(s[i] && s[i] != ',')
				i++;
			if(!s[i])
				continue;
			i++;
			if(!s[i])
				continue;
			res = MIValue(s.Mid(i));
			continue;
		}
		else if(s.StartsWith("^error"))
		{
			// first array element is reserved for command result status
			s = s.Right(12); // '^error,msg=\"'
			s = s.Left(s.GetCount() - 1);
			res.SetError(s);
		}
		else
			continue;
	}
	return res;
}

MIValue Gdb_MI2::ReadGdb(bool wait)
{
	String output, s;
	MIValue res;
	
	if(wait)
	{
		// blocking path
		// waits for 3 seconds max, then return empty value
		// some commands (in particular if they return python exceptions)
		// have a delay between returned exception text and command result
		// so we shall wait up to the final (gdb)
		int retries = 13 * 50;
		while(dbg && retries--)
		{
			dbg->Read(s);
			output += s;
			if(TrimRight(s).EndsWith("(gdb)"))
				break;
			Sleep(20);
			continue;
		}
	}
	else if(dbg)
		dbg->Read(output);
	if(output.IsEmpty())
		return res;
	return ParseGdb(output);
}

// new-way commands using GDB MI interface
// on input  : MI interface command line
// on output : an MIValue containing GDB output
// STREAM OUTPUT
// ~						command response
// @						target output
// &						gdb log/debug messages
//
// RESULT RECORDS
// "^done" [ "," results ]
// "^running"				same as "^done"
//	"^connected"			gdb has connected to a remote target.
//	"^error" "," c-string	The operation failed. The c-string contains the corresponding error message.
//	"^exit"					gdb has terminate
//
// ASYNCHRONOUS RECORDS
// *running,thread-id="thread"
// *stopped,reason="reason",thread-id="id",stopped-threads="stopped",core="core"
// =thread-group-added,id="id"
// =thread-group-removed,id="id"
// =thread-group-started,id="id",pid="pid"
// =thread-group-exited,id="id"[,exit-code="code"]
// =thread-created,id="id",group-id="gid"
// =thread-exited,id="id",group-id="gid"
// =thread-selected,id="id"
// =library-loaded,...
// =library-unloaded,...
// =breakpoint-created,bkpt={...}
// =breakpoint-modified,bkpt={...}
// =breakpoint-deleted,bkpt={...}
//
// FRAME INFO INSIDE RESPONSES
// level		The level of the stack frame. The innermost frame has the level of zero. This field is always present.
// func		The name of the function corresponding to the frame. This field may be absent if gdb is unable to determine the function name.
// addr		The code address for the frame. This field is always present.
// file		The name of the source files that correspond to the frame's code address. This field may be absent.
// line		The source line corresponding to the frames' code address. This field may be absent.
// from		The name of the binary file (either executable or shared library) the corresponds to the frame's code address. This field may be absent. 

// THREAD INFO INSIDE RESPONSES
// id			The numeric id assigned to the thread by gdb. This field is always present.
// target-id	Target-specific string identifying the thread. This field is always present.
// details		Additional information about the thread provided by the target. It is supposed to be human-readable and not interpreted by the frontend. This field is optional.
// state		Either `stopped' or `running', depending on whether the thread is presently running. This field is always present.
// core		The value of this field is an integer number of the processor core the thread was last seen on. This field is optional. 
//
// REMARKS : by now, we just handle synchronous output and check asynchronous one just to detect
// debugger run/stop status -- all remaining asynchrnonous output is discarded
MIValue Gdb_MI2::MICmd(const char *cmdLine)
{
	LDUMP(cmdLine);
	// sends command to debugger and get result data

	// should handle dbg unexpected termination ?
	if(!dbg || !dbg->IsRunning() /* || IdeIsDebugLock() */)
		return MIValue();

	// consume previous output from gdb... don't know why sometimes
	// is there and gives problems to MI interface. We shall maybe
	// parse and store it somewhere
	ReadGdb(false);

	dbg->Write(String("-") + cmdLine + "\n");
	return ReadGdb();
}

// format breakpoint line from ide file and line
String Gdb_MI2::BreakPos(String const &file, int line)
{
	return String().Cat() << Filter(host->GetHostPath(NormalizePath(file)), CharFilterReSlash) << ":" << line;
}

// set breakpoint
MIValue Gdb_MI2::InsertBreakpoint(const char *file, int line)
{
	return MICmd("break-insert " + BreakPos(file, line));
}

// get breakpoints info
MIValue Gdb_MI2::GetBreakpoints(void)
{
	// issue a break-list command
	return MICmd("break-list")["BreakpointTable"];
}

MIValue Gdb_MI2::GetBreakpoint(int id)
{
	return MIValue();
}

MIValue Gdb_MI2::GetBreakPoint(const char *file, int line)
{
	return MIValue();
}

bool Gdb_MI2::TryBreak(adr_t addr, bool temp)
{
	String bp;
	if(temp)
		bp = "-t ";
	bp += Format("*0x%X", (int64)addr);

	MIValue res = MICmd(String("break-insert ") + bp);
	return !res.IsError();
}

bool Gdb_MI2::TryBreak(String const &file, int line, bool temp)
{
	String bp;
	if(temp)
		bp = "-t ";
	bp += BreakPos(file, line);
	MIValue res = MICmd(String("break-insert ") + bp);
	return !res.IsError();
}

void Gdb_MI2::SyncDisas(MIValue &fInfo, bool fr)
{
	if(!disas.IsVisible())
		return;

	// get current frame's address
	adr_t adr = stou(~fInfo["addr"].Get().Mid(2), NULL, 16);
	if(!disas.InRange(adr))
	{
		MIValue code;
		
		// if frame is inside a source file, disassemble current function
		if(fInfo.Find("file") >= 0 && fInfo.Find("line") >= 0)
		{
			String file = fInfo["file"];
			String line = fInfo["line"];
			code = MICmd(Format("data-disassemble -f %s -l %s -n -1 -- 0", file, line))["asm_insns"];
		}
		else
			// otherwise disassemble some -100 ... +100 bytes around address
			code = MICmd(Format("data-disassemble -s %x -e %x -- 0", (void *)(adr - 100), (void *)(adr + 100)))["asm_insns"];
			
		disas.Clear();
		for(int iLine = 0; iLine < code.GetCount(); iLine++)
		{
			MIValue &line = code[iLine];
			adr_t address = stou(~line["address"].Get().Mid(2), NULL, 16);
			String inst = line["inst"];
			int spc = inst.Find(' ');
			String opCode, operand;
			if(spc >= 0)
			{
				opCode = inst.Left(spc);
				operand = TrimBoth(inst.Mid(spc));
			}
			else
				opCode = inst;
			disas.Add(address, opCode, operand);
		}
	}
	
	// setup disassembler cursor
	disas.SetCursor(adr);
	disas.SetIp(adr, fr ? DbgImg::FrameLinePtr() : DbgImg::IpLinePtr());

	// update registers
	MIValue rNames = MICmd("data-list-register-names")["register-names"];
	MIValue rValues = MICmd("data-list-register-values x")["register-values"];
	Index<String> iNames;
	for(int i = 0; i < rNames.GetCount(); i++)
		iNames.Add(rNames[i]);
	for(int iReg = 0; iReg < regname.GetCount(); iReg++)
	{
		int i = iNames.Find(regname[iReg]);
		String rValue = "****************";
		if(i >= 0)
			rValue = "0000000000000000" + rValues[i]["value"].Get().Mid(2);
#ifdef CPU_64
		rValue = rValue.Right(16);
#else
		rValue = rValue.Right(8);
#endif
		if(reglbl[iReg]->GetText() != rValue)
		{
			reglbl[iReg]->SetLabel(rValue);
			reglbl[iReg]->SetInk(LtRed);
		}
		else
			reglbl[iReg]->SetInk(Black);
	}
			
}

// sync ide display with breakpoint position
void Gdb_MI2::SyncIde(bool fr)
{
	// kill pending update callbacks
	timeCallback.Kill();

	// get current frame info and level
	MIValue fInfo = MICmd("stack-info-frame")["frame"];
	int level = atoi(fInfo["level"].Get());
	
	// if we got file and line info, we can sync the source editor position
	autoLine = "";
	if(fInfo.Find("file") >= 0 && fInfo.Find("line") >= 0)
	{
		String file = GetLocalPath(fInfo["file"]);
		int line = atoi(fInfo["line"].Get());
		if(FileExists(file))
		{
			IdeSetDebugPos(GetLocalPath(file), line - 1, fr ? DbgImg::FrameLinePtr() : DbgImg::IpLinePtr(), 0);
			IdeSetDebugPos(GetLocalPath(file), line - 1, disas.HasFocus() ? fr ? DbgImg::FrameLinePtr() : DbgImg::IpLinePtr() : Image(), 1);
			autoLine = IdeGetLine(line - 2) + ' ' + IdeGetLine(line - 1) + ' ' + IdeGetLine(line);
		}
	}

	// setup threads droplist
	{
		threadSelector.Clear();
		MIValue tInfo = MICmd("thread-info");
		if(!tInfo.IsError() && !tInfo.IsEmpty())
		{
			int curThread = atoi(tInfo["current-thread-id"].Get());
			MIValue &threads = tInfo["threads"];
			for(int iThread = 0; iThread < threads.GetCount(); iThread++)
			{
				int id = atoi(threads[iThread]["id"].Get());
				if(id == curThread)
				{
					threadSelector.Add(id, Format("#%03x(*)", id));
					break;
				}
			}
			threadSelector <<= curThread;
		}
	}

	// get the arguments for current frame
	MIValue fArgs = MICmd(Format("stack-list-arguments 1 %d %d", level, level))["stack-args"][0]["args"];
	if(!fArgs.IsError() && !fArgs.IsEmpty())
	{
		// setup frame droplist
		frame.Clear();
		frame.Add(0, FormatFrame(fInfo, fArgs));
		frame <<= 0;
	}

	// sync disassembly
	SyncDisas(fInfo, fr);
	
	// update vars only on idle times
	timeCallback.Set(500, THISBACK(SyncData));
}

// logs frame data on console
void Gdb_MI2::LogFrame(String const &msg, MIValue &frame)
{
	String file = frame("file", "<unknown>");
	String line = frame("line", "<unknown>");
	String function = frame("function", "<unknown>");
	String addr = frame("addr", "<unknown>");

	PutConsole(Format(msg + " at %s, function '%s', file '%s', line %s", addr, function, file, line));
}

/*
// stop all running threads and re-select previous current thread
void Gdb_MI2::StopAllThreads(void)
{
	// get thread info for all threads
	MIValue tInfo = MICmd("thread-info");
	MIValue &threads = tInfo["threads"];
	bool someRunning = false;
	for(int iThread = 0; iThread < threads.GetCount(); iThread++)
	{
		if(threads[iThread]["state"].Get() != "stopped")
		{
			someRunning = true;
			break;
		}
	}
	// don't issue any stop command if no threads running
	// (brings problems....)
	if(!someRunning)
		return;

	// stores current thread id
	String current = tInfo("current-thread-id", "");
	
	// stops all threads
	MICmd("exec-interrupt --all");
	
	// just to be sure, reads out GDB output
	ReadGdb(false);
	
	// reselect current thread as it was before stopping all others
	if(current != "")
		MICmd("thread-select " + current);
}
*/

// check for stop reason
void Gdb_MI2::CheckStopReason(void)
{
	// data must be synced on stop...
	dataSynced = false;

	// we need to store stop reason BEFORE interrupting all other
	// threads, otherwise it'll be lost
	MIValue stReason = stopReason;
	
	// get the reason string
	String reason;
	if(stReason.IsEmpty())
		reason = "unknown reason";
	else
		reason = stReason["reason"];

/*
	// as we are in non-stop mode, to allow async break to work
	// we shall stop ALL running threads here, otherwise we'll have
	// problems when single stepping a gui MT app
	// single step will be done so for a single thread, while other
	// are idle. Maybe we could make this behaviour optional
	StopAllThreads();
*/

	if(reason == "exited-normally")
	{
		Stop();
		PutConsole("Program exited normally.");
	}
	else if(reason == "exited")
	{
		Stop();
		PutConsole("Program exited with code ");
	}
	else if(reason == "breakpoint-hit")
	{
		LogFrame("Hit breakpoint", stReason["frame"]);
		SyncIde();
	}
	else if(reason == "end-stepping-range")
	{
		LogFrame("End stepping range", stReason["frame"]);
		SyncIde();
	}
	else if(reason == "unknown reason")
	{
		PutConsole("Stopped by unknown reason");
		SyncIde();
	}
	else
	{
		// weird stop reasons (i.e., signals, segfaults... may not have a frame
		// data inside
		if(stReason.Find("frame") < 0)
			PutConsole(Format("Stopped, reason '%s'", reason));
		else
			LogFrame(Format("Stopped, reason '%s'", reason), stReason["frame"]);
		SyncIde();
	}
}

void Gdb_MI2::Step(const char *cmd)
{
	bool b = disas.HasFocus();

	MIValue res = MICmd(cmd);

	int i = 50;
	while(!started && --i)
	{
		GuiSleep(20);
		Ctrl::ProcessEvents();
		ReadGdb(false);
	}
	if(!started)
	{
		Stop();
		Exclamation(t_("Step failed - terminating debugger"));
		return;
	}

	Lock();
	ReadGdb(false);
	while(dbg && !stopped)
	{
		GuiSleep(20);
		Ctrl::ProcessEvents();
		ReadGdb(false);
	}
	Unlock();

	firstRun = false;
	if(stopped)
		CheckStopReason();
	started = stopped = false;
	IdeActivateBottom();

	// reset focus to disassembly pane if was there before
	if(b)
		disas.SetFocus();
}

// setup ide cursor based on disassembler one
void Gdb_MI2::DisasCursor()
{
	if(!disas.HasFocus())
		return;

	// temporary disable disas lost-focus handler
	// otherwise cursor will not be correctly set by following code
	disas.WhenFocus.Clear();

	// to get info of corresponding file/line of an address, the only way
	// we found is to insert a breakpoint, note the position and remove it
	MIValue b = MICmd(Format("break-insert *0x%X", (int64)disas.GetCursor()))["bkpt"];
	if(b.Find("file") >= 0 && b.Find("line") >= 0)
		IdeSetDebugPos(b["file"], atoi(b["line"].Get()) - 1, DbgImg::DisasPtr(), 1);
	if(b.Find("number") >= 0)
		MICmd(Format("break-delete %s", b["number"].Get()));
	disas.SetFocus();

	// re-enable disas lost focus handler
	disas.WhenFocus = THISBACK(DisasFocus);
}

// reset ide default cursor image when disassembler loose focus
void Gdb_MI2::DisasFocus()
{
	if(!disas.HasFocus())
		IdeSetDebugPos(IdeGetFileName(), IdeGetFileLine(), Null, 1);
}

// create a string representation of frame given its info and args
String Gdb_MI2::FormatFrame(MIValue &fInfo, MIValue &fArgs)
{
	int idx = atoi(fInfo("level", "-1"));
	if(idx < 0)
		return t_("invalid frame info");
	if(!fArgs.IsArray())
		return t_("invalid frame args");
	String func = fInfo("func", "<unknown>");
	String file = fInfo("file", "<unknown>");
	String line = fInfo("line", "<unknown>");
	int nArgs = fArgs.GetCount();
	String argLine;
	for(int iArg = 0; iArg < nArgs; iArg++)
	{
		argLine += fArgs[iArg]["name"].Get();
		if(fArgs[iArg].Find("value") >= 0)
			argLine << "=" << fArgs[iArg]["value"];
		argLine << ',';
	}
	if(!argLine.IsEmpty())
		argLine = "(" + argLine.Left(argLine.GetCount() - 1) + ")";
	return Format("%02d-%s%s at %s:%s", idx, func, argLine, file, line);
}

// re-fills frame's droplist when dropping it
void Gdb_MI2::DropFrames()
{
	int q = ~frame;
	frame.Clear();
	
	// get a list of frames
	MIValue frameList = MICmd("stack-list-frames")["stack"];
	if(frameList.IsError() || !frameList.IsArray())
	{
		Exclamation("Couldn't get stack frame list");
		return;
	}
	
	// get the arguments for all frames, values just for simple types
	MIValue frameArgs = MICmd("stack-list-arguments 1")["stack-args"];
	if(frameArgs.IsError() || !frameArgs.IsArray())
	{
		Exclamation("Couldn't get stack arguments list");
		return;
	}
	
	// fill the droplist
	for(int iFrame = 0; iFrame < frameArgs.GetCount(); iFrame++)
	{
		MIValue &fInfo = frameList[iFrame];
		MIValue &fArgs = frameArgs[iFrame]["args"];
		frame.Add(iFrame, FormatFrame(fInfo, fArgs));
	}
	frame <<= q;
}

// shows selected stack frame in editor
void Gdb_MI2::ShowFrame()
{
	int i = (int)~frame;
	if(!MICmd(Format("stack-select-frame %d", i)))
	{
		Exclamation(Format(t_("Couldn't select frame #%d"), i));
		return;
	}
	SyncIde(i);
}

// re-fills thread selector droplist on drop
void Gdb_MI2::dropThreads()
{
	int q = ~threadSelector;
	threadSelector.Clear();

	// get a list of all available threads
	MIValue tInfo = MICmd("thread-info");
	MIValue &threads = tInfo["threads"];
	if(!tInfo.IsTuple() || !threads.IsArray())
	{
		Exclamation(t_("couldn't get thread info"));
		return;
	}
	int currentId = atoi(tInfo["current-thread-id"].Get());
	for(int iThread = 0; iThread < threads.GetCount(); iThread++)
	{
		MIValue &t = threads[iThread];
		int id = atoi(t["id"].Get());
		String threadStr = Format("#%03x%s", id, (id == currentId ? "(*)" : ""));
		threadSelector.Add(id, threadStr);
	}
	threadSelector <<= q;
}

// selects current thread
void Gdb_MI2::showThread(void)
{
	int i = (int)~threadSelector;
	MICmd(Format("thread-select %d", i));

	SyncIde();	
}

struct CapitalLess
{
	bool operator()(String const &a, String const &b) const { return ToUpper(a) < ToUpper(b); }
};

// update local variables on demand
void Gdb_MI2::SyncLocals(MIValue val)
{
	bool firstCall = false;
	static VectorMap<String, String> prev;

	// if not done, start first evaluation step
	if(val.IsEmpty())
	{
		prev = DataMap(locals);

		// get local vars names
		MIValue locs = MICmd("stack-list-variables 1");
		if(!locs.IsTuple() || locs.Find("variables") < 0)
			return;
		
		// variables are returned as a tuple, named "variables"
		// containing a array of variables
		MIValue lLocs = locs["variables"];
		if(!lLocs.IsArray())
			return;

		// as values are in string format we must parse them
		// so we simply build a big string with all variables
		// and feed into MIValue parser
		String s = "{";
		for(int iLoc = 0; iLoc < lLocs.GetCount(); iLoc++)
		{
			MIValue &lLoc = lLocs[iLoc];
			String name = lLoc["name"];

			// skip 'this', we've a page for it
			if(name == "this")
				continue;
			s << name << "=" << lLoc["value"] << ",";
		}
		if(s.EndsWith(","))
			s = s.Left(s.GetCount()-1);
		s << "}";
		val = s;

		val.PackNames();
		AddAttribs("", val);
		val.FixArrays();
		
		TypeSimplify(val, false);
		
		firstCall = true;
	 }

	bool more = TypeSimplify(val, true);

	// collect variables names and values
	CollectVariables(val, localExpressions, localValues, localHints);

	// update locals pane
	FillPane(locals, localExpressions, localValues);
	
	if(more)
		// more evaluations are needed, respawn the function with a delay
		// big delay on first respawn to allow quick stepping without lag
		timeCallback.Set(firstCall ? 2000 : 200, THISBACK1(SyncLocals, val));
	else
		// when finished, mark changed values
		MarkChanged(prev, locals);
}

// update 'this' inspector data
void Gdb_MI2::SyncThis(MIValue val)
{
	bool firstCall = false;
	static VectorMap<String, String> prev;

	// if not done, start first evaluation step
	if(val.IsEmpty())
	{
		val = Evaluate("*this", false);
		prev = DataMap(members);

		// this is the first evaluation step
		// just simple types -- lag some seconds to full evaluation
		// which is quite slow
		firstCall = true;
	}

	// deep evaluation step
	bool more = TypeSimplify(val, true);
	
	// collect variables names and values
	CollectVariables(val, thisExpressions, thisValues, thisHints);

	// strip the (*this) from variables names
	thisShortExpressions.Clear();
	for(int iExpr = 0; iExpr < thisExpressions.GetCount(); iExpr++)
		thisShortExpressions.Add(thisExpressions[iExpr].Mid(8));

	// update 'this' pane
	FillPane(members, thisShortExpressions, thisValues);
	
	if(more)
		// more evaluations are needed, respawn the function with a delay
		// big delay on first respawn to allow quick stepping without lag
		timeCallback.Set(firstCall ? 2000 : 200, THISBACK1(SyncThis, val));
	else
		// when finished, mark changed values
		MarkChanged(prev, members);
}
		
// sync auto vars treectrl
void Gdb_MI2::SyncAutos()
{
	VectorMap<String, String> prev = DataMap(autos);
	autos.Clear();

	// read expressions around cursor line
	CParser p(autoLine);
	while(!p.IsEof())
	{
		if(p.IsId())
		{
			String exp = p.ReadId();
			for(;;)
			{
				if(p.Char('.') && p.IsId())
					exp << '.';
				else
				if(p.Char2('-', '>') && p.IsId())
					exp << "->";
				else
					break;
				exp << p.ReadId();
			}
			int idx = localExpressions.Find(exp);
			if(idx >= 0)
				autos.Add(exp, localValues[idx]);
		}
		p.SkipTerm();
	}
	autos.Sort();
	MarkChanged(prev, autos);
}

// sync watches treectrl
void Gdb_MI2::SyncWatches(MIValue val)
{
	static VectorMap<String, String> prev;
	bool firstCall = false;
	
	// on first cycle, we fast evaluate all expressions
	// without going deep, and put all results into an array
	if(val.IsEmpty())
	{
		prev = DataMap(watches);

		for(int i = 0; i < watches.GetCount(); i++)
		{
			String expr = watches.Get(i,0);
			MIValue res;
			res.Set("<can't evaluate>");
			MIValue valExpr = MICmd("data-evaluate-expression " + expr);

			if(valExpr.IsTuple() && valExpr.Find("value") >= 0)
			{
				MIValue const &tup = valExpr.Get("value");
				if(tup.IsString())
				{
					String s = tup.ToString();
					res = s;
				}
			}
			res.PackNames();
			AddAttribs("", res);
			res.FixArrays();

			val.Add(res);
		}
		firstCall = true;
	}

	bool more = false;
	
	// simplify loop, returns when at least one simplify step happens
	for(int iVal = 0; iVal < val.GetCount(); iVal++)
	{
		MIValue &v = val[iVal];
		more |= TypeSimplify(v, true);
		if(more)
			break;
	}
	// collects all results
	watchesValues.Clear();
	for(int iVal = 0; iVal < val.GetCount(); iVal++)
		watchesValues << CollectVariablesShort(val[iVal]);

	// fill expressions with ctrl expr contents
	watchesExpressions.Clear();
	for(int iVal = 0; iVal < watchesValues.GetCount(); iVal++)
		watchesExpressions << watches.Get(iVal, 0);

	// update locals pane
	FillPane(watches, watchesExpressions, watchesValues);
	
	if(more)
		// more evaluations are needed, respawn the function with a delay
		// big delay on first respawn to allow quick stepping without lag
		timeCallback.Set(firstCall ? 2000 : 200, THISBACK1(SyncWatches, val));
	else
		// when finished, mark changed values
		MarkChanged(prev, watches);
}

// sync data tabs, depending on which tab is shown
void Gdb_MI2::SyncData()
{
	if(dataSynced)
		return;
	
	// update stored 'this' member data
	// also used for tooltips and 'this' pane page
	SyncThis();
	
	// updated locals variables
	SyncLocals();

	SyncAutos();

	SyncWatches();
	
	dataSynced = true;
}

// watches arrayctrl key handling
bool Gdb_MI2::Key(dword key, int count)
{
	if(key >= 32 && key < 65535 && tab.Get() == 2)
	{
		watches.DoInsertAfter();
		Ctrl* f = GetFocusCtrl();
		if(f && watches.HasChildDeep(f))
			f->Key(key, count);
		return true;
	}
	if(key == K_ENTER && explorerExprEdit.HasFocus())
	{
		onExploreExpr();
		return true;
	}
	return Ctrl::Key(key, count);
}

// format watch line
String Gdb_MI2::FormatWatchLine(String exp, String const &val, int level)
{
	if(exp.GetCount() < 20)
		exp = exp + String(' ', 20);
	else
		exp = exp.Left(17) + "...";
	exp = exp.Left(20) + " = " + val;
	return String(' ', level * 4) + exp;
}

// deep watch current quickwatch variable
void Gdb_MI2::WatchDeep0(String parentExp, String const &var, int level, int &maxRemaining)
{
	// avoid endless recursion for circularly linked vars
	if(--maxRemaining <= 0)
		return;
	
	MIValue childInfo = MICmd("var-list-children 1 \"" + var + "\" 0 100");
	if(!childInfo || !childInfo.IsTuple())
		return;
	int nChilds = min(atoi(childInfo("numchild", "-1")), 100);
	if(nChilds <= 0)
		return;

	MIValue &childs = childInfo["children"];
	for(int i = 0; i < childs.GetCount() && maxRemaining > 0; i++)
	{
		MIValue child = childs[i];
		String exp = child["exp"];
		
		// handle pseudo children...
/*
		while(exp == "public" || exp == "private" || exp == "protected")
		{
			child = MICmd(String("var-list-children 1 \"") + child["name"] + "\"")["children"][0];
			exp = child["exp"];
		}
*/
		if(isdigit(exp[0]))
		{
			exp = '[' + exp + ']';
			if(parentExp.Mid(parentExp.GetCount() - 1, 1) == ".")
				parentExp = parentExp.Left(parentExp.GetCount() - 1);
		}
		if(exp[0] != '.' && exp[0] != '[')
			exp = '.' + exp;

		String type = child("type", "");
		if(!type.IsEmpty())
			type = "(" + type + ")";
		String value = child["value"];
		
		// try to format nicely results...
		quickwatch.value <<= (String)~quickwatch.value + "\n" + FormatWatchLine(parentExp + exp, type + value, level);
		
		// recursive deep watch
		WatchDeep0(exp, child["name"], level + 1, maxRemaining);
	}
}

void Gdb_MI2::WatchDeep(String parentExp, String const &name)
{
	// this is to avoid circular endless recursion
	// we limit the total watched (sub)variables to this count
	int maxRemaining = 300;
	
	WatchDeep0(parentExp, name, 1, maxRemaining);
}

// opens quick watch dialog
void Gdb_MI2::QuickWatch()
{
	// try to figure out if we've got the cursor in some interesting
	// place... if it is, grab the expression from there
	// otherwise let it unchanged
	Ctrl *c = GetFocusCtrl();
	if(typeid(*c) == typeid(AssistEditor))
	{
		AssistEditor *a = dynamic_cast<AssistEditor *>(c);
		String s = a->ReadIdBack(a->GetCursor());
		quickwatch.expression <<= s;
	}
	else if(c == &autos)
	{
		int i = autos.GetCursor();
		if(i >= 0)
			quickwatch.expression <<= autos.Get(i, 0);
	}
	else if(c == &locals)
	{
		int i = locals.GetCursor();
		if(i >= 0)
			quickwatch.expression <<= locals.Get(i, 0);
	}
	else if(c == &watches)
	{
		int i = watches.GetCursor();
		if(i >= 0)
			quickwatch.expression <<= watches.Get(i, 0);
	}
	else if(c == &explorer || c == &explorerExprEdit)
	{
		quickwatch.expression <<= ~explorerExprEdit;
	}
	
	for(;;)
	{
		String exp = ~quickwatch.expression;
		if(!exp.IsEmpty())
		{
			MIValue v = MICmd("var-create - @ " + exp);
			if(!v.IsError())
			{
				String type = v("type", "");
				if(!type.IsEmpty())
					type = "(" + type + ")";
				String value = v["value"];
				quickwatch.value <<= FormatWatchLine(exp, type + value, 0);
				quickwatch.expression.AddHistory();
				String name = v["name"];
				WatchDeep(exp, name);
				MICmd("var-delete " + name);
			}
			else
				quickwatch.value <<= t_("<can't evaluate expression>");
		}
		else
			quickwatch.value.Clear();
		int q = quickwatch.Run();
		if(q == IDCANCEL)
			break;
	}
	quickwatch.Close();
}

// copy stack frame list to clipboard
void Gdb_MI2::CopyStack()
{
	DropFrames();
	String s;
	for(int i = 0; i < frame.GetCount(); i++)
		s << frame.GetValue(i) << "\n";
	WriteClipboardText(s);
}

// copy disassembly listing to clipboard
void Gdb_MI2::CopyDisas()
{
	disas.WriteClipboard();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

// create GDB process and initializes it
bool Gdb_MI2::Create(One<Host> _host, const String& exefile, const String& cmdline, bool console)
{
	host = _host;
	dbg = host->StartProcess(GdbCommand(console) + GetHostPath(exefile) + " --interpreter=mi -q");
	if(!dbg) {
		Exclamation(t_("Error invoking gdb !"));
		return false;
	}
	IdeSetBottom(*this);
	IdeSetRight(disas);

	disas.AddFrame(regs);
	disas.WhenCursor = THISBACK(DisasCursor);
	disas.WhenFocus = THISBACK(DisasFocus);

	frame.WhenDrop = THISBACK(DropFrames);
	frame <<= THISBACK(ShowFrame);

	threadSelector.WhenDrop = THISBACK(dropThreads);
	threadSelector <<= THISBACK(showThread);

	watches.WhenAcceptEdit = THISBACK(SyncData);
	tab <<= THISBACK(SyncData);

	// this one will allow asynchronous break of running app
// 2012-07-08 -- DISABLED because of GDB bugs...
//	MICmd("gdb-set target-async 1");
	MICmd("gdb-set pagination off");

// Don't enable this one -- brings every sort of bugs with
// It was useful to issue Asynchronous break, but too many bugs
// to be useable
//	MICmd("gdb-set non-stop on");

//	MICmd("gdb-set interactive-mode off");

	MICmd("gdb-set disassembly-flavor intel");
	MICmd("gdb-set exec-done-display off");
	MICmd("gdb-set annotate 1");
	MICmd("gdb-set height 0");
	MICmd("gdb-set width 0");
	MICmd("gdb-set confirm off");
	MICmd("gdb-set print asm-demangle");
	
	// limit array printing to 200 elements
	MICmd("gdb-set print elements 200");
	
	// do NOT print repeated counts -- they confuse the parser
	MICmd("gdb-set print repeat 0");

	// we wanna addresses, we skip them in parser if useless
//	MICmd("gdb-set print address off");

	// print true objects having their pointers (if have vtbls)
	MICmd("gdb-set print object on");
	
	// print symbolic addresses, if found
	MICmd("gdb-set print symbol on");
	
	// do not search of symbolic address near current one
	MICmd("gdb-set print max-symbolic-offset 1");
	
	// avoids debugger crash if caught inside ungrabbing function
	// (don't solves all cases, but helps...)
	MICmd("gdb-set unwindonsignal on");

	if(!IsNull(cmdline))
		MICmd("gdb-set args " + cmdline);
	
	firstRun = true;

	return true;
}

One<Debugger> Gdb_MI2Create(One<Host> host, const String& exefile, const String& cmdline, bool console)
{
	Gdb_MI2 *dbg = new Gdb_MI2;
	if(!dbg->Create(host, exefile, cmdline, console))
	{
		delete dbg;
		return NULL;
	}
	return dbg;
}
