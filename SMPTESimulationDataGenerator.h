/* Copyright (c) 2011 Dirk-Willem van Gulik, All Rights Reserved.
 *                    dirkx(at)webweaving(dot)org
 *
 * This file is licensed to you under the Apache License, Version 2.0 
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * $Id: SMPTESimulationDataGenerator.h 996 2011-08-23 16:47:34Z dirkx $
 */
#ifndef SMPTE_SIMULATION_DATA_GENERATOR
#define SMPTE_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class SMPTEAnalyzerSettings;

class SMPTESimulationDataGenerator
{
public:
	SMPTESimulationDataGenerator();
	~SMPTESimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, SMPTEAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	SMPTEAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	U8 mSMPTEPacket[10];
	time_t mSimulationStartTime;
	U32 mPacketIndex;

	SimulationChannelDescriptor mSMPTESimulationData;

};
#endif //SMPTE_SIMULATION_DATA_GENERATOR
