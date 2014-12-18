/*
 * CaceVariableSync.cpp
 *
 *  Created on: 04.12.2014
 *      Author: endy
 */

#include "constraintmodul/CaceVariableSync.h"

#include <cace/CaceType.h>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "caceSpace.h"
#include "communication/CaceCommunication.h"
#include "CaceTypes.h"
#include "timeManager/TimeManager.h"
#include "variables/ConsensusVariable.h"
#include "variableStore/CVariableStore.h"

using namespace cace;

namespace alicaCaceProxy
{
	CaceVariableSync::CaceVariableSync(cace::Cace* cace) :
			cace(cace)
	{
	}

	CaceVariableSync::~CaceVariableSync()
	{
	}

	void CaceVariableSync::init()
	{
	}

	void CaceVariableSync::close()
	{
	}

	void CaceVariableSync::clear()
	{
		cace->variableStore->deleteAllVariables();
	}

	void CaceVariableSync::onSolverResult(shared_ptr<SolverResult> msg)
	{
	}

	void CaceVariableSync::postResult(long vid, shared_ptr<vector<uint8_t>>& result)
	{
		uint8_t type = CaceType::Custom;
		acceptStrategy strategy = acceptStrategy::FireAndForgetElection;

		auto v1 = make_shared<ConsensusVariable>(std::to_string(vid), strategy, std::numeric_limits<long>::max(),
													cace->communication->getOwnID(),
													cace->timeManager->getDistributedTime(),
													cace->timeManager->lamportTime, type);
		v1->setValue(*result);

		cace->caceSpace->distributeVariable(v1);
	}

	shared_ptr<vector<shared_ptr<vector<shared_ptr<vector<uint8_t>>>>>> CaceVariableSync::getSeeds(
			shared_ptr<vector<Variable*> > query, shared_ptr<vector<shared_ptr<vector<double> > > > limits)
	{
		int dim = query->size();
		auto ret = make_shared<vector<shared_ptr<vector<shared_ptr<vector<uint8_t>>>>>>(dim);
		return ret;
	}

} /* namespace alicaCaceProxy */
