#include "ide.h"

#define IMAGECLASS IdeImg
#define IMAGEFILE  <ide/ide.iml>
#include <Draw/iml_source.h>

#include <art/BlueBar/BlueBar.h>

void ChStdSkinBlue()
{
	ChStdSkin();
	InstallBlueBar();
}

void ChHostSkinBlue()
{
	ChHostSkin();
	InstallBlueBar();
}

void ChClassicSkinBlue()
{
	ChClassicSkin();
	InstallBlueBar();
}

void Ide::SyncCh()
{
	switch(chstyle) {
	case 0:
		SetSkin(ChHostSkin);
		break;
	case 1:
		SetSkin(ChStdSkin);
		break;
	case 2:
		SetSkin(ChClassicSkin);
		break;
	case 3:
		SetSkin(ChHostSkinBlue);
		break;
	case 4:
		SetSkin(ChStdSkinBlue);
		break;
	}
}

void Ide::ToggleVerboseBuild() {
	console.verbosebuild = !console.verbosebuild;
}

void Ide::ToggleStopOnErrors() {
	stoponerrors = !stoponerrors;
}

void Ide::SwapPackagesFiles()
{
	wesplit.Zoom(wesplit.GetZoom() == 1 ? -1 : 1);
}

void Ide::ConsoleClear()
{
	console <<= Null;
}

void Ide::ConsoleCopy()
{
	console.Copy();
}

void Ide::ConsolePaste()
{
	String s = ReadClipboardText();
	if(!IsNull(s)) {
		s.Insert(0, '\n');
		int len = console.GetLength();
		console.Insert(len, s.ToWString());
		console.SetCursor(len + 1);
	}
}

void Ide::Serialize(Stream& s) {
	int version = 5;
	s.Magic(0x12346);
	s / version;
	s % main;
	SerializePlacement(s);
	s % AnySourceFs();
	s % BasedSourceFs();
	s % AnyPackageFs();
	s % pfsplit;
	s % wesplit;
	s % browser;
	s % package;
	s % filelist;
	s % editorfont;
	s % veditorfont;
	s % consolefont;
	s % font1;
	s % font2;
	s % show_status_bar;
	s % toolbar_in_row;
	s % filetabs;
	s % auto_enclose;
	s % show_tabs;
	s % tabs_icons;
	s % tabs_crosses;
	s % no_parenthesis_indent;
	s % hilite_scope;
	s % hilite_if_endif;
	s % hilite_bracket;
	s % hilite_ifdef;
	s % wrap_console_text;
	s % mute_sounds;
	s % line_numbers;
	s % browser.clickpos;
	s % browser_closeesc;
	s % bookmark_pos;
	s % use_target;
	s % default_charset;
	s % header_guards;
	s % insert_include;
	String varsname = GetVarsName();
	s % varsname;
	s % pocfg;
	if(s.IsLoading())
		LoadVars(varsname);
	SerializeGlobalConfigs(s);
	doc.Serialize(s);
	s % right_split;
	s % splash_screen;
	s % editor.auto_assist;
	if(version >= 1)
		s % editor.commentdp;
	if(version >= 2) {
		s % bordercolumn;
		s % bordercolor;
	}
	if(version >= 3) {
		s % hydra1_threads;
		if(s.IsLoading())
			console.SetSlots(hydra1_threads);
	}
	if(version >= 4) {
		s % doc;
	}
	if(version >= 5) {
		s % chstyle;
	}
	s.Magic();
}

void Ide::PutConsole(const char *s)
{
	ShowConsole();
	console << s << "\n";
}

void Ide::PutVerbose(const char *s)
{
	if(console.verbosebuild) {
		PutConsole(s);
		console.Sync();
	}
}

bool Ide::IdeIsBuilding() const
{
	return idestate == Ide::BUILDING;
}

String Ide::IdeGetOneFile() const
{
	return onefile;
}

int Ide::IdeConsoleExecute(const char *cmdline, Stream *out, const char *envptr, bool quiet)
{
	return console.Execute(cmdline, out, envptr, quiet);
}

