#include <Core/SMTP/SMTP.h>

using namespace Upp;

String infolog;
String errors;

String input;

Vector<String> exclude;

void Do(const char *nest, const char *bm, bool release, bool test)
{
	String flags = release ? "r" : "b";
	String mn = release ? "R" : "D";
	String n = String().Cat() << nest << '-' << bm << '-' << mn;
	Cout() << n << '\n';
	infolog << "========== " << nest << " " << bm << (release ? "-R" : "") << " =============\n";
	FindFile ff(AppendFileName(AppendFileName(input, nest), "*.*"));
	bool first = true;
	while(ff) {
		String name = ff.GetName();
		String upp = AppendFileName(ff.GetPath(), name + ".upp");
	    String h = String(nest) + "/" + name;
		if(ff.IsFolder() && !ff.IsHidden() && FindIndex(exclude, h) < 0) {
		    String txt;
		    txt << bm;
		    if(release)
		        txt << "-R ";
		    txt << ' ' << h << ' ';
			String c;
			c << "umk " << nest << ' ' << name << ' ' << bm << " -" << flags;
			if(first)
				c << 'a';
		#ifdef PLATFORM_POSIX
			c << 's';
		#endif
			String exe = GetHomeDirFile("autotest.tst");
			c << ' ' << exe;
			Cout() << c << '\n';
			infolog << txt;
			String out;
			if(Sys(c, out)) {
				Cout() << " *** FAILED TO COMPILE\n";
				infolog << ": FAILED TO COMPILE\n";
				errors << txt << ": FAILED TO COMPILE\n";
			}
			else {
				infolog << ": BUILD OK";
				if(test) {
					LocalProcess p;
					if(!p.Start(exe)) {
						Cout() << "FAILED TO RUN\n";
						infolog << ", FAILED TO RUN";
						errors << txt << ": FAILED TO RUN\n";
					}
					else {
						Cout() << "RUN\n";
					#ifdef _DEBUG
						int timeout = 10000;
					#else
						int timeout = 60000;
					#endif
						String h = LoadFile(upp);
						int q = h.Find("#WAIT:");
						if(q >= 0) {
							timeout = max(60 * 1000 * atoi(~h + q + strlen("#WAIT:")), timeout);
						}
						int msecs0 = msecs();
						for(;;) {
							if(p.IsRunning()) {
								if(msecs(msecs0) > timeout) {
									infolog << ", TIMEOUT";
									errors << txt << ": TIMEOUT\n";
									Cout() << "*** TIMEOUT\n";
									break;
								}
							}
							else {
								if(p.GetExitCode()) {
									infolog << ", FAILED";
									errors << txt << ": FAILED\n";
									Cout() << "*** FAILED\n";
								}
								else
									infolog << ", OK";
									Cout() << "OK\n";
								break;
							}
							Sleep(10);
						}
					}
				}
				infolog << "\n";
			}
			first = false;
			DeleteFile(exe);
		}
		ff.Next();
	}
}

CONSOLE_APP_MAIN
{
	Value ini = ParseJSON(LoadFile(GetHomeDirFile("autotest.ini")));

	input = ini["upp_sources"];
	Vector<String> test = Split((String)ini["auto_test"], ';');
	Vector<String> build = Split((String)ini["build_test"], ';');
	Vector<String> bm = Split((String)ini["build_method"], ';');
	exclude = Split((String)ini["exclude"], ';');
	
	infolog << "Started " << GetSysTime() << "\n";
	
	bm.Add("GCC");
	bm.Add("GCC11");
	
	for(int i = 0; i < bm.GetCount(); i++) {
		for(int j = 0; j < test.GetCount(); j++) {
			Do(test[j], bm[i], false, true);
			Do(test[j], bm[i], true, false);
		}
		for(int j = 0; j < build.GetCount(); j++) {
			Do(build[j], bm[i], false, false);
			Do(build[j], bm[i], true, false);
		}
	}
	

	infolog << "Finished " << GetSysTime() << "\n";

	String body;
	if(errors.GetCount()) {
		body << "FAILED TESTS:\n" << errors << "\n---------------------------\n\n";
		SetExitCode(1);
	}
	body << "TEST LOG:\n" << infolog;

	Smtp smtp;
	smtp.Trace();
	smtp.Host(ini["smtp_server"])
	    .Port(Nvl((int)ini["smtp_port"], 465))
	    .SSL()
		.Auth(ini["smtp_user"], ini["smtp_password"])
	    .Subject(String().Cat() << "U++ autotest: " << (errors.GetCount() ? "** FAILED **" : "OK"))
	    .Body(body)
	;

	Vector<String> s = Split((String)ini["emails"], ';');
	for(int i = 0; i < s.GetCount(); i++)
		smtp.To(s[i]);
	smtp.Send();
}
