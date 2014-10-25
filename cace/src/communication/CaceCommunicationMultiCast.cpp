/*
 * CaceCommunicationMultiCast.cpp
 *
 *  Created on: 23.06.2014
 *      Author: endy
 */

#include <boost/smart_ptr/shared_array.hpp>
#include <cace/CaceAcknowledge.h>
#include <cace/CaceBelieveNotification.h>
#include <cace/CaceCommand.h>
#include <cace/CaceShortAck.h>
#include <cace/CaceTime.h>
#include <cace/CaceVariableRequest.h>
#include <cace.h>
#include <communication/CaceCommunicationMultiCast.h>
#include <Configuration.h>
#include <ros/serialization.h>
#include <SystemConfig.h>
#include <timeManager/TimeManager.h>
#include <variables/ConsensusVariable.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ser = ros::serialization;

using namespace cacemulticast;

namespace cace
{

	void CaceCommunicationMultiCast::cleanUp()
	{
	}

	CaceCommunicationMultiCast::CaceCommunicationMultiCast()
	{
	}

	CaceCommunicationMultiCast::CaceCommunicationMultiCast(CommunicationWorker* worker, string& nodePrefix, Cace* cace) //:
	//CaceCommunication(worker, nodePrefix, cace)
	{
		init(worker, nodePrefix, cace);
		supplementary::SystemConfig* sc = supplementary::SystemConfig::getInstance();

		setOwnID((*sc)["Globals"]->tryGet<int>(-1, "Globals", "Team", sc->getHostname().c_str(), "ID", NULL));
		if (ownID == -1)
		{
			//cout << "ATTENTION!!! OwnID is set to -1!!! ROBOT ID is not in Globals.conf [Globals][Team]!!!" << endl;
		}
	}

	void CaceCommunicationMultiCast::init(CommunicationWorker* worker, string& nodePrefix, Cace* cace)
	{
		this->cace = cace;
		this->worker = worker;

		port = 50001;
		commandChannel = new CaceMultiCastChannel<CaceCommunicationMultiCast>(
				addr, port++, &CaceCommunicationMultiCast::mhandleCaceCommand, this);
		ackChannel = new CaceMultiCastChannel<CaceCommunicationMultiCast>(
				addr, port++, &CaceCommunicationMultiCast::mhandleCaceAcknowledge, this);
		shortChannel = new CaceMultiCastChannel<CaceCommunicationMultiCast>(
				addr, port++, &CaceCommunicationMultiCast::mhandleCaceShortAck, this);
		notificationChannel = new CaceMultiCastChannel<CaceCommunicationMultiCast>(
				addr, port++, &CaceCommunicationMultiCast::mhandleCaceBelieveNotification, this);
		timeChannel = new CaceMultiCastChannel<CaceCommunicationMultiCast>(
				addr, port++, &CaceCommunicationMultiCast::mhandleCaceTime, this);
		varRequestChannel = new CaceMultiCastChannel<CaceCommunicationMultiCast>(
				addr, port++, &CaceCommunicationMultiCast::mhandleCaceVariableRequest, this);
		writeChannel = new CaceMultiCastChannel<CaceCommunicationMultiCast>(
				addr, port++, &CaceCommunicationMultiCast::mhandleCaceWrite, this);
		writeAckChannel = new CaceMultiCastChannel<CaceCommunicationMultiCast>(
				addr, port++, &CaceCommunicationMultiCast::mhandleCaceWriteAck, this);
		responseChannel = new CaceMultiCastChannel<CaceCommunicationMultiCast>(
				addr, port++, &CaceCommunicationMultiCast::mhandleCaceResponse, this);
		evalChannel = nullptr;
	}

	CaceCommunicationMultiCast::~CaceCommunicationMultiCast()
	{
		delete commandChannel;
		delete ackChannel;
		delete shortChannel;
		delete notificationChannel;
		delete timeChannel;
		delete varRequestChannel;
		delete writeChannel;
		delete writeAckChannel;
		delete responseChannel;
	}

	void CaceCommunicationMultiCast::mhandleCaceVariableRequest(char* buffer, int size)
	{
		CaceVariableRequest* value = new CaceVariableRequest();
		CaceVariableRequestPtr valueptr(value);
		//uint32_t serial_size = ros::serialization::serializationLength(value);
		ser::IStream stream((uint8_t*)buffer, size);
		ser::Serializer<CaceVariableRequest>::read(stream, *value);
		//ser::deserialize(stream, *value);
		handleCaceVariableRequest(valueptr);
	}

