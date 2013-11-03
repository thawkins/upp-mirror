#include <CtrlCore/CtrlCore.h>

#ifdef GUI_GTK

#include <cairo/cairo-ft.h>

#include <plugin/FT_fontsys/ftoutln.h>

NAMESPACE_UPP

#define LLOG(x)

FcPattern *CreateFcPattern(Font font);
FT_Face    FTFace(Font fnt, String *rpath = NULL);

struct FontSysData {
	cairo_scaled_font_t *scaled_font;
	
	void Init(Font font, int angle);
	~FontSysData() { cairo_scaled_font_destroy(scaled_font); }
};

void FontSysData::Init(Font font, int angle)
{
	LLOG("FontSysData::Init " << font << ", " << angle);
	FcPattern *p = CreateFcPattern(font);
	cairo_font_face_t *font_face = cairo_ft_font_face_create_for_pattern(p);
	FcPatternDestroy(p);
	
	cairo_matrix_t font_matrix[1], ctm[1];
	cairo_matrix_init_identity(ctm);
	cairo_matrix_init_scale(font_matrix, font.GetHeight(), font.GetHeight());

	FT_Face ft = FTFace(font);
	if(font.IsItalic() && !(FTFace(font)->style_flags & FT_STYLE_FLAG_ITALIC)) {
		cairo_matrix_t sheer[1];	
		cairo_matrix_init_identity(sheer);
		sheer->xy = -0.2;
		cairo_matrix_multiply(font_matrix, font_matrix, sheer);
	}
	
	if(angle)
		cairo_matrix_rotate(font_matrix, -angle * M_2PI / 3600);
	cairo_font_options_t *opt = cairo_font_options_create();
	scaled_font = cairo_scaled_font_create(font_face, font_matrix, ctm, opt);

#if 0
	int synth = 0;
	FT_Face ft = cairo_ft_scaled_font_lock_face(scaled_font);
	if(ft) {
//		if(font.IsBold() && !(ft->style_flags & FT_STYLE_FLAG_BOLD))
//			synth |= CAIRO_FT_SYNTHESIZE_BOLD;    
		if() {
			FT_Matrix    transform;
			transform.xx = 0x10000L;
			transform.yx = 0x00000L;
			
			transform.xy = 0x03000L;
			transform.yy = 0x10000L;
			
			FT_Outline_Transform( outline, &transform );
		}
		//	synth |= CAIRO_FT_SYNTHESIZE_OBLIQUE;
	 	cairo_ft_scaled_font_unlock_face(scaled_font);
	}
//	if(synth) {
//		cairo_ft_font_face_set_synthesize(font_face, synth);
 //		cairo_scaled_font_destroy(scaled_font);
//		scaled_font = cairo_scaled_font_create(font_face, font_matrix, ctm, opt);
//	}
#endif
	cairo_font_options_destroy(opt);
 	cairo_font_face_destroy(font_face);
}

struct FontDataSysMaker : LRUCache<FontSysData, Tuple2<Font, int> >::Maker {
	Font font;
	int  angle;

	virtual Tuple2<Font, int>  Key() const        { return MakeTuple(font, angle); }
	virtual int Make(FontSysData& object) const   { object.Init(font, angle); return 1; }
};

int    gtk_antialias = -1;
int    gtk_hinting = -1;
String gtk_hintstyle;
String gtk_rgba;

void SystemDraw::DrawTextOp(int x, int y, int angle, const wchar *text, Font font, Color ink, int n, const int *dx)
{
	GuiLock __;
	
	int ascent = font.GetAscent();
	double sina = 0;
	double cosa = 1;
	if(angle)
		Draw::SinCos(angle, sina, cosa);
	int xpos = 0;	
	Buffer<cairo_glyph_t> gs(n);
	for(int i = 0; i < n; i++) {
		cairo_glyph_t& g = gs[i];
		g.index = GetGlyphInfo(font, text[i]).glyphi;
		g.x = fround(x + cosa * xpos + sina * ascent);
		g.y = fround(y + cosa * ascent - sina * xpos);
		xpos += dx ? dx[i] : font[text[i]];
	}

	static LRUCache<FontSysData, Tuple2<Font, int> > cache;
	FontDataSysMaker m;
	m.font = font;
	m.angle = angle;
	FontSysData& sf = cache.Get(m);
	
	cairo_set_scaled_font(cr, sf.scaled_font);

	SetColor(ink);
 	cairo_show_glyphs(cr, gs, n);
 	
 	cache.Shrink(64);
}

END_UPP_NAMESPACE

#endif
