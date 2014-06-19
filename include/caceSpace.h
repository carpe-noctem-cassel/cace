/*
 * caceSpace.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef CACESPACE_H_
#define CACESPACE_H_

#include <CaceTypes.h>
#include <variables/ConsensusVariable.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace std;

namespace cace
{
	class CVariableStore;
	class Cace;
	class CommunicationWorker;

	class CaceSpace
	{
	public:
		CaceSpace(CVariableStore* store, CommunicationWorker* worker, Cace* cace);
		virtual ~CaceSpace();

		void distributeValue(string name, vector<uint8_t> value, short type, acceptStrategy strategy);
		shared_ptr<ConsensusVariable> getRequestedVariable(string name);
		bool checkAvailableResponse(string& name);
		void requestVariable(string& name, short agentID);
		void writeVariable(shared_ptr<ConsensusVariable> cv, short agentID);
		double getProbabilityForBelieveConsistency(string& name, int agentID);

		void distributeValue(string name, double value, acceptStrategy consensusLevel);
		bool getVariableValue(string name, double* value);
		void distributeValue(string name, int value, acceptStrategy consensusLevel);
		bool getVariableValue(string name, int* value);

		shared_ptr<ConsensusVariable> getVariable(string& name);
		bool addVariable(shared_ptr<ConsensusVariable> var, bool distribute);
		void distributeVariable(shared_ptr<ConsensusVariable> var);

	protected:
		CVariableStore* store;
		CommunicationWorker* worker;
		Cace* cace;
		int ownID;

	};

} /* namespace cace */

#endif /* CACESPACE_H_ */
