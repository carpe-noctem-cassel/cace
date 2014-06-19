// Bring in my package's API, which is what I'm testing
//#include "SystemConfig.h"
// Bring in gtest

#include <boost/smart_ptr/make_shared_object.hpp>
#include <cace/CaceAcknowledge.h>
#include <cace/CaceType.h>
#include <cace.h>
#include <caceSpace.h>
#include <communication/CaceCommunication.h>
#include <communication/CommunicationWorker.h>
#include <communication/jobs/AbstractCommunicationJob.h>
#include <communication/jobs/CommandJob.h>
#include <communication/JobStateEntity.h>
#include <CaceTypes.h>
#include <gtest/gtest.h>
#include <gtest/gtest-message.h>
#include <gtest/internal/gtest-internal.h>
#include <gtest/internal/gtest-string.h>
#include <ros/init.h>
#include <timeManager/AgentCommunicationModel.h>
#include <timeManager/AgentTimeData.h>
#include <timeManager/TimeManager.h>
#include <variables/ConsensusVariable.h>
#include <variableStore/CVariableStore.h>
#include <cstdint>
#include <iostream>
#include <limits>
#include <list>
#include <memory>
#include <string>
#include <vector>

using namespace cace;

class CaceBasics : public ::testing::Test
{
protected:
	Cace* mycace = nullptr;

	CaceBasics()
	{
		// You can do set-up work for each test here.
	}

	virtual ~CaceBasics()
	{
		// You can do clean-up work that doesn't throw exceptions here.
	}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp()
	{
		mycace = Cace::getEmulated("", 254);
		// Code here will be called immediately after the constructor (right
		// before each test).
	}

	virtual void TearDown()
	{
		delete mycace;
		// Code here will be called immediately after each test (right
		// before the destructor).
	}

