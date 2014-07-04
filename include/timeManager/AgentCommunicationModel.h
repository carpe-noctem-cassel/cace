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

		/*!
		 * Adds a new time data element for the network estimations
		 */
		void addData(AgentTimeData* adt);

		/*!
		 * Returns the estimated time difference to this agent
		 */
		long getEstimatedTimeDifference();

		/*!
		 * Computes the estimated networkdelay based on a laplacian propability distribution using
		 * a maximum likelihood estimator
		 */
		ctime getMaxLikelihoodDelay();

		/*!
		 * Computes the estimated networkdelay variance based on a laplacian propability distribution using
		 * a maximum likelihood estimator
		 */
		ctime getMaxLikelihoodDelayVariance(ctime averageDelay);

		/*!
		 * Computes the probability of a packet sent 'timeSincePacketSend' ago,
		 * beeing arrived
		 */
		double getPropabilitySinglePacketArrival(ctime timeSincePacketSend);

		/*!
		 * Computes the probability of a packet sent 'timeSincePacketSend' ago,
		 * beeing arrived
		 */
		double getPropabilitySinglePacketArrival(ctime timeSincePacketSend, long m, long v, double PLoss);

		/*!
		 * Returns the probability of consensus given the apriori probability for resending,
		 * for a time 'timeSincePacketSend' since the last update has been sent
		 */
		double getPropabilityConsensus(ctime timeSincePacketSend, double minResendProbability);

		/*!
		 * Returns the probability of consensus given the apriori probability for resending,
		 * for a time 'timeSincePacketSend' since the last update has been sent
		 */
		double getPropabilityConsensus(ctime timeSincePacketSend, double minResendProbability, long m, long v, double PLoss);

		/*!
		 * Returns the estimated packet loss propability
		 */
		double getProbabilityForPacketLoss();

		/*!
		 * Computes the estimated resendtime based on a laplacian distribution of network jitter
		 */
		ctime getEstimatedResendTime(double propability);

		/*!
		 * Computes the estimated resendtime based on a laplacian distribution of network jitter
		 */
		ctime getEstimatedResendTime(double propability, long m, long v);

		/*!
		 * Computes the estimated time for packet arrival based on a laplacian distribution of network jitter
		 */
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
