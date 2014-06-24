/*
 * CaceCommunicationMultiCast.h
 *
 *  Created on: 23.06.2014
 *      Author: endy
 */

#ifndef CACECOMMUNICATIONMULTICAST_H_
#define CACECOMMUNICATIONMULTICAST_H_

#include <communication/CaceCommunication.h>
#include <communication/multicast/CaceMultiCastChannel.h>

using namespace cacemulticast;

namespace cace
{

	class CaceCommunicationMultiCast : public CaceCommunication
	{
	public:
		CaceCommunicationMultiCast(CommunicationWorker* worker, string& nodePrefix, Cace* cace);
		virtual ~CaceCommunicationMultiCast();

		virtual void cleanUp();
		virtual void init(CommunicationWorker* worker, string& nodePrefix, Cace* cace);

		virtual void mhandleCaceVariableRequest(char* buffer, int size);
		virtual void mhandleCaceResponse(char* buffer, int size);
		virtual void mhandleCaceWrite(char* buffer, int size);
		virtual void mhandleCaceAcknowledge(char* buffer, int size);
		virtual void mhandleCaceWriteAck(char* buffer, int size);
		virtual void mhandleCaceCommand(char* buffer, int size);
		virtual void mhandleCaceBelieveNotification(char* buffer, int size);
		virtual void mhandleCaceShortAck(char* buffer, int size);
		virtual void mhandleCaceTime(char* buffer, int size);

		virtual void sendEvalString(string m);
		virtual void sendCaceVariableRequest(short receiverID, short msgID, string& name, unsigned long lamportTime);
		virtual void sendCaceResponse(shared_ptr<ConsensusVariable> cv, short msgID, short receiver,
										unsigned long lamportTime);
		virtual void sendWriteCommand(shared_ptr<ConsensusVariable> cv, short msgID, short receiver,
										unsigned long lamportTime);
		virtual void sendCaceCommand(shared_ptr<ConsensusVariable> cv, short msgID, vector<uint8_t>& value,
										short receiver, unsigned long lamportTime);
		virtual void sendCaceAcknowledge(string& name, vector<uint8_t>& value, short messageID, short receiver,
											short type, unsigned long lamportTime);
		virtual void sendCaceBelieveNotification(shared_ptr<ConsensusVariable> cv, short receiverID, short msgID,
													unsigned long lamportTime);
		virtual void sendCaceWriteAck(string& name, short messageID, short receiver, unsigned long lamportTime);
		virtual void sendCaceShortAck(string& name, short messageID, short receiver, unsigned long lamportTime);
		virtual void sendTime(TimeManager* m);

	protected:
		CaceCommunicationMultiCast();
		string addr = "224.16.32.40";
		unsigned short port;

		CaceMultiCastChannel<CaceCommunicationMultiCast>* commandChannel;
		CaceMultiCastChannel<CaceCommunicationMultiCast>* ackChannel;
		CaceMultiCastChannel<CaceCommunicationMultiCast>* shortChannel;
		CaceMultiCastChannel<CaceCommunicationMultiCast>* notificationChannel;
		CaceMultiCastChannel<CaceCommunicationMultiCast>* timeChannel;
		CaceMultiCastChannel<CaceCommunicationMultiCast>* varRequestChannel;
		CaceMultiCastChannel<CaceCommunicationMultiCast>* writeChannel;
		CaceMultiCastChannel<CaceCommunicationMultiCast>* writeAckChannel;
		CaceMultiCastChannel<CaceCommunicationMultiCast>* responseChannel;
		CaceMultiCastChannel<CaceCommunicationMultiCast>* evalChannel;
	};

} /* namespace cace */

#endif /* CACECOMMUNICATIONMULTICAST_H_ */