	// Objects declared here can be used by all tests in the test case for Foo.
	void commandjobTest()
	{
		mycace->activeRobots.clear();
		mycace->agentEngangement(20, false);

		string name = "CommandJob" + to_string(mycace->timeManager->lamportTime);
		double doubleValue = 1.1 + (double)mycace->timeManager->lamportTime;
		vector<uint8_t> value; // = new List<byte>(BitConverter.GetBytes(doubleValue));
		char* it = (char*)&doubleValue;
		value.clear();
		value.reserve(sizeof(double));
		for (int i = 0; i < sizeof(double); i++)
		{
			value.push_back(*it);
			it++;
		}
		uint8_t type = CaceType::CDouble;
		acceptStrategy strategy = acceptStrategy::ThreeWayHandShake;

		// copy from Cacespace.Dstribute Value;
		shared_ptr<ConsensusVariable> variable;
		if (!mycace->variableStore->existsVariable(name))
		{
			variable = make_shared<ConsensusVariable>(name, strategy, std::numeric_limits<long>::max(),
														mycace->communication->getOwnID(),
														mycace->timeManager->getDistributedTime(),
														mycace->timeManager->lamportTime, type);
			variable->setValue(value);
			variable->setAcceptStrategy(strategy);
			bool varexists = mycace->variableStore->addVariable(variable);
			EXPECT_TRUE(varexists) << "Var didn't exist during test, but does during add!";
		}
		else
		{
			variable = mycace->variableStore->getVariable(name);
			variable->setValue(value);
			variable->setType(type);
			variable->setLamportAge(mycace->timeManager->lamportTime);
			variable->setAcceptStrategy(strategy);
		}

		vector<int> all;
		all.push_back(20);
		CommandJob* job = new CommandJob(name, variable, value, all, mycace->timeManager->lamportTime, mycace);
		job->entities.begin()->lastSent = std::numeric_limits<long>::max();
		mycace->worker->appendJob(job);
		EXPECT_TRUE(job->expectedRobotIDs.size() == 1) << "Wrong Number of Expected Acks. Should be 1 but is: "
				<< job->expectedRobotIDs.size();

		EXPECT_TRUE(variable->proposals.size() == 0) << "Wrong number of Robot believes: Should be 0 but is "
				<< variable->proposals.size() << "\n" << variable->toString();
		//cace.Step(null);
		EXPECT_FALSE(job->process()) << "Job is finished, but shouldn't be...";

		EXPECT_TRUE(job->expectedRobotIDs.size() == 1) << "Wrong Number of Expected Acks. Should be 1 but is: "
				<< job->expectedRobotIDs.size();

		CaceAcknowledgePtr ack = boost::make_shared<CaceAcknowledge>();
		ack->lamportTime = variable->getLamportAge() + 1;
		ack->msgID = job->msgID;
		ack->receiverID = mycace->communication->getOwnID();
		ack->senderID = 20;
		ack->type = type;
		ack->value = value;
		ack->variableName = name;
		mycace->communication->handleCaceAcknowledge(ack);
		EXPECT_TRUE(variable->proposals.size() == 1) << "Wrong number of Robot believes: Should be 1 but is "
				<< variable->proposals.size() << "\n" << variable->toString();

		EXPECT_TRUE(job->expectedRobotIDs.size() == 1) << "Wrong Number of Expected Acks. Should be 1 but is: "
				<< job->expectedRobotIDs.size();

		EXPECT_TRUE(job->process()) << "Job isn't finished, but should be...";
		EXPECT_TRUE(job->expectedRobotIDs.size() == 0) << "Wrong Number of Expected Acks. Should be 0 but is: "
				<< job->expectedRobotIDs.size();
		//cace.Step(null);
		EXPECT_TRUE(
				job->variable->proposals.size() == 1) << "Wrong number of Robot believes: Should be 1 but is "
				<< job->variable->proposals.size() << "\n" << job->variable->toString();
		EXPECT_FALSE(job->variable->checkConflict(*mycace)) << "Did detect a Conflict, but shouldn't.\n "
				<< job->variable->toString();
		EXPECT_GE(job->variable->proposals.at(0)->getLamportAge(), variable->getLamportAge())
				<< "LamportAge didn't match.";

		CaceAcknowledgePtr ack2 = boost::make_shared<CaceAcknowledge>();

		ack2->lamportTime = 0;
		ack2->msgID = job->msgID;
		ack2->receiverID = mycace->communication->getOwnID();
		ack2->senderID = 20;
		ack2->type = type;

		double twotwo = 2.2;
		it = (char*)&twotwo;
		vector<uint8_t> twoVal;
		twoVal.reserve(sizeof(double));
		for (int i = 0; i < sizeof(double); i++)
		{
			twoVal.push_back(*it);
			it++;
		}

		ack2->value = twoVal;
		ack2->variableName = name;
		mycace->communication->handleCaceAcknowledge(ack2);

		EXPECT_TRUE(job->process()) << "2 Job isn't finished, but should be...";
		//cace.Step(null);
		EXPECT_TRUE(job->variable->proposals.size() == 1) << "2 Wrong number of Robot believes: Should be 1 but is "
				<< job->variable->proposals.size() << "\n" << job->variable->toString();
		double newValue = 0;
		variable->getValue(&newValue);
		EXPECT_TRUE(newValue == doubleValue) << "Wrong Variable Value: Should be " << doubleValue << " but is: "
				<< newValue;

		//Test other robot believe
		variable->proposals.at(0)->getValue(&newValue);
		EXPECT_TRUE(newValue == doubleValue) << "Wrong Variable Value: Should be " << doubleValue << " but is: "
				<< newValue;

		EXPECT_FALSE(job->variable->checkConflict(*mycace)) << "2 Did detect a Conflict, but shouldn't.";
		EXPECT_GE(job->variable->proposals.at(0)->getLamportAge(), variable->getLamportAge())
				<< "2 LamportAge didn't match.";
	}

