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

using namespace alica;

namespace alicaRosProxy
{

	AlicaCaceCommunication::AlicaCaceCommunication(AlicaEngine* ae) :
			IAlicaCommunication(ae) //, rosNode()
	{
		this->isRunning = false;
		rosNode = new ros::NodeHandle();
		spinner = new ros::AsyncSpinner(4);


		//supplementary::SystemConfig* sc = supplementary::SystemConfig::getInstance();

		/*ownID = ((*sc)["Globals"]->tryGet<int>(-1, "Globals", "Team", sc->getHostname().c_str(), "ID", NULL));
		 if (ownID == -1)
		 {
		 cout << "ATTENTION!!! OwnID is set to -1!!! ROBOT ID is not in Globals.conf [Globals][Team]!!!" << endl;
		 }*/

		AllocationAuthorityInfoPublisher = rosNode->advertise<alica_ros_proxy::AllocationAuthorityInfo>(
				"/AlicaEngine/AllocationAuthorityInfo", 2);
		AllocationAuthorityInfoSubscriber = rosNode->subscribe("/AlicaEngine/AllocationAuthorityInfo", 10,
																&AlicaCaceCommunication::handleAllocationAuthorityRos,
																(AlicaCaceCommunication*)this);

		BehaviourEngineInfoPublisher = rosNode->advertise<alica_ros_proxy::BehaviourEngineInfo>(
				"/AlicaEngine/BehaviourEngineInfo", 2);
		RoleSwitchPublisher = rosNode->advertise<alica_ros_proxy::SyncTalk>("/AlicaEngine/OwnRole", 10);

		PlanTreeInfoPublisher = rosNode->advertise<alica_ros_proxy::PlanTreeInfo>("/AlicaEngine/PlanTreeInfo", 10);
		PlanTreeInfoSubscriber = rosNode->subscribe("/AlicaEngine/PlanTreeInfo", 1,
													&AlicaCaceCommunication::handlePlanTreeInfoRos,
													(AlicaCaceCommunication*)this);

		SyncReadyPublisher = rosNode->advertise<alica_ros_proxy::SyncTalk>("/AlicaEngine/SyncTalk", 10);
		SyncReadySubscriber = rosNode->subscribe("/AlicaEngine/SyncTalk", 5, &AlicaCaceCommunication::handleSyncTalkRos,
												(AlicaCaceCommunication*)this);
		SyncTalkPublisher = rosNode->advertise<alica_ros_proxy::SyncReady>("/AlicaEngine/SyncReady", 10);
		SyncTalkSubscriber = rosNode->subscribe("/AlicaEngine/SyncReady", 5, &AlicaCaceCommunication::handleSyncReadyRos,
												(AlicaCaceCommunication*)this);

	}

	AlicaCaceCommunication::~AlicaCaceCommunication()
	{
		if(this->isRunning)
		{
			spinner->stop();
		}
		delete spinner;

		AllocationAuthorityInfoSubscriber.shutdown();
		RoleSwitchPublisher.shutdown();
		PlanTreeInfoSubscriber.shutdown();
		SyncReadySubscriber.shutdown();
		SyncTalkSubscriber.shutdown();
		rosNode->shutdown();
		delete rosNode;

	}

	void AlicaCaceCommunication::tick()
	{
		if(this->isRunning)
		{
			ros::spinOnce();
		}
	}

	void AlicaCaceCommunication::sendAllocationAuthority(AllocationAuthorityInfo& aai)
	{
		alica_ros_proxy::AllocationAuthorityInfo aais;
		aais.senderID = aai.senderID;
		aais.planID = aai.planId;
		aais.parentState = aai.parentState;
		aais.planType = aai.planType;
		aais.authority = aai.authority;
		for (auto &ep : aai.entryPointRobots)
		{
			alica_ros_proxy::EntryPointRobots newEP;
			newEP.entryPoint = ep.entrypoint;
			for (int i : ep.robots)
			{
				newEP.robots.push_back(i);
			}

			aais.entrypoints.push_back(newEP);
		}

		if(this->isRunning)
		{
			this->AllocationAuthorityInfoPublisher.publish(aais);
		}
	}

	void AlicaCaceCommunication::sendBehaviourEngineInfo(BehaviourEngineInfo& bi)
	{
		alica_ros_proxy::BehaviourEngineInfo bis;
		bis.currentPlan = bi.currentPlan;
		bis.currentRole = bi.currentRole;
		bis.currentState = bi.currentState;
		bis.currentTask = bi.currentTask;
		bis.masterPlan = bi.masterPlan;
		for (int i : bi.robotIDsWithMe)
		{
			bis.robotIDsWithMe.push_back(i);
		}
		bis.senderID = bi.senderID;

		if(this->isRunning)
		{
			this->BehaviourEngineInfoPublisher.publish(bis);
		}
	}