int Ide::IdeConsoleExecute(One<SlaveProcess> process, const char *cmdline, Stream *out, bool quiet)
{
	return console.Execute(process, cmdline, out, quiet);
}

int Ide::IdeConsoleAllocSlot()
{
	return console.AllocSlot();
}

bool Ide::IdeConsoleRun(const char *cmdline, Stream *out, const char *envptr, bool quiet, int slot, String key, int blitz_count)
{
	return console.Run(cmdline, out, envptr, quiet, slot, key, blitz_count);
}

bool Ide::IdeConsoleRun(One<SlaveProcess> process, const char *cmdline, Stream *out, bool quiet, int slot, String key, int blitz_count)
{
	return console.Run(process, cmdline, out, quiet, slot, key, blitz_count);
}

void Ide::IdeConsoleFlush()
{
	console.Flush();
}

void Ide::IdeConsoleBeginGroup(String group)
{
	console.BeginGroup(group);
}

void Ide::IdeConsoleEndGroup()
{
	console.EndGroup();
}

bool Ide::IdeConsoleWait()
{
	return console.Wait();
}

void Ide::IdeSetRight(Ctrl& ctrl)
{
	right.Add(ctrl.SizePos());
	right_split.Zoom(-1);
}

void Ide::IdeRemoveRight(Ctrl& ctrl)
{
	ctrl.Remove();
	if(!right.GetFirstChild())
		right_split.Zoom(0);
}

bool Ide::IdeIsDebug() const
{
	return debugger;
}

void Ide::IdeEndDebug()
{
	console.Kill();
	debugger.Clear();
	debuglock = 0;
	MakeTitle();
	editor.EnableBreakpointing();
	SetBar();
	editor.HidePtr();
	for(int i = 0; i < 2; i++)
		posfile[i].Clear();
	PosSync();
	const Workspace& wspc = IdeWorkspace();
	for(int i = 0; i < wspc.GetCount(); i++) {
		const Package& pk = wspc.GetPackage(i);
		String n = wspc[i];
		for(int i = 0; i < pk.file.GetCount(); i++) {
			String file = SourcePath(n, pk.file[i]);
			ValidateBreakpoints(Filedata(file).lineinfo);
		}
	}
	editor.ValidateBreakpoints();
}

void Ide::IdeSetDebugPos(const String& file, int line, const Image& img, int i)
{
	posfile[i] = file;
	posline[i] = line;
	posimg[i] = img;
	EditFile(file);
	editor.GotoLine(line);
	PosSync();
	Size sz = editor.GetPageSize();
	Point p = editor.GetScrollPos();
	int l = editor.GetCursorLine();
	if(sz.cy > 8 && l - p.y > sz.cy - 4)
		editor.SetScrollPos(Point(p.x, max(0, l - 4)));
	SetForeground();
	editor.SetFocus();
}

void Ide::IdeHidePtr()
{
	editor.HidePtr();
}

bool Ide::IdeDebugLock()
{
	if(debuglock == 0) {
		editor.DisableBreakpointing();
		MakeTitle();
		SetBar();
		Sync();
		debuglock = 1;
		return true;
	}
	debuglock++;
	return false;
}

bool Ide::IdeDebugUnLock()
{
	debuglock--;
	ASSERT(debuglock >= 0);
	if(debuglock == 0) {
		MakeTitle();
		editor.EnableBreakpointing();
		SetBar();
		return true;
	}
	return false;
}

bool Ide::IdeIsDebugLock() const
{
	return debuglock;
}

String Ide::IdeGetFileName() const
{
	return !designer ? editfile : Null;
}

int Ide::IdeGetFileLine()
{
	return !designer ? editor.GetNoLine(editor.GetCursorLine()) : 0;
}

String Ide::IdeGetLine(int i) const
{
	if(i >= 0 && i < editor.GetLineCount())
		return editor.GetUtf8Line(i);
	else
		return Null;
}

void Ide::IdeSetBar()
{
	SetBar();
}

