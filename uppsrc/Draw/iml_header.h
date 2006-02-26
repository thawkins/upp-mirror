#pragma BLITZ_APPROVE

//$-

#ifdef IMAGESTATIC
	#error Do not include iml_header.h in IMAGESTATIC mode (just iml_source)
#endif

#define IMAGE_META(k, v)
#define IMAGE_BEGIN(n)
#define IMAGE_SCAN(s)

#ifdef IMAGEOBJECT
	#define IMAGE_END(n, x, y) IMAGE_STORAGE Image n;
	#define IMAGE_PACKED(n, d) IMAGE_STORAGE Image n;
#else
	#define IMAGE_END(n, x, y) IMAGE_STORAGE const Image& n();
	#define IMAGE_PACKED(n, d) IMAGE_STORAGE const Image& n();
#endif//IMAGEOBJECT

#if defined(IMAGECLASS)
	#define IMAGE_STORAGE      static
	class IMAGECLASS
	{
	public:
#elif defined(IMAGEOBJECT)
	#define IMAGE_STORAGE
	class COMBINE(IMAGEOBJECT, _cls);
	extern const COMBINE(IMAGEOBJECT, _cls)& IMAGEOBJECT();
	class COMBINE(IMAGEOBJECT, _cls)
	{
	public:
#elif defined(IMAGESPACE)
	#define IMAGE_STORAGE      extern
	namespace IMAGESPACE
	{
#else
	#define IMAGE_STORAGE      extern
#endif//CLASS/SPACE

#include IMAGEFILE

#ifdef IMAGEVECTOR
	#undef IMAGE_BEGIN
	#undef IMAGE_SCAN
	#undef IMAGE_END
	#undef IMAGE_PACKED
	#define IMAGE_BEGIN(n) I_##n,
	#define IMAGE_SCAN(s)
	#define IMAGE_END(n, x, y)
	#define IMAGE_PACKED(n, d)
	#define IMAGE_COUNT(n) IMAGE_COUNT_(n)
	#define IMAGE_COUNT_(n) n##_COUNT

	enum
	{
	#include IMAGEFILE
		IMAGE_COUNT(IMAGEVECTOR)
	};

	IMAGE_STORAGE Image IMAGEVECTOR[];

	#undef IMAGE_COUNT
	#undef IMAGE_COUNT_
#endif//IMAGEVECTOR

#if defined(IMAGECLASS) || defined(IMAGEOBJECT)
	};
#endif//IMAGECLASS/OBJECT

#ifdef IMAGESPACE
	} // end of namespace
#endif //IMAGESPACE

#undef IMAGE_BEGIN
#undef IMAGE_SCAN
#undef IMAGE_END
#undef IMAGE_PACKED
#undef IMAGE_STORAGE
#undef IMAGE_META

// kill modifier macros when asked to
#ifndef IMAGE_KEEP

	#ifdef IMAGECLASS
		#undef IMAGECLASS
	#endif//IMAGECLASS

	#ifdef IMAGEOBJECT
		#undef IMAGEOBJECT
	#endif//IMAGEOBJECT

	#ifdef IMAGESPACE
		#undef IMAGESPACE
	#endif//IMAGESPACE

	#ifdef IMAGENONAMES
		#undef IMAGENONAMES
	#endif//IMAGENONAMES

	#ifdef IMAGEVECTOR
		#undef IMAGEVECTOR
	#endif//IMAGEVECTOR

	#undef IMAGEFILE

#endif //IMAGE_KEEP
