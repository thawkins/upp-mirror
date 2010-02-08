topic "Ultimate++ vs D programming language";
[2 $$0,0#00000000000000000000000000000000:Default]
[i448;a25;kKO9;*@(64)2 $$1,0#37138531426314131252341829483380:class]
[l288;2 $$2,0#27521748481378242620020725143825:desc]
[a83;*R6 $$3,0#31310162474203024125188417583966:caption]
[l288;i1121;b17;O9;~~~.1408;2 $$4,0#10431211400427159095818037425705:param]
[i448;a25;kKO9;*@(64)2 $$5,0#37138531426314131252341829483370:item]
[*+117 $$6,6#14700283458701402223321329925657:header]
[l288;a17;*1 $$7,7#70004532496200323422659154056402:requirement]
[b42;a42;2 $$8,8#45413000475342174754091244180557:text]
[{_}%EN-US 
[s0; [*R6 U`+`+ Core vs D programming language]&]
[s0;> [*^topic`:`/`/uppweb`/www`/vsd`$en`-us^1 `[en`]][*1  ][*^topic`:`/`/uppweb`/www`/vsd`$ru`-ru^1 `[
ru`]]&]
[s0;^topic`:`/`/uppweb`/www`/comparison`$ru`-ru^ &]
[s0; D programming language authors give a nice example of D language 
string and map here:&]
[s0; &]
[s0; [^http`:`/`/www`.digitalmars`.com`/d`/2`.0`/cppstrings`.html^ http://www.digitalmar
s.com/d/2.0/cppstrings.html]&]
[s0; &]
[s0; We have taken the chance to reimplement this in U`+`+ and tested 
with Ubuntu 64 platform. `"gdc`" compiler was used with flags 
suggested in the article.&]
[s0; &]
[s0; We had to test with larger file than `"Alice30.txt`", because 
host platform was too fast for such small file, therefore we 
combined a couple of text files from the [^http`:`/`/www`.gutenberg`.org`/ebooks`/11^ s
ame source] to form a single 2MB file.&]
[s0; &]
[s0; We have also removed the code to produce the output to make 
results more relevant.&]
[s0; &]
[s0; Results:&]
[s0; &]
[ {{3333:3333:3334<768;>800;h1;@7 [s0; [* D language]]
:: [s0; [* U`+`+]]
:: [s0; [* U`+`+ / D language]]
::@2 [s0; 0.072s]
:: [s0; 0.043s]
:: [s0; 1.7]}}&]
[s0; &]
[s0; Means C`+`+ is still well ahead of D (by 70%) if not being hold 
back by standard library design and average implementation...&]
[s0; &]
[ {{10000@(254.254.208) [s0; [*C@5 #include <Core/Core.h>]&]
[s0;*C@5 &]
[s0; [*C@5 using namespace Upp;]&]
[s0;*C@5 &]
[s0; [*C@5 #define NOOUTPUT]&]
[s0;*C@5 &]
[s0; [*C@5 int main(int argc, const char `*argv`[`])]&]
[s0; [*C@5 `{]&]
[s0; [*C@5 -|int n;]&]
[s0; [*C@5 -|VectorMap<String, int> map;]&]
[s0; [*C@5 -|Cout() << `"   lines   words   bytes file`\n`";]&]
[s0; [*C@5 -|int total`_lines `= 0;]&]
[s0; [*C@5 -|int total`_words `= 0;]&]
[s0; [*C@5 -|int total`_bytes `= 0;]&]
[s0; [*C@5 -|for(int i `= 1; i < argc; i`+`+) `{]&]
[s0; [*C@5 -|-|String f `= LoadFile(argv`[i`]);]&]
[s0; [*C@5 -|-|int lines `= 0;]&]
[s0; [*C@5 -|-|int words `= 0;]&]
[s0; [*C@5 -|-|const char `*q `= f;]&]
[s0; [*C@5 -|-|for(;;) `{]&]
[s0; [*C@5 -|-|-|int c `= `*q;]&]
[s0; [*C@5 -|-|-|if(IsAlpha(c)) `{]&]
[s0; [*C@5 -|-|-|-|const char `*b `= q`+`+;]&]
[s0; [*C@5 -|-|-|-|while(IsAlNum(`*q)) q`+`+;]&]
[s0; [*C@5 -|-|-|-|map.GetAdd(String(b, q), 0)`+`+;]&]
[s0; [*C@5 -|-|-|-|words`+`+;]&]
[s0; [*C@5 -|-|-|`}]&]
[s0; [*C@5 -|-|-|else `{]&]
[s0; [*C@5 -|-|-|-|if(!c) break;]&]
[s0; [*C@5 -|-|-|-|if(c `=`= `'`\n`')]&]
[s0; [*C@5 -|-|-|-|-|`+`+lines;]&]
[s0; [*C@5 -|-|-|-|q`+`+;]&]
[s0; [*C@5 -|-|-|`}]&]
[s0; [*C@5 -|-|`}]&]
[s0; [*C@5 -|-|Cout() << Format(`"%8d%8d%8d %s`\n`", lines, words, f.GetCount(), 
argv`[i`]);]&]
[s0; [*C@5 -|-|total`_lines `+`= lines;]&]
[s0; [*C@5 -|-|total`_words `+`= words;]&]
[s0; [*C@5 -|-|total`_bytes `+`= f.GetCount();]&]
[s0; [*C@5 -|`}]&]
[s0; [*C@5 -|Vector<int> order `= GetSortOrder(map.GetKeys());]&]
[s0; [*C@5 #ifndef NOOUTPUT]&]
[s0; [*C@5 -|Cout() << Format(`"`-`-`-`-`-`-`-`-`-`-`-`-`-`-`-`-`-`-`-`-`-`-`-`-`-`-`-`-
`-`-`-`-`-`-`-`-`-`-%8d%8d%8d total`\n`", total`_lines, total`_words, 
total`_bytes);]&]
[s0;*C@5 &]
[s0; [*C@5 -|for(int i `= 0; i < order.GetCount(); i`+`+)]&]
[s0; [*C@5 -|-|Cout() << map.GetKey(order`[i`]) << `": `" << map`[order`[i`]`] 
<< `'`\n`';]&]
[s0; [*C@5 #endif]&]
[s0; [*C@5 -|return 0;]&]
[s0; [*C@5 `}]&]
[s0;*C@5 ]}}&]
[s0; &]
[s0; This article in [^topic`:`/`/uppweb`/www`/vsd`$ru`-ru^ Russian]]