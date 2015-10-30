/*
 * CaceMultiCastThread.h
 *
 *  Created on: 23.06.2014
 *      Author: endy
 */

#ifndef CACEMULTICASTTHREAD_H_
#define CACEMULTICASTTHREAD_H_

#include <string>
#include <thread>
#include "communication/multicast/PracticalSocket.h"
#include <random>

using namespace std;


//#define MAX_PACKETSIZE 8192
#define MAX_PACKETSIZE 100000

namespace cacemulticast
{
	template<class CommunicationClass>
	using t_multicastcallback = void (CommunicationClass::*)(char*, int);
	//typedef void (*t_multicastcallback)(char*, int size);

	template<class CommunicationClass>
	class CaceMultiCastChannel
	{
	public:
		static long traffic;
		static double packetLossPropability;
		static std::default_random_engine re;
		CaceMultiCastChannel(string address, unsigned short port, t_multicastcallback<CommunicationClass> callback,
								CommunicationClass* obj);
		~CaceMultiCastChannel();
		void publish(const char* bytes, int size);

	protected:
		double randomNumber();
		static unsigned short sourcePort;
		bool running;
		std::thread* t;
		string address;
		unsigned short port;
		UDPSocket udpsocket;
		void call();
		char* recvArray;
		t_multicastcallback<CommunicationClass> callback;
		CommunicationClass* obj;
	};

	template<class CommunicationClass>
	unsigned short CaceMultiCastChannel<CommunicationClass>::sourcePort = 30000;
	template<class CommunicationClass>
	long CaceMultiCastChannel<CommunicationClass>::traffic = 0;
	template<class CommunicationClass>
	double CaceMultiCastChannel<CommunicationClass>::packetLossPropability = 0;
	template<class CommunicationClass>
	std::default_random_engine CaceMultiCastChannel<CommunicationClass>::re;

	template<class CommunicationClass>
	inline CaceMultiCastChannel<CommunicationClass>::CaceMultiCastChannel(
			string address, unsigned short port, t_multicastcallback<CommunicationClass> callback,
			CommunicationClass* obj) :
			udpsocket(port)
	{
		running = true;

		this->obj = obj;
		this->callback = callback;
		this->address = address;
		this->port = port;

		recvArray = new char[MAX_PACKETSIZE];

		udpsocket.setMulticastTTL(1);

		udpsocket.joinGroup(address);
		t = new std::thread(&CaceMultiCastChannel<CommunicationClass>::call, this);
	}

	template<class CommunicationClass>
	inline CaceMultiCastChannel<CommunicationClass>::~CaceMultiCastChannel()
	{
		running = false;
		//udpsocket.leaveGroup(address);
		udpsocket.disconnect();
		t->join();
		//udpsocket.disconnect();
		delete t;
		delete[] recvArray;
	}

	template<class CommunicationClass>
	inline void CaceMultiCastChannel<CommunicationClass>::publish(const char* bytes, int size)
	{
		if(randomNumber()>=packetLossPropability) {
			udpsocket.sendTo(bytes, size, address, port);
		}
		traffic+=size;
	}

	template<class CommunicationClass>
	void CaceMultiCastChannel<CommunicationClass>::call()
	{

		int numBytes = 0;
		while (true)
		{
			numBytes = udpsocket.recvFrom(recvArray, MAX_PACKETSIZE, address, port);
			if (!running)
				return;
			(obj->*callback)(recvArray, numBytes);
		}
	}

	template<class CommunicationClass>
	double CaceMultiCastChannel<CommunicationClass>::randomNumber()
	{
	   double lower_bound = 0;
	   double upper_bound = 1;
	   std::uniform_real_distribution<double> unif(lower_bound,upper_bound);
	   return unif(re);
	}


} /* namespace cace */

#endif /* CACEMULTICASTTHREAD_H_ */
