#ifndef _Core_Topic_h_
#define _Core_Topic_h_

struct Topic : Moveable<Topic> {
	String title;
	String text;
	
	operator const String&() const { return text; }
	operator const char *() const  { return text; }
};

struct TopicLink {
	String package;
	String group;
	String topic;

	operator bool() const { return !IsNull(topic); }
};

String     TopicLinkString(const TopicLink& tl);
TopicLink  ParseTopicLink(const char *link);

Topic          GetTopic(const String& package, const String& group, const String& topic);
Topic          GetTopic(const char *path);

VectorMap<String, VectorMap<String, VectorMap<String, Topic> > >& TopicBase();

#endif
