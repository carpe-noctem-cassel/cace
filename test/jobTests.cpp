#include <communication/CaceCommunication.h>
#include <cace/CaceType.h>
#include <cace.h>
#include <communication/jobs/CommandJob.h>
#include <communication/jobs/CommandAcknowledgeJob.h>
#include <communication/jobs/BelieveAcknowledgeJob.h>
#include <CaceTypes.h>
#include <gtest/gtest.h>
#include <gtest/gtest-message.h>
#include <gtest/internal/gtest-internal.h>
#include <gtest/internal/gtest-string.h>
#include <timeManager/TimeManager.h>
#include <variables/ConsensusVariable.h>
#include <variableStore/CVariableStore.h>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

using namespace cace;

class DummyCaceCom : public CaceCommunication
{
public:
	int sendBNot = 0;
	int sendCmd = 0;
	int sendAck = 0;
	int sendSAck = 0;

	DummyCaceCom(CommunicationWorker* worker, string& nodePrefix, Cace* cace, int ownID) :
			CaceCommunication(worker, nodePrefix, cace)
	{
		this->ownID = ownID;
	}

	virtual void sendCaceCommand(shared_ptr<ConsensusVariable> cv, short msgID, vector<uint8_t>& value, short receiver,
									unsigned long lamportTime)
	{
		sendCmd++;
	}

	/// <summary>
	/// Sends a CaceBelieveNotification Message to achieve consensus on the consensusvariable 'cv'
	/// </summary>
	virtual void sendCaceBelieveNotification(shared_ptr<ConsensusVariable> cv, short receiverID, short msgID,
												unsigned long lamportTime)
	{
		sendBNot++;
	}

	/// <summary>
	/// Sends a CaceCAcknowledge Message to achieve consensus on the consensusvariable 'cv'
	/// </summary>
	virtual void sendCaceAcknowledge(string& name, vector<uint8_t>& value, short messageID, short receiver, short type,
										unsigned long lamportTime)
	{
		sendAck++;
	}

	/// <summary>
	/// Sends a ShortAck Message to 'receiver'
	/// </summary>
	virtual void sendCaceShortAck(string& name, short messageID, short receiver, unsigned long lamportTime)
	{
		sendSAck++;
	}

	/// <summary>
	/// Anounce Disengange
	/// </summary>
	virtual void anounceDisengange()
	{
	}
};

//Cace* cace;
class JobTests : public ::testing::Test
{
protected:
	Cace* cace = nullptr;
	DummyCaceCom* dcom;

	JobTests()
	{
		// You can do set-up work for each test here.
	}

	virtual ~JobTests()
	{
		// You can do clean-up work that doesn't throw exceptions here.
	}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp()
	{
		int testID = 254;
		cace = Cace::getEmulated("", testID);
		string s = string("");
		dcom = new DummyCaceCom(cace->worker, s, this->cace, testID);
		cace->substituteCaceCommunication(dcom);
		// Code here will be called immediately after the constructor (right
		// before each test).
	}

	virtual void TearDown()
	{
		delete cace;
		// Code here will be called immediately after each test (right
		// before the destructor).
	}

};

