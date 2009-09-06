topic "";
[ $$0,0#00000000000000000000000000000000:Default]
[H6;0 $$1,0#05600065144404261032431302351956:begin]
[i448;a25;kKO9;2 $$2,0#37138531426314131252341829483370:codeitem]
[l288;2 $$3,0#27521748481378242620020725143825:desc]
[0 $$4,0#96390100711032703541132217272105:end]
[{_}%EN-US 
[s0;%- &]
[ {{10000F(128)G(128)@1 [s0; [* Page segmentation/layout analysis functions]]}}&]
[s1;%- &]
[s2;:PixRaster`:`:GetRegionsBinary`(int`):%- [@(0.0.255) bool]_[* GetRegionsBinary]([@(0.0.255) i
nt]_[*@3 page]_`=_PIXRASTER`_CURPAGE)&]
[s3; Gets layout masks for image at [%-*@3 page]. Masks are appended 
at the end of PixRaster in this sequence :&]
[s3;i150;O0; Halftone mask-|-|1 bpp mask for halftone page parts&]
[s3;i150;O0; Textline mask-|-|1 bpp mask for text lines inside page&]
[s3;i150;O0; Textblock mask-|-|1 bpp mask for text blocks inside page&]
[s3; Active PixRaster page is set to first (Halftone) of the three 
resulting masks.&]
[s3; &]
[s3; Returns [%-@(0.0.255) true] on success, [%-@(0.0.255) false] otherwise.&]
[s3; &]
[s4; &]
[s0; ]