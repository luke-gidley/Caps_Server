#ifndef __REQUESTPARSER_H
#define __REQUESTPARSER_H

#include <string>

class PostRequest
{
public:
	PostRequest();
	~PostRequest();
	static PostRequest parse(std::string request);
	std::string getTopicId();
	std::string getMessage();
	std::string toString();

	std::string topicId;
	std::string message;
	bool valid;
};

class ReadRequest
{
public:
	ReadRequest();
	~ReadRequest();
	static ReadRequest parse(std::string request);
	std::string getTopicId();
	int getPostId();
	std::string toString();

	std::string topicId;
	int postId;
	bool valid;
};

class CountRequest
{
public:
	CountRequest();
	~CountRequest();
	static CountRequest parse(std::string request);
	std::string getTopicId();
	std::string toString();

	std::string topicId;
	bool valid;
};

class ListRequest
{
public:
	ListRequest();
	~ListRequest();
	static ListRequest parse(std::string request);
	std::string toString();
	bool valid;
};

class ExitRequest
{
public:
	ExitRequest();
	~ExitRequest();
	static ExitRequest parse(std::string request);
	std::string toString();
	bool valid;
};

#endif //__REQUESTPARSER_H