TEST_F(JobTests, CommandJob)
{
	dcom->sendCmd = 0;
	cace->activeRobots.clear();
	cace->agentEngangement(20, false);

	string name = "CommandJobNetwork" + to_string(cace->timeManager->lamportTime);
	double doubleValue = 1.1 + (double)cace->timeManager->lamportTime;
	char* it = (char*)&doubleValue;
	vector<uint8_t> value;
	value.clear();
	value.reserve(sizeof(double));
	for (int i = 0; i < sizeof(double); i++)
	{
		value.push_back(*it);
		it++;
	}
	char type = CaceType::CDouble;
	acceptStrategy strategy = acceptStrategy::ThreeWayHandShake;

	// copy from Cacespace.Dstribute Value;
	shared_ptr<ConsensusVariable> variable;
	if (!cace->variableStore->existsVariable(name))
	{
		variable = make_shared<ConsensusVariable>(name, strategy, std::numeric_limits<long>::max(),
													cace->communication->getOwnID(),
													cace->timeManager->getDistributedTime(),
													cace->timeManager->lamportTime, type);
		variable->setValue(value);
		variable->setAcceptStrategy(strategy);
		bool varexists = cace->variableStore->addVariable(variable);
		EXPECT_TRUE(varexists) << "Var didn't exist during test, but does during add!";
	}
	else
	{
		variable = cace->variableStore->getVariable(name);
		variable->setValue(value);
		variable->setType(type);
		variable->setLamportAge(cace->timeManager->lamportTime);
		variable->setAcceptStrategy(strategy);
	}

	vector<int> all;
	all.push_back(20);
	CommandJob* job = new CommandJob(name, variable, value, all, cace->timeManager->lamportTime, cace);
	cace->worker->appendJob(job);
	EXPECT_EQ(1, dcom->sendCmd) << "Init Send";
	EXPECT_EQ(job->expectedRobotIDs.size(), 1) << "Wrong Number of Expected Acks. Should be 1 but is: "
			<< job->expectedRobotIDs.size();

	EXPECT_EQ(
			variable->proposals.size(), 0) << "Wrong number of Robot believes: Should be 0 but is "
			<< variable->proposals.size() << "\n" << variable->toString();
	job->entities.begin()->lastSent = cace->timeManager->getDistributedTime()
			+ TimeManager::timeResolutionDevisor * 1000000ul;

	EXPECT_FALSE(job->process()) << "Job is finished, but shouldn't be...";
	EXPECT_EQ(job->expectedRobotIDs.size(), 1) << "Wrong Number of Expected Acks. Should be 1 but is: "
			<< job->expectedRobotIDs.size();
	EXPECT_EQ(1, dcom->sendCmd) << "First Process send";

	job->entities.begin()->lastSent = 0;

	EXPECT_FALSE(job->process()) << "Job is finished, but shouldn't be...";
	EXPECT_EQ(job->expectedRobotIDs.size(), 1) << "Wrong Number of Expected Acks. Should be 1 but is: "
			<< job->expectedRobotIDs.size();
	EXPECT_EQ(2, dcom->sendCmd) << "Didn't resend";

	job->entities.begin()->lastSent = 0;
	job->entities.begin()->retrys = 0;

	EXPECT_TRUE(job->process()) << "Job is finished, but shouldn't be...";
	EXPECT_EQ(2, dcom->sendCmd) << "Didn't resend 2";
}

