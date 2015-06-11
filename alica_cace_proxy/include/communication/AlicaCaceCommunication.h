/*
 * AlicaRosCommunication.h
 *
 *  Created on: 10.09.2014
 *      Author: endy
 */

#ifndef ALICACACECOMMUNICATION_H_
#define ALICACACECOMMUNICATION_H_

#include "engine/IAlicaCommunication.h"
#include "ros/ros.h"
#include <cace.h>
#include <variables/ConsensusVariable.h>

using namespace alica;
using namespace cace;

namespace alicaCaceProxy
{

	class AlicaCaceCommunication : public alica::IAlicaCommunication
	{
	public:
		AlicaCaceCommunication(AlicaEngine* ae, Cace* cace);
		virtual ~AlicaCaceCommunication();

		virtual void tick();

		virtual void sendAllocationAuthority(AllocationAuthorityInfo& aai);
		virtual void sendAlicaEngineInfo(AlicaEngineInfo& bi);
		virtual void sendPlanTreeInfo(PlanTreeInfo& pti);
		virtual void sendRoleSwitch(RoleSwitch& rs);
		virtual void sendSyncReady(SyncReady& sr);
		virtual void sendSyncTalk(SyncTalk& st);
		virtual void sendSolverResult(SolverResult& sr);

		virtual void handleAllocationAuthorityCace(ConsensusVariable* aai);
		virtual void handlePlanTreeInfoCace(ConsensusVariable* pti);
		virtual void handleSyncReadyCace(ConsensusVariable* sr);
		virtual void handleSyncTalkCace(ConsensusVariable* st);

		virtual void startCommunication();
		virtual void stopCommunication();

	protected:
		int ownID;
		Cace* cace;

		bool isRunning;
	};

} /* namespace alicaRosProxy */

#endif /* ALICAROSCOMMUNICATION_H_ */
