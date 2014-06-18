/*
 * AgentCommunicationModel.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include "timeManager/AgentCommunicationModel.h"

namespace cace
{
	ctime AgentCommunicationModel::defaultDelay=0;
	ctime AgentCommunicationModel::defaultDelayVariance=0;
	const double AgentCommunicationModel::GSL_DBL_EPSILON = 2.2204460492503131e-16;

	AgentCommunicationModel::AgentCommunicationModel(int robotID)
	{
		this->robotID = robotID;
		maxEntrys = 500;
	}

	AgentCommunicationModel::~AgentCommunicationModel()
	{
		for (AgentTimeData* atd : data)
		{
			delete atd;
		}
	}

	void AgentCommunicationModel::addData(AgentTimeData* adt)
	{
		data.push_back(adt);
		while (data.size() > maxEntrys)
		{
			delete (*data.begin());
			data.erase(data.begin());
		}
	}

	ctime AgentCommunicationModel::getEstimatedTimeDifference()
	{
		long ret = 0;
		ctime delay = getMaxLikelihoodDelay();
		list<AgentTimeData*>::iterator it;
		for (it = data.begin(); it != data.end(); it++)
		{
			ret += ((long)((*it)->localTime + delay)) - ((long)(*it)->localMessageArrivalTime);
		}
		return ret / (ctime)data.size();
	}

	ctime AgentCommunicationModel::getMaxLikelihoodDelay()
	{
		//maximum likelhood estimation of laplacian distribution
		long m = 0;
		long num = (long)data.size();

		if (data.size() <= 1)
		{
			return defaultDelay;
		}

		list<AgentTimeData*>::iterator it;
		for (it = data.begin(); it != data.end(); it++)
		{
			ctime delay = (ctime)abs(((long)(*it)->distributedMessageArrivalTime) - ((long)(*it)->distributedTime));
			m += ((long)delay) / num;
		}
		return (ctime)m;
	}

	ctime AgentCommunicationModel::getMaxLikelihoodDelayVariance(ctime averageDelay)
	{
		//maximum likelhood estimation of laplacian distribution
		long v = 0;
		long num = (long)data.size();

		if (data.size() <= 1)
		{
			return defaultDelayVariance;
		}

		//computing variance (laplace distribution interpretation)
		list<AgentTimeData*>::iterator it;
		for (it = data.begin(); it != data.end(); it++)
		{
			long delay = (long)(*it)->distributedMessageArrivalTime - (long)(*it)->distributedTime;
			v += (abs(delay - (long)averageDelay)) / num;
		}
		return (ctime)v;
	}

	double AgentCommunicationModel::getPropabilitySinglePacketArrival(ctime timeSincePacketSend)
	{ //maximum likelhood estimation of laplacian distribution
		long m = (long)getMaxLikelihoodDelay();
		long v = (long)getMaxLikelihoodDelayVariance((ctime)m);

		double ret = 0;
		double PLoss = getProbabilityForPacketLoss();

		double x = ((double)(timeSincePacketSend - m)) / (double)v;
		if (m > timeSincePacketSend)
		{
			ret = 0.5 * exp(x);
		}
		else
		{
			ret = 1.0 - 0.5 * exp(-x);
		}
		ret *= (1 - PLoss);

		return ret;
	}

	double AgentCommunicationModel::getPropabilitySinglePacketArrival(ctime timeSincePacketSend, long m, long v,
																		double PLoss)
	{
		double ret = 0;

		double x = ((double)(timeSincePacketSend - m)) / (double)v;
		if (m > timeSincePacketSend)
		{
			ret = 0.5 * exp(x);
		}
		else
		{
			ret = 1.0 - 0.5 * exp(-x);
		}
		ret *= (1 - PLoss);

		return ret;
	}

	double AgentCommunicationModel::getPropabilityConsensus(ctime timeSincePacketSend, double minResendProbability)
	{
		ctime resendTime = getEstimatedResendTime(minResendProbability);
		int trails = (int)((ulong)timeSincePacketSend / resendTime);

		double ret = 0;
		double PLoss = getProbabilityForPacketLoss();
		for (int i = 0; i <= trails; i++)
		{
			if (i == 0)
			{
				if (timeSincePacketSend > 0)
				{
					ret += getPropabilitySinglePacketArrival(timeSincePacketSend);
				}
			}
			else
			{
				if (timeSincePacketSend - ((long)i * (long)resendTime) > 0)
				{
					double t = PLoss;
					for (int n = 1; n < i; n++)
						t *= PLoss;
					ret += t * getPropabilitySinglePacketArrival(timeSincePacketSend - ((long)i * (long)resendTime));
				}
			}
		}

		return ret;
	}

	double AgentCommunicationModel::getPropabilityConsensus(ctime timeSincePacketSend, double minResendProbability,
															long m, long v, double PLoss)
	{
		ulong resendTime = getEstimatedResendTime(minResendProbability, m, v);
		int trails = (int)((ctime)timeSincePacketSend / resendTime);

		double ret = 0;
		for (int i = 0; i <= trails; i++)
		{
			if (i == 0)
			{
				if (timeSincePacketSend > 0)
				{
					ret += getPropabilitySinglePacketArrival(timeSincePacketSend, m, v, PLoss);
				}
			}
			else
			{
				if (timeSincePacketSend - ((long)i * (long)resendTime) > 0)
				{
					double t = PLoss;
					for (int n = 1; n < i; n++)
						t *= PLoss;
					ret += t
							* getPropabilitySinglePacketArrival(timeSincePacketSend - ((long)i * (long)resendTime), m,
																v, PLoss);
				}
			}
		}

		return ret;
	}

	double AgentCommunicationModel::getProbabilityForPacketLoss()
	{
		AgentTimeData* oldest = nullptr;
		AgentTimeData* newest = nullptr;
		if (data.size() < 2)
		{
			return 0.1;
		}

		list<AgentTimeData*>::iterator it;
		for (it = data.begin(); it != data.end(); it++)
		{
			if (oldest == nullptr)
				oldest = *it;
			if (newest == nullptr)
				newest = *it;
			if ((*it)->distributedTime < oldest->distributedTime)
				oldest = *it;
			if ((*it)->distributedTime > newest->distributedTime)
				newest = *it;
		}
		if (oldest == nullptr || newest == nullptr)
			return 0.1;
		ulong diff = newest->distributedTime - oldest->distributedTime;
		if (diff == 0)
			return 0.1;
		return ((double)data.size())
				/ (1.0 + (((double)diff) / ((double)(TimeManager::timeMessageInterval * 1000000UL))));
	}

	ctime AgentCommunicationModel::getEstimatedResendTime(double propability)
	{
		//maximum likelhood estimation of laplacian distribution
		long m = (long)getMaxLikelihoodDelay();
		long v = (long)getMaxLikelihoodDelayVariance((ulong)m);

		//commulative distribution of the sum two laplacian distributed variables, solved for x:
		long ret = 0;
		double val, err;
		if (propability >= 0.5)
		{
			gSLLambertW1e((4.0 * (propability - 1) / exp(2)), val, err);
			ret = (long)(-((double)v) * (val + 2));
		}
		else
		{
			gSLLambertW1e(-4.0 * propability / exp(2), val, err);
			ret = (long)((double)v * (val + 2));
		}
		ret += 2 * m;
		return (ulong)ret;
	}

	ctime AgentCommunicationModel::getEstimatedResendTime(double propability, long m, long v)
	{
		//commulative distribution of the sum two laplacian distributed variables, solved for x:
		long ret = 0;
		double val, err;
		if (propability >= 0.5)
		{
			gSLLambertW1e((4.0 * (propability - 1) / exp(2)), val, err);
			ret = (long)(-((double)v) * (val + 2));
		}
		else
		{
			gSLLambertW1e(-4.0 * propability / exp(2), val, err);
			ret = (long)((double)v * (val + 2));
		}
		ret += 2 * m;
		return (ulong)ret;
	}

	ctime AgentCommunicationModel::getEstimatedArrivalTime(double propability)
	{
		//maximum likelhood estimation of laplacian distribution
		long m = (long)getMaxLikelihoodDelay();
		long v = (long)getMaxLikelihoodDelayVariance((ulong)m);

		//comulative laplacian distribution solved for x:
		//gnuplot> set xrange [0:1]
		//gnuplot> p 1*log(2*x), -1*log(-2*x+2)
		long ret = 0;
		if (propability >= 0.5)
		{
			ret = (long)(-(double)v * log(-2.0 * propability + 2.0) + (double)m);
		}
		else
		{
			ret = (long)((double)v * log(2.0 * propability) + (double)m);
		}
		return (ulong)ret;
	}

	string AgentCommunicationModel::toString()
	{
		ctime m = getMaxLikelihoodDelay();
		return "AgendCommunicationModel: RobotID " + to_string(robotID) + " DataCount " + to_string(data.size())
				+ " PaketLoss: " + to_string(getProbabilityForPacketLoss()) + " NetworkDelay " + to_string(m)
				+ " NetworkVariance " + to_string(getMaxLikelihoodDelayVariance(m)) + " EstimatedTimeDifference "
				+ to_string(getEstimatedTimeDifference());
	}

	int AgentCommunicationModel::halleyIteration(double x, double w_initial, int max_iters, double& val, double& err)
	{
		double w = w_initial;
		int i;

		for (i = 0; i < max_iters; i++)
		{
			double tol;
			double e = exp(w);
			double p = w + 1.0;
			double t = w * e - x;

			if (w > 0)
			{
				t = (t / p) / e; /* Newton iteration */
			}
			else
			{
				t /= e * p - 0.5 * (p + 1.0) * t / p; /* Halley iteration */
			};

			w -= t;

			tol = 10 * GSL_DBL_EPSILON * max(abs(w), 1.0 / (abs(p) * e));

			if (abs(t) < tol)
			{
				val = w;
				err = 2.0 * tol;
				return 1;
			}
		}

		/* should never get here */
		val = w;
		err = abs(w);
		return max_iters + 1;
	}

	double AgentCommunicationModel::seriesEval(double r)
	{
		double c[] = {-1.0, 2.331643981597124203363536062168, -1.812187885639363490240191647568,
						1.936631114492359755363277457668, -2.353551201881614516821543561516,
						3.066858901050631912893148922704, -4.175335600258177138854984177460,
						5.858023729874774148815053846119, -8.401032217523977370984161688514, 12.250753501314460424,
						-18.100697012472442755, 27.029044799010561650};
		double t_8 = c[8] + r * (c[9] + r * (c[10] + r * c[11]));
		double t_5 = c[5] + r * (c[6] + r * (c[7] + r * t_8));
		double t_1 = c[1] + r * (c[2] + r * (c[3] + r * (c[4] + r * t_5)));
		return c[0] + r * t_1;
	}

	int AgentCommunicationModel::gSLLambertW0e(double x, double& val, double& err)
	{
		double one_over_E = 1.0 / exp(1);
		double q = x + one_over_E;

		if (x == 0.0)
		{
			val = 0.0;
			err = 0.0;
			return 1;
		}
		else if (q < 0.0)
		{
			/* Strictly speaking this is an error. But because of the
			 * arithmetic operation connecting x and q, I am a little
			 * lenient in case of some epsilon overshoot. The following
			 * answer is quite accurate in that case. Anyway, we have
			 * to return GSL_EDOM.
			 */
			val = -1.0;
			err = sqrt(-q);
			return 0;
		}
		else if (q == 0.0)
		{
			val = -1.0;
			err = GSL_DBL_EPSILON; /* cannot error is zero, maybe q == 0 by "accident" */
			return 1;
		}
		else if (q < 1.0e-03)
		{
			/* series near -1/E in sqrt(q) */
			double r = sqrt(q);
			val = seriesEval(r);
			err = 2.0 * GSL_DBL_EPSILON * abs(val);
			return 1;
		}
		else
		{
			const int MAX_ITERS = 10;
			double w;

			if (x < 1.0)
			{
				/* obtain initial approximation from series near x=0;
				 * no need for extra care, since the Halley iteration
				 * converges nicely on this branch
				 */
				double p = sqrt(2.0 * exp(1) * q);
				w = -1.0 + p * (1.0 + p * (-1.0 / 3.0 + p * 11.0 / 72.0));
			}
			else
			{
				/* obtain initial approximation from rough asymptotic */
				w = log(x);
				if (x > 3.0)
					w -= log(w);
			}

			return halleyIteration(x, w, MAX_ITERS, val, err);
		}
	}

	int AgentCommunicationModel::gSLLambertW1e(double x, double& val, double& err)
	{
		if (x > 0.0)
		{
			return gSLLambertW0e(x, val, err);
		}
		else if (x == 0.0)
		{
			val = 0.0;
			err = 0.0;
			return 1;
		}
		else
		{
			int MAX_ITERS = 32;
			double one_over_E = 1.0 / exp(1);
			double q = x + one_over_E;
			double w;

			if (q < 0.0)
			{
				/* As in the W0 branch above, return some reasonable answer anyway. */
				val = -1.0;
				err = sqrt(-q);
				return 0;
			}

			if (x < -1.0e-6)
			{
				/* Obtain initial approximation from series about q = 0,
				 * as long as we're not very close to x = 0.
				 * Use full series and try to bail out if q is too small,
				 * since the Halley iteration has bad convergence properties
				 * in finite arithmetic for q very small, because the
				 * increment alternates and p is near zero.
				 */
				double r = -sqrt(q);
				w = seriesEval(r);
				if (q < 3.0e-3)
				{
					/* this approximation is good enough */
					val = w;
					err = 5.0 * GSL_DBL_EPSILON * abs(w);
					return 1;
				}
			}
			else
			{
				/* Obtain initial approximation from asymptotic near zero. */
				double L_1 = log(-x);
				double L_2 = log(-L_1);
				w = L_1 - L_2 + L_2 / L_1;
			}

			return halleyIteration(x, w, MAX_ITERS, val, err);
		}
	}

} /* namespace cace */
