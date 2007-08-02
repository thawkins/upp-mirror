#include "Draw.h"

NAMESPACE_UPP

Image CreateImage(Size sz, Color color)
{
	ImageBuffer ib(sz);
	RGBA rgba = color;
	memsetex(ib, &rgba, sizeof(RGBA), ib.GetLength());
	return ib;
}

Size DstSrc(ImageBuffer& dest, Point& p, const Image& src, Rect& sr)
{
	if(p.x < 0) {
		sr.left += -p.x;
		p.x = 0;
	}
	if(p.y < 0) {
		sr.top += -p.y;
		p.y = 0;
	}
	sr = sr & src.GetSize();
	Size sz = dest.GetSize() - p;
	sz.cx = min(sz.cx, sr.GetWidth());
	sz.cy = min(sz.cy, sr.GetHeight());
	return sz;
}

void Copy(ImageBuffer& dest, Point p, const Image& src, const Rect& srect)
{
	Rect sr = srect;
	Size sz = DstSrc(dest, p, src, sr);
	if(sz.cx > 0)
		while(--sz.cy >= 0)
			memcpy(dest[p.y++] + p.x, src[sr.top++] + sr.left, sz.cx * sizeof(RGBA));
}

void Over(ImageBuffer& dest, Point p, const Image& src, const Rect& srect)
{
	Rect sr = srect;
	Size sz = DstSrc(dest, p, src, sr);
	if(sz.cx > 0)
		while(--sz.cy >= 0)
			AlphaBlend(dest[p.y++] + p.x, src[sr.top++] + sr.left, sz.cx);
}

void  Copy(Image& dest, Point p, const Image& _src, const Rect& srect)
{
	Image src = _src;
	ImageBuffer b(dest);
	Copy(b, p, src, srect);
	dest = b;
}

void  Over(Image& dest, Point p, const Image& _src, const Rect& srect)
{
	Image src = _src;
	ImageBuffer b(dest);
	Over(b, p, src, srect);
	dest = b;
}

void Crop(RasterEncoder& tgt, Raster& img, const Rect& rc)
{
	Rect r = rc & img.GetSize();
	tgt.Create(r.Size(), img);
	for(int y = r.top; y < r.bottom; y++)
		tgt.WriteLine(~img[y] + r.left);
}

Image Crop(const Image& img, const Rect& rc)
{
	if(rc.left == 0 && rc.top == 0 && rc.Size() == img.GetSize())
		return img;
	if((rc & img.GetSize()).IsEmpty())
		return Image();
	ImageRaster  src(img);
	ImageEncoder tgt;
	Crop(tgt, src, rc);
	return tgt;
}

Image Crop(const Image& img, int x, int y, int cx, int cy)
{
	return Crop(img, RectC(x, y, cx, cy));
}

Image ColorMask(const Image& src, Color key)
{
	ImageBuffer ib(src.GetSize());
	const RGBA *s = src;
	const RGBA *e = src + src.GetLength();
	RGBA *t = ~ib;
	byte kr = key.GetR();
	byte kg = key.GetG();
	byte kb = key.GetB();
	while(s < e) {
		if(s->r == kr && s->g == kg && s->b == kb)
			*t++ = RGBAZero();
		else
			*t++ = *s;
		s++;
	}
	return ib;
}

void CanvasSize(RasterEncoder& tgt, Raster& img, int cx, int cy)
{
	tgt.Create(cx, cy, img);
	int ccx = min(img.GetWidth(), cx);
	int ccy = min(img.GetHeight(), cy);
	for(int y = 0; y < ccy; y++) {
		memcpy(~tgt, img[y], ccx * sizeof(RGBA));
		memset(~tgt + ccx, 0, (cx - ccx) * sizeof(RGBA));
		tgt.WriteLine();
	}
	for(int y = cy - ccy; --y >= 0;) {
		memset(~tgt, 0, cx * sizeof(RGBA));
		tgt.WriteLine();
	}
}

Image CanvasSize(const Image& img, int cx, int cy)
{
	ImageRaster  src(img);
	ImageEncoder tgt;
	CanvasSize(tgt, src, cx, cy);
	return tgt;
}