	void believeNotificationJob()
	{
		mycace->agentEngangement(20, false);

		string name = string("BelieveNotification") + to_string(mycace->timeManager->lamportTime);
		double doubleValue = 1.1 + (double)mycace->timeManager->lamportTime;
		vector<uint8_t> value; // = new List<byte>(BitConverter.GetBytes(doubleValue));
		char* it = (char*)&doubleValue;
		value.clear();
		value.reserve(sizeof(double));
		for (int i = 0; i < sizeof(double); i++)
		{
			value.push_back(*it);
			it++;
		}
		uint8_t type = CaceType::CDouble;
		acceptStrategy strategy = acceptStrategy::ThreeWayHandShake;

		// copy from Cacespace.Dstribute Value;
		shared_ptr<ConsensusVariable> variable;
		if (!mycace->variableStore->existsVariable(name))
		{
			variable = make_shared<ConsensusVariable>(name, strategy, std::numeric_limits<long>::max(),
														mycace->communication->getOwnID(),
														mycace->timeManager->getDistributedTime(),
														mycace->timeManager->lamportTime, type);
			variable->setValue(value);
			variable->setAcceptStrategy(strategy);
			bool varexists = mycace->variableStore->addVariable(variable);
			EXPECT_TRUE(varexists) << "Var didn't exist during test, but does during add!";
		}
		else
		{
			variable = mycace->variableStore->getVariable(name);
			variable->setValue(value);
			variable->setType(type);
			variable->setLamportAge(mycace->timeManager->lamportTime);
			variable->setAcceptStrategy(strategy);
		}

		vector<int> all;
		all.push_back(20);
		BelieveNotificationJob* job = new BelieveNotificationJob(name, variable, value, all,
																	mycace->timeManager->lamportTime, mycace);
		job->entities.begin()->lastSent = std::numeric_limits<long>::max();
		mycace->worker->appendJob(job);
		EXPECT_TRUE(job->expectedRobotIDs.size() == 1) << "Wrong Number of Expected Acks. Should be 1 but is: "
				<< job->expectedRobotIDs.size();

		EXPECT_TRUE(
				variable->proposals.size() == 0) << "Wrong number of Robot believes: Should be 0 but is "
				<< variable->proposals.size() << "\n" << variable->toString();

		EXPECT_FALSE(job->process()) << "Job is finished, but shouldn't be...";

		EXPECT_TRUE(job->expectedRobotIDs.size() == 1) << "Wrong Number of Expected Acks. Should be 1 but is: "
				<< job->expectedRobotIDs.size();

		CaceAcknowledgePtr ack = boost::make_shared<CaceAcknowledge>();
		ack->lamportTime = variable->getLamportAge() + 1;
		ack->msgID = job->msgID;
		ack->receiverID = mycace->communication->getOwnID();
		ack->senderID = 20;
		ack->type = type;
		ack->value = value;
		ack->variableName = name;
		mycace->communication->handleCaceAcknowledge(ack);
		EXPECT_TRUE(
				variable->proposals.size() == 1) << "Wrong number of Robot believes: Should be 1 but is "
				<< variable->proposals.size() << "\n" << variable->toString();
		EXPECT_TRUE(job->expectedRobotIDs.size() == 1) << "Wrong Number of Expected Acks. Should be 1 but is: "
				<< job->expectedRobotIDs.size();

		EXPECT_TRUE(job->process()) << "Job isn't finished, but should be...";
		EXPECT_TRUE(job->expectedRobotIDs.size() == 0) << "Wrong Number of Expected Acks. Should be 0 but is: "
				<< job->expectedRobotIDs.size();
		EXPECT_TRUE(
				job->variable->proposals.size() == 1) << "Wrong number of Robot believes: Should be 1 but is "
				<< job->variable->proposals.size() + "\n" + job->variable->toString();
		EXPECT_FALSE(job->variable->checkConflict(*mycace)) << "Did detect a Conflict, but shouldn't.\n "
				<< job->variable->toString();
		EXPECT_GE(job->variable->proposals.at(0)->getLamportAge(), variable->getLamportAge())
				<< "LamportAge didn't match.";

		CaceAcknowledgePtr ack2 = boost::make_shared<CaceAcknowledge>();
		ack2->lamportTime = 0;
		ack2->msgID = job->msgID;
		ack2->receiverID = mycace->communication->getOwnID();
		ack2->senderID = 20;
		ack2->type = type;

		double twotwo = 2.2;
		it = (char*)&twotwo;
		vector<uint8_t> twoVal;
		twoVal.reserve(sizeof(double));
		for (int i = 0; i < sizeof(double); i++)
		{
			twoVal.push_back(*it);
			it++;
		}

		ack2->value = twoVal;

		ack2->variableName = name;
		mycace->communication->handleCaceAcknowledge(ack2);

		EXPECT_TRUE(job->process()) << "2 Job isn't finished, but should be...";
//cace.Step(null);
		EXPECT_TRUE(
				job->variable->proposals.size() == 1) << "2 Wrong number of Robot believes: Should be 1 but is "
				<< job->variable->proposals.size() << "\n" << job->variable->toString();
		double newValue = 0;
		variable->getValue(&newValue);
		EXPECT_TRUE(newValue == doubleValue) << "Wrong Variable Value: Should be " << doubleValue << " but is: "
				<< newValue;

//Test other robot believe
		variable->proposals.at(0)->getValue(&newValue);
		EXPECT_TRUE(newValue == doubleValue) << "Wrong Variable Value: Should be " << doubleValue << " but is: "
				<< newValue;

		EXPECT_FALSE(job->variable->checkConflict(*mycace)) << "2 Did detect a Conflict, but shouldn't.";
		EXPECT_GE(job->variable->proposals.at(0)->getLamportAge(), variable->getLamportAge())
				<< "2 LamportAge didn't match.";
	}

};