TEST_F(JobTests, BNotJob)
{
	dcom->sendBNot = 0;
	cace->activeRobots.clear();
	cace->agentEngangement(20, false);

	string name = "BNotJobNetwork" + to_string(cace->timeManager->lamportTime);
	double doubleValue = 1.1 + (double)cace->timeManager->lamportTime;
	char* it = (char*)&doubleValue;
	vector<uint8_t> value;
	value.clear();
	value.reserve(sizeof(double));
	for (int i = 0; i < sizeof(double); i++)
	{
		value.push_back(*it);
		it++;
	}
	char type = CaceType::CDouble;
	acceptStrategy strategy = acceptStrategy::ThreeWayHandShake;

	// copy from Cacespace.Dstribute Value;
	shared_ptr<ConsensusVariable> variable;
	if (!cace->variableStore->existsVariable(name))
	{
		variable = make_shared<ConsensusVariable>(name, strategy, std::numeric_limits<long>::max(),
													cace->communication->getOwnID(),
													cace->timeManager->getDistributedTime(),
													cace->timeManager->lamportTime, type);
		variable->setValue(value);
		variable->setAcceptStrategy(strategy);
		bool varexists = cace->variableStore->addVariable(variable);
		EXPECT_TRUE(varexists) << "Var didn't exist during test, but does during add!";
	}
	else
	{
		variable = cace->variableStore->getVariable(name);
		variable->setValue(value);
		variable->setType(type);
		variable->setLamportAge(cace->timeManager->lamportTime);
		variable->setAcceptStrategy(strategy);
	}

	vector<int> all;
	all.push_back(20);
	BelieveNotificationJob* job = new BelieveNotificationJob(name, variable, value, all, cace->timeManager->lamportTime,
																cace);
	cace->worker->appendJob(job);
	EXPECT_EQ(1, dcom->sendBNot) << "Init Send";
	EXPECT_EQ(job->expectedRobotIDs.size(), 1) << "Wrong Number of Expected Acks. Should be 1 but is: "
			<< job->expectedRobotIDs.size();

	job->entities.begin()->lastSent = cace->timeManager->getDistributedTime() / TimeManager::timeResolutionDevisor
			+ 150000ul * 1000000ul;

	EXPECT_FALSE(job->process()) << "Job is finished, but shouldn't be...";
	EXPECT_TRUE(job->expectedRobotIDs.size() == 1) << "Wrong Number of Expected Acks. Should be 1 but is: "
			<< job->expectedRobotIDs.size();
	EXPECT_EQ(1, dcom->sendBNot) << "First Process send";

	job->entities.begin()->lastSent = 0;

	EXPECT_FALSE(job->process()) << "Job is finished, but shouldn't be...";
	EXPECT_EQ(job->expectedRobotIDs.size(), 1) << "Wrong Number of Expected Acks. Should be 1 but is: "
			<< job->expectedRobotIDs.size();
	EXPECT_EQ(2, dcom->sendBNot) << "Didn't resend";

	job->entities.begin()->lastSent = 0;
	job->entities.begin()->retrys = 0;

	EXPECT_TRUE(job->process()) << "Job is finished, but shouldn't be...";
	EXPECT_EQ(2, dcom->sendBNot) << "Didn't resend 2";
}

TEST_F(JobTests, BAckJob)
{
	cace->activeRobots.clear();
	cace->agentEngangement(20, false);
	dcom->sendAck = 0;

	string name = "AckJobNetwork" + to_string(cace->timeManager->getDistributedTime());
	double doubleValue = 1.1 + (double)cace->timeManager->lamportTime;
	char* it = (char*)&doubleValue;
	vector<uint8_t> value;
	value.clear();
	value.reserve(sizeof(double));
	for (int i = 0; i < sizeof(double); i++)
	{
		value.push_back(*it);
		it++;
	}
	char type = CaceType::CDouble;
	acceptStrategy strategy = acceptStrategy::ThreeWayHandShake;

	CaceCommandPtr cc = boost::make_shared<CaceCommand>();
	cc->decissionTime = 0;
	cc->lamportTime = cace->timeManager->lamportTime + 5;
	cc->level = strategy;
	cc->msgID = 100;
	cc->receiverID = cace->communication->getOwnID();
	cc->senderID = 20;
	cc->type = type;
	cc->validityTime = std::numeric_limits<long>::max();
	cc->value = value;
	cc->variableName = name;

	vector<int> all;
	all.push_back(20);
	shared_ptr<ConsensusVariable> np;
	CommandAcknowledgeJob* job = new CommandAcknowledgeJob(name, np, all, cace->timeManager->lamportTime, cace, cc);
	cace->worker->appendJob(job);
	EXPECT_EQ(1, dcom->sendAck) << "Init Send";
	EXPECT_EQ(job->expectedRobotIDs.size(), 1) << "Wrong Number of Expected Acks. Should be 1 but is: "
			<< job->expectedRobotIDs.size();

	job->entities.begin()->lastSent = cace->timeManager->getDistributedTime() / TimeManager::timeResolutionDevisor
			+ 150000ul * 1000000ul;

	EXPECT_FALSE(job->process()) << "Job is finished, but shouldn't be...";
	EXPECT_TRUE(job->expectedRobotIDs.size() == 1) << "Wrong Number of Expected Acks. Should be 1 but is: "
			<< job->expectedRobotIDs.size();
	EXPECT_EQ(1, dcom->sendAck) << "First Process send";

	job->entities.begin()->lastSent = 0;

	EXPECT_FALSE(job->process()) << "Job is finished, but shouldn't be...";
	EXPECT_TRUE(job->expectedRobotIDs.size() == 1) << "Wrong Number of Expected Acks. Should be 1 but is: "
			<< job->expectedRobotIDs.size();
	EXPECT_EQ(2, dcom->sendAck) << "Didn't resend";

	job->entities.begin()->lastSent = 0;
	job->entities.begin()->retrys = 0;

	EXPECT_TRUE(job->process()) << "Job is finished, but shouldn't be...";
	EXPECT_EQ(2, dcom->sendAck) << "Didn't resend 2";
}