Image AssignAlpha(const Image& img, const Image& alpha)
{
	Size sz = Size(min(img.GetWidth(), alpha.GetWidth()),
	               min(img.GetHeight(), alpha.GetHeight()));
	if(sz.cx == 0 || sz.cy == 0)
		return Image();
	ImageBuffer ib(sz);
	for(int y = 0; y < sz.cy; y++) {
		const RGBA *s = img[y];
		const RGBA *e = s + sz.cx;
		const RGBA *a = alpha[y];
		RGBA *t = ib[y];
		while(s < e) {
			*t = *s++;
			(t++)->a = (a++)->a;
		}
	}
	return ib;
}

int   EqualightCh(int c, int l, int h)
{
	return Saturate255((c - l) * 255 / (h - l) + l);
}

Image Equalight(const Image& img, int thold)
{
	int histogram[256];
	ZeroArray(histogram);
	const RGBA *s = ~img;
	const RGBA *e = s + img.GetLength();
	while(s < e) {
		histogram[Grayscale(*s)]++;
		s++;
	}
	int n = (thold * img.GetLength()) >> 8;
	int h = 255;
	int l = 0;
	while(l < h) {
		if(n < 0)
			break;
		n -= histogram[l++];
		if(n < 0)
			break;
		n -= histogram[h--];
	}
	if(l >= h)
		return img;
	ImageBuffer w(img.GetSize());
	RGBA *t = w;
	s = ~img;
	while(s < e) {
		t->r = EqualightCh(s->r, l, h);
		t->g = EqualightCh(s->g, l, h);
		t->b = EqualightCh(s->b, l, h);
		t->a = s->a;
		s++;
		t++;
	}
	return w;
}

Image Grayscale(const Image& img)
{
	const RGBA *s = ~img;
	const RGBA *e = s + img.GetLength();
	ImageBuffer w(img.GetSize());
	RGBA *t = w;
	while(s < e) {
		int q = Grayscale(*s);
		t->r = q;
		t->g = q;
		t->b = q;
		t->a = s->a;
		t++;
		s++;
	}
	return w;
}

Image Grayscale(const Image& img, int amount)
{
	const RGBA *s = ~img;
	const RGBA *e = s + img.GetLength();
	ImageBuffer w(img.GetSize());
	RGBA *t = w;
	int na = 256 - amount;
	while(s < e) {
		int q = Grayscale(*s);
		t->r = Saturate255((amount * q + na * s->r) >> 8);
		t->g = Saturate255((amount * q + na * s->g) >> 8);
		t->b = Saturate255((amount * q + na * s->b) >> 8);
		t->a = s->a;
		t++;
		s++;
	}
	return w;
}

Image Colorize(const Image& img, Color color, int alpha)
{
	const RGBA *s = ~img;
	const RGBA *e = s + img.GetLength();
	ImageBuffer w(img.GetSize());
	RGBA *t = w;
	byte r = color.GetR();
	byte g = color.GetG();
	byte b = color.GetB();
	alpha = alpha + (alpha >> 7);
	while(s < e) {
		int ga = Grayscale(*s);
		ga = ga + (ga >> 7);
		t->r = (alpha * (((ga * r) >> 8) - s->r) >> 8) + s->r;
		t->g = (alpha * (((ga * g) >> 8) - s->g) >> 8) + s->g;
		t->b = (alpha * (((ga * b) >> 8) - s->b) >> 8) + s->b;
		t->a = s->a;
		t++;
		s++;
	}
	return w;
}

inline
byte ContrastCh(int amount, int ch)
{
	return Saturate255(128 + (amount * (ch - 128) >> 8));
}

Image Contrast(const Image& img, int amount)
{
	const RGBA *s = ~img;
	const RGBA *e = s + img.GetLength();
	ImageBuffer w(img.GetSize());
	RGBA *t = w;
	while(s < e) {
		t->r = ContrastCh(amount, s->r);
		t->g = ContrastCh(amount, s->g);
		t->b = ContrastCh(amount, s->b);
		t->a = s->a;
		t++;
		s++;
	}
	return w;
}