	void AlicaCaceCommunication::sendPlanTreeInfo(PlanTreeInfo& pti)
	{
		alica_ros_proxy::PlanTreeInfo ptis;
		ptis.senderID = pti.senderID;
		for (long i : pti.stateIDs)
		{
			ptis.stateIDs.push_back(i);
		}
		for (long i : pti.succeededEPs)
		{
			ptis.succeededEps.push_back(i);
		}

		if(this->isRunning)
		{
			this->PlanTreeInfoPublisher.publish(ptis);
		}
	}

	void AlicaCaceCommunication::sendRoleSwitch(RoleSwitch& rs)
	{
		alica_ros_proxy::RoleSwitch rss;

		rss.roleID = rs.roleID;
		rss.senderID = rs.roleID;

		if(this->isRunning)
		{
			this->RoleSwitchPublisher.publish(rss);
		}
	}

	void AlicaCaceCommunication::sendSyncReady(SyncReady& sr)
	{
		alica_ros_proxy::SyncReady srs;

		srs.senderID = sr.senderID;
		srs.syncTransitionID = sr.syncTransitionID;

		if(this->isRunning)
		{
			this->RoleSwitchPublisher.publish(srs);
		}
	}

	void AlicaCaceCommunication::sendSyncTalk(SyncTalk& st)
	{
		alica_ros_proxy::SyncTalk sts;

		sts.senderID = st.senderID;
		for (auto sd : st.syncData)
		{
			alica_ros_proxy::SyncData sds;
			sds.ack = sd->ack;
			sds.conditionHolds = sd->conditionHolds;
			sds.robotID = sd->robotID;
			sds.transitionID = sd->transitionID;
			sts.syncData.push_back(sds);
		}

		if(this->isRunning)
		{
			this->SyncTalkPublisher.publish(sts);
		}
	}

	void AlicaCaceCommunication::handleAllocationAuthorityRos(alica_ros_proxy::AllocationAuthorityInfoPtr aai)
	{
		auto aaiPtr = make_shared<AllocationAuthorityInfo>();
		aaiPtr->senderID = aai->senderID;
		aaiPtr->planId = aai->planID;
		aaiPtr->parentState = aai->parentState;
		aaiPtr->planType = aai->planType;
		aaiPtr->authority = aai->authority;
		for (auto &ep : aai->entrypoints)
		{
			alica::EntryPointRobots newEP;
			newEP.entrypoint = ep.entryPoint;
			newEP.robots = ep.robots;

			aaiPtr->entryPointRobots.push_back(newEP);
		}

		if(this->isRunning)
		{
			this->onAuthorityInfoReceived(aaiPtr);
		}
	}

	void AlicaCaceCommunication::handlePlanTreeInfoRos(alica_ros_proxy::PlanTreeInfoPtr pti)
	{
		auto ptiPtr = make_shared<PlanTreeInfo>();
		ptiPtr->senderID = pti->senderID;
		for (long i : pti->stateIDs)
		{
			ptiPtr->stateIDs.push_back(i);
		}
		for (long i : pti->succeededEps)
		{
			ptiPtr->succeededEPs.push_back(i);
		}

		if(this->isRunning)
		{
			this->onPlanTreeInfoReceived(ptiPtr);
		}
	}

	void AlicaCaceCommunication::handleSyncReadyRos(alica_ros_proxy::SyncReadyPtr sr)
	{
		auto srPtr = make_shared<SyncReady>();

		srPtr->senderID = sr->senderID;
		srPtr->syncTransitionID = sr->syncTransitionID;

		if(this->isRunning)
		{
			this->onSyncReadyReceived(srPtr);
		}
	}

	void AlicaCaceCommunication::handleSyncTalkRos(alica_ros_proxy::SyncTalkPtr st)
	{
		auto stPtr = make_shared<SyncTalk>();

		stPtr->senderID = st->senderID;
		for (auto &sd : st->syncData)
		{
			SyncData* sds = new SyncData();
			sds->ack = sd.ack;
			sds->conditionHolds = sd.conditionHolds;
			sds->robotID = sd.robotID;
			sds->transitionID = sd.transitionID;
			stPtr->syncData.push_back(sds);
		}

		if(this->isRunning)
		{
			this->onSyncTalkReceived(stPtr);
		}
	}

	void AlicaCaceCommunication::startCommunication() {
		this->isRunning = true;
		spinner->start();
	}
	void AlicaCaceCommunication::stopCommunication() {
		this->isRunning = false;
		spinner->stop();
	}

} /* namespace alicaRosProxy */
