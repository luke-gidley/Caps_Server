#include <iostream>
#include <string>
#include <vector>
#include <conio.h>
#include <thread>
#include <algorithm>
#include <map>
#include <mutex>
#include <shared_mutex>

#include "RequestParser.h"
#include "TCPServer.h"
#include "TCPClient.h"
#include "threadpool.h"


#define DEFAULT_PORT 12345


using namespace std;


bool terminateServer = false;

shared_mutex mtx;




void threadParser(TCPServer* server, ReceivedSocketData&& data, map<string, vector<string>>* messageBoard);
//void readerTask(TCPServer* server, ReceivedSocketData data, map<string, vector<string>>* messageBoard, ReadRequest read);

int main() {

	TCPServer server(DEFAULT_PORT);

	map<string, vector<string>>* messageBoard = new map<string, vector<string>>();

	ReceivedSocketData receivedData;

	vector<thread> serverThreads;

	cout << "Starting server. Send \"exit\" (without quotes) to terminate." << endl;

	


	while (!terminateServer)
	{
		receivedData = server.accept();

		cout << "Client connected on socket " << receivedData.ClientSocket << endl;

		if (!terminateServer)
		{
			serverThreads.emplace_back(threadParser, &server, receivedData, messageBoard);
		}
	}

	for (auto& th : serverThreads)
		th.join();



	cout << "Server terminated." << endl;

	delete messageBoard;
}


void threadParser(TCPServer* server, ReceivedSocketData&& data, map<string, vector<string>>* messageBoard)
{
	unsigned int socketIndex = (unsigned int)data.ClientSocket;

	do
	{
		server->receiveData(data, 1);

		if (!data.request.empty())
		{

			//when a POST request is made, server needs to save the POST request to a data structure, in order for easy access for the READ request.
			PostRequest post = PostRequest::parse(data.request);
			if (post.valid)
			{


				if (post.getTopicId().length() > 140)
				{
					post.topicId = post.getTopicId().substr(0, 140);
				}

				if (post.getMessage().length() > 140)
				{
					post.message = post.getMessage().substr(0, 140);
				}

				map<string, vector<string>>::iterator it;

				vector<string> temp;

				mtx.lock_shared();
				it = messageBoard->find(post.getTopicId());
				if (it != messageBoard->end())
					temp = it->second;
				mtx.unlock_shared();

				temp.push_back(post.getMessage());

				mtx.lock();
				messageBoard->insert_or_assign(post.getTopicId(), temp);	
				mtx.unlock();

				data.reply = to_string(temp.size() - 1);
				server->sendReply(data);
				

				continue;
			}

			ReadRequest read = ReadRequest::parse(data.request);
			if (read.valid)
			{
				
				if (read.getTopicId().length() > 140)
					read.topicId = read.getTopicId().substr(0, 140);


				map<string, vector<string>>::iterator it;

				vector<string> temp;


				mtx.lock_shared();
				it = messageBoard->find(read.getTopicId());			
				if (it != messageBoard->end())
				{
					mtx.unlock_shared();
					temp = it->second;
					if (temp.size() > read.getPostId())
					{
						string message = temp.at(read.getPostId());
						data.reply = message;
					}
				}
				else
					mtx.unlock_shared();
				
				server->sendReply(data);
				
				
				continue;
			}

			CountRequest count = CountRequest::parse(data.request);
			if (count.valid)
			{

				map<string, vector<string>>::iterator it;

				it = messageBoard->find(count.getTopicId());
				data.reply = "0";

				if (it != messageBoard->end())
				{
					data.reply = to_string(it->second.size());
				}


				server->sendReply(data);
				continue;
			}

			ListRequest list = ListRequest::parse(data.request);
			if (list.valid)
			{
				string reply = "";
				for (map<string, vector<string>>::iterator iter = messageBoard->begin(); iter != messageBoard->end(); ++iter)
				{
					reply += iter->first + "#";
				}
				reply = reply.substr(0, reply.length() - 1);
				data.reply = reply;

				server->sendReply(data);
				continue;
			}

			ExitRequest exitReq = ExitRequest::parse(data.request);
			if (exitReq.valid)
			{
				terminateServer = true;
				data.reply = "TERMINATING";
				server->sendReply(data);
				TCPClient tempClient(string("127.0.0.1"), DEFAULT_PORT);
				tempClient.OpenConnection();
				tempClient.CloseConnection();
				continue;
			}

			cout << "Unknown request: " << data.request << endl;
			cout << endl;

		}
	
	} while (data.request != "exit" && data.request != "EXIT" && !terminateServer);

	server->closeClientSocket(data);
}