void sLine(RGBA *t, int cx, const RasterLine l[3], ImageFilter9& filter)
{
	RGBA h[3][3];
	const RGBA *x[3];
	x[0] = h[0];
	x[1] = h[1];
	x[2] = h[2];
	if(cx == 1) {
		h[0][0] = l[0][0]; h[0][1] = l[0][0]; h[0][2] = l[0][0];
		h[1][0] = l[1][0]; h[1][1] = l[1][0]; h[1][2] = l[1][0];
		h[2][0] = l[2][0]; h[2][1] = l[2][0]; h[2][2] = l[2][0];
		*t = filter(x);
		return;
	}
	h[0][0] = l[0][0]; h[0][1] = l[0][0]; h[0][2] = l[0][1];
	h[1][0] = l[1][0]; h[1][1] = l[1][0]; h[1][2] = l[1][1];
	h[2][0] = l[2][0]; h[2][1] = l[2][0]; h[2][2] = l[2][1];
	*t++ = filter(x);
	for(int i = 1; i < cx - 1; i++) {
		x[0] = ~l[0] + i - 1;
		x[1] = ~l[1] + i - 1;
		x[2] = ~l[2] + i - 1;
		*t++ = filter(x);
	}
	h[0][0] = l[0][cx - 2]; h[0][1] = l[0][cx - 1]; h[0][2] = l[0][cx - 1];
	h[1][0] = l[1][cx - 2]; h[1][1] = l[1][cx - 1]; h[1][2] = l[1][cx - 1];
	h[2][0] = l[2][cx - 2]; h[2][1] = l[2][cx - 1]; h[2][2] = l[2][cx - 1];
	x[0] = h[0];
	x[1] = h[1];
	x[2] = h[2];
	*t++ = filter(x);
}

void  Filter(RasterEncoder& target, Raster& src, ImageFilter9& filter)
{
	Size sz = src.GetSize();
	target.Create(sz, src);
	if(sz.cy < 1)
		return;
	RasterLine l[3];
	if(sz.cy == 1) {
		l[0] = src[0];
		l[1] = src[0];
		l[2] = src[0];
		sLine(target, sz.cx, l, filter);
		return;
	}
	l[0] = src[0];
	l[1] = src[0];
	l[2] = src[1];
	sLine(target, sz.cx, l, filter);
	target.WriteLine();
	for(int y = 1; y < sz.cy - 1; y++) {
		l[0] = l[1];
		l[1] = l[2];
		l[2] = src[y + 1];
		sLine(target, sz.cx, l, filter);
		target.WriteLine();
	}
	l[0] = l[1];
	l[1] = l[2];
	l[2] = src[sz.cy - 1];
	sLine(target, sz.cx, l, filter);
	target.WriteLine();
}

Image Filter(const Image& img, ImageFilter9& filter)
{
	ImageEncoder tgt;
	ImageRaster src(img);
	Filter(tgt, src, filter);
	return tgt;
}

struct RGBAI {
	int r, g, b, a;

	RGBAI() { r = g = b = a= 0; }
};

static void sGetS(RGBA q, RGBAI& p, int mul)
{
	p.r += mul * q.r;
	p.g += mul * q.g;
	p.b += mul * q.b;
	p.a += mul * q.a;
}

struct sSharpenFilter : ImageFilter9 {
	int amount;

	virtual RGBA operator()(const RGBA **mx);
};

RGBA sSharpenFilter::operator()(const RGBA **mx)
{
	RGBAI q;
	sGetS(mx[0][0], q, 7);
	sGetS(mx[0][1], q, 9);
	sGetS(mx[0][2], q, 7);
	sGetS(mx[1][0], q, 9);
	sGetS(mx[1][2], q, 9);
	sGetS(mx[2][0], q, 7);
	sGetS(mx[2][1], q, 9);
	sGetS(mx[2][2], q, 7);
	const RGBA& s = mx[1][1];
	RGBA t;
	int na = 256 + amount;
	t.b = Saturate255((na * (s.b << 6) - amount * q.b) >> 14);
	t.g = Saturate255((na * (s.g << 6) - amount * q.g) >> 14);
	t.r = Saturate255((na * (s.r << 6) - amount * q.r) >> 14);
	t.a = Saturate255((na * (s.a << 6) - amount * q.a) >> 14);
	return t;
}

