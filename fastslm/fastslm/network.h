#ifndef NETWORK_H_
#define NETWORK_H_

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#define BUFSIZE 65536
#define MAX_SOCKS 10

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
	std::vector<zmq_pollitem_t> polls_;
	std::vector<void*> channels_;
	int nsockets_;

	HANDLE thread_; // windows thread for recieving data
	concurrency::concurrent_queue<std::string> queue_;
	char* recv_buffer_;
};

#endif