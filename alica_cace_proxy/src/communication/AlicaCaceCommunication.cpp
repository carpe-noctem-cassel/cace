/*
 * AlicaCaceCommunication.cpp
 *
 *  Created on: 10.09.2014
 *      Author: endy
 */

#include "communication/AlicaCaceCommunication.h"

#include "engine/containers/AllocationAuthorityInfo.h"
#include "engine/containers/BehaviourEngineInfo.h"
#include "engine/containers/PlanTreeInfo.h"
#include "engine/containers/RoleSwitch.h"
#include "engine/containers/SyncReady.h"
#include "engine/containers/SyncTalk.h"
#include "engine/containers/SyncData.h"

#include <ros/node_handle.h>
#include <ros/publisher.h>
#include <ros/subscriber.h>
#include <SystemConfig.h>
#include <Configuration.h>

#include <communication/CaceCommunication.h>
#include <timeManager/TimeManager.h>
#include <caceSpace.h>

using namespace alica;
using namespace cace;

namespace alicaCaceProxy
{

	AlicaCaceCommunication::AlicaCaceCommunication(AlicaEngine* ae, Cace* cace) :
			IAlicaCommunication(ae), cace(cace) //, rosNode()
	{
		this->isRunning = false;

		uint8_t type = CaceType::Custom;
		acceptStrategy strategy = acceptStrategy::NoDistribution;

		auto v1 = make_shared<ConsensusVariable>("AllocationAuthorityInfo", strategy, std::numeric_limits<long>::max(),
													cace->communication->getOwnID(),
													cace->timeManager->getDistributedTime(),
													cace->timeManager->lamportTime, type);
		cace->caceSpace->addVariable(v1, false);
		v1->changeNotify.push_back(
				delegate<void(ConsensusVariable*)>(this, &AlicaCaceCommunication::handleAllocationAuthorityCace));

		v1 = make_shared<ConsensusVariable>("PlanTreeInfo", strategy, std::numeric_limits<long>::max(),
											cace->communication->getOwnID(), cace->timeManager->getDistributedTime(),
											cace->timeManager->lamportTime, type);
		cace->caceSpace->addVariable(v1, false);
		v1->changeNotify.push_back(
				delegate<void(ConsensusVariable*)>(this, &AlicaCaceCommunication::handlePlanTreeInfoCace));

		v1 = make_shared<ConsensusVariable>("SyncTalk", strategy, std::numeric_limits<long>::max(),
											cace->communication->getOwnID(), cace->timeManager->getDistributedTime(),
											cace->timeManager->lamportTime, type);
		cace->caceSpace->addVariable(v1, false);
		v1->changeNotify.push_back(
				delegate<void(ConsensusVariable*)>(this, &AlicaCaceCommunication::handleSyncTalkCace));

		v1 = make_shared<ConsensusVariable>("SyncReady", strategy, std::numeric_limits<long>::max(),
											cace->communication->getOwnID(), cace->timeManager->getDistributedTime(),
											cace->timeManager->lamportTime, type);
		cace->caceSpace->addVariable(v1, false);
		v1->changeNotify.push_back(
				delegate<void(ConsensusVariable*)>(this, &AlicaCaceCommunication::handleSyncReadyCace));
	}

	AlicaCaceCommunication::~AlicaCaceCommunication()
	{
		cace->halt();
	}

	void AlicaCaceCommunication::tick()
	{
		if (this->isRunning)
		{
			ros::spinOnce();
		}
	}

	void AlicaCaceCommunication::sendAllocationAuthority(AllocationAuthorityInfo& aai)
	{
		auto v1 = make_shared<ConsensusVariable>("AllocationAuthorityInfo", acceptStrategy::FireAndForget,
													std::numeric_limits<long>::max(), cace->communication->getOwnID(),
													cace->timeManager->getDistributedTime(),
													cace->timeManager->lamportTime, CaceType::Custom);
		v1->setValue(aai.toStandard());

		if (this->isRunning)
		{
			cace->caceSpace->distributeVariable(v1);
		}
	}

	void AlicaCaceCommunication::sendBehaviourEngineInfo(BehaviourEngineInfo& bi)
	{
		auto v1 = make_shared<ConsensusVariable>("BehaviourEngineInfo", acceptStrategy::FireAndForget,
													std::numeric_limits<long>::max(), cace->communication->getOwnID(),
													cace->timeManager->getDistributedTime(),
													cace->timeManager->lamportTime, CaceType::Custom);
		v1->setValue(bi.toStandard());

		if (this->isRunning)
		{
			cace->caceSpace->distributeVariable(v1);
		}
	}

