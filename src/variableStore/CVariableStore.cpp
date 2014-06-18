/*
 * CVariableStore.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include "variableStore/CVariableStore.h"

namespace cace
{

	CVariableStore::CVariableStore(Cace* cace)
	{
		this->cace = cace;
	}

	CVariableStore::~CVariableStore()
	{
	}

	bool CVariableStore::addVariable(shared_ptr<ConsensusVariable> var)
	{
		lock_guard<std::mutex> lock(storeMutex);
		if (store.find(var->getName()) == store.end())
		{
			store[var->getName()] = var;
			return true;
		}
		return false;
	}

	void CVariableStore::addResponse(shared_ptr<ConsensusVariable> var)
	{
		lock_guard<std::mutex> lock(responseMutex);
		responses.push_back(var);
	}

	bool CVariableStore::existsVariable(string& name)
	{
		lock_guard<std::mutex> lock(storeMutex);
		return store.find(name) != store.end();
	}

	bool CVariableStore::existsResponse(string& name)
	{
		lock_guard<std::mutex> lock(responseMutex);
		for (shared_ptr<ConsensusVariable> v : responses)
		{
			if (v->getName() == name)
			{
				return true;
			}
		}
		return false;
	}

	void CVariableStore::deleteVariable(string& name)
	{
		lock_guard<std::mutex> lock(storeMutex);
		if (store.find(name) != store.end())
		{
			store.erase(name);
		}
	}

	void CVariableStore::deleteResponse(string& name)
	{
		lock_guard<std::mutex> lock(responseMutex);
		for (int i = 0; i < responses.size(); i++)
		{
			shared_ptr<ConsensusVariable> v = responses.at(i);
			if (v->getName() == name)
			{
				responses.erase(responses.begin() + i);
			}
		}
	}
	shared_ptr<ConsensusVariable> CVariableStore::getVariable(string& name)
	{
		lock_guard<std::mutex> lock(storeMutex);
		if (store.find(name) != store.end())
		{
			return store[name];
		}
		shared_ptr<ConsensusVariable> np;
		return np;
	}

	shared_ptr<ConsensusVariable> CVariableStore::getResponse(string& name)
	{
		lock_guard<std::mutex> lock(responseMutex);
		for (int i = 0; i < responses.size(); i++)
		{
			shared_ptr<ConsensusVariable> v = responses.at(i);
			if (v->getName() == name)
			{
				responses.erase(responses.begin() + i);
				return v;
			}
		}
		return nullptr;
	}

	void CVariableStore::deleteAllVariables()
	{
		lock_guard<std::mutex> lock(storeMutex);
		store.clear();
	}

	void CVariableStore::sendBelieveUpdates(int id, string& currentScope, CommunicationWorker& worker,
											unsigned long lamportTime)
	{
		lock_guard<std::mutex> lock(storeMutex);
		map<string, shared_ptr<ConsensusVariable>>::iterator it;
		for (it = store.begin(); it != store.end(); it++)
		{
			shared_ptr<ConsensusVariable> var = it->second;
			if (currentScope.find(var->getScope()) == 0 && var->hasValue)
			{ //localcontext.StartsWith(var.GetLocalContext())
				unsigned long lastKnownUpdateTime = 0;
				for (ConsensusVariable* c : var->proposals)
				{
					if (c->getRobotID() == id)
					{
						lastKnownUpdateTime = c->getLamportAge();
						break;
					}
				}
				//only send an update if the variable changed since we saw that robot last time
				if (lastKnownUpdateTime < var->getLamportAge()
						&& var->getAcceptStrategy() > acceptStrategy::FireAndForget)
				{
					vector<int> tmp;
					tmp.push_back(id);
					cace->worker->appendJob(new BelieveNotificationJob(var->getName(), var, var->getValue(), tmp, lamportTime, cace));
				}
			}
		}
	}

	void CVariableStore::invalidate(TimeManager& timeManager)
	{
		lock_guard<std::mutex> lock(storeMutex);
		map<string, shared_ptr<ConsensusVariable>>::iterator it;
		for (it = store.begin(); it != store.end(); it++)
		{
			shared_ptr<ConsensusVariable> var = it->second;
			if (var->getValidityTime() < timeManager.getDistributedTime())
			{
				var->hasValue = false;
			}
		}
	}

	string CVariableStore::toString()
	{
		lock_guard<std::mutex> lock(storeMutex);
		string ret = "Entries: " + to_string(store.size()) + "\n";
		map<string, shared_ptr<ConsensusVariable>>::iterator it;
		for (it = store.begin(); it != store.end(); it++)
		{
			ret += it->second->toString() + "\n";
		}
		return ret;
	}

	string CVariableStore::toStringNoNewLine()
	{
		lock_guard<std::mutex> lock(storeMutex);
		string ret = "Entries: " + to_string((int) store.size()) + "\t";
		map<string, shared_ptr<ConsensusVariable>>::iterator it;
		for (it = store.begin(); it != store.end(); it++)
		{
			ret += "> " + it->second->toString() + " <\t";
		}
		return ret;
	}

	string CVariableStore::toString(string& context)
	{
		lock_guard<std::mutex> lock(storeMutex);
		string ret = "";
		map<string, shared_ptr<ConsensusVariable>>::iterator it;
		for (it = store.begin(); it != store.end(); it++)
		{
			if (it->second->getScope() == context)
			{
				ret += it->second->toString() + "\n";
			}
		}
		return ret;
	}

} /* namespace cace */
