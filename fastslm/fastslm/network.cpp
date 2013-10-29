#include "network.h"

// function called in thread
unsigned int __stdcall ThreadReceiveData(void* arg) {
	NetworkHandler* nh = (NetworkHandler*) arg;

	while (nh->running) {
		Sleep(10);
		//printf("Waiting for data...\n");
		nh->ReceiveData();
	}
	return 1;
}

NetworkHandler::NetworkHandler() {
	context_ = zmq_ctx_new();
	subscriber_ = zmq_socket(context_, ZMQ_SUB);
	recv_buffer_ = new char[BUFSIZE];
	running = false;
}

NetworkHandler::~NetworkHandler() {
	if (running) {
		StopListen();
	}
	delete[] recv_buffer_;
	zmq_close(subscriber_);
	zmq_ctx_destroy(context_);
}

int NetworkHandler::ReceiveData() {
	//char temp[BUFSIZE];
	std::string buff;
	//int nbytes = zmq_recv(subscriber_, recv_buffer, BUFSIZE, 0);
	int nbytes = zmq_recv(subscriber_, recv_buffer_, BUFSIZE, 0);
	if (nbytes == -1) {
		//printf("Timed out...\n");
	} else {
		//printf("Received %d bytes\n", nbytes);
		buff.assign(recv_buffer_, nbytes+1);
		buff[nbytes] = '\0';
		queue_.push(buff);
	}
	return nbytes;
}

void NetworkHandler::Connect(const char* ipaddress, int port) {
	char channel[200];
	sprintf(channel, "tcp://%s:%d", ipaddress, port);
	printf("Connected to %s...\n", channel);
	int rc = zmq_connect(subscriber_, channel);
	zmq_setsockopt(subscriber_, ZMQ_SUBSCRIBE, "", 0);
	//int timeout = 3000;
	//zmq_setsockopt(subscriber_, ZMQ_RCVTIMEO, &timeout, sizeof(int));
	//assert(rc == 0);
}

void NetworkHandler::Disconnect() {
	zmq_close(subscriber_);
}

void NetworkHandler::StartListen() {
	// create a thread to process data
	running = true;
	thread_ = (HANDLE) _beginthreadex(NULL, 0, &::ThreadReceiveData, this, 0, NULL);
}

void NetworkHandler::StopListen() {
	// Terminate the thread
	running = false;
	DWORD rc = WaitForSingleObject(thread_, INFINITE);
	BOOL rc2 = CloseHandle(thread_);
	std::cout << "Stopped listening" << std::endl;
}

