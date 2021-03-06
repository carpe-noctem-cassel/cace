/*
 * test_alica_multi_agent_plan.cpp
 *
 *  Created on: Oct 13, 2014
 *      Author: Stefan Jakob
 */

#include <gtest/gtest.h>
#include <engine/AlicaEngine.h>
#include <engine/IAlicaClock.h>
#include "engine/IAlicaCommunication.h"
#include "engine/model/State.h"
#include "TestBehaviourCreator.h"
#include "engine/model/Behaviour.h"
#include "engine/PlanRepository.h"
#include "engine/BasicBehaviour.h"
#include "engine/IBehaviourPool.h"
#include "engine/PlanBase.h"
#include <clock/AlicaROSClock.h>
#include <communication/AlicaRosCommunication.h>
#include  "engine/DefaultUtilityFunction.h"
#include  "engine/ITeamObserver.h"
#include "engine/model/Plan.h"
#include "TestConditionCreator.h"
#include "TestConstraintCreator.h"
#include "TestUtilityFunctionCreator.h"
#include "Attack.h"
#include "MidFieldStandard.h"
#include "engine/Assignment.h"
#include "engine/collections/AssignmentCollection.h"
#include "engine/collections/StateCollection.h"
#include "TestWorldModel.h"
#include "cace.h"
#include "clock/AlicaCaceClock.h"
#include "communication/AlicaCaceCommunication.h"

class AlicaMultiAgent : public ::testing::Test
{
protected:
	supplementary::SystemConfig* sc;
	alica::AlicaEngine* ae;
	Cace* cace;
	alica::AlicaEngine* ae2;
	Cace* cace2;
	alicaTests::TestBehaviourCreator* bc;
	alicaTests::TestConditionCreator* cc;
	alicaTests::TestUtilityFunctionCreator* uc;
	alicaTests::TestConstraintCreator* crc;

	virtual void SetUp()
	{
		// determine the path to the test config
		string path = supplementary::FileSystem::getSelfPath();
		int place = path.rfind("devel");
		path = path.substr(0, place);
		path = path + "src/alica/alica_test/src/test";

		// bring up the SystemConfig with the corresponding path
		sc = supplementary::SystemConfig::getInstance();
		sc->setRootPath(path);
		sc->setConfigPath(path + "/etc");
		// setup the engine
		bc = new alicaTests::TestBehaviourCreator();
		cc = new alicaTests::TestConditionCreator();
		uc = new alicaTests::TestUtilityFunctionCreator();
		crc = new alicaTests::TestConstraintCreator();

	}

	virtual void TearDown()
	{
		ae->shutdown();
		ae2->shutdown();
		delete cace;
		delete ae->getCommunicator();
		delete cace2;
		delete ae2->getCommunicator();
		delete ae->getIAlicaClock();
		delete ae2->getIAlicaClock();
		sc->shutdown();
		delete cc;
		delete bc;
		delete uc;
		delete crc;
	}
};
/**
 * Tests whether it is possible to use multiple agents.
 */