TEST_F(JobTests, AckJob)
{
	cace->activeRobots.clear();
	cace->agentEngangement(20, false);
	dcom->sendAck = 0;

	string name = "AckJobNetwork" + to_string(cace->timeManager->getDistributedTime());
	double doubleValue = 1.1 + (double)cace->timeManager->lamportTime;
	char* it = (char*)&doubleValue;
	vector<uint8_t> value;
	value.clear();
	value.reserve(sizeof(double));
	for (int i = 0; i < sizeof(double); i++)
	{
		value.push_back(*it);
		it++;
	}
	char type = CaceType::CDouble;
	acceptStrategy strategy = acceptStrategy::ThreeWayHandShake;

	CaceBelieveNotificationPtr cc = boost::make_shared<CaceBelieveNotification>();
	cc->decissionTime = 0;
	cc->lamportTime = cace->timeManager->lamportTime + 5;
	cc->level = strategy;
	cc->msgID = 100;
	cc->receiverID = cace->communication->getOwnID();
	cc->senderID = 20;
	cc->type = type;
	cc->validityTime = std::numeric_limits<long>::max();
	cc->value = value;
	cc->variableName = name;

	vector<int> all;
	all.push_back(20);
	shared_ptr<ConsensusVariable> np;
	BelieveAcknowledgeJob* job = new BelieveAcknowledgeJob(name, np, all, cace->timeManager->lamportTime, cace, cc);
	cace->worker->appendJob(job);

	EXPECT_EQ(1, dcom->sendAck) << "Init Send";
	EXPECT_EQ(job->expectedRobotIDs.size(), 1) <<
					"Wrong Number of Expected Acks. Should be 1 but is: " << job->expectedRobotIDs.size();

	job->entities.begin()->lastSent = cace->timeManager->getDistributedTime() / TimeManager::timeResolutionDevisor
			+ 150000ul * 1000000ul;

	EXPECT_FALSE(job->process()) << "Job is finished, but shouldn't be...";
	EXPECT_EQ(job->expectedRobotIDs.size(), 1) <<
					"Wrong Number of Expected Acks. Should be 1 but is: " << job->expectedRobotIDs.size();
	EXPECT_EQ(1, dcom->sendAck) << "First Process send";

	job->entities.begin()->lastSent = 0;

	EXPECT_FALSE(job->process()) << "Job is finished, but shouldn't be...";
	EXPECT_TRUE(job->expectedRobotIDs.size() == 1) <<
					"Wrong Number of Expected Acks. Should be 1 but is: " << job->expectedRobotIDs.size();
	EXPECT_EQ(2, dcom->sendAck) << "Didn't resend";

	job->entities.begin()->lastSent = 0;
	job->entities.begin()->retrys = 0;

	EXPECT_TRUE(job->process()) << "Job is finished, but shouldn't be...";
	EXPECT_EQ(2, dcom->sendAck) << "Didn't resend 2";
}
