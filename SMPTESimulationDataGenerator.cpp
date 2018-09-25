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
 * $Id: SMPTESimulationDataGenerator.cpp 992 2011-08-19 13:53:40Z dirkx $
 */
#include "SMPTESimulationDataGenerator.h"
#include "SMPTEAnalyzerSettings.h"
#include "SMPTEAnalyzer.h"
#include <stdlib.h>

#include <AnalyzerHelpers.h>

SMPTESimulationDataGenerator::SMPTESimulationDataGenerator()
:	mPacketIndex( 0 ),
	mSimulationStartTime( 0 )
{
}

SMPTESimulationDataGenerator::~SMPTESimulationDataGenerator()
{
}

void SMPTESimulationDataGenerator::Initialize( U32 simulation_sample_rate, SMPTEAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSimulationStartTime = time(NULL);
	mSettings = settings;

	mSMPTESimulationData.SetChannel( mSettings->mInputChannel );
	mSMPTESimulationData.SetSampleRate( simulation_sample_rate );
	mSMPTESimulationData.SetInitialBitState( BIT_HIGH );
}

#define FRAMERATE (30)
#define	TARGET_RATE (FRAMERATE*80)

#define BCD_L(x) (x%10)
#define BCD_H(x) (x/10)

U32 SMPTESimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );

	U32 samples_per_bit = mSimulationSampleRateHz / TARGET_RATE;

	while( mSMPTESimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
        	U32 samples_per_bit = mSimulationSampleRateHz / TARGET_RATE;
		if (mPacketIndex == 0) {
			U64 delta = FRAMERATE * mSimulationStartTime + FRAMERATE * mSMPTESimulationData.GetCurrentSampleNumber() / mSimulationSampleRateHz;
			U32 frame = delta % FRAMERATE;
			time_t now = delta / FRAMERATE;
	
			struct tm hms;
			gmtime_r(&now, &hms);

			// http://www.alpermann-velte.com/proj_e/tc_intro/ltc_code.html
			// http://en.wikiaudio.org/SMPTE_time_code
			// SMPTE/EBU and LTC format
			//
#if 1
			mSMPTEPacket[0] = BCD_L(frame);
			mSMPTEPacket[1] = BCD_H(frame);
			mSMPTEPacket[2] = BCD_L(hms.tm_sec);
			mSMPTEPacket[3] = BCD_H(hms.tm_sec);
			mSMPTEPacket[4] = BCD_L(hms.tm_min);
			mSMPTEPacket[5] = BCD_H(hms.tm_min);
			mSMPTEPacket[6] = BCD_L(hms.tm_hour);
			mSMPTEPacket[7] = BCD_H(hms.tm_hour);
#else
			mSMPTEPacket[0] = 1; mSMPTEPacket[1] = 2; mSMPTEPacket[2] = 3; mSMPTEPacket[3] = 4;
			mSMPTEPacket[4] = 5; mSMPTEPacket[5] = 6; mSMPTEPacket[6] = 7; mSMPTEPacket[7] = 8;
#endif

			mSMPTEPacket[8] = CODEWORD_1;
			mSMPTEPacket[9] = CODEWORD_2;

			U32 p = 1; // for bte 8 & 9
			for(U32 i = 0; i < 8; i++) 
				p ^= mSMPTEPacket[i];
			p ^= p >> 4; p ^= p >> 2; p ^= p >> 1; 

			// introcude 5% error.
			//
			if (rand() < RAND_MAX/20)
				p ^= p;
			if (p & 1) 
				SETBIT(mSMPTEPacket,27,1);

#ifdef GAP	
	        	mSMPTESimulationData.Advance( samples_per_bit * GAP );
#endif
		};

        	U8 byte = mSMPTEPacket[ mPacketIndex ];
	        mPacketIndex++;
	       	if( mPacketIndex == sizeof(mSMPTEPacket))
        	        mPacketIndex = 0;

        	for( U32 i=0; i<8; i++ )
        	{
               	 	if ((byte & (1<<i)) != 0 ) {
				// introduce a litte error.
				//
				int j1 = samples_per_bit * (0.5 -  rand()  / RAND_MAX ) / 100.0;
				int j2 = samples_per_bit * (0.5 -  rand()  / RAND_MAX ) / 100.0;
        			mSMPTESimulationData.Transition();  
               	 		mSMPTESimulationData.Advance( samples_per_bit / 2 + j1);
        			mSMPTESimulationData.Transition(); 
               	 		mSMPTESimulationData.Advance( samples_per_bit / 2 + j2);
			} else {
				int j2 = samples_per_bit * (0.5 -  rand()  / RAND_MAX ) / 100.0;
               	         	mSMPTESimulationData.Transition();
               	 		mSMPTESimulationData.Advance( samples_per_bit + j2);
			}
        	}
	}

	*simulation_channel = &mSMPTESimulationData;
	return 1;
}

