#include "CppBase.h"

#ifdef _MSC_VER
#pragma inline_depth(255)
#pragma optimize("t", on)
#endif

static Index<String> cpp_file;

int GetCppFileIndex(const String& path)
{
	return cpp_file.FindAdd(path);
}

const String& GetCppFile(int i)
{
	return cpp_file[i];
}
/*
void  CppPos::Serialize(Stream& s)
{
	s % impl % line;
	String fn = GetCppFile(file);
	s % fn;
	file = GetCppFileIndex(fn);
}
*/

String SSpaces(const char *txt)
{
	String r;
	while(*txt)
		if(*txt == ' ') {
			while((byte)*txt <= ' ' && *txt) txt++;
			r.Cat(' ');
		}
		else
			r.Cat(*txt++);
	return r;
}

void SLPos(SrcFile& res)
{
	res.linepos.Add(res.text.GetLength());
}

SrcFile PreProcess(Stream& in)
{
	SrcFile res;
	bool include = true;
	while(!in.IsEof()) {
		String ln = in.GetLine();
		SLPos(res);
		while(*ln.Last() == '\\') {
			ln.Trim(ln.GetLength() - 1);
			ln.Cat(in.GetLine());
			SLPos(res);
		}
		const char *rm = ln;
		while(*rm == ' ' || *rm == '\t') rm++;
		if(*rm == '#') {
			if(rm[1] == 'd' && rm[2] == 'e' && rm[3] == 'f' &&
			   rm[4] == 'i' && rm[5] == 'n' && rm[6] == 'e' && !iscid(rm[7])) {
				const char *s = rm + 8;
				while(*s == ' ') s++;
				String macro;
				while(iscid(*s))
					macro.Cat(*s++);
				if(*s == '(') {
					while(*s != ')' && *s)
						macro.Cat(*s++);
					macro << ')';
				}
				res.text << '#' << AsCString(SSpaces(macro));
			}
		}
		else {
			String cmd;
			while(*rm) {
				if(*rm == '\"') {
					res.text << '\"';
					rm++;
					while((byte)*rm && *rm != '\r' && *rm != '\n') {
						if(*rm == '\"') {
							res.text << '\"';
							rm++;
							break;
						}
						if(*rm == '\\' && rm[1]) {
							if(include)
								res.text.Cat(*rm);
							rm++;
						}
						if(include)
							res.text.Cat(*rm);
						rm++;
					}
				}
				else
				if(*rm == '\\' && rm[1]) {
					if(include) {
						res.text.Cat(*rm++);
						res.text.Cat(*rm++);
					}
					else
						rm += 2;
				}
				else
				if(rm[0] == '/' && rm[1] == '/') {
					cmd = rm + 2;
					break;
				}
				else
				if(rm[0] == '/' && rm[1] == '*') {
					rm += 2;
					for(;;) {
						if(*rm == '\0') {
							if(in.IsEof()) break;
							SLPos(res);
							ln = in.GetLine();
							rm = ~ln;
						}
						if(rm[0] == '*' && rm[1] == '/') {
							rm += 2;
							break;
						}
						rm++;
					}
					if(include)
						res.text.Cat(' ');
				}
				else {
					if(include)
						res.text.Cat(*rm);
					rm++;
				}
			}
			if(include)
				res.text << ' ';
			if(cmd[0] == '$') {
				if(cmd[1] == '-') include = false;
				if(cmd[1] == '+') include = true;
				if(cmd[1]) {
					res.text.Cat(~cmd + 2);
					res.text.Cat(' ');
				}
			}
		}
	}
	return res;
}
