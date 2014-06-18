/*
 * AgentCommunicationModel.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef AGENTCOMMUNICATIONMODEL_H_
#define AGENTCOMMUNICATIONMODEL_H_

#include <iostream>
#include <list>
#include <stdlib.h>
#include <math.h>
#include <algorithm>

#include "AgentTimeData.h"
#include "TimeManager.h"

using namespace std;

namespace cace
{

	class AgentCommunicationModel
	{
	public:
		AgentCommunicationModel(int robotID=0);
		virtual ~AgentCommunicationModel();

		list<AgentTimeData*> data;
		int robotID=0;
		int maxEntrys=500;
		static ctime defaultDelay;
		static ctime defaultDelayVariance;

		void addData(AgentTimeData* adt);
		ctime getEstimatedTimeDifference();
		ctime getMaxLikelihoodDelay();
		ctime getMaxLikelihoodDelayVariance(ctime averageDelay);
		double getPropabilitySinglePacketArrival(ctime timeSincePacketSend);
		double getPropabilitySinglePacketArrival(ctime timeSincePacketSend, long m, long v, double PLoss);
		double getPropabilityConsensus(ctime timeSincePacketSend, double minResendProbability);
		double getPropabilityConsensus(ctime timeSincePacketSend, double minResendProbability, long m, long v, double PLoss);
		double getProbabilityForPacketLoss();
		ctime getEstimatedResendTime(double propability);
		ctime getEstimatedResendTime(double propability, long m, long v);
		ctime getEstimatedArrivalTime(double propability);

		string toString();

		static const double GSL_DBL_EPSILON;
		static int halleyIteration(double x, double w_initial,	int max_iters,	double& val,	double& err);
		static double seriesEval(double r);
		static int gSLLambertW0e(double x, double& val, double& err);
		static int gSLLambertW1e(double x, double& val, double& err);
	};

} /* namespace cace */

#endif /* AGENTCOMMUNICATIONMODEL_H_ */
