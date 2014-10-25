/*
 * CaceCommunicationRos.h
 *
 *  Created on: 25.06.2014
 *      Author: endy
 */

#ifndef CACECOMMUNICATIONROS_H_
#define CACECOMMUNICATIONROS_H_

#include <set>
#include <list>
#include <mutex>
#include <memory>

#include "communication/CaceCommunication.h"

namespace cace
{
	class Cace;
	class CommunicationWorker;
	class TimeManager;

	class CaceCommunicationRos : public CaceCommunication
	{
	public:
		CaceCommunicationRos();
		CaceCommunicationRos(CommunicationWorker* worker, string& nodePrefix, Cace* cace);
		virtual ~CaceCommunicationRos();

		virtual void step();
		virtual void startAsynchronous();

		virtual void init(CommunicationWorker* worker, string& nodePrefix, Cace* cace);
		virtual void cleanUp();
		virtual void sendEvalString(string m);

		virtual void sendCaceVariableRequest(short receiverID, short msgID, string& name,
												unsigned long lamportTime);
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

		ros::NodeHandle rosNode;
		ros::AsyncSpinner* spinner;
	protected:
		ros::Publisher commandPublisher;
		ros::Subscriber commandSubscriber;
		ros::Publisher ackPublisher;
		ros::Subscriber ackSubscriber;
		ros::Publisher shortAckPublisher;
		ros::Subscriber shortAckSubscriber;
		ros::Publisher notificationPublisher;
		ros::Subscriber notificationSubscriber;
		ros::Publisher timePublisher;
		ros::Subscriber timeSubscriber;
		ros::Publisher varRequestPublisher;
		ros::Subscriber varRequestSubscriber;
		ros::Publisher writePublisher;
		ros::Subscriber writeSubscriber;
		ros::Publisher writeAckPublisher;
		ros::Subscriber writeAckSubscriber;
		ros::Publisher responsePublisher;
		ros::Subscriber responseSubscriber;
		ros::Publisher evalPublisher;
	};

} /* namespace cace */

#endif /* CACECOMMUNICATIONROS_H_ */
