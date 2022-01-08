#include <iostream>
#include <algorithm>
#include <string>

#include "config.h"
#include "TCPServer.h"

#ifdef NON_BLOCKING

#define DEFAULT_PORT 12345

int main()
{
	TCPServer server(DEFAULT_PORT);

	ReceivedSocketData receivedData;

	std::cout << "Starting server. Send \"exit\" (without quotes) to terminate." << std::endl;

	receivedData = server.accept();

	do {
		server.receiveData(receivedData, 0);

		if (receivedData.request != "")
		{
			std::cout << "Bytes received: " << receivedData.request.size() << std::endl;
			std::cout << "Data received: " << receivedData.request << std::endl;

			receivedData.reply = receivedData.request;
			std::reverse(receivedData.reply.begin(), receivedData.reply.end());

			server.sendReply(receivedData);
		}
	} while (receivedData.request != "exit" && receivedData.request != "EXIT");

	server.closeClientSocket(receivedData);

	std::cout << "Server terminated." << std::endl;
}

#endif //NON_BLOCKING
