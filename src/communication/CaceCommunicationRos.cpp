/*
 * CaceCommunicationRos.cpp
 *
 *  Created on: 25.06.2014
 *      Author: endy
 */

#include <communication/CaceCommunicationRos.h>
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

	CaceCommunicationRos::CaceCommunicationRos()
	{
		// TODO Auto-generated constructor stub

	}

	CaceCommunicationRos::~CaceCommunicationRos()
	{
		// TODO Auto-generated destructor stub
	}

	CaceCommunicationRos::CaceCommunicationRos(CommunicationWorker* worker, string& nodePrefix, Cace* cace)
	{
		init(worker, nodePrefix, cace);
		supplementary::SystemConfig* sc = supplementary::SystemConfig::getInstance();

		setOwnID((*sc)["Globals"]->tryGet<int>(-1, "Globals", "Team", sc->getHostname().c_str(), "ID", NULL));
		if (ownID == -1)
		{
			//cout << "ATTENTION!!! OwnID is set to -1!!! ROBOT ID is not in Globals.conf [Globals][Team]!!!" << endl;
		}
	}

	void CaceCommunicationRos::init(CommunicationWorker* worker, string& nodePrefix, Cace* cace)
	{
		this->cace = cace;
		this->worker = worker;

		//RosSharp.Init("Cace", null);
		//rosNode = new Node(nodePrefix+SystemConfig.RobotNodeName("Cace"));
		//RosSharp.Init(nodePrefix+SystemConfig.RobotNodeName("Cace"), null);

		spinner = new ros::AsyncSpinner(4);

		commandPublisher = rosNode.advertise<CaceCommand>("/Cace/CaceCommand", 10);
		ackPublisher = rosNode.advertise<CaceAcknowledge>("/Cace/CaceAcknowledge", 10);
		shortAckPublisher = rosNode.advertise<CaceShortAck>("/Cace/CaceShortAck", 10);
		notificationPublisher = rosNode.advertise<CaceBelieveNotification>("/Cace/CaceBelieveNotification", 10);
		timePublisher = rosNode.advertise<CaceTime>("/Cace/CaceTime", 10);
		varRequestPublisher = rosNode.advertise<CaceVariableRequest>("/Cace/CaceVariableRequest", 10);
		responsePublisher = rosNode.advertise<CaceCommand>("/Cace/CaceResponse", 10);
		writePublisher = rosNode.advertise<CaceCommand>("/Cace/CaceWriteCommand", 10);
		writeAckPublisher = rosNode.advertise<CaceShortAck>("/Cace/CaceWriteAck", 10);
		evalPublisher = rosNode.advertise<std_msgs::String>("/Cace/Evaluation", 10);

		commandSubscriber = rosNode.subscribe("/Cace/CaceCommand", 10, &CaceCommunication::handleCaceCommand, (CaceCommunication*)this);
		ackSubscriber = rosNode.subscribe("/Cace/CaceAcknowledge", 10, &CaceCommunication::handleCaceAcknowledge, (CaceCommunication*)this);
		shortAckSubscriber = rosNode.subscribe("/Cace/CaceShortAck", 10, &CaceCommunication::handleCaceShortAck, (CaceCommunication*)this);
		notificationSubscriber = rosNode.subscribe("/Cace/CaceBelieveNotification", 10,
													&CaceCommunication::handleCaceBelieveNotification, (CaceCommunication*)this);
		timeSubscriber = rosNode.subscribe("/Cace/CaceTime", 10, &CaceCommunication::handleCaceTime, (CaceCommunication*)this);
		varRequestSubscriber = rosNode.subscribe("/Cace/CaceVariableRequest", 10,
													&CaceCommunication::handleCaceVariableRequest, (CaceCommunication*)this);
		writeSubscriber = rosNode.subscribe("/Cace/CaceWriteCommand", 10, &CaceCommunication::handleCaceWrite, (CaceCommunication*)this);
		writeAckSubscriber = rosNode.subscribe("/Cace/CaceWriteAck", 10, &CaceCommunication::handleCaceWriteAck, (CaceCommunication*)this);
		responseSubscriber = rosNode.subscribe("/Cace/CaceResponse", 10, &CaceCommunication::handleCaceResponse, (CaceCommunication*)this);

	}

	void CaceCommunicationRos::startAsynchronous()
	{
		this->spinner->start();
	}

	void CaceCommunicationRos::step()
	{
		ros::spinOnce();
	}

	void CaceCommunicationRos::cleanUp()
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
		rosNode.shutdown();
	}

	void CaceCommunicationRos::sendEvalString(string m)
	{
		std_msgs::String s;
		s.data = m;
		evalPublisher.publish(s);
	}

	void CaceCommunicationRos::sendCaceVariableRequest(short receiverID, short msgID, string& name,
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

	void CaceCommunicationRos::sendCaceResponse(shared_ptr<ConsensusVariable> cv, short msgID, short receiver,
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

	void CaceCommunicationRos::sendWriteCommand(shared_ptr<ConsensusVariable> cv, short msgID, short receiver,
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

	void CaceCommunicationRos::sendCaceCommand(shared_ptr<ConsensusVariable> cv, short msgID, vector<uint8_t>& value,
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

	void CaceCommunicationRos::sendCaceAcknowledge(string& name, vector<uint8_t>& value, short messageID,
													short receiver, short type, unsigned long lamportTime)
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

	void CaceCommunicationRos::sendCaceBelieveNotification(shared_ptr<ConsensusVariable> cv, short receiverID,
															short msgID, unsigned long lamportTime)
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

	void CaceCommunicationRos::sendCaceWriteAck(string& name, short messageID, short receiver,
												unsigned long lamportTime)
	{
		CaceShortAck sa;
		sa.lamportTime = lamportTime;
		sa.msgID = messageID;
		sa.senderID = ownID;
		sa.receiverID = receiver;
		sa.variableName = name;

		writeAckPublisher.publish(sa);
	}

	void CaceCommunicationRos::sendCaceShortAck(string& name, short messageID, short receiver,
												unsigned long lamportTime)
	{
		CaceShortAck sa;
		sa.lamportTime = lamportTime;
		sa.msgID = messageID;
		sa.senderID = ownID;
		sa.receiverID = receiver;
		sa.variableName = name;

		shortAckPublisher.publish(sa);
	}

	void CaceCommunicationRos::sendTime(TimeManager* m)
	{
		CaceTime ct;
		ct.senderID = ownID;
		ct.distributedTime = m->getDistributedTime();
		ct.localtime = m->getLocalTime();
		ct.lamportTime = cace->timeManager->lamportTime++;

		timePublisher.publish(ct);
	}
} /* namespace cace */
