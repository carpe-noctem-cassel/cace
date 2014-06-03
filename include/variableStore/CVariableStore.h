/*
 * CVariableStore.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef CVARIABLESTORE_H_
#define CVARIABLESTORE_H_
#include <map>
#include <mutex>
#include <memory>

#include "../variables/ConsensusVariable.h"
#include "../communication/CommunicationWorker.h"
#include "../timeManager/TimeManager.h"

using namespace std;


namespace cace
{

	class CVariableStore
	{
	public:
		CVariableStore(Cace* cace);
		virtual ~CVariableStore();

		bool addVariable(shared_ptr<ConsensusVariable> var);
		void addResponse(shared_ptr<ConsensusVariable> var);
		bool existsVariable(string& name);
		bool existsResponse(string& name);
		void deleteVariable(string& name);
		void deleteResponse(string& name);
		shared_ptr<ConsensusVariable> getVariable(string& name);
		shared_ptr<ConsensusVariable> getResponse(string& name);
		void deleteAllVariables();

		void sendBelieveUpdates(int id, string& currentScope, CommunicationWorker& worker, unsigned long lamportTime);
		void invalidate(TimeManager& timeManager);

		string toString();
		string toStringNoNewLine();
		string toStringNoNewLine(string& context);


	protected:
		Cace* cace;
		map<string, shared_ptr<ConsensusVariable>> store;
		vector<shared_ptr<ConsensusVariable>> responses;
		mutex responseMutex;
		mutex storeMutex;
	};

} /* namespace cace */

#endif /* CVARIABLESTORE_H_ */
