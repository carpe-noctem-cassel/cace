/*
 * CaceCommunicationQuiet.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include "communication/CaceCommunicationQuiet.h"

namespace cace
{
	CaceCommunicationQuiet::CaceCommunicationQuiet(CommunicationWorker* worker, string& nodePrefix, Cace* cace)
	{
		init(worker, nodePrefix, cace);
		if (ownID != 0)
			this->ownID = ownID;
		else
		{
			//this->ownID = (short)SystemConfig.GetOwnRobotID();
		}
	}

	CaceCommunicationQuiet::~CaceCommunicationQuiet()
	{
	}

	void CaceCommunicationQuiet::sendCaceCommand(shared_ptr<ConsensusVariable> cv, short msgID, vector<uint8_t> value,
													short receiver)
	{
	}

	void CaceCommunicationQuiet::sendCaceBelieveNotification(shared_ptr<ConsensusVariable> cv, short receiverID,
																short msgID, unsigned long lamportTime)
	{
	}

	void CaceCommunicationQuiet::sendCaceAcknowledge(string name, vector<uint8_t> value, short messageID,
														short receiver, short type, unsigned long lamportTime)
	{
	}

	void CaceCommunicationQuiet::sendCaceShortAck(string name, short messageID, short receiver,
													unsigned long lamportTime)
	{
	}

	void CaceCommunicationQuiet::sendTime(TimeManager* m)
	{
	}

	void CaceCommunicationQuiet::handleCaceCommand(CaceCommandPtr cc)
	{
		cc->lamportTime = cace->timeManager->lamportTime+1;
		CaceCommunication::handleCaceCommand(cc);
	}

	void CaceCommunicationQuiet::handleCaceAcknowledge(CaceAcknowledgePtr ca)
	{
		ca->lamportTime = cace->timeManager->lamportTime+1;
		CaceCommunication::handleCaceAcknowledge(ca);
	}

} /* namespace cace */
