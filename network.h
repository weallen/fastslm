#ifndef NETWORK_H_
#define NETWORK_H_

#define BUFSIZE 1024

#include "stdafx.h"

#include <string>

#include <ppl.h>
#include <concurrent_queue.h>

#include <zmq.h>

#include "common.h"

unsigned int __stdcall ThreadReceiveData(void* arg);


class NetworkHandler
{
public:
	NetworkHandler();
	virtual ~NetworkHandler();

	void Connect(const char* ipaddress, int port);
	void Disconnect();

	void StartListen();
	void StopListen();

	int ReceiveData();

	concurrency::concurrent_queue<std::string>* GetQueue() { return &queue_; }
	
	bool volatile running;

private:
	void* context_;
	void* subscriber_;

	HANDLE thread_; // windows thread for recieving data
	concurrency::concurrent_queue<std::string> queue_;
	char recv_buffer[BUFSIZE];
};

#endif