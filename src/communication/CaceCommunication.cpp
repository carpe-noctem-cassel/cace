/*
 * caceCommunication.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include <communication/CaceCommunication.h>
#include <communication/jobs/ShortAckJob.h>
#include <communication/jobs/CommandAcknowledgeJob.h>
#include <Configuration.h>
#include <ros/node_handle.h>
#include <ros/publisher.h>
#include <ros/subscriber.h>
#include <SystemConfig.h>
#include <timeManager/TimeManager.h>
#include <variableStore/CVariableStore.h>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace cace
{

	CaceCommunication::CaceCommunication()
	{
	}

	CaceCommunication::~CaceCommunication()
	{
	}

	CaceCommunication::CaceCommunication(CommunicationWorker* worker, string& nodePrefix, Cace* cace)
	{
		init(worker, nodePrefix, cace);
		supplementary::SystemConfig* sc = supplementary::SystemConfig::getInstance();

		setOwnID((*sc)["Globals"]->tryGet<int>(-1, "Globals", "Team", sc->getHostname().c_str(), "ID", NULL));
		if (ownID == -1)
		{
			//cout << "ATTENTION!!! OwnID is set to -1!!! ROBOT ID is not in Globals.conf [Globals][Team]!!!" << endl;
		}
	}

	void CaceCommunication::init(CommunicationWorker* worker, string& nodePrefix, Cace* cace)
	{
		this->cace = cace;
		this->worker = worker;
	}

	void CaceCommunication::startAsynchronous()
	{
	}

	int CaceCommunication::getOwnID()
	{
		return ownID;
	}

	void CaceCommunication::setOwnID(int id)
	{
		lock_guard<std::mutex> lock(blacklistMutex);
		set<int>::iterator i = agentBlacklist.find(ownID);
		if (i != agentBlacklist.end())
		{
			agentBlacklist.erase(i);
		}

		agentBlacklist.insert(id);
		ownID = id;
	}

	void CaceCommunication::step()
	{
	}

	bool CaceCommunication::isBlacklisted(int agentID)
	{
		lock_guard<std::mutex> lock(blacklistMutex);
		set<int>::iterator i = agentBlacklist.find(agentID);
		if (i != agentBlacklist.end())
		{
			return true;
		}
		return false;
	}

	void CaceCommunication::addToBlacklist(int agentID)
	{
		lock_guard<std::mutex> lock(blacklistMutex);
		agentBlacklist.insert(agentID);
	}

	void CaceCommunication::removeFromBlackList(int agentID)
	{
		lock_guard<std::mutex> lock(blacklistMutex);
		set<int>::iterator i = agentBlacklist.find(agentID);
		if (i != agentBlacklist.end())
		{
			agentBlacklist.erase(i);
			return;
		}
	}

	void CaceCommunication::cleanUp()
	{
	}

	void CaceCommunication::clearAllMessageLists()
	{
		{
			lock_guard<std::mutex> lock(cmdMutex);
			commands.clear();
		}
		{
			lock_guard<std::mutex> lock(writeMutex);
			writeCommands.clear();
		}
		{
			lock_guard<std::mutex> lock(ackMutex);
			acknowledges.clear();
		}
		{
			lock_guard<std::mutex> lock(wackMutex);
			writeAcknowledges.clear();
		}
		{
			lock_guard<std::mutex> lock(sackMutex);
			shortAcks.clear();
		}
		{
			lock_guard<std::mutex> lock(notMutex);
			believeNotifications.clear();
		}
	}

	void CaceCommunication::sendEvalString(string m)
	{
	}

	void CaceCommunication::sendCaceVariableRequest(short receiverID, short msgID, string& name)
	{
		//sendCaceVariableRequest(receiverID, msgID, name, 0);
		sendCaceVariableRequest(receiverID, msgID, name, cace->timeManager->lamportTime);
	}

	void CaceCommunication::sendCaceResponse(shared_ptr<ConsensusVariable> cv, short msgID, short receiver)
	{
		cace->timeManager->lamportTime++;
		sendCaceResponse(cv, msgID, receiver, cace->timeManager->lamportTime);
	}

	void CaceCommunication::handleCaceVariableRequest(CaceVariableRequestPtr cvr)
	{
		//cout << "---------------------------received request" << endl;
		if (isBlacklisted(cvr->senderID))
			return;
		//0 is broadcast

		if (cvr->receiverID == 0 || cvr->receiverID == ownID)
		{
			if (cace->variableStore->existsVariable(cvr->variableName))
			{
				auto cv = cace->variableStore->getVariable(cvr->variableName);
				sendCaceResponse(cv, cvr->msgID, cvr->senderID);
			}
			else
			{
				auto cv = make_shared<ConsensusVariable>(cvr->variableName, acceptStrategy::NoDistribution, 0, ownID, 0,
															0, 0);
				sendCaceResponse(cv, cvr->msgID, cvr->senderID);
			}
		}
	}

	void CaceCommunication::handleCaceResponse(CaceCommandPtr cc)
	{
		if (isBlacklisted(cc->senderID))
		{
			return;
		}
		cace->timeManager->updateLamportTime(cc->lamportTime);
		//0 is broadcast
		if (cc->receiverID == 0 || cc->receiverID == ownID)
		{
			auto cv = make_shared<ConsensusVariable>(cc->variableName, (acceptStrategy)cc->level, cc->validityTime,
														cc->senderID, cc->decissionTime, cc->lamportTime,
														(short)cc->type);
			if (cc->value.size() > 0)
			{
				cv->setValue(cc->value);
				cv->setArrivalTime(cace->timeManager->getLocalTime());
			}
			cace->variableStore->addResponse(cv);
		}
	}

	void CaceCommunication::handleCaceWrite(CaceCommandPtr cc)
	{
		//Console.WriteLine("OwnID "+OwnID+" Sender: " + cc.SenderID + " Receiver " +  cc.ReceiverID);
		if (isBlacklisted(cc->senderID))
		{
			return;
		}
		cace->timeManager->updateLamportTime(cc->lamportTime);
		//0 is broadcast
		if (cc->receiverID == 0 || cc->receiverID == ownID)
		{
			shared_ptr<ConsensusVariable> ptr;
			vector<int> empty;
			worker->appendJob(
					(AbstractCommunicationJob*)new WriteAckJob(cc->variableName, ptr, empty,
																cace->timeManager->lamportTime, cace, cc));
		}
	}

	void CaceCommunication::sendWriteCommand(shared_ptr<ConsensusVariable> cv, short msgID, short receiver)
	{
		cace->timeManager->lamportTime++;
		sendWriteCommand(cv, msgID, receiver, cace->timeManager->lamportTime);
	}

	void CaceCommunication::handleCaceWriteAck(CaceShortAckPtr ca)
	{
		if (isBlacklisted(ca->senderID))
		{
			return;
		}
		cace->timeManager->updateLamportTime(ca->lamportTime);
		if (ca->receiverID == 0 || ca->receiverID == ownID)
		{
			lock_guard<std::mutex> lock(wackMutex);
			{
				writeAcknowledges.push_back(ca);
			}
		}
	}

	void CaceCommunication::handleCaceCommand(CaceCommandPtr cc)
	{
		//Console.WriteLine("OwnID "+OwnID+" Sender: " + cc.SenderID + " Receiver " +  cc.ReceiverID);
		if (isBlacklisted(cc->senderID))
		{
			return;
		}
		cace->timeManager->updateLamportTime(cc->lamportTime);
		//0 is broadcast
		if (cc->receiverID == 0 || cc->receiverID == ownID)
		{
			//lock_guard<std::mutex> lock(cmdMutex);
			{
				vector<int> nall = cace->activeRobots;
				shared_ptr<ConsensusVariable> np;
				//worker->appendJob(
				//		new CommandAcknowledgeJob(cc->variableName, np, nall, cace->timeManager->lamportTime, cace, cc));
				commands.push_back(cc);
			}
		}
	}

	list<CaceCommandPtr> CaceCommunication::getCommands()
	{
		list<CaceCommandPtr> ret;
		lock_guard<std::mutex> lock(cmdMutex);
		for (CaceCommandPtr cc : commands)
		{
			ret.push_back(cc);
		}
		commands.clear();
		return ret;
	}

	void CaceCommunication::sendCaceCommand(shared_ptr<ConsensusVariable> cv, short msgID, vector<uint8_t>& value,
											short receiver)
	{
		cace->timeManager->lamportTime++;
		sendCaceCommand(cv, msgID, value, receiver, cace->timeManager->lamportTime);
	}

	void CaceCommunication::handleCaceAcknowledge(CaceAcknowledgePtr ca)
	{
		//Console.WriteLine("Ca:"+ca.SenderID+" "+OwnID+" R: "+ca.ReceiverID + " ID " +ca.MsgID + " LT: " +ca.LamportTime);
		if (isBlacklisted(ca->senderID))
		{
			return;
		}
		cace->timeManager->updateLamportTime(ca->lamportTime);
		//Can this be done somewhere else?
		if (ca->receiverID == 0 || ca->receiverID == ownID)
		{
			shared_ptr<ConsensusVariable> ptr;
			vector<int> empty;
			lock_guard<std::mutex> lock(ackMutex);
			//acknowledges.push_back(ca);

			worker->appendJob(
					(AbstractCommunicationJob*)new ShortAckJob(ca->variableName, ptr, empty,
																cace->timeManager->lamportTime, cace, ca));
		}
	}

	/*list<CaceAcknowledgePtr> CaceCommunication::getAcknowledgesAndRemove(short msgID)
	 {
	 list<CaceAcknowledgePtr> ret;
	 lock_guard<std::mutex> lock(ackMutex);
	 list<CaceAcknowledgePtr>::iterator it = acknowledges.begin();
	 while (it != acknowledges.end())
	 {
	 if ((*it)->msgID == msgID)
	 {
	 ret.push_back(*it);
	 list<CaceAcknowledgePtr>::iterator tmp = it;
	 it--;
	 acknowledges.erase(tmp);
	 }
	 it++;
	 }
	 return ret;
	 }*/

	list<CaceShortAckPtr> CaceCommunication::getWriteAcknowledgesAndRemove(short msgID)
	{
		list<CaceShortAckPtr> ret;
		lock_guard<std::mutex> lock(wackMutex);
		list<CaceShortAckPtr>::iterator it = writeAcknowledges.begin();
		while (it != writeAcknowledges.end())
		{
			if ((*it)->msgID == msgID)
			{
				ret.push_back(*it);
				list<CaceShortAckPtr>::iterator tmp = it;
				it--;
				writeAcknowledges.erase(tmp);
			}
			it++;
		}
		return ret;
	}

	void CaceCommunication::sendCaceAcknowledge(string& name, vector<uint8_t>& value, short messageID, short receiver,
												short type)
	{
		cace->timeManager->lamportTime++;
		sendCaceAcknowledge(name, value, messageID, receiver, type, cace->timeManager->lamportTime);
	}

	void CaceCommunication::handleCaceBelieveNotification(CaceBelieveNotificationPtr cbn)
	{
		if (isBlacklisted(cbn->senderID))
		{
			return;
		}
		cace->timeManager->updateLamportTime(cbn->lamportTime);
		if (cbn->receiverID == 0 || cbn->receiverID == ownID)
		{
			lock_guard<std::mutex> lock(notMutex);
			{
				believeNotifications.push_back(cbn);
			}
		}
	}

	list<CaceBelieveNotificationPtr> CaceCommunication::getCaceBelieveNotifications()
	{
		list<CaceBelieveNotificationPtr> ret;
		lock_guard<std::mutex> lock(notMutex);
		for (CaceBelieveNotificationPtr cc : believeNotifications)
		{
			ret.push_back(cc);
		}
		believeNotifications.clear();

		return ret;
	}

	void CaceCommunication::sendCaceBelieveNotification(shared_ptr<ConsensusVariable> cv, short receiverID, short msgID)
	{
		cace->timeManager->lamportTime++;
		sendCaceBelieveNotification(cv, receiverID, msgID, cace->timeManager->lamportTime);
	}

	void CaceCommunication::sendCaceWriteAck(string& name, short messageID, short receiver)
	{
		cace->timeManager->lamportTime++;
		sendCaceShortAck(name, messageID, receiver, cace->timeManager->lamportTime);
	}

	void CaceCommunication::sendCaceShortAck(string& name, short messageID, short receiver)
	{
		cace->timeManager->lamportTime++;
		sendCaceShortAck(name, messageID, receiver, cace->timeManager->lamportTime);
	}

	void CaceCommunication::handleCaceShortAck(CaceShortAckPtr sa)
	{
		//Console.WriteLine("Ca:"+sa.SenderID+" "+OwnID+" R: "+sa.ReceiverID + " ID " +sa.MsgID);
		//sometomes we are missing stuff here! oO
		//Console.WriteLine(sa.SenderID + " " + sa.ReceiverID+" " + sa.MsgID);
		if (isBlacklisted(sa->senderID))
		{
			return;
		}

		cace->timeManager->updateLamportTime(sa->lamportTime);
		if (sa->receiverID == 0 || sa->receiverID == ownID)
		{
			lock_guard<std::mutex> lock(sackMutex);
			{
				shortAcks.push_back(sa);
			}
		}
	}

	void CaceCommunication::handleCaceTime(CaceTimePtr ct)
	{
		ctime current = cace->timeManager->getLocalTime();
		if (isBlacklisted(ct->senderID))
		{
			return;
		}
		cace->timeManager->updateLamportTime(ct->lamportTime);
		cace->timeManager->addTimeMessage(ct, current);

		bool found = false;
		for (int i : cace->activeRobots)
		{
			if (i == ct->senderID)
			{
				found = true;
			}
		}
		if (!found)
		{
			cace->agentEngangement(ct->senderID, true);
		}
	}

	list<CaceShortAckPtr> CaceCommunication::getCaceShortAcksAndRemove(short msgID)
	{
		list<CaceShortAckPtr> ret;
		lock_guard<std::mutex> lock(wackMutex);
		list<CaceShortAckPtr>::iterator it = shortAcks.begin();
		while (it != shortAcks.end())
		{
			if ((*it)->msgID == msgID)
			{
				ret.push_back(*it);
				list<CaceShortAckPtr>::iterator tmp = it;
				it--;
				shortAcks.erase(tmp);
			}
			it++;
		}
		return ret;
	}

	void CaceCommunication::anounceDisengange()
	{
	}

	string CaceCommunication::printMessageQueueStates()
	{
		lock_guard<std::mutex> lock1(cmdMutex);
		lock_guard<std::mutex> lock2(ackMutex);
		lock_guard<std::mutex> lock3(sackMutex);
		lock_guard<std::mutex> lock4(notMutex);
		return "Commands: " + to_string(commands.size()) + "\tAcks: " + to_string(acknowledges.size()) + "\tShortAcks: "
				+ to_string(shortAcks.size()) + "\tNotifications: " + to_string(believeNotifications.size());
	}
} /* namespace cace */
