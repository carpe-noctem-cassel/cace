/*
 * caceCommunication.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef CACECOMMUNICATION_H_
#define CACECOMMUNICATION_H_

#include <set>
#include <list>
#include <mutex>
#include <memory>

#include "ros/ros.h"
#include "std_msgs/String.h"
#include "cace/CaceCommand.h"
#include "cace/CaceAcknowledge.h"
#include "cace/CaceShortAck.h"
#include "cace/CaceBelieveNotification.h"
#include "cace/CaceTime.h"
#include "cace/CaceVariableRequest.h"
#include "cace/CaceType.h"

#include "cace.h"
#include "CaceTypes.h"
#include "../variables/ConsensusVariable.h"
#include "CommunicationWorker.h"
#include "../timeManager/AgentTimeData.h"
#include "CaceTypes.h"
#include "jobs/AbstractCommunicationJob.h"
#include "jobs/WriteAckJob.h"

using namespace std;

namespace cace
{
	class Cace;
	class CommunicationWorker;
	class TimeManager;

	class CaceCommunication
	{
	public:
		CaceCommunication(CommunicationWorker* worker, string& nodePrefix, Cace* cace);
		virtual ~CaceCommunication();

		virtual void init(CommunicationWorker* worker, string& nodePrefix, Cace* cace);

		void step();

		int getOwnID();
		void setOwnID(int id);
		bool isBlacklisted(int agentID);
		void addToBlacklist(int agentID);
		void removeFromBlackList(int agentID);
		virtual void cleanUp();
		void clearAllMessageLists();
		virtual void sendEvalString(string m);
		virtual void sendCaceVariableRequest(short receiverID, short msgID, string& name);
		virtual void sendCaceVariableRequest(short receiverID, short msgID, string& name, unsigned long lamportTime);
		virtual void sendCaceResponse(shared_ptr<ConsensusVariable> cv, short msgID, short receiver);
		virtual void sendCaceResponse(shared_ptr<ConsensusVariable> cv, short msgID, short receiver,
										unsigned long lamportTime);

		virtual void handleCaceVariableRequest(CaceVariableRequestPtr cvr);
		virtual void handleCaceResponse(CaceCommandPtr cc);
		virtual void handleCaceWrite(CaceCommandPtr cc);
		virtual void sendWriteCommand(shared_ptr<ConsensusVariable> cv, short msgID, short receiver);
		virtual void sendWriteCommand(shared_ptr<ConsensusVariable> cv, short msgID, short receiver,
										unsigned long lamportTime);
		virtual void handleCaceWriteAck(CaceShortAckPtr ca);
		virtual void handleCaceCommand(CaceCommandPtr cc);
		list<CaceCommandPtr> getCommands();
		virtual void sendCaceCommand(shared_ptr<ConsensusVariable> cv, short msgID, vector<uint8_t>& value,
										short receiver);
		virtual void sendCaceCommand(shared_ptr<ConsensusVariable> cv, short msgID, vector<uint8_t>& value,
										short receiver, unsigned long lamportTime);
		virtual void handleCaceAcknowledge(CaceAcknowledgePtr ca);
		list<CaceAcknowledgePtr> getAcknowledgesAndRemove(short msgID);
		list<CaceShortAckPtr> getWriteAcknowledgesAndRemove(short msgID);
		virtual void sendCaceAcknowledge(string& name, vector<uint8_t>& value, short messageID, short receiver,
											short type);
		virtual void sendCaceAcknowledge(string& name, vector<uint8_t>& value, short messageID, short receiver,
											short type, unsigned long lamportTime);
		virtual void handleCaceBelieveNotification(CaceBelieveNotificationPtr cbn);
		list<CaceBelieveNotificationPtr> getCaceBelieveNotifications();
		virtual void sendCaceBelieveNotification(shared_ptr<ConsensusVariable> cv, short receiverID, short msgID);
		virtual void sendCaceBelieveNotification(shared_ptr<ConsensusVariable> cv, short receiverID, short msgID,
													unsigned long lamportTime);
		virtual void sendCaceWriteAck(string& name, short messageID, short receiver);
		virtual void sendCaceWriteAck(string& name, short messageID, short receiver, unsigned long lamportTime);
		virtual void sendCaceShortAck(string& name, short messageID, short receiver);
		virtual void sendCaceShortAck(string& name, short messageID, short receiver, unsigned long lamportTime);
		virtual void handleCaceShortAck(CaceShortAckPtr sa);
		virtual void sendTime(TimeManager* m);
		virtual void handleCaceTime(CaceTimePtr ct);
		list<CaceShortAckPtr> getCaceShortAcksAndRemove(short msgID);
		virtual void anounceDisengange();
		string printMessageQueueStates();

		set<int> agentBlacklist;

		ros::NodeHandle rosNode;
		ros::AsyncSpinner* spinner;
		int ownID;
	protected:
		CaceCommunication();

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

		list<CaceCommandPtr> commands;
		mutex cmdMutex;
		list<CaceCommandPtr> writeCommands;
		mutex writeMutex;
		list<CaceAcknowledgePtr> acknowledges;
		mutex ackMutex;
		list<CaceShortAckPtr> writeAcknowledges;
		mutex wackMutex;
		list<CaceShortAckPtr> shortAcks;
		mutex sackMutex;
		list<CaceBelieveNotificationPtr> believeNotifications;
		mutex notMutex;

		CommunicationWorker* worker;
		Cace* cace;

		mutex blacklistMutex;
	};

} /* namespace cace */

#endif /* CACECOMMUNICATION_H_ */