TEST_F(CaceBasics, LocalContext)
{
	mycace->localScope.clear();
	bool test = (mycace->getLocalScopeString() == string("/"));
	EXPECT_TRUE(test) << "Default Namespace is Wrong";

	mycace->localScope.push_back("Test");
	mycace->localScope.push_back("Namespace");
	EXPECT_TRUE((mycace->getLocalScopeString() == string("/Test/Namespace/"))) << " should be /Test/Namespace/";
}

TEST_F(CaceBasics, VariableContext)
{
	string name = "/TestContext/a";
	mycace->caceSpace->distributeValue(name, 1.0, acceptStrategy::NoDistribution);
	shared_ptr<ConsensusVariable> var = mycace->caceSpace->getVariable(name);
	string varScope = var->getScope();
	string vars = mycace->variableStore->toString(varScope); //, var.GetLocalContext() + "does not exist");

	EXPECT_TRUE(vars.find(varScope)!=string::npos) << var->getScope() << " does not exist in " << vars;
}

TEST_F(CaceBasics, AgentCommunicationModelResendTime)
{
	AgentCommunicationModel acm(1);

	double ResendArrivalPropability = 0.99;
	unsigned long resendtime = acm.getEstimatedResendTime(ResendArrivalPropability);
	/*Console.WriteLine(
	 "For ResendArrivalPropability of: " + ResendArrivalPropability + " the resendTime is: " + resendtime);*/

	unsigned long init = mycace->timeManager->getDistributedTime();

	unsigned long t = acm.getEstimatedResendTime(0.99);
	//Assert.AreEqual(0, t);

	//delay 10
	AgentTimeData* atd = new AgentTimeData(0, init, init + 10, init, 5);
	//delay 20
	AgentTimeData* atd2 = new AgentTimeData(0, init + 100, init + 120, init + 100, 5);
	acm.addData(atd);
	//t = acm.GetMaxLikelihoodDelay();
	//Assert.AreEqual(10*1000000, t);

	acm.addData(atd2);
	t = acm.getMaxLikelihoodDelay();
	EXPECT_EQ(15, t) << "Wrong Delay Estimation";

	t = acm.getEstimatedResendTime(0.5);
	EXPECT_EQ(30, t) << "Wrong Resendtime Estimation 0.5";

	t = acm.getEstimatedResendTime(0.99);
	EXPECT_GT(t, 40) << "Wrong Resendtime Estimation 0.99";

	t = acm.getEstimatedResendTime(0.01);
	EXPECT_LT(t, 20) << "Wrong Resendtime Estimation 0.01";
}

