/*
 * AgentTimeData.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include "timeManager/AgentTimeData.h"

namespace cace
{

	AgentTimeData::AgentTimeData(ctime localTime, ctime distributedTime, ctime distributedMessageArrivalTime,
									ctime localMessageArrivalTime, int robotID)
	{
		this->localTime = localTime;
		this->distributedTime = distributedTime;
		this->distributedMessageArrivalTime = distributedMessageArrivalTime;
		this->robotID = robotID;
		this->localMessageArrivalTime = localMessageArrivalTime;
	}

	AgentTimeData::~AgentTimeData()
	{
	}

} /* namespace cace */
