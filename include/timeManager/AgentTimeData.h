/*
 * AgentTimeData.h
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#ifndef AGENTTIMEDATA_H_
#define AGENTTIMEDATA_H_

#include "CaceTypes.h"

namespace cace
{

	class AgentTimeData
	{
	public:
		/*!
		 * Information about Cace Time receivement
		 */
		AgentTimeData(ctime localTime, ctime distributedTime, ctime distributedMessageArrivalTime, ctime localMessageArrivalTime, int robotID);
		virtual ~AgentTimeData();

		/*!
		 * Local time when sending timestamp
		 */
		ctime localTime;

		/*!
		 * Distributed time when sending timestamp
		 */
		ctime distributedTime;

		/*!
		 * Distributed time when timestamp arrived
		 */
		ctime distributedMessageArrivalTime;

		/*!
		 * Local time when timestamp arrived
		 */
		ctime localMessageArrivalTime;

		/*!
		 * robot sending time
		 */
		int robotID;
	};

} /* namespace cace */

#endif /* AGENTTIMEDATA_H_ */
