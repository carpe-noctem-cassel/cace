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

		/*!
		 * Distributed a Variable value
		 * Overwrites local value if existent
		 * Note: 'value' should be a serialized value of 'type'
		 */
		void distributeValue(string name, vector<uint8_t> value, short type, acceptStrategy strategy);

		/*!
		 * Returns a requested variable of another robot if present
		 */
		shared_ptr<ConsensusVariable> getRequestedVariable(string name);

		/*!
		 * Checks whether a response to a request is available
		 */
		bool checkAvailableResponse(string& name);

		/*!
		 * Requests a variable at an agent with 'agentID'
		 */
		void requestVariable(string& name, short agentID);

		/*!
		 * Writes a variable in the local variable store of ageht 'agentID'
		 */
		void writeVariable(shared_ptr<ConsensusVariable> cv, short agentID);

		/*!
		 * Returns the probability of 'agentID' having received
		 * the own believe of variable with name 'name'
		 */
		double getProbabilityForBelieveConsistency(string& name, int agentID);

		/*!
		 * Distributed a Variable value
		 * Overwrites local value if existent
		 */
		void distributeValue(string name, double value, acceptStrategy consensusLevel);
		bool getVariableValue(string name, double* value);

		/*!
		 * Distributed a Variable value
		 * Overwrites local value if existent
		 */
		void distributeValue(string name, int value, acceptStrategy consensusLevel);
		bool getVariableValue(string name, int* value);

		/*!
		 * Returns variable 'name'
		 * Attention: check for null. Prefer direct querying for variable value.
		 */
		shared_ptr<ConsensusVariable> getVariable(string& name);

		/*!
		 * Adds / overrides a Variable to the local variable store
		 * 'distribute' enforces consistency w.r.t. the consistency level
		 */
		bool addVariable(shared_ptr<ConsensusVariable> var, bool distribute);

		/*!
		 * Distributes a variable 'var' and adds to store if necessary
		 */
		void distributeVariable(shared_ptr<ConsensusVariable>& var);

	protected:
		CVariableStore* store;
		CommunicationWorker* worker;
		Cace* cace;
		int ownID;

	};

} /* namespace cace */

#endif /* CACESPACE_H_ */
