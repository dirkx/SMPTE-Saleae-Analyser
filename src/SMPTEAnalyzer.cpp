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
 * $Id: SMPTEAnalyzer.cpp 4383 2018-10-02 08:30:40Z dirkx $
 */
#include "SMPTEAnalyzer.h"
#include "SMPTEAnalyzerSettings.h"
#include "AnalyzerChannelData.h"

#include <stdio.h>
#include <strings.h>

#define SMPTE_DEBUG_2 0

SMPTEAnalyzer::SMPTEAnalyzer()
:	Analyzer2(),  
	mSettings( new SMPTEAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}

SMPTEAnalyzer::~SMPTEAnalyzer()
{
	KillThread();
}

void SMPTEAnalyzer::SetupResults()
{
	mResults.reset( new SMPTEAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );
}

// returns start of SMPTE sequence, terminated by
// the code Word.
int findCodeWord(U8 packet[]) {

	if (packet[8] == CODEWORD_1 && packet[9] == CODEWORD_2) {
		return 0;
	};

	U8 i = 0;
	for(i = 0; i < 80; i++) {
		U32 c1 = ((packet[(i/8+0)%10] + ((packet[(i/8+1)%10])<<8)) >> (i % 8)) & 0xFF;

		if (c1 != CODEWORD_1)
			continue;
		U32 c2 = ((packet[(i/8+1)%10] + ((packet[(i/8+2)%10])<<8)) >> (i % 8)) & 0xFF;
		if (c2 != CODEWORD_2)
			continue;

		return (i-8*8 + 80) % 80;
	};

	return -1;
}

void SMPTEAnalyzer::WorkerThread()
{
	mSampleRateHz = GetSampleRate();
	mSMPTE = GetAnalyzerChannelData( mSettings->mInputChannel );

	U8 packet[10];
	U64 snos[80];
	U64 starting_sample = 0;
	U64 at = 0;

	U64 t0 = 0, t1;
	U8 off = 10;
	U64 delta = 0;
	for( ; ; )
	{
		U64 newDelta = delta;

		mSMPTE->AdvanceToNextEdge();
		t0 = t1;
		t1 = mSMPTE->GetSampleNumber();

		U64 d = t1 - t0;

		if (d < delta * 12 / 10) {
			newDelta = d;

			mSMPTE->AdvanceToNextEdge();
			U64 t2 = mSMPTE->GetSampleNumber();

			d = t2 - t1;
			if ((d > delta * 12 / 10) && (d < delta * 22/10)) {
				// we got it the wrong way round prolly.
#if SMPTE_DEBUG_2
				fprintf(stderr,"Swap 0/1 concepts. Should be rare\n");
#endif
				newDelta = d;
			} else {
				packet[ at/8 ] |= (1 << (at % 8));
			};
			t1 = t2;
		} else {
			newDelta = d / 2;
		};

		// Allow a bit of tracking left and right - but bail after a couple of cycles
		// and then do what we have.
		const U8 tracking = 10;  
		const U8 maxlen = 4;
		if ((delta < (100-tracking)*newDelta/100) || (delta > (100+tracking)*newDelta/100)) 
			off ++; 
		else 
			off = 0;

		if (off < maxlen) {
			delta = (3.*delta + newDelta)/4;
		} else {
			delta = newDelta;
		}

		snos[at] = t0;
		at++;

		// Chechk for any sync bits; and roll the
		// array if such is the case.
		//
		if (at < 80) 
			continue;



#if SMPTE_DEBUG_2
		fprintf(stderr,"\n---\n");
		for(int j = 0; j < 8; j++) {
			fprintf(stderr, "bit %d: ", j); 
			for(int i = 0; i < 10; i++)  {
				U32 c = ((packet[(i+1) % 10]*256 + packet[i])>>j) & 0xFF;
				fprintf(stderr,"%02x ", c);
			};
			fprintf(stderr,"\n");
		}
#endif

		int cw = findCodeWord(packet);
		if (cw < 0) { 
#if SMPTE_DEBUG_2
			fprintf(stderr,"Complete miss - no sign of codewords (should only happen initially once)\n");
#endif
			bzero(packet,10);
			at = 0;
			continue;
		};

		if (cw != 0) {
			at = 80 - cw;
#if SMPTE_DEBUG_2
			fprintf(stderr,"Partial find cw=%ld, fill from at=%ld - try to retrain\n", cw, at);
#endif
			// not very efficient - but rare.
			for(U32 i = 0; i < 80; i++)  {
				U32 b = 0;
				if (i < at) {
					b = (packet[ (cw+i)/8 ] & (1<<((cw+i)%8))) ? 1 : 0;
					snos[i] = snos[(i+cw) % 80];
				};
				packet[i/8] = ( packet[i/8] & (~(1<<(i % 8)))) | ((b<<(i % 8)));
			};
#if SMPTE_DEBUG_2
			fprintf(stderr,"Post - shuffle - break at %d:%d:\n", at/8, at%8);
			for(int i = 0; i < 10; i++)  {
				U8 c =packet[i];
				fprintf(stderr,"%02x ", c);
			};
			fprintf(stderr,"\n");
#endif	
			starting_sample = snos[0];
#if SMPTE_DEBUG_2
			fprintf(stderr," -- Full set %f - restart D=%f, t0=%f, t1=%f %f\n", at, delta, t0, t1, starting_sample);
#endif
			continue;
		};
#if SMPTE_DEBUG_2
		fprintf(stderr,"Processing %ld\n", starting_sample);
#endif

		mResults->AddMarker( snos[0] - delta / 2, AnalyzerResults::Start, mSettings->mInputChannel );
		for(int i=0; i <80; i++ ) {
			mResults->AddMarker( snos[i],
					(packet[i/8]&(1<<(i%8))) ? AnalyzerResults::One : AnalyzerResults::Zero,
					mSettings->mInputChannel );
			if (i && (i % 8) == 0)
				mResults->AddMarker( snos[i]+delta/2, AnalyzerResults::Dot, mSettings->mInputChannel );
		};

		mResults->AddMarker( snos[79] + delta / 2, AnalyzerResults::Stop, mSettings->mInputChannel );
		// mResults->CommitPacketAndStartNewPacket();

		// We have something ending with the right
		// codeword. Assume that the rest is more
		// or less cosher.
		//
		Frame frame;

		// Just byte 0 ..7; as 8 and 9 are only codeWords for sync.
		//
		frame.mData1 = *(U64 *)packet;
		frame.mFlags = 0;

		frame.mStartingSampleInclusive = starting_sample;
		frame.mEndingSampleInclusive = mSMPTE->GetSampleNumber();

		mResults->AddFrame( frame );
		mResults->CommitResults();

		ReportProgress( frame.mEndingSampleInclusive );

		starting_sample = frame.mEndingSampleInclusive;
		at = 0;
		bzero(packet,10);

		CheckIfThreadShouldExit();
	}
}

bool SMPTEAnalyzer::NeedsRerun()
{
	return false;
}

U32 SMPTEAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 SMPTEAnalyzer::GetMinimumSampleRateHz()
{
	return 0;
}

const char* SMPTEAnalyzer::GetAnalyzerName() const
{
	return TITLE;
}

const char* GetAnalyzerName()
{
	return TITLE;
}

Analyzer* CreateAnalyzer()
{
	return new SMPTEAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