	void CaceCommunicationMultiCast::mhandleCaceResponse(char* buffer, int size)
	{
		CaceCommand* value = new CaceCommand();
		CaceCommandPtr valueptr(value);
		//uint32_t serial_size = ros::serialization::serializationLength(value);
		ser::IStream stream((uint8_t*)buffer, size);
		ser::Serializer<CaceCommand>::read(stream, *value);
		//ser::deserialize(stream, *value);
		handleCaceResponse(valueptr);
	}

	void CaceCommunicationMultiCast::mhandleCaceWrite(char* buffer, int size)
	{
		CaceCommand* value = new CaceCommand();
		CaceCommandPtr valueptr(value);
		//uint32_t serial_size = ros::serialization::serializationLength(value);
		ser::IStream stream((uint8_t*)buffer, size);
		ser::Serializer<CaceCommand>::read(stream, *value);
		//ser::deserialize(stream, *value);
		handleCaceWrite(valueptr);
	}

	void CaceCommunicationMultiCast::mhandleCaceAcknowledge(char* buffer, int size)
	{
		CaceAcknowledge* value = new CaceAcknowledge();
		CaceAcknowledgePtr valueptr(value);
		//uint32_t serial_size = ros::serialization::serializationLength(value);
		ser::IStream stream((uint8_t*)buffer, size);
		ser::Serializer<CaceAcknowledge>::read(stream, *value);
		//ser::deserialize(stream, *value);
		handleCaceAcknowledge(valueptr);
	}

	void CaceCommunicationMultiCast::mhandleCaceWriteAck(char* buffer, int size)
	{
		CaceShortAck* value = new CaceShortAck();
		CaceShortAckPtr valueptr(value);
		//uint32_t serial_size = ros::serialization::serializationLength(value);
		ser::IStream stream((uint8_t*)buffer, size);
		ser::Serializer<CaceShortAck>::read(stream, *value);
		//ser::deserialize(stream, *value);
		handleCaceWriteAck(valueptr);
	}

	void CaceCommunicationMultiCast::mhandleCaceCommand(char* buffer, int size)
	{
		CaceCommand* value = new CaceCommand();
		CaceCommandPtr valueptr(value);
		//uint32_t serial_size = ros::serialization::serializationLength(value);
		ser::IStream stream((uint8_t*)buffer, size);
		ser::Serializer<CaceCommand>::read(stream, *value);
		//ser::deserialize(stream, *value);
		handleCaceCommand(valueptr);
	}

	void CaceCommunicationMultiCast::mhandleCaceBelieveNotification(char* buffer, int size)
	{
		CaceBelieveNotification* value = new CaceBelieveNotification();
		CaceBelieveNotificationPtr valueptr(value);
		//uint32_t serial_size = ros::serialization::serializationLength(value);
		ser::IStream stream((uint8_t*)buffer, size);
		ser::Serializer<CaceBelieveNotification>::read(stream, *value);
		//ser::deserialize(stream, *value);
		handleCaceBelieveNotification(valueptr);
	}

	void CaceCommunicationMultiCast::mhandleCaceShortAck(char* buffer, int size)
	{
		CaceShortAck* value = new CaceShortAck();
		CaceShortAckPtr valueptr(value);
		//uint32_t serial_size = ros::serialization::serializationLength(value);
		ser::IStream stream((uint8_t*)buffer, size);
		ser::Serializer<CaceShortAck>::read(stream, *value);
		//ser::deserialize(stream, *value);
		handleCaceShortAck(valueptr);
	}

	void CaceCommunicationMultiCast::mhandleCaceTime(char* buffer, int size)
	{
		CaceTime* value = new CaceTime();
		CaceTimePtr valueptr(value);
		//uint32_t serial_size = ros::serialization::serializationLength(value);
		ser::IStream stream((uint8_t*)buffer, size);
		ser::Serializer<CaceTime>::read(stream, *value);
		//ser::deserialize(stream, *value);
		handleCaceTime(valueptr);
	}

	void CaceCommunicationMultiCast::sendEvalString(string m)
	{
	}

	void CaceCommunicationMultiCast::sendCaceVariableRequest(short receiverID, short msgID, string& name,
																unsigned long lamportTime)
	{
		cace->timeManager->lamportTime++;
		CaceVariableRequest val;
		val.senderID = ownID;
		//val.receiverID = receiverID;
		val.receiverID.push_back(receiverID);
		val.msgID = msgID;
		val.variableName = name;

		uint32_t serial_size = ros::serialization::serializationLength(val);
		boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);