void Ide::SetupBars()
{
	ClearFrames();
	int r = HorzLayoutZoom(100);
	int l = HorzLayoutZoom(320);
	menubar.Transparent();
	if(toolbar_in_row) {
		toolbar.SetFrame(NullFrame());
		bararea.Add(menubar.LeftPos(0, l).VCenterPos(menubar.GetStdHeight()));
		bararea.Add(toolbar.HSizePos(l, r).VCenterPos(toolbar.GetStdHeight()));
		bararea.Add(display.RightPos(4, r).VSizePos(2, 3));
		bararea.Height(max(menubar.GetStdHeight(), toolbar.GetStdHeight()));
		AddFrame(bararea);
		toolbar.Transparent();
	}
	else {
		bararea.Add(menubar.LeftPos(0, l).VCenterPos(menubar.GetStdHeight()));
		bararea.Add(display.RightPos(4, r).VSizePos(2, 3));
		bararea.Height(menubar.GetStdHeight());
		AddFrame(bararea);
		AddFrame(TopSeparatorFrame());
		AddFrame(toolbar);
		toolbar.NoTransparent();
	}
	AddFrame(statusbar);
	SetBar();
}

Ide::Ide()
{
	start_time = GetSysTime();
	stat_build_time = 0;
	build_start_time = Null;
	hydra1_threads = CPU_Cores();

	chstyle = 0;

	Sizeable().Zoomable();

	display.SetFrame(ThinInsetFrame());
	display.SetAlign(ALIGN_CENTER);

	filelist.Columns(2);
	package.Columns(2);

	menubar.WhenHelp = statusbar;
	menubar.AreaLook(1);
	toolbar.WhenHelp = statusbar;
	toolbar.AreaLook(1);
	SetupBars();
	SetBar();
	WhenClose = THISBACK(Exit);

	editorsplit.Vert(editor, editor2);
	editorsplit.Zoom(0);
	SyncEditorSplit();

	right_split.Horz(editpane, right);
	right_split.SetPos(7000);
	right_split.Zoom(0);

	editor_bottom.Vert(right_split, bottom);
	console.WhenBar = THISBACK(ConsoleMenu);
	editor_bottom.SetPos(8000);
	bottom.SetFrame(btabs);
	bottom.Add(console.SizePos().SetFrame(NullFrame()));
	bottom.Add(browser.SizePos());
	bottom.Add(calc.SizePos().SetFrame(NullFrame()));
	btabs <<= THISBACK(SyncBottom);
	BTabs();
	browser.WhenPos = THISBACK(GotoPos);

	pfsplit.SetPos(2000);
	pfsplit.Vert(package, filelist);
	wesplit.Horz(pfsplit, editor_bottom);
	wesplit.SetPos(2000);
	Add(wesplit);

	editor.topsbbutton.ScrollStyle().NoWantFocus().Show();
	editor.topsbbutton1.ScrollStyle().NoWantFocus().Show();
	tabs <<= THISBACK(TabFile);
	tabs.WhenCloseRest = THISBACK1(CloseRest, &tabs);
	editor2.SetFrame(tabs2);
	tabs2 <<= THISBACK(TabFile2);
	tabs2.WhenCloseRest = THISBACK1(CloseRest, &tabs2);
	editor2.topsbbutton.ScrollStyle().NoWantFocus().Show();
	editor2.topsbbutton1.ScrollStyle().NoWantFocus().Show();
	editor2.WhenLeftDown = THISBACK(SwapEditors);
	editor.WhenAction = THISBACK(AddHistory);

	macro_api = MacroEditor();

	mainconfiglist.WhenClick = THISBACK(MainConfig);
	mainconfiglist <<= THISBACK(OnMainConfigList);
	mainconfiglist.NoDropFocus();

	buildmode.WhenClick = THISBACK(SetupOutputMode);
	buildmode.NoWantFocus();
	buildmode.Tip("Output mode");
	buildmode.AddButton().Tip("Build method").Left() <<= THISBACK(DropMethodList);
	buildmode.AddButton().Tip("Build mode") <<= THISBACK(DropModeList);
	methodlist.Normal();
	methodlist.WhenSelect = THISBACK(SelectMethod);
	modelist.Normal();
	modelist.WhenSelect = THISBACK(SelectMode);
	modelist.Add("Debug");
	modelist.Add("Optimal");
	modelist.Add("Speed");
	modelist.Add("Size");

	tabi = 0;
	blocktabs = false;

	package.WhenBar = THISBACK(PackageMenu);
	editor.WhenState = THISBACK(MakeTitle);

	package.NoWantFocus();
	filelist.NoWantFocus();
	filelist.WhenLeftDouble = THISBACK(Group);

	Icon(IdeImg::Package(), IdeImg::Package());

	editorfont = font2 = Courier(13);
	veditorfont = consolefont = font1 = Courier(11);
	editortabsize = 4;
	indent_amount = 4;
	indent_spaces = false;
	show_status_bar = false;
	show_tabs = false;
	tabs_icons = false;
	tabs_crosses = true;
	tabs_grouping = true;
	no_parenthesis_indent = false;

	idestate = EDITING;
	debuglock = 0;

	console.WhenSelect = THISBACK(FindError);
	console.SetSlots(hydra1_threads);

	editor.WhenSelection = THISBACK(Display);
	stoponerrors = true;
	hilite_scope = 1;
	hilite_bracket = 1;
	hilite_ifdef = 1;
	hilite_if_endif = false;
	wrap_console_text = true;
	mute_sounds = false;
	line_numbers = false;

	use_target = true;

	runmode = RUN_WINDOW;
	runexternal = false;

	browser_closeesc = true;

	bookmark_pos = true;

	header_guards = true;
	insert_include = 1;

	SetTimeCallback(-20, THISBACK(Periodic), TIMEID_PERIODIC);

	editor.WhenBreakpoint = THISBACK(OnBreakpoint);

	ConstructFindInFiles();

	ff.style <<= STYLE_NO_REPLACE;

	default_charset = CHARSET_WIN1252;

	HideBottom();
	TheIde(this);

	targetmode = 0;

	browser.WhenShowTopic = THISBACK(ShowTopic);
	doc.WhenTopic = THISBACK(OpenATopic);

	editor.NoCutLine();

	filetabs = true;
	auto_enclose = false;
	mark_lines = true;

	bordercolumn = 96;
	bordercolor = SColorFace();

	state_icon = -1;

	editor.theide = this;

	histi = 0;

	doc_serial = -1;
}

