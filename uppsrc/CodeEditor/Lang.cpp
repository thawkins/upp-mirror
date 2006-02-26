#include "CodeEditor.h"

Array<IdentPos> GetLineIdent(const char *line)
{
	Array<IdentPos> out;
	const char *p = line;
	while(*p && *p != '\n')
		if(*p == '\"' || *p == '\'')
		{
			char term = *p++;
			while(*p && *p != term)
				if(*p++ == '\\' && *p)
					p++;
			if(*p == term)
				p++;
		}
		else if(*p == 's' && p[1] == '_' && p[2] == '(' && (IsAlpha(p[3]) || p[3] == '_'))
		{
			IdentPos& pos = out.Add();
			pos.begin = p - line;
			const char *b = (p += 3);
			while(IsAlNum(*++p) || *p == '_')
				;
			pos.ident = String(b, p);
			if(*p == ')')
				p++;
			pos.end = p - line;
		}
		else if(IsAlpha(*p) || *p == '_')
		{
			while(IsAlNum(*++p) || *p == '_')
				;
		}
		else
			p++;
	return out;
}

Vector<Point> GetLineString(const wchar *wline, bool& is_begin, bool& is_end)
{
	Vector<Point> out;
	const wchar *p = wline;
	while(*p <= ' ' && *p && *p != '\n')
		p++;
	is_begin = (*p == '\"');
	is_end = false;
	while(*p && *p != '\n')
	{
		wchar term = *p;
		if(term == '\"' || term == '\'')
		{
			int begin = p++ - wline;
			while(*p && *p != '\n')
				if(*p == term)
				{
					const wchar *lim = ++p;
					while((byte)*p <= ' ' && *p && *p != '\n')
						p++;
					if(*p != term)
					{
						is_end = (*p == '\n' || *p == 0);
						p = lim;
						break;
					}
					p++;
				}
				else if(*p++ == '\\' && *p && *p != '\n')
					p++;
			if(term == '\"')
				out.Add(Point(begin, p - wline));
		}
		else
			p++;
	}
	return out;
}

bool CodeEditor::GetStringRange(int cursor, int& b, int &e) const
{
	int cl = GetLine(cursor);
	cursor -= GetPos(cl);
	bool is_begin, is_end; //@@@@@@
	Vector<Point> list = GetLineString(GetWLine(cl), is_begin, is_end);
	int i = list.GetCount();
	while(--i >= 0 && (list[i].x > cursor || list[i].y < cursor))
		;
	if(i < 0)
		return false;
	int bl = cl, bp = list[i].x;
	int el = cl, ep = list[i].y;
	while(is_begin && bl > 0)
	{
		list = GetLineString(GetWLine(bl - 1), is_begin, is_end);
		if(list.IsEmpty() || !is_end)
			break;
		bl--;
		bp = list.Top().x;
	}
	while(el + 1 < GetLineCount() && ep >= GetLineLength(el))
	{
		list = GetLineString(GetWLine(el + 1), is_begin, is_end);
		if(list.IsEmpty() || !is_begin)
			break;
		el++;
		ep = list[0].y;
	}
	b = GetPos(bl, bp);
	e = GetPos(el, ep);
	return b < e;
}

bool CodeEditor::FindString(bool back)
{
	int l, h;
	if(!GetSelection(l, h)) h = GetCursor();
	else h = (back ? l : h);
	h -= GetPos(l = GetLine(h));
	while(l >= 0 && l < GetLineCount())
	{
		bool is_begin, is_end;
		Vector<Point> list = GetLineString(GetWLine(l), is_begin, is_end);
		if(back)
		{
			int i = list.GetCount();
			while(--i >= 0 && list[i].x >= h)
				;
			if(i >= 0)
			{
				h = list[i].x;
				break;
			}
			h = 1000000;
			--l;
		}
		else
		{
			int i = 0;
			while(i < list.GetCount() && list[i].y <= h)
				i++;
			if(i < list.GetCount())
			{
				h = list[i].x;
				break;
			}
			h = 0;
			++l;
		}
	}
	int b, e;
	if(l < 0 || l >= GetLineCount() || !GetStringRange(GetPos(l, h), b, e))
		return false;
	SetSelection(b, e);
	return true;
}

bool CodeEditor::FindLangString(bool back)
{
	int l, h;
	if(!GetSelection(l, h)) h = GetCursor();
	else h = (back ? l : h);
	h -= GetPos(l = GetLine(h));
	for(;;)
	{
		Array<IdentPos> list = GetLineIdent(GetUtf8Line(l));
		int b, e;
		if(back)
		{
			int i = list.GetCount();
			while(--i >= 0 && list[i].begin >= h)
				;
			if(i < 0)
			{
				h = 1000000;
				if(--l < 0)
					break;
				continue;
			}
			b = GetPos(l, list[i].begin);
			e = GetPos(l, list[i].end);
		}
		else
		{
			int i = 0;
			while(i < list.GetCount() && list[i].end <= h)
				i++;
			if(i >= list.GetCount())
			{
				h = 0;
				if(++l >= GetLineCount())
					break;
				continue;
			}
			b = GetPos(l, list[i].begin);
			e = GetPos(l, list[i].end);
		}
		SetSelection(b, e);
		return true;
	}
	return false;
}