void Sharpen(RasterEncoder& target, Raster& src, int amount)
{
	Size sz = src.GetSize();
	target.Create(sz, src);
	sSharpenFilter f;
	f.amount = amount;
	Filter(target, src, f);
}

Image Sharpen(const Image& img, int amount)
{
	ImageEncoder tgt;
	ImageRaster src(img);
	Sharpen(tgt, src, amount);
	return tgt;
}

struct sEtchFilter : ImageFilter9 {
	virtual RGBA operator()(const RGBA **mx);
};

RGBA sEtchFilter::operator()(const RGBA **mx)
{
	RGBA t;
	RGBA s = mx[1][1];
	if(s.a == 255 && s.r + s.g + s.b < 400) {
		t.r = t.g = t.b = 128;
		t.a = 255;
		return t;
	}
	s = mx[0][0];
	if(s.a == 255 && s.r + s.g + s.b < 400) {
		t.r = t.g = t.b = t.a = 255;
		return t;
	}
	Zero(t);
	return t;
}

Image Etched(const Image& img)
{
	sEtchFilter ef;
	return Filter(img, ef);
}

Image SetColorKeepAlpha(const Image& img, Color c)
{
	RGBA rgba = c;
	const RGBA *s = ~img;
	const RGBA *e = s + img.GetLength();
	ImageBuffer w(img.GetSize());
	RGBA *t = w;
	while(s < e) {
		*t = rgba;
		(t++)->a = (s++)->a;
	}
	w.SetHotSpot(img.GetHotSpot());
	w.Set2ndSpot(img.Get2ndSpot());
	return w;
}

Image CreateHorzFadeOut(Size sz, Color color)
{
	ImageBuffer ib(sz);
	RGBA c = color;
	for(int q = 0; q < sz.cx; q++) {
		c.a = q * 255 / sz.cx;
		RGBA *t = ~ib + q;
		for(int n = sz.cy; n > 0; n--) {
			*t = c;
			t += sz.cx;
		}
	}
	Premultiply(ib);
	return ib;
}

struct FadeOutMaker : ImageMaker {
	Size  sz;
	Color color;

	virtual String Key() const {
		char h[sizeof(Size) + sizeof(Color)];
		memcpy(h, &sz, sizeof(sz));
		memcpy(h + sizeof(Size), &color, sizeof(Color));
		return String(h, sizeof(h));
	}

	virtual Image  Make() const {
		return CreateHorzFadeOut(sz, color);
	}
};

Image  HorzFadeOut(Size sz, Color color)
{
	FadeOutMaker m;
	m.sz = sz;
	m.color = color;
	return MakeImage(m);
}

Image  RotateClockwise(const Image& img)
{
	Size sz = img.GetSize();
	ImageBuffer ib(sz.cy, sz.cx);
	for(int x = 0; x < sz.cx; x++)
		for(int y = 0; y < sz.cy; y++)
			ib[x][y] = img[sz.cy - y - 1][x];
	return ib;
}

Image  RotateAntiClockwise(const Image& img)
{
	Size sz = img.GetSize();
	ImageBuffer ib(sz.cy, sz.cx);
	for(int x = 0; x < sz.cx; x++)
		for(int y = 0; y < sz.cy; y++)
			ib[x][y] = img[y][sz.cx - x - 1];
	return ib;
}

Image Magnify(const Image& img, int nx, int ny)
{
	if(nx == 1 && ny == 1)
		return img;
	if(nx == 0 || ny == 0)
		return Image();
	Size sz = img.GetSize();
	bool xdown = nx < 0;
	nx = abs(nx);
	int ncx = xdown ? sz.cx / nx : sz.cx * nx;
	ImageBuffer b(ncx, sz.cy * ny);
	const RGBA *s = ~img;
	const RGBA *e = s + img.GetLength();
	RGBA *t = ~b;
	while(s < e) {
		RGBA *q = t;
		const RGBA *le = s + sz.cx;
		while(s < le) {
			Fill(q, *s, nx);
			q += nx;
			s++;
		}
		for(int n = ny - 1; n--;) {
			memcpy(q, t, ncx * sizeof(RGBA));
			q += ncx;
		}
		t = q;
	}
	return b;
}

END_UPP_NAMESPACE