TEST_F(CaceBasics, AgentCommunicationModel)
{
	AgentCommunicationModel acm(1);
	unsigned long init = mycace->timeManager->getDistributedTime();

	long t = (long)acm.getEstimatedResendTime(0.99);
	//Assert.AreEqual(0, t);

	//delay 10
	AgentTimeData* atd = new AgentTimeData(0, init, init + 10, init + 10, 6);
	//delay 20
	AgentTimeData* atd2 = new AgentTimeData(0, init + 100, init + 120, init + 100, 6);
	acm.addData(atd);
	//t = acm.GetMaxLikelihoodDelay();
	//Assert.AreEqual(10*1000000, t);

	acm.addData(atd2);
	t = (long)acm.getMaxLikelihoodDelay();
	EXPECT_EQ(15, t) << "Wrong Delay Estimation";

	long v = (long)acm.getMaxLikelihoodDelayVariance((unsigned long)t);
	EXPECT_LT(v, 10) << "Variance Estimation";

	/*for(int i=0; i<300; i++) {
	 Console.WriteLine(i+" "+acm.GetPropabilityConsensus((long)(i), 0.9)+" "+acm.GetPropabilitySinglePacketArrival((long)i));
	 }*/

	double p = acm.getPropabilitySinglePacketArrival((long)t, t, v, 0.1);
	EXPECT_TRUE(p < 0.51 && p > 0.40) << "Probability for single packet arrival was " << p;

	double q = acm.getPropabilityConsensus((long)t, 0.99, t, v, 0.1);
	EXPECT_TRUE(q < 0.51 && q > 0.40) << "Probability for packet arrival was " << q;

	p = acm.getPropabilitySinglePacketArrival((long)(3 * t), t, v, 0.1);
	EXPECT_TRUE(p > 0.8 && p <= 0.99) << "Probability for single packet arrival (after 3*t) was " << p;

	q = acm.getPropabilityConsensus((long)(3 * t), 0.5, t, v, 0.1);
	EXPECT_TRUE(q >= p && q > 0.9) << "Probability for packet arrival (2*t, 0.5) was " << q;
}

TEST_F(CaceBasics, VariableTypesAndByteConversion)
{
	int i = 10;
	int outInt = 0;
	double d = 20.0;
	double outDouble = 0;
	double d2 = 30.0;
	double outDouble2 = 0;
	string s = "name";
	string outString = "";
	/* List<string> sl = new List<string>();
	 sl.Add("A");
	 sl.Add("B");
	 List<string> outsl = null;

	 List<double> dl = new List<double>();
	 dl.Add(20.0);
	 dl.Add(30.0);
	 List<double> outdl = null;

	 List<int> il = new List<int>();
	 dl.Add(2);
	 dl.Add(3);
	 List<int> outil = null;*/

	ConsensusVariable v("Type", (acceptStrategy)acceptStrategy::ThreeWayHandShake, std::numeric_limits<long>::max(), 1,
						1000, 1, CaceType::CInt);
	v.setValue(d);
	v.getValue(&outDouble);
	EXPECT_EQ(v.getType(), CaceType::CDouble) << "Double Type";
	EXPECT_EQ(d, outDouble) << "Double";

	v.setValue(i);
	v.getValue(&outInt);
	EXPECT_EQ(v.getType(), CaceType::CInt) << "Int Type";
	EXPECT_EQ(i, outInt) << "Int";

	v.setValue(&s);
	v.getValue(outString);
	EXPECT_EQ(v.getType(), CaceType::CString) << "String Type";
	EXPECT_TRUE(s == (outString)) << "String";

	/* v.SetValue(sl);
	 v.GetValue(out outsl);
	 Assert.AreEqual(v.Type, RosCS.ConsensusEngine.CaceType.CStringList, "StringList Type");
	 Assert.IsTrue(sl[0].Equals(outsl[0]), "StringList");
	 Assert.IsTrue(sl[1].Equals(outsl[1]), "StringList");

	 v.SetValue(il);
	 v.GetValue(out outil);
	 Assert.AreEqual(v.Type, RosCS.ConsensusEngine.CaceType.CIntList, "IntList Type");
	 Assert.AreEqual(sl[0], (outsl[0]), "IntList");
	 Assert.AreEqual(sl[1], (outsl[1]), "IntList");

	 v.SetValue(dl);
	 v.GetValue(out outdl);
	 Assert.AreEqual(v.Type, RosCS.ConsensusEngine.CaceType.CDoubleList, "DoubleList Type");
	 Assert.AreEqual(dl[0], (outdl[0]), "DoubleList");
	 Assert.AreEqual(dl[1], (outdl[1]), "DoubleList");*/

	/*v.setValue(d, d2);
	 v.getValue(out outDouble, out outDouble2);
	 Assert.AreEqual(v.Type, RosCS.ConsensusEngine.CaceType.CPoint2, "Point Type");
	 Assert.AreEqual(d, outDouble, "Point");
	 Assert.AreEqual(d2, outDouble2, "Point");*/
}

