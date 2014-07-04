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

		/*!
		 * Unsubscripes all Topics
		 */
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

		/*!
		 * Requests a variable from another agent
		 * Note: Receiver 0 indicates broadcast
		 */
		virtual void sendCaceVariableRequest(short receiverID, short msgID, string& name,
												unsigned long lamportTime) = 0;

		/*!
		 * Sends a CaceResponse
		 * Note: Receiver 0 indicates broadcast
		 */
		virtual void sendCaceResponse(shared_ptr<ConsensusVariable> cv, short msgID, short receiver,
										unsigned long lamportTime) = 0;

		/*!
		 * Sends a CaceCommand Message to achieve consensus on the consensusvariable 'cv'
		 * Note: Receiver 0 indicates broadcast
		 */
		virtual void sendWriteCommand(shared_ptr<ConsensusVariable> cv, short msgID, short receiver,
										unsigned long lamportTime) = 0;

		/*!
		 * Sends a CaceCommand Message to achieve consensus on the consensusvariable 'cv'
		 */
		virtual void sendCaceCommand(shared_ptr<ConsensusVariable> cv, short msgID, vector<uint8_t>& value,
										short receiver, unsigned long lamportTime) = 0;

		/*!
		 * Sends a CaceCAcknowledge Message to achieve consensus on the consensusvariable 'cv'
		 */
		virtual void sendCaceAcknowledge(string& name, vector<uint8_t>& value, short messageID, short receiver,
											short type, unsigned long lamportTime) = 0;

		/*!
		 * Sends a CaceBelieveNotification Message to achieve consensus on the consensusvariable 'cv'
		 */
		virtual void sendCaceBelieveNotification(shared_ptr<ConsensusVariable> cv, short receiverID, short msgID,
													unsigned long lamportTime) = 0;

		/*!
		 * Sends a ShortAck Message to 'receiver'
		 */
		virtual void sendCaceWriteAck(string& name, short messageID, short receiver, unsigned long lamportTime) = 0;

		/*!
		 * Sends a ShortAck Message to 'receiver'
		 */
		virtual void sendCaceShortAck(string& name, short messageID, short receiver, unsigned long lamportTime) = 0;

		/*!
		 * Sends a time message including current distributed and local time
		 */
		virtual void sendTime(TimeManager* m) = 0;

		/*!
		 * Anounce Disengange
		 */
		virtual void anounceDisengange();

		/*!
		 * Returns ID of this Agent
		 */
		int getOwnID();

		/*!
		 * Sets ID of this Agent
		 */
		void setOwnID(int id);

		/*!
		 * Checks whether 'agentID' is balcklisted
		 */
		bool isBlacklisted(int agentID);

		/*!
		 * Adds 'agentID' to balcklisted
		 */
		void addToBlacklist(int agentID);

		/*!
		 * removes 'agentID' from blacklist
		 */
		void removeFromBlackList(int agentID);

		/*!
		 * Clears all queued messages
		 */
		void clearAllMessageLists();

		/*!
		 * Returns a list of all command messages in Message queue and clears queue
		 */
		list<CaceCommandPtr> getCommands();

		/*!
		 * Returns a list of all Ack messages with MsgID in Message queue and removes them
		 */
		//list<CaceAcknowledgePtr> getAcknowledgesAndRemove(short msgID);

		/*!
		 * Returns a list of all WriteAck messages with MsgID in Message queue and removes them
		 */
		list<CaceShortAckPtr> getWriteAcknowledgesAndRemove(short msgID);

		/*!
		 * Returns a list of all believe notification messages in Message queue and clears queue
		 */
		list<CaceBelieveNotificationPtr> getCaceBelieveNotifications();

		/*!
		 * Returns a list of all ShortAck messages in Message queue and removes them from the queue
		 */
		list<CaceShortAckPtr> getCaceShortAcksAndRemove(short msgID);

		/*!
		 * Returns printable string of message queue states
		 */
		string printMessageQueueStates();

		/*!
		 * Contains a List of Blacklisted agents, which are cared during communication
		 * Note, also includes the ownID, as packages from ourself are ignored
		 */
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