TEST_F(AlicaMultiAgent, runMultiAgentPlan)
{
	sc->setHostname("nase");
	cace = Cace::getEmulated(sc->getHostname(), sc->getOwnRobotID());
	cace->safeStepMode = true;
	ae = new alica::AlicaEngine();
	ae->setIAlicaClock(new alicaCaceProxy::AlicaCaceClock(cace));
	ae->setCommunicator(new alicaCaceProxy::AlicaCaceCommunication(ae, cace));
	ASSERT_TRUE(ae->init(bc, cc, uc, crc, "RolesetTA", "MultiAgentTestMaster", ".", true))
			<< "Unable to initialise the Alica Engine!";

	sc->setHostname("hairy");
	cace2 = Cace::getEmulated(sc->getHostname(), sc->getOwnRobotID());
	cace2->safeStepMode = true;
	ae2 = new alica::AlicaEngine();
	ae2->setIAlicaClock(new alicaCaceProxy::AlicaCaceClock(cace2));
	ae2->setCommunicator(new alicaCaceProxy::AlicaCaceCommunication(ae2, cace2));
	ASSERT_TRUE(ae2->init(bc, cc, uc, crc, "RolesetTA", "MultiAgentTestMaster", ".", true))
			<< "Unable to initialise the Alica Engine!";

	ae->start();
	ae2->start();

	for (int i = 0; i < 20; i++)
	{
		ae->stepNotify();
		chrono::milliseconds duration(33);
		this_thread::sleep_for(duration);
		ae2->stepNotify();
		this_thread::sleep_for(duration);

//		if (i > 24)
//		{
//			if (ae->getPlanBase()->getDeepestNode() != nullptr)
//				cout << "AE: " << ae->getPlanBase()->getDeepestNode()->toString() << endl;
//			if (ae2->getPlanBase()->getDeepestNode() != nullptr)
//				cout << "AE2: " << ae2->getPlanBase()->getDeepestNode()->toString() << endl;
//			cout << "-------------------------" << endl;
//		}

		if (i < 10)
		{
			ASSERT_EQ(ae->getPlanBase()->getRootNode()->getActiveState()->getId(), 1413200842974);
			ASSERT_EQ(ae2->getPlanBase()->getRootNode()->getActiveState()->getId(), 1413200842974);
		}
		if (i == 10)
		{
			cout << "1--------- Initial State passed ---------" << endl;
			alicaTests::TestWorldModel::getOne()->setTransitionCondition1413201227586(true);
			alicaTests::TestWorldModel::getTwo()->setTransitionCondition1413201227586(true);
		}
		if (i > 11 && i < 15)
		{
			ASSERT_EQ(ae->getPlanBase()->getRootNode()->getActiveState()->getId(), 1413201213955);
			ASSERT_EQ(ae2->getPlanBase()->getRootNode()->getActiveState()->getId(), 1413201213955);
			ASSERT_EQ((*ae->getPlanBase()->getRootNode()->getChildren()->begin())->getPlan()->getName(),
						string("MultiAgentTestPlan"));
			ASSERT_EQ((*ae2->getPlanBase()->getRootNode()->getChildren()->begin())->getPlan()->getName(),
						string("MultiAgentTestPlan"));
		}
		if (i == 15)
		{
			for (auto iter : *ae->getBehaviourPool()->getAvailableBehaviours())
			{
				if (iter.second->getName() == "Attack")
				{
					ASSERT_GT(((alicaTests::Attack* )&*iter.second)->callCounter, 5);
					if (((alicaTests::Attack*)&*iter.second)->callCounter > 3)
					{
						alicaTests::TestWorldModel::getOne()->setTransitionCondition1413201052549(true);
						alicaTests::TestWorldModel::getTwo()->setTransitionCondition1413201052549(true);
						alicaTests::TestWorldModel::getOne()->setTransitionCondition1413201370590(true);
						alicaTests::TestWorldModel::getTwo()->setTransitionCondition1413201370590(true);
					}
				}
			}
			cout << "2--------- Engagement to cooperative plan passed ---------" << endl;
		}
		if (i == 16)
		{
			ASSERT_TRUE(
					(*ae2->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->getId() == 1413201030936
					|| (*ae->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->getId() == 1413201030936)
					<< endl << (*ae2->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->getId()
					<< " " << (*ae->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->getId()
					<< endl;

			ASSERT_TRUE(
					(*ae2->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->getId() == 1413807264574
					|| (*ae->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->getId() == 1413807264574)
					<< endl << (*ae2->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->getId()
					<< " " << (*ae->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->getId()
					<< endl;
			alicaTests::TestWorldModel::getOne()->setTransitionCondition1413201227586(false);
			alicaTests::TestWorldModel::getTwo()->setTransitionCondition1413201227586(false);
			cout << "3--------- Passed transitions in subplan passed ---------" << endl;
		}
		if (i >= 17 && i <= 18)
		{
			ASSERT_TRUE(
					(*ae2->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->getId() == 1413201030936
					|| (*ae->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->getId() == 1413201030936)
					<< "AE State: "
					<< (*ae->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->getId()
					<< " AE2 State: "
					<< (*ae2->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->getId() << endl;
			ASSERT_TRUE(
					(*ae2->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->getId() == 1413807264574
					|| (*ae->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->getId() == 1413807264574)
					<< "AE State: "
					<< (*ae->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->getId() << " "
					<< (*ae->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->toString() << endl
					<< " AE2 State: "
					<< (*ae2->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->getId() << " "
					<< (*ae2->getPlanBase()->getRootNode()->getChildren()->begin())->getActiveState()->toString() << endl;
			if(i==18) {
			cout << "4--------- Stayed in these state although previous transitions are not true anymore ---------"
					<< endl;
				alicaTests::TestWorldModel::getOne()->setTransitionCondition1413201389955(true);
				alicaTests::TestWorldModel::getTwo()->setTransitionCondition1413201389955(true);
			}
		}
		if (i == 19)
		{
			ASSERT_TRUE(
					ae2->getPlanBase()->getRootNode()->getActiveState()->getId() == 1413201380359
					&& ae->getPlanBase()->getRootNode()->getActiveState()->getId() == 1413201380359)
					<< " AE State: "
					<< ae->getPlanBase()->getRootNode()->getActiveState()->getId()
					<< " AE2 State: "
					<< ae2->getPlanBase()->getRootNode()->getActiveState()->getId() << endl;
		}
	}
}