TEST_F(CaceBasics, DefaultConflictResolutionOtherRobot)
{
	mycace->activeRobots.clear();
	mycace->agentEngangement(2, false);
	mycace->agentEngangement(3, false);

	ConsensusVariable v("A1", (acceptStrategy)acceptStrategy::ThreeWayHandShake, std::numeric_limits<long>::max(), 1,
						1000, 1, CaceType::CDouble);
	v.setValue(1.0);
	v.setLamportAge(1);

	//Lamport Age = 2
	auto v2 = make_shared<ConsensusVariable>("A1", (acceptStrategy)acceptStrategy::ThreeWayHandShake,
												std::numeric_limits<long>::max(), 2, 1000, 2, CaceType::CDouble);
	v2->setValue(2.0);
	v2->setLamportAge(2);
	auto v3 = make_shared<ConsensusVariable>("A1", (acceptStrategy)acceptStrategy::ThreeWayHandShake,
												std::numeric_limits<long>::max(), 3, 1000, 1, CaceType::CDouble);
	v3->setValue(1.0);
	v3->setLamportAge(1);

	v.proposals.push_back(v2);
	v.proposals.push_back(v3);

	EXPECT_TRUE(v.checkConflict(*mycace)) << "No Conflict detected: " << v.checkConflict(*mycace);
	v.acceptProposals(*mycace, nullptr);
	//Assert.IsFalse(v.CheckConflict(cace),"Conflict detected after CR: "+v.CheckConflict(cace));

	double value = 0;
	v.getValue(&value);
	EXPECT_TRUE(value == 2) << "1 Wrong Value: " << value;
	EXPECT_TRUE(v.getLamportAge() == 2) << "Wrong Lamporttime: " << v.getLamportAge();

	v2->getValue(&value);
	EXPECT_TRUE(value == 2) << "2 Wrong Value: " << value;
	v3->getValue(&value);
	EXPECT_TRUE(value == 1) << "3 Wrong Value: " << value;
}

TEST_F(CaceBasics, DefaultConflictResolutionNoChangeAtDisengagedRobot)
{
	mycace->activeRobots.clear();
	mycace->agentEngangement(2, false);

	ConsensusVariable v("A1", acceptStrategy::ThreeWayHandShake, std::numeric_limits<long>::max(), 1, 1000, 1,
						CaceType::CDouble);
	v.setValue(1.0);
	v.setLamportAge(1);

	//Lamport Age = 2
	auto v2 = make_shared<ConsensusVariable>("A1", acceptStrategy::ThreeWayHandShake, std::numeric_limits<long>::max(),
												2, 1000, 2, CaceType::CDouble);
	v2->setValue(2.0);
	v2->setLamportAge(2);
	auto v3 = make_shared<ConsensusVariable>("A1", acceptStrategy::ThreeWayHandShake, std::numeric_limits<long>::max(),
												3, 1000, 1, CaceType::CDouble);
	v3->setValue(1.0);
	v3->setLamportAge(1);

	v.proposals.push_back(v2);
	v.proposals.push_back(v3);

	EXPECT_TRUE(v.checkConflict(*mycace)) << "No Conflict detected: " << v.checkConflict(*mycace);
	v.acceptProposals(*mycace, nullptr);
	EXPECT_FALSE(v.checkConflict(*mycace)) << "Conflict detected after CR: " << v.checkConflict(*mycace)
			<< " activeRobots " << mycace->printActiveRobots();

	double value = 0;
	v.getValue(&value);
	EXPECT_TRUE(value == 2) << "Wrong Value: " << value;
	EXPECT_TRUE(v.getLamportAge() == 2) << "Wrong Lamporttime: " << v.getLamportAge();

	v2->getValue(&value);
	EXPECT_TRUE(value == 2) << "Wrong Value: " << value;
	v3->getValue(&value);
	EXPECT_TRUE(value == 1) << "Wrong Value: " << value;
}