	void AlicaCaceCommunication::sendPlanTreeInfo(PlanTreeInfo& pti)
	{
		auto v1 = make_shared<ConsensusVariable>("PlanTreeInfo", acceptStrategy::FireAndForget,
													std::numeric_limits<long>::max(), cace->communication->getOwnID(),
													cace->timeManager->getDistributedTime(),
													cace->timeManager->lamportTime, CaceType::Custom);
		v1->setValue(pti.toStandard());

		if (this->isRunning)
		{
			cace->caceSpace->distributeVariable(v1);
		}
	}

	void AlicaCaceCommunication::sendRoleSwitch(RoleSwitch& rs)
	{
		auto v1 = make_shared<ConsensusVariable>("RoleSwitch", acceptStrategy::FireAndForget,
													std::numeric_limits<long>::max(), cace->communication->getOwnID(),
													cace->timeManager->getDistributedTime(),
													cace->timeManager->lamportTime, CaceType::Custom);
		v1->setValue(rs.roleID);

		if (this->isRunning)
		{
			cace->caceSpace->distributeVariable(v1);
		}
	}

	void AlicaCaceCommunication::sendSyncReady(SyncReady& sr)
	{
		auto v1 = make_shared<ConsensusVariable>("SyncReady", acceptStrategy::FireAndForget,
													std::numeric_limits<long>::max(), cace->communication->getOwnID(),
													cace->timeManager->getDistributedTime(),
													cace->timeManager->lamportTime, CaceType::Custom);
		v1->setValue(sr.toStandard());

		if (this->isRunning)
		{
			cace->caceSpace->distributeVariable(v1);
		}
	}

	void AlicaCaceCommunication::sendSyncTalk(SyncTalk& st)
	{
		auto v1 = make_shared<ConsensusVariable>("SyncTalk", acceptStrategy::FireAndForget,
													std::numeric_limits<long>::max(), cace->communication->getOwnID(),
													cace->timeManager->getDistributedTime(),
													cace->timeManager->lamportTime, CaceType::Custom);
		v1->setValue(st.toStandard());

		if (this->isRunning)
		{
			cace->caceSpace->distributeVariable(v1);
		}
	}

	void AlicaCaceCommunication::sendSolverResult(SolverResult& sr) {

	}

	void AlicaCaceCommunication::handleAllocationAuthorityCace(ConsensusVariable* aai)
	{
		stdAllocationAuthorityInfo* saai = new stdAllocationAuthorityInfo();
		if (aai->getValue<stdAllocationAuthorityInfo>(*saai))
		{
			auto aaiPtr = make_shared<AllocationAuthorityInfo>(*saai);

			if (this->isRunning)
			{
				this->onAuthorityInfoReceived(aaiPtr);
			}
		}
	}

	void AlicaCaceCommunication::handlePlanTreeInfoCace(ConsensusVariable* pti)
	{
		stdPlanTreeInfo* spti = new stdPlanTreeInfo();
		if (pti->getValue<stdPlanTreeInfo>(*spti))
		{
			auto ptiPtr = make_shared<PlanTreeInfo>(*spti);

			if (this->isRunning)
			{
				this->onPlanTreeInfoReceived(ptiPtr);
			}
		}
	}

	void AlicaCaceCommunication::handleSyncReadyCace(ConsensusVariable* sr)
	{
		stdSyncReady* ssr = new stdSyncReady();
		if (sr->getValue<stdSyncReady>(*ssr))
		{
			auto srPtr = make_shared<SyncReady>(*ssr);

			if (this->isRunning)
			{
				this->onSyncReadyReceived(srPtr);
			}
		}
	}

	void AlicaCaceCommunication::handleSyncTalkCace(ConsensusVariable* st)
	{
		stdSyncTalk* sst = new stdSyncTalk();
		if (st->getValue<stdSyncTalk>(*sst))
		{
			auto stPtr = make_shared<SyncTalk>(*sst);

			if (this->isRunning)
			{
				this->onSyncTalkReceived(stPtr);
			}
		}
	}

	void AlicaCaceCommunication::startCommunication()
	{
		this->isRunning = true;
		cace->run();
	}
	void AlicaCaceCommunication::stopCommunication()
	{
		this->isRunning = false;
		cace->halt();
	}

} /* namespace alicaRosProxy */
