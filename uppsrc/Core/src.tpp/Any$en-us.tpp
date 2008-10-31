topic "Any";
[2 $$0,0#00000000000000000000000000000000:Default]
[i448;a25;kKO9;2 $$1,0#37138531426314131252341829483380:class]
[l288;2 $$2,0#27521748481378242620020725143825:desc]
[0 $$3,0#96390100711032703541132217272105:end]
[H6;0 $$4,0#05600065144404261032431302351956:begin]
[i448;a25;kKO9;2 $$5,0#37138531426314131252341829483370:item]
[l288;a4;*@5;1 $$6,6#70004532496200323422659154056402:requirement]
[l288;i1121;b17;O9;~~~.1408;2 $$7,0#10431211400427159095818037425705:param]
[i448;b42;O9;2 $$8,8#61672508125594000341940100500538:tparam]
[b42;2 $$9,9#13035079074754324216151401829390:normal]
[{_} 
[s1;:Any`:`:class: [@(0.0.255) class]_[* Any]_:_[@(0.0.255) private]_[*@3 Moveable][@(0.0.255) <
][* Any][@(0.0.255) >]&]
[s0;%% Any is a special type of container capable of containing none 
or single element of [/ any] type. It also provides methods for 
querying the type stored and retrieving the content of specific 
type.&]
[s0;%% Any has pick semantics.&]
[s0;%% &]
[ {{10000F(128)G(128)@1 [s0;%% [* Public Method List]]}}&]
[s3; &]
[s5;:Any`:`:Any`(pick`_ Any`&`): [* Any]([@(0.128.128) pick`_]_[* Any][@(0.0.255) `&]_[*@3 s])&]
[s2;%% Pick constructor. Transfers content of source Any while destroying 
its content by picking.&]
[s7;%% [%-*C@3 s]-|Source Any.&]
[s3; &]
[s4; &]
[s5;:Any`:`:Any`(`): [* Any]()&]
[s2;%% Constructs an empty Any.&]
[s3; &]
[s4; &]
[s5;:Any`:`:`~Any`(`): [@(0.0.255) `~][* Any]()&]
[s2;%% Destructor.&]
[s3; &]
[s4; &]
[s5;:Any`:`:Create`(`): [@(0.0.255) template]_<[@(0.0.255) class]_[*@4 T][@(0.0.255) >]_[*@4 T][@(0.0.255) `&
]_[* Create]()&]
[s2;%% Creates content of type T inside Any.&]
[s7;%% [*C@4 T]-|Type of content.&]
[s7;%% [*/ Return value]-|Reference to the newly created content.&]
[s3; &]
[s4; &]
[s5;:Any`:`:Is`(`)const: [@(0.0.255) template]_<[@(0.0.255) class]_[*@4 T][@(0.0.255) >]_[@(0.0.255) b
ool]_[* Is]()_[@(0.0.255) const]&]
[s2;%% Tests whether Any contains content of type T.&]
[s7;%% [*C@4 T]-|Required type.&]
[s7;%% [*/ Return value]-|true if there is content with type T in Any.&]
[s3; &]
[s4; &]
[s5;:Any`:`:Get`(`): [@(0.0.255) template]_<[@(0.0.255) class]_[*@4 T][@(0.0.255) >]_[*@4 T][@(0.0.255) `&
]_[* Get]()&]
[s2;%% Returns reference to content. Is<T> must be true, otherwise 
this operation is illegal.&]
[s7;%% [*C@4 T]-|Required type.&]
[s7;%% [*/ Return value]-|Reference to content.&]
[s3; &]
[s4; &]
[s5;:Any`:`:Get`(`)const: [@(0.0.255) template]_<[@(0.0.255) class]_[*@4 T][@(0.0.255) >]_[@(0.0.255) c
onst]_[*@4 T][@(0.0.255) `&]_[* Get]()_[@(0.0.255) const]&]
[s2;%% Returns constant reference to content. Is<T> must be true, 
otherwise this operation is illegal.&]
[s7;%% [*C@4 T]-|Required type.&]
[s7;%% [*/ Return value]-|Reference to content.&]
[s3; &]
[s4; &]
[s5;:Any`:`:Clear`(`): [@(0.0.255) void]_[* Clear]()&]
[s2;%% Removes (and destroys) content.&]
[s3; &]
[s4; &]
[s5;:Any`:`:IsEmpty`(`)const: [@(0.0.255) bool]_[* IsEmpty]()_[@(0.0.255) const]&]
[s7;%% [*/ Return value]-|true if there is no content.&]
[s3; &]
[s4; &]
[s5;:Any`:`:IsPicked`(`)const: [@(0.0.255) bool]_[* IsPicked]()_[@(0.0.255) const]&]
[s7;%% [*/ Return value]-|true if Any is picked.&]
[s3; &]
[s4; &]
[s5;:Any`:`:operator`=`(pick`_ Any`&`): [@(0.0.255) void]_[* operator`=]([@(0.128.128) pick
`_]_Any[@(0.0.255) `&]_[*@3 s])&]
[s2;%% Pick operator. Transfers content while destroying source.&]
[s7;%% [%-*C@3 s]-|Source Any.&]
[s0; ]