TEST_F(CaceBasics, DefaultConflictResolutionSelf)
{
	mycace->activeRobots.clear();
	mycace->agentEngangement(2, false);
	mycace->agentEngangement(3, false);

	ConsensusVariable v("A1", (acceptStrategy)acceptStrategy::ThreeWayHandShake, std::numeric_limits<long>::max(), 1,
						1000, 1, CaceType::CDouble);
	v.setValue(1.0);
	v.setLamportAge(3);

	//Lamport Age = 2
	auto v2 = make_shared<ConsensusVariable>("A1", (acceptStrategy)acceptStrategy::ThreeWayHandShake,
												std::numeric_limits<long>::max(), 2, 1000, 2, CaceType::CDouble);
	v2->setValue(2.0);
	v2->setLamportAge(1);
	auto v3 = make_shared<ConsensusVariable>("A1", (acceptStrategy)acceptStrategy::ThreeWayHandShake,
												std::numeric_limits<long>::max(), 3, 1000, 1, CaceType::CDouble);
	v3->setValue(1.0);
	v3->setLamportAge(1);

	v.proposals.push_back(v2);
	v.proposals.push_back(v3);

	EXPECT_TRUE(v.checkConflict(*mycace)) << "No Conflict detected: " << v.checkConflict(*mycace);
	v.acceptProposals(*mycace, nullptr);
	//Assert.IsFalse(v.CheckConflict(cace),"Conflict detected after CR: "+v.CheckConflict(cace));

	double value = 0;
	v.getValue(&value);
	EXPECT_TRUE(value == 1) << "1 Wrong Value: " << value;
	EXPECT_TRUE(v.getLamportAge() == 3) << "Wrong Lamporttime: " << v.getLamportAge();

	v2->getValue(&value);
	EXPECT_TRUE(value == 2) << "2 Wrong Value: " << value;
	v3->getValue(&value);
	EXPECT_TRUE(value == 1) << "3 Wrong Value: " << value;
}

TEST_F(CaceBasics, CommandJob)
{
	commandjobTest();
}

TEST_F(CaceBasics, CommandJobSecondRun)
{
	commandjobTest();
}

TEST_F(CaceBasics, BelieveNotificationJob)
{
	believeNotificationJob();
}

TEST_F(CaceBasics, BelieveNotificationJobSecondRun)
{
	believeNotificationJob();
}

TEST_F(CaceBasics, CommandArrival)
{
	string name = "CommandArrival";

	double doubleValue = 1.0;
	vector<uint8_t> value; // = new List<byte>(BitConverter.GetBytes(doubleValue));
	char* it = (char*)&doubleValue;
	value.clear();
	value.reserve(sizeof(double));
	for (int i = 0; i < sizeof(double); i++)
	{
		value.push_back(*it);
		it++;
	}

	char type = CaceType::CDouble;
	acceptStrategy strategy = acceptStrategy::ThreeWayHandShake;
	mycace->activeRobots.clear();
	mycace->activeRobots.push_back(20);

	CaceCommandPtr cmd = boost::make_shared<CaceCommand>();

	cmd->lamportTime = 2;
	cmd->msgID = 100;
	cmd->receiverID = mycace->communication->getOwnID();
	cmd->senderID = 20;
	cmd->type = type;
	cmd->value = value;
	cmd->variableName = name;
	cmd->decissionTime = 1000;
	cmd->level = strategy;
	cmd->validityTime = std::numeric_limits<long>::max();
	mycace->communication->handleCaceCommand(cmd);

	ros::TimerEvent e;
	mycace->step(e);

	double val = 0;

	shared_ptr<ConsensusVariable> v = mycace->caceSpace->getVariable(name);
	EXPECT_TRUE(v.operator bool()) << "Variable unknown";
	v->getValue(&val);
	EXPECT_EQ(1.0, val) << "value doest work\n" << v->toString();

	double ptr;
	EXPECT_TRUE(mycace->caceSpace->getVariableValue(name, &ptr)) << "Variablevalue is unknown\n" + v->toString();
	EXPECT_EQ(1, val) << "Own Believe doesn't equal commanded value\n" << v->toString();

	//delete ptr;
}

// Run all the tests that were declared with TEST()
int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);

	ros::init(argc, argv, "Cace");

	int ret = RUN_ALL_TESTS();
	cout << "All tests Completed" << endl;
	return ret;
}

