/*
 * caceCommunication.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include <communication/CaceCommunication.h>
#include <communication/jobs/ShortAckJob.h>
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

		setOwnID((*sc)["Globals"]->tryGet<int>(-1,"Globals", "Team", sc->getHostname().c_str(), "ID", NULL));
		if(ownID==-1) {
			//cout << "ATTENTION!!! OwnID is set to -1!!! ROBOT ID is not in Globals.conf [Globals][Team]!!!" << endl;
		}
	}

	void CaceCommunication::init(CommunicationWorker* worker, string& nodePrefix, Cace* cace)
	{
		this->cace = cace;
		this->worker = worker;

		//RosSharp.Init("Cace", null);
		//rosNode = new Node(nodePrefix+SystemConfig.RobotNodeName("Cace"));
		//RosSharp.Init(nodePrefix+SystemConfig.RobotNodeName("Cace"), null);

		rosNode = new ros::NodeHandle();
		spinner = new ros::AsyncSpinner(4);

		commandPublisher = rosNode->advertise<CaceCommand>("/Cace/CaceCommand", 10);
		ackPublisher = rosNode->advertise<CaceAcknowledge>("/Cace/CaceAcknowledge", 10);
		shortAckPublisher = rosNode->advertise<CaceShortAck>("/Cace/CaceShortAck", 10);
		notificationPublisher = rosNode->advertise<CaceBelieveNotification>("/Cace/CaceBelieveNotification", 10);
		timePublisher = rosNode->advertise<CaceTime>("/Cace/CaceTime", 10);
		varRequestPublisher = rosNode->advertise<CaceVariableRequest>("/Cace/CaceVariableRequest", 10);
		responsePublisher = rosNode->advertise<CaceCommand>("/Cace/CaceResponse", 10);
		writePublisher = rosNode->advertise<CaceCommand>("/Cace/CaceWriteCommand", 10);
		writeAckPublisher = rosNode->advertise<CaceShortAck>("/Cace/CaceWriteAck", 10);
		evalPublisher = rosNode->advertise<std_msgs::String>("/Cace/Evaluation", 10);

		commandSubscriber = rosNode->subscribe("/Cace/CaceCommand", 10, &CaceCommunication::handleCaceCommand, this);
		ackSubscriber = rosNode->subscribe("/Cace/CaceAcknowledge", 10, &CaceCommunication::handleCaceAcknowledge,
											this);
		shortAckSubscriber = rosNode->subscribe("/Cace/CaceShortAck", 10, &CaceCommunication::handleCaceShortAck, this);
		notificationSubscriber = rosNode->subscribe("/Cace/CaceBelieveNotification", 10,
													&CaceCommunication::handleCaceBelieveNotification, this);
		timeSubscriber = rosNode->subscribe("/Cace/CaceTime", 10, &CaceCommunication::handleCaceTime, this);
		varRequestSubscriber = rosNode->subscribe("/Cace/CaceVariableRequest", 10,
													&CaceCommunication::handleCaceVariableRequest, this);
		writeSubscriber = rosNode->subscribe("/Cace/CaceWriteCommand", 10, &CaceCommunication::handleCaceWrite, this);
		writeAckSubscriber = rosNode->subscribe("/Cace/CaceWriteAck", 10, &CaceCommunication::handleCaceWriteAck, this);
		responseSubscriber = rosNode->subscribe("/Cace/CaceResponse", 10, &CaceCommunication::handleCaceResponse, this);
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
		ros::spinOnce();
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
		spinner->stop();
		delete spinner;
		commandSubscriber.shutdown();
		ackSubscriber.shutdown();
		shortAckSubscriber.shutdown();
		notificationSubscriber.shutdown();
		timeSubscriber.shutdown();
		varRequestSubscriber.shutdown();
		writeSubscriber.shutdown();
		writeAckSubscriber.shutdown();
		responseSubscriber.shutdown();

		delete rosNode;
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
		std_msgs::String s;
		s.data = m;
		evalPublisher.publish(s);
	}

	void CaceCommunication::sendCaceVariableRequest(short receiverID, short msgID, string& name)
	{
		//sendCaceVariableRequest(receiverID, msgID, name, 0);
		sendCaceVariableRequest(receiverID, msgID, name, cace->timeManager->lamportTime);
	}

	void CaceCommunication::sendCaceVariableRequest(short receiverID, short msgID, string& name,
													unsigned long lamportTime)
	{
		cace->timeManager->lamportTime++;
		CaceVariableRequest cvr;
		cvr.senderID = ownID;
		cvr.receiverID = receiverID;
		cvr.msgID = msgID;
		cvr.variableName = name;

		varRequestPublisher.publish(cvr);
	}

	void CaceCommunication::sendCaceResponse(shared_ptr<ConsensusVariable> cv, short msgID, short receiver)
	{
		cace->timeManager->lamportTime++;
		sendCaceResponse(cv, msgID, receiver, cace->timeManager->lamportTime);
	}

	void CaceCommunication::sendCaceResponse(shared_ptr<ConsensusVariable> cv, short msgID, short receiver,
												unsigned long lamportTime)
	{
		CaceCommand cc;
		cc.decissionTime = cv->getDecissionTime();
		cc.lamportTime = lamportTime;
		cc.level = cv->getAcceptStrategy();
		cc.msgID = (short)msgID;
		cc.validityTime = cv->getValidityTime();
		cc.value = cv->getValue();
		cc.variableName = cv->getName();
		cc.senderID = ownID;
		//Broadcast
		cc.receiverID = receiver;
		cc.type = (char)cv->getType();

		responsePublisher.publish(cc);
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

	void CaceCommunication::sendWriteCommand(shared_ptr<ConsensusVariable> cv, short msgID, short receiver,
												unsigned long lamportTime)
	{
		CaceCommand cc;
		cc.decissionTime = cv->getDecissionTime();
		cc.lamportTime = lamportTime;
		cc.level = cv->getAcceptStrategy();
		cc.msgID = (short)msgID;
		cc.validityTime = cv->getValidityTime();
		cc.value = cv->getValue();
		cc.variableName = cv->getName();
		cc.senderID = ownID;
		cc.receiverID = receiver;
		cc.type = (char)cv->getType();

		writePublisher.publish(cc);
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
			lock_guard<std::mutex> lock(cmdMutex);
			{
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

	void CaceCommunication::sendCaceCommand(shared_ptr<ConsensusVariable> cv, short msgID, vector<uint8_t>& value,
											short receiver, unsigned long lamportTime)
	{
		CaceCommand cc;
		cc.decissionTime = cv->getDecissionTime();
		cc.lamportTime = lamportTime;
		cc.level = cv->getAcceptStrategy();
		cc.msgID = (short)msgID;
		cc.validityTime = cv->getValidityTime();
		cc.value = value;
		cc.variableName = cv->getName();
		cc.senderID = ownID;
		//Broadcast
		cc.receiverID = receiver;
		cc.type = (char)cv->getType();

		commandPublisher.publish(cc);
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
			//Worker.AppendJob(new ShortAckJob(ca.VariableName, null, null, cace.TimeManager.LamportTime, cace, ca));
			worker->appendJob(
					(AbstractCommunicationJob*)new ShortAckJob(ca->variableName, ptr, empty,
																cace->timeManager->lamportTime, cace, ca));
			lock_guard<std::mutex> lock(ackMutex);
			acknowledges.push_back(ca);
		}
	}

	list<CaceAcknowledgePtr> CaceCommunication::getAcknowledgesAndRemove(short msgID)
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
	}

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

	void CaceCommunication::sendCaceAcknowledge(string& name, vector<uint8_t>& value, short messageID, short receiver,
												short type, unsigned long lamportTime)
	{
		CaceAcknowledge ca;

		ca.lamportTime = lamportTime;
		ca.msgID = messageID;
		ca.senderID = ownID;
		ca.receiverID = receiver;
		ca.type = (char)type;

		//is This a problem?
		ca.value = value;
		ca.variableName = name;

		ackPublisher.publish(ca);
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

	void CaceCommunication::sendCaceBelieveNotification(shared_ptr<ConsensusVariable> cv, short receiverID, short msgID,
														unsigned long lamportTime)
	{
		CaceBelieveNotification cbn;
		cbn.decissionTime = cv->getDecissionTime();
		cbn.lamportTime = lamportTime;
		cbn.level = cv->getAcceptStrategy();
		cbn.msgID = msgID;
		cbn.validityTime = cv->getValidityTime();
		cbn.value = cv->getValue();
		cbn.variableName = cv->getName();
		cbn.senderID = ownID;
		cbn.receiverID = receiverID;
		cbn.type = (char)cv->getType();

		notificationPublisher.publish(cbn);
	}

	void CaceCommunication::sendCaceWriteAck(string& name, short messageID, short receiver)
	{
		cace->timeManager->lamportTime++;
		sendCaceShortAck(name, messageID, receiver, cace->timeManager->lamportTime);
	}

	void CaceCommunication::sendCaceWriteAck(string& name, short messageID, short receiver, unsigned long lamportTime)
	{
		CaceShortAck sa;
		sa.lamportTime = lamportTime;
		sa.msgID = messageID;
		sa.senderID = ownID;
		sa.receiverID = receiver;
		sa.variableName = name;

		writeAckPublisher.publish(sa);
	}

	void CaceCommunication::sendCaceShortAck(string& name, short messageID, short receiver)
	{
		cace->timeManager->lamportTime++;
		sendCaceShortAck(name, messageID, receiver, cace->timeManager->lamportTime);
	}

	void CaceCommunication::sendCaceShortAck(string& name, short messageID, short receiver, unsigned long lamportTime)
	{
		CaceShortAck sa;
		sa.lamportTime = lamportTime;
		sa.msgID = messageID;
		sa.senderID = ownID;
		sa.receiverID = receiver;
		sa.variableName = name;

		shortAckPublisher.publish(sa);
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

	void CaceCommunication::sendTime(TimeManager* m)
	{
		CaceTime ct;
		ct.senderID = ownID;
		ct.distributedTime = m->getDistributedTime();
		ct.localtime = m->getLocalTime();
		ct.lamportTime = cace->timeManager->lamportTime++;

		timePublisher.publish(ct);
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
		return "Commands: " + to_string(commands.size()) + "\tAcks: " + to_string(acknowledges.size()) + "\tShortAcks: " + to_string(shortAcks.size()) + "\tNotifications: " + to_string(believeNotifications.size());
	}
} /* namespace cace */
