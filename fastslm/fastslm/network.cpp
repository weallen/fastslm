#include "network.h"

// function called in thread
unsigned int __stdcall ThreadReceiveData(void* arg) {
	NetworkHandler* nh = (NetworkHandler*) arg;

	while (nh->running) {
		Sleep(1);
		//printf("Waiting for data...\n");
		nh->ReceiveData();
	}
	return 1;
}

NetworkHandler::NetworkHandler() {
	context_ = zmq_ctx_new();
	//subscriber_ = zmq_socket(context_, ZMQ_SUB);
	recv_buffer_ = new char[BUFSIZE];
	running = false;
}

NetworkHandler::~NetworkHandler() {
	if (running) {
		StopListen();
	}
	delete[] recv_buffer_;
	for (int i = 0; i < channels_.size(); ++i) {
		void* chan = channels_[i];
		zmq_close(chan);
	}
	zmq_ctx_destroy(context_);
}

int NetworkHandler::ReceiveData() {
	//char temp[BUFSIZE];
	std::string buff;

	zmq_poll(&polls_[0], polls_.size(), -1);
	for (int i = 0; i < polls_.size(); ++i) {
		zmq_pollitem_t pollitem = polls_[i];
		if (pollitem.revents & ZMQ_POLLIN) {
			int nbytes = zmq_recv(channels_[i], recv_buffer_, BUFSIZE, 0);
			if (nbytes == -1) {
				//printf("Timed out...\n");
			}
			else {
				//printf("Received %d bytes\n", nbytes);
				buff.assign(recv_buffer_, nbytes + 1);
				buff[nbytes] = '\0';
				queue_.push(buff);
			}
			return nbytes;
		}
	}
}

void NetworkHandler::Connect(const char* ipaddress, int port) {
	char channel[200];
	zmq_pollitem_t pollitem;

	sprintf(channel, "tcp://%s:%d", ipaddress, port);
	printf("Connecting to %s...\n", channel);

	void* chan = zmq_socket(context_, ZMQ_SUB);
	int rc = zmq_connect(chan, channel);
	zmq_setsockopt(chan, ZMQ_SUBSCRIBE, "", 0);
	channels_.push_back(chan);

	pollitem.socket = chan;
	pollitem.fd = 0;
	pollitem.events = ZMQ_POLLIN;
	pollitem.revents = 0;
	polls_.push_back(pollitem);

	//int timeout = 3000;
	//zmq_setsockopt(subscriber_, ZMQ_RCVTIMEO, &timeout, sizeof(int));
	//assert(rc == 0);
}

void NetworkHandler::Disconnect() {
	for (int i = 0; i < channels_.size(); ++i) {
		void* chan = channels_[i];
		zmq_close(chan);
	}
	channels_.clear();
	polls_.clear();
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