		ser::OStream stream(buffer.get(), serial_size);
		ser::serialize(stream, val);
		varRequestChannel->publish((char*)buffer.get(), serial_size);
	}

	void CaceCommunicationMultiCast::sendCaceResponse(shared_ptr<ConsensusVariable> cv, short msgID, short receiver,
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
		//cc.receiverID = receiver;
		cc.receiverID.push_back(receiver);
		cc.type = (char)cv->getType();

		uint32_t serial_size = ros::serialization::serializationLength(cc);
		boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);

		ser::OStream stream(buffer.get(), serial_size);
		ser::serialize(stream, cc);
		responseChannel->publish((char*)buffer.get(), serial_size);
	}

	void CaceCommunicationMultiCast::sendWriteCommand(shared_ptr<ConsensusVariable> cv, short msgID, short receiver,
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
		//cc.receiverID = receiver;
		cc.receiverID.push_back(receiver);
		cc.type = (char)cv->getType();

		uint32_t serial_size = ros::serialization::serializationLength(cc);
		boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);

		ser::OStream stream(buffer.get(), serial_size);
		ser::serialize(stream, cc);
		writeChannel->publish((char*)buffer.get(), serial_size);
	}

	void CaceCommunicationMultiCast::sendCaceCommand(shared_ptr<ConsensusVariable> cv, short msgID,
														vector<uint8_t>& value, short receiver,
														unsigned long lamportTime)
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
		//cc.receiverID = receiver;
		cc.receiverID.push_back(receiver);
		cc.type = (char)cv->getType();

		uint32_t serial_size = ros::serialization::serializationLength(cc);
		boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);

		ser::OStream stream(buffer.get(), serial_size);
		ser::serialize(stream, cc);
		commandChannel->publish((char*)buffer.get(), serial_size);
	}

	void CaceCommunicationMultiCast::sendCaceAcknowledge(string& name, vector<uint8_t>& value, short messageID,
															short receiver, short type, unsigned long lamportTime)
	{
		CaceAcknowledge ca;

		ca.lamportTime = lamportTime;
		ca.msgID = messageID;
		ca.senderID = ownID;
		//ca.receiverID = receiver;
		ca.receiverID.push_back(receiver);
		ca.type = (char)type;

		//is This a problem?
		ca.value = value;
		ca.variableName = name;

		uint32_t serial_size = ros::serialization::serializationLength(ca);
		boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);

		ser::OStream stream(buffer.get(), serial_size);
		ser::serialize(stream, ca);
		ackChannel->publish((char*)buffer.get(), serial_size);
	}

	void CaceCommunicationMultiCast::sendCaceBelieveNotification(shared_ptr<ConsensusVariable> cv, short receiverID,
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
		//cbn.receiverID = receiverID;
		cbn.receiverID.push_back(receiverID);
		cbn.type = (char)cv->getType();

		uint32_t serial_size = ros::serialization::serializationLength(cbn);
		boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);

		ser::OStream stream(buffer.get(), serial_size);
		ser::serialize(stream, cbn);
		notificationChannel->publish((char*)buffer.get(), serial_size);
	}

	void CaceCommunicationMultiCast::sendCaceWriteAck(string& name, short messageID, short receiver,
														unsigned long lamportTime)
	{
		CaceShortAck sa;
		sa.lamportTime = lamportTime;
		sa.msgID = messageID;
		sa.senderID = ownID;
		//sa.receiverID = receiver;
		sa.receiverID.push_back(receiver);
		sa.variableName = name;

		uint32_t serial_size = ros::serialization::serializationLength(sa);
		boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);

		ser::OStream stream(buffer.get(), serial_size);
		ser::serialize(stream, sa);
		writeAckChannel->publish((char*)buffer.get(), serial_size);
	}

	void CaceCommunicationMultiCast::sendCaceShortAck(string& name, short messageID, short receiver,
														unsigned long lamportTime)
	{
		CaceShortAck sa;
		sa.lamportTime = lamportTime;
		sa.msgID = messageID;
		sa.senderID = ownID;
		//sa.receiverID = receiver;
		sa.receiverID.push_back(receiver);
		sa.variableName = name;

		uint32_t serial_size = ros::serialization::serializationLength(sa);
		boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);

		ser::OStream stream(buffer.get(), serial_size);
		ser::serialize(stream, sa);
		shortChannel->publish((char*)buffer.get(), serial_size);
	}

	void CaceCommunicationMultiCast::sendTime(TimeManager* m)
	{
		CaceTime ct;
		ct.senderID = ownID;
		ct.distributedTime = m->getDistributedTime();
		ct.localtime = m->getLocalTime();
		ct.lamportTime = cace->timeManager->lamportTime++;

		uint32_t serial_size = ros::serialization::serializationLength(ct);
		boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);

		ser::OStream stream(buffer.get(), serial_size);
		ser::serialize(stream, ct);
		timeChannel->publish((char*)buffer.get(), serial_size);
	}

} /* namespace cace */

