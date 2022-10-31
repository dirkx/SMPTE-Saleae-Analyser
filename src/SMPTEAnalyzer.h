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
 * $Id: SMPTEAnalyzer.h 4383 2018-10-02 08:30:40Z dirkx $
 */

#ifndef SMPTE_ANALYZER_H
#define SMPTE_ANALYZER_H

#include "Analyzer.h"

#include "SMPTEAnalyzerResults.h"
#include "SMPTESimulationDataGenerator.h"

#define NAME "SMPTE/LTC"
#define TITLE "SMPTE/EBU Timecode (BiPhase Mark 'LTC' 80 bits)"
#define VERSION "0.02 - 2011-08-19"

class SMPTEAnalyzerSettings;
class ANALYZER_EXPORT SMPTEAnalyzer : public Analyzer2
{
public:
	SMPTEAnalyzer();
	virtual ~SMPTEAnalyzer();
	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars
	std::auto_ptr< SMPTEAnalyzerSettings > mSettings;
	std::auto_ptr< SMPTEAnalyzerResults > mResults;
	AnalyzerChannelData* mSMPTE;

	SMPTESimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Serial analysis vars:
	U32 mSampleRateHz;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#define DROPBIT	 (10)	
#define COLBIT   (11)	// also marking timecode connection
#define PHASEBIT (27)
#define GR1BIT	 (43)	// also identification bit
#define RESBIT	 (58)	// reserved/not allocated
#define GR2BIT	 (59)

#define	CODEWORD_1 (0xFC)
#define	CODEWORD_2 (0xBF)

#define	BIT(bytes,i) (((bytes)[((i)/8)] & (1<<(i) % 8)) ? 1 : 0)
#define SETBIT(bytes,i,s) ((bytes)[(i)/8] = ((bytes)[(i)/8] & (~(1<<((i) % 8)))) | (s ? (1<<((i)%8)) : 0))

#endif //SMPTE_ANALYZER_H
