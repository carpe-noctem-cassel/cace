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
		AgentTimeData(ctime localTime, ctime distributedTime, ctime distributedMessageArrivalTime, ctime localMessageArrivalTime, int robotID);
		virtual ~AgentTimeData();

		ctime localTime;
		ctime distributedTime;
		ctime distributedMessageArrivalTime;
		ctime localMessageArrivalTime;
		int robotID;
	};

} /* namespace cace */

#endif /* AGENTTIMEDATA_H_ */
