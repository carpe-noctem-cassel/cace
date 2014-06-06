/*
 * CaceCommunicationQuiet.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef CACECOMMUNICATIONQUIET_H_
#define CACECOMMUNICATIONQUIET_H_

#include <cace/CaceAcknowledge.h>
#include <cace/CaceCommand.h>
#include <variables/ConsensusVariable.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace std;

namespace cace
{
	class Cace;
	class CommunicationWorker;

	class CaceCommunicationQuiet : public CaceCommunication
	{
	public:
		CaceCommunicationQuiet(CommunicationWorker* worker, string& nodePrefix, Cace* cace);
		virtual ~CaceCommunicationQuiet();

		void sendCaceCommand(shared_ptr<ConsensusVariable> cv, short msgID, vector<uint8_t> value, short receiver);
		void sendCaceBelieveNotification(shared_ptr<ConsensusVariable> cv, short receiverID, short msgID, unsigned long lamportTime);
		void sendCaceAcknowledge(string name, vector<uint8_t> value, short messageID, short receiver, short type, unsigned long lamportTime);
		void sendCaceShortAck(string name, short messageID, short receiver, unsigned long lamportTime);
		void sendTime(TimeManager* m);

		void handleCaceCommand(CaceCommandPtr cc);
		void handleCaceAcknowledge(CaceAcknowledgePtr ca);

	};

} /* namespace cace */

#endif /* CACECOMMUNICATIONQUIET_H_ */
