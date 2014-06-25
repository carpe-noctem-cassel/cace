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

		virtual void step();
		virtual void startAsynchronous();

		virtual void sendCaceVariableRequest(short receiverID, short msgID, string& name);
		virtual void sendCaceResponse(shared_ptr<ConsensusVariable> cv, short msgID, short receiver);
		virtual void sendWriteCommand(shared_ptr<ConsensusVariable> cv, short msgID, short receiver);
		virtual void sendCaceCommand(shared_ptr<ConsensusVariable> cv, short msgID, vector<uint8_t>& value,
										short receiver);
		virtual void sendCaceAcknowledge(string& name, vector<uint8_t>& value, short messageID, short receiver,
											short type);
		virtual void sendCaceBelieveNotification(shared_ptr<ConsensusVariable> cv, short receiverID, short msgID);
		virtual void sendCaceWriteAck(string& name, short messageID, short receiver);
		virtual void sendCaceShortAck(string& name, short messageID, short receiver);

		virtual void init(CommunicationWorker* worker, string& nodePrefix, Cace* cace);
		virtual void cleanUp();
		virtual void sendEvalString(string m);
		virtual void handleCaceVariableRequest(CaceVariableRequestPtr cvr);
		virtual void handleCaceResponse(CaceCommandPtr cc);
		virtual void handleCaceWrite(CaceCommandPtr cc);
		virtual void handleCaceWriteAck(CaceShortAckPtr ca);
		virtual void handleCaceCommand(CaceCommandPtr cc);
		virtual void handleCaceAcknowledge(CaceAcknowledgePtr ca);
		virtual void handleCaceBelieveNotification(CaceBelieveNotificationPtr cbn);
		virtual void handleCaceShortAck(CaceShortAckPtr sa);
		virtual void handleCaceTime(CaceTimePtr ct);

		virtual void sendCaceVariableRequest(short receiverID, short msgID, string& name,
												unsigned long lamportTime) = 0;
		virtual void sendCaceResponse(shared_ptr<ConsensusVariable> cv, short msgID, short receiver,
										unsigned long lamportTime) = 0;
		virtual void sendWriteCommand(shared_ptr<ConsensusVariable> cv, short msgID, short receiver,
										unsigned long lamportTime) = 0;
		virtual void sendCaceCommand(shared_ptr<ConsensusVariable> cv, short msgID, vector<uint8_t>& value,
										short receiver, unsigned long lamportTime) = 0;
		virtual void sendCaceAcknowledge(string& name, vector<uint8_t>& value, short messageID, short receiver,
											short type, unsigned long lamportTime) = 0;
		virtual void sendCaceBelieveNotification(shared_ptr<ConsensusVariable> cv, short receiverID, short msgID,
													unsigned long lamportTime) = 0;
		virtual void sendCaceWriteAck(string& name, short messageID, short receiver, unsigned long lamportTime) = 0;
		virtual void sendCaceShortAck(string& name, short messageID, short receiver, unsigned long lamportTime) = 0;
		virtual void sendTime(TimeManager* m) = 0;

		virtual void anounceDisengange();

		int getOwnID();
		void setOwnID(int id);
		bool isBlacklisted(int agentID);
		void addToBlacklist(int agentID);
		void removeFromBlackList(int agentID);
		void clearAllMessageLists();
		list<CaceCommandPtr> getCommands();
		//list<CaceAcknowledgePtr> getAcknowledgesAndRemove(short msgID);
		list<CaceShortAckPtr> getWriteAcknowledgesAndRemove(short msgID);
		list<CaceBelieveNotificationPtr> getCaceBelieveNotifications();
		list<CaceShortAckPtr> getCaceShortAcksAndRemove(short msgID);
		string printMessageQueueStates();

		set<int> agentBlacklist;
		int ownID;
	protected:
		CaceCommunication();

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
