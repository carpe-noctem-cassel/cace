#include <cace.h>
#include <caceSpace.h>
#include <communication/CaceCommunication.h>
#include <communication/CommunicationWorker.h>
#include <communication/jobs/CommandJob.h>
#include <communication/jobs/RequestJob.h>
#include <communication/jobs/WriteJob.h>
#include <timeManager/TimeManager.h>
#include <variableStore/CVariableStore.h>
#include <limits>

namespace cace
{

	CaceSpace::CaceSpace(CVariableStore* store, CommunicationWorker* worker, Cace* cace)
	{
		this->cace = cace;
		this->worker = worker;
		this->store = store;
		ownID = cace->communication->ownID;
	}

	CaceSpace::~CaceSpace()
	{
	}

	void CaceSpace::distributeValue(string name, vector<uint8_t> value, short type, acceptStrategy strategy)
	{
		shared_ptr<ConsensusVariable> variable;
		if (!store->existsVariable(name))
		{
			variable = make_shared<ConsensusVariable>(name, strategy, std::numeric_limits<long>::max(), ownID,
														cace->timeManager->getDistributedTime(),
														cace->timeManager->lamportTime, type);
			variable->setValue(value);
			store->addVariable(variable);
		}
		else
		{
			variable = store->getVariable(name);
			variable->setValue(value);
			variable->setType(type);
			variable->setLamportAge(cace->timeManager->lamportTime);
			variable->setAcceptStrategy(strategy);
			variable->setDecissionTime(cace->timeManager->getDistributedTime());
		}
		vector<int> all = *cace->getActiveRobots();
		worker->appendJob(new CommandJob(name, variable, value, all, cace->timeManager->lamportTime, cace));
	}

	shared_ptr<ConsensusVariable> CaceSpace::getRequestedVariable(string name)
	{
		return store->getResponse(name);
	}

	bool CaceSpace::checkAvailableResponse(string& name)
	{
		return store->existsResponse(name);
	}

	void CaceSpace::requestVariable(string& name, short agentID)
	{
		vector<int> agent;
		agent.push_back(agentID);
		store->deleteResponse(name);
		shared_ptr<ConsensusVariable> np;
		worker->appendJob(new RequestJob(name, np, agent, cace->timeManager->lamportTime, cace));
	}

	void CaceSpace::writeVariable(shared_ptr<ConsensusVariable> cv, short agentID)
	{
		vector<int> agent;
		agent.push_back(agentID);
		worker->appendJob(new WriteJob(cv->getName(), cv, cv->getValue(), agent, cace->timeManager->lamportTime, cace));
	}

	double CaceSpace::getProbabilityForBelieveConsistency(string& name, int agentID)
	{
		if (!store->existsVariable(name))
			return 0.0;
		shared_ptr<ConsensusVariable> variable = store->getVariable(name);
		if (variable->getAcceptStrategy() <= acceptStrategy::NoDistribution)
		{
			return 0.0;
		}
		if (variable->getAcceptStrategy() <= acceptStrategy::FireAndForget)
		{
			return cace->timeManager->getEstimatedPacketLoss(agentID);
		}

		AbstractCommunicationJob* job = worker->getNewestVariableJob(name, variable->getAcceptStrategy());
		if (job == nullptr)
		{
			return 1.0;
		}
		bool contains = false;
		for (int i : job->expectedRobotIDs)
		{
			if (i == agentID)
			{
				contains = true;
			}
		}
		if (!contains)
		{
			return 1.0;
		}

		unsigned long curTime = cace->timeManager->getDistributedTime();
		unsigned long timeDiff = curTime - job->startTime;
		return cace->timeManager->getPropabilityforConsensus(agentID, timeDiff);
	}

	void CaceSpace::distributeValue(string name, double value, acceptStrategy strategy)
	{
		char* p = (char*)&value;
		vector<uint8_t> send;
		for (int i = 0; i < sizeof(double); i++)
		{
			send.push_back(*p);
			p++;
		}
		distributeValue(name, send, CaceType::CDouble, strategy);
	}

	bool CaceSpace::getVariableValue(string name, double* value)
	{
		shared_ptr<ConsensusVariable> variable;
		variable = store->getVariable(name);

		if (variable.operator bool() && variable->hasValue)
		{
			variable->getValue(value);
		}
		else
		{
			value = nullptr;
		}
		return variable.operator bool();

	}

	shared_ptr<ConsensusVariable> CaceSpace::getVariable(string& name)
	{
		return store->getVariable(name);
	}

	bool CaceSpace::addVariable(shared_ptr<ConsensusVariable> var, bool distribute)
	{
		bool ret = store->addVariable(var);

		if (distribute)
		{
			vector<int> all = *cace->getActiveRobots();
			worker->appendJob(
					new CommandJob(var->getName(), var, var->getValue(), all, cace->timeManager->lamportTime, cace));
		}
		return ret;
	}

	void CaceSpace::distributeVariable(shared_ptr<ConsensusVariable> var)
	{
		if (store->existsVariable(var->getName()))
		{
			shared_ptr<ConsensusVariable> inStoreVar = store->getVariable(var->getName());
			if (inStoreVar == var)
			{
				vector<int> all = *cace->getActiveRobots();
				worker->appendJob(new CommandJob(var->getName(), var, var->getValue(), all, cace->timeManager->lamportTime, cace));
			}
			else
			{
				cout <<	"Cannot distribute Variables, when a variable with the same key already exists." << endl;
			}
		}
		else
		{
			addVariable(var, true);
		}
	}

}