Ide::~Ide()
{
	TheIde(NULL);
}

void DelTemps() {
	FindFile ff(ConfigFile("*.tmp"));
	while(ff) {
		DeleteFile(ConfigFile(ff.GetName()));
		ff.Next();
	}
}

#ifdef PLATFORM_WIN32
#include <mmsystem.h>
#pragma comment( lib, "winmm.lib" )
#endif

void Ide::Paint(Draw&) {}

extern int MemoryProbeFlags;

void Uninstall();

bool SilentMode;

#ifdef PLATFORM_WIN32
#include <Wincon.h>

void Puts(const char *s)
{
	dword dummy;
	if(!SilentMode)
		WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), s, (int)strlen(s), &dummy, NULL);
}
#endif


#ifdef PLATFORM_POSIX
void Puts(const char *s)
{
	if(!SilentMode)
		puts(s);
}
#endif

bool splash_screen;

int CommaSpace(int c)
{
	return c == ',' ? ' ' : c;
}

#ifdef flagMAIN
GUI_APP_MAIN
#else
void AppMain___()
#endif
{
#ifdef PLATFORM_WIN32
	InstallCrashDump();
#endif

	SetLanguage(LNG_ENGLISH);
	SetDefaultCharset(CHARSET_UTF8);

	const Vector<String>& arg = CommandLine();

	bool firstinstall;

#ifdef PLATFORM_POSIX
	String home = Environment().Get("UPP_HOME", Null);
	if(!IsNull(home))
		SetHomeDirectory(home);
	FindFile ff(ConfigFile("*.var"));
	if(!ff)
		if(!Install())
			return;
	firstinstall = !ff;
#endif

#ifdef _DEBUG
//	Ctrl::ShowRepaint(50);
#endif

#ifdef PLATFORM_WIN32
	if(!CheckLicense())
		return;
	firstinstall = !IsNull(LoadFile(GetExeDirFile("install.upp")));
	if(firstinstall)
		if(!Install())
			return;
#endif

	for(int i = 0; i < arg.GetCount(); i++) {
		if(arg[i] == "-uninstall") {
			Uninstall();
			return;
		}
		if(!firstinstall && arg[i] == "-install" && !Install()) return;

	#ifdef PLATFORM_WIN32
		if(arg[i] == "!") {
			String cmdline;
			for(++i; i < arg.GetCount(); i++) {
				if(!IsNull(cmdline))
					cmdline << ' ';
				cmdline << arg[i];
			}
			int n = cmdline.GetLength() + 1;
			Buffer<char> cmd(n);
			memcpy(cmd, cmdline, n);
			SECURITY_ATTRIBUTES sa;
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.lpSecurityDescriptor = NULL;
			sa.bInheritHandle = TRUE;
			PROCESS_INFORMATION pi;
			STARTUPINFO si;
			ZeroMemory(&si, sizeof(STARTUPINFO));
			si.cb = sizeof(STARTUPINFO);
			AllocConsole();
			SetConsoleTitle(cmdline);
			int time = msecs();
			if(CreateProcess(NULL, cmd, &sa, &sa, TRUE,
				             NORMAL_PRIORITY_CLASS,
			                 NULL, NULL, &si, &pi)) {
				WaitForSingleObject(pi.hProcess, INFINITE);
				dword exitcode = 255;
				GetExitCodeProcess(pi.hProcess, &exitcode);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				Puts("<--- Finished in " + GetPrintTime(time) + ", exitcode: " + AsString(exitcode) + " --->");
			}
			else
				Puts("Unable to launch " + cmdline);
			char h[1];
			dword dummy;
			ReadFile(GetStdHandle(STD_INPUT_HANDLE), h, 1, &dummy, NULL);
			return;
		}
	#endif
	}

#ifndef _DEBUG
	SetVppLogSizeLimit(0);
#endif

	try {
		void RegisterLayDes(); RegisterLayDes();
		void RegisterIconDes(); RegisterIconDes();
	//	void RegisterIdeVectorDes(); RegisterIdeVectorDes();
		void RegisterGccBuilder(); RegisterGccBuilder();
		void RegisterMscBuilder(); RegisterMscBuilder();
		void RegisterJavaBuilder(); RegisterJavaBuilder();
		void RegisterScriptBuilder(); RegisterScriptBuilder();

		splash_screen = true;

		Ide ide;
		ide.Maximize();
		bool clset = false;
		if(arg.GetCount() >= 2 && IsAlpha(arg[0][0]) && IsAlpha(arg[1][0])) {
			bool build = arg.GetCount() >= 3 && IsAlpha(arg[2][0]);
		#ifdef PLATFORM_WIN32
			if(build) {
				HMODULE hDLL = LoadLibrary ("kernel32");
				bool attach = false;
				if(hDLL) {
					typedef BOOL (WINAPI *AttachConsoleType)(DWORD dwProcessId);
					AttachConsoleType AttachConsole;
					AttachConsole = (AttachConsoleType) GetProcAddress(hDLL, "AttachConsole");
					if(AttachConsole)
						attach = AttachConsole((DWORD)-1);
				}
				if(!attach)
					AllocConsole();
			}
		#endif
			for(int i = 3; i < arg.GetCount(); i++)
				if(arg[i][0] == '-') {
					String x = arg[i];
					for(int i = 1; i < x.GetCount(); i++)
						if(x[i] == 'l')
							SilentMode = true;
				}
			if(!LoadVars(arg[0])) {
				if(build)
					Puts("TheIDE: Invalid assembly\n");
				else
					Exclamation("Invalid assembly!");
				SetExitCode(2);
				return;
			}
			if(!FileExists(SourcePath(arg[1], GetFileTitle(arg[1]) + ".upp"))) {
				if(build)
					Puts("TheIDE: Package does not exist\n");
				else
					Exclamation("Package does not exist!");
				SetExitCode(2);
				return;
			}
			ide.SetMain(arg[1]);
			clset = true;
			bool stoponerror = false;
			if(build) {
				const Workspace& wspc = ide.IdeWorkspace();
				if(!wspc.GetCount())
					return;
				const Array<Package::Config>& f = wspc.GetPackage(0).config;
				if(f.GetCount())
					ide.mainconfigparam = f[0].param;
				String m = arg[2];
				if(!FileExists(ConfigFile((String)m + ".bm"))) {
					SilentMode = false;
					Puts("TheIDE: Invalid build method\n");
					SetExitCode(3);
					return;
				}
				ide.method <<= m;
				ide.debug.def.blitz = ide.release.def.blitz = 0;
				ide.debug.def.debug = 2;
				ide.release.def.debug = 0;
				ide.debug.package.Clear();
				ide.release.package.Clear();
				ide.debug.linkmode = ide.release.linkmode = 0;
				ide.release.createmap = ide.debug.createmap = false;
				ide.targetmode = 0;
				ide.console.console = true;
				bool clean = false;
				bool makefile = false;
				String mkf;
				for(int i = 3; i < arg.GetCount(); i++)
					if(arg[i][0] == '>')
						ide.mainconfigparam = Filter(~arg[i] + 1, CommaSpace);
					else
					if(arg[i][0] == '-') {
						String x = arg[i];
						for(int i = 1; i < x.GetCount(); i++)
							switch(x[i]) {
							case 'a':
								clean = true;
								break;
							case 'r':
								ide.targetmode = 1;
								break;
							case '1':
								ide.targetmode = 2;
								break;
							case '2':
								ide.targetmode = 3;
								break;
							case 'm':
								ide.release.createmap = ide.debug.createmap = true;
								break;
							case 'b':
								ide.release.def.blitz = ide.debug.def.blitz = 1;
								break;
							case 's':
								ide.debug.linkmode = ide.release.linkmode = 1;
								break;
							case 'S':
								ide.debug.linkmode = ide.release.linkmode = 2;
								break;
							case 'e':
								stoponerror = true;
								break;
							case 'M':
								makefile = true;
								break;
							case 'v':
								ide.console.verbosebuild = true;
								break;
							case 'l':
								break;
							default:
								SilentMode = false;
								Puts("Invalid build option(s)");
								SetExitCode(3);
								return;
							}
					}
					else {
						ide.debug.target_override = ide.release.target_override = true;
						ide.debug.target = ide.release.target = mkf = arg[i];
					}
				if(clean)
					ide.Clean();
				if(makefile) {
					ide.SaveMakeFile(IsNull(mkf) ? "Makefile" : mkf);
					SetExitCode(0);
				}
				else
				if(ide.Build())
					SetExitCode(0);
				else {
					if(stoponerror)
						PromptOK("TheIDE: Error while building " + DeQtf(arg[1]) + "!");
					SetExitCode(1);
				}
				return;
			}
		}

		LoadFromFile(ide);

		ide.SyncCh();

		DelTemps();
		if(splash_screen) {
			ShowSplash();
			Ctrl::ProcessEvents();
		}

		ide.editor_bottom.Zoom(0);
		ide.right_split.Zoom(0);
		ide.UpdateFormat();
		RestoreKeys(LoadFile(ConfigFile("ide.key")));
		ide.editor.LoadHlStyles(LoadFile(ConfigFile("ide.colors")));
		ActivateUsrLog();
		if(clset || ide.OpenMainPackage()) {
			StoreToFile(ide);
			SyncRefs();
			ide.FileSelected();
			ide.Run();
		}
		StoreToFile(ide);
		SaveBrowserBase();
		SaveFile(ConfigFile("ide.key"), StoreKeys());
		SaveFile(ConfigFile("ide.colors"), ide.editor.StoreHlStyles());
		DelTemps();
	}
	catch(const CParser::Error& e) {
		Exclamation("Parser error " + e);
		LOG("!!!!! Parser error " + e);
	}
	catch(const Exc& e) {
		Exclamation("Exception " + e);
		LOG("!!!!! Exception " << e);
	}
#ifdef PLATFORM_POSIX
	catch(...) {
		Exclamation("Unknown exception !");
		LOG("!!!!! Uknown exception");
	}
#endif
}
