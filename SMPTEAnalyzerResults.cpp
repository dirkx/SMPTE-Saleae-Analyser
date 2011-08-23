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
 * $Id: SMPTEAnalyzerResults.cpp 996 2011-08-23 16:47:34Z dirkx $
 */
#include "SMPTEAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "SMPTEAnalyzer.h"
#include "SMPTEAnalyzerSettings.h"
#include <iostream>
#include <fstream>
#include <string.h>
#include <strings.h>


SMPTEAnalyzerResults::SMPTEAnalyzerResults( SMPTEAnalyzer* analyzer, SMPTEAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

SMPTEAnalyzerResults::~SMPTEAnalyzerResults()
{
}

void SMPTEAnalyzerResults::fillSMPTE(Frame frame, char * resultstr) 
{
	//                    frame     seconds   minutes   hours  
	const int max[8] = {   9,   4,   9,   5,   9,   5,   9,   2 };
	const int msk[8] = { 0xF, 0x3, 0xF, 0x7, 0xF, 0x7, 0xF, 0x3 };

	char str[128];
	char str2[128];
	int i;

	// We avoid direct bit operations on mData - as we're not
	// sure of the byte order of the varios platforms.
	//
	U8 *m = (U8*)&(frame.mData1);

	bzero(str, sizeof(str));
	for(i = 7; i >= 0; i--) {
		U8 c = m[i] & msk[i];
		if (c <= max[i])
			c += '0';
		else
			c = '?';
		str[ strlen(str) ] = c;

		if (i && (i %2) == 0) {
			str[ strlen(str) ] = (i == 2) ? '.' : ':';
		};
	};
	if (!resultstr) {
		AddResultString("T");
		AddResultString("TS");
		AddResultString( str + 8); // Just the .framecount
		AddResultString( str + 6); // Seconds, frames
		AddResultString( str + 3); // min/seconds/frames
		AddResultString( str );	   // whole HMS.frame
	};
	
	// From here we work on an abridge string and a fully
	// elaborated string in parallel.
	//	
	strcpy(str2,str);

	if (BIT(m,DROPBIT)) {
		strncat(str,"d", sizeof(str)-1);
		strncat(str2," drop", sizeof(str)-1);
	} else {
		strncat(str2," no-drop", sizeof(str)-1);
	};

	if (BIT(m,COLBIT)) {
		strncat(str,"c", sizeof(str)-1);
		strncat(str2," colour", sizeof(str)-1);
	} else {
		strncat(str2," no-colour", sizeof(str)-1);
	};
	
	// stick to machine byte order indepennt method.
	//
	// U64 p = frame.mData1; p ^= p >> 32; p ^= p >> 8; p ^= p >> 4; p ^= p >> 2; p ^= p >> 1; 
	//
	U32 p = 1; // for bte 8 & 9
	for(U32 i = 0; i < 8; i++) p ^= m[i];
	p ^= p >> 4; p ^= p >> 2; p ^= p >> 1; 

	if ((p & 1) == BIT(m,PHASEBIT)) {
		strncat(str,"p", sizeof(str)-1);
		strncat(str2," Parity-ok", sizeof(str)-1);
	} else {
		strncat(str," P", sizeof(str)-1);
		strncat(str2," Parity-fail", sizeof(str)-1);
	};
	char buff[128];
	snprintf(buff,sizeof(buff)," User=%X-%X-%X-%X-%X-%X-%X-%X, groupBits=%d%d, reserved=%d",
			       //  0 ..  3  -- L frame
		(m[0]>>4)&0xF, //  4 ..  7     user 1
			       //  8 ..  9  -- H frame
			       //  10          drop frame
			       //  11          colour
		(m[1]>>4)&0xF, // 12 .. 15     user 2
			       // 16 .. 19     L sec
		(m[2]>>4)&0xF, // 20 .. 23     user 3
			       // 24 .. 26     H sec
                               //              parity bit
		(m[3]>>4)&0xF, // 28 .. 31     user 4
			       // 32 .. 35     L min
		(m[4]>>4)&0xF, // 36 .. 39     user 5
			       // 40 .. 42     H min
                               // 43           group bit 
		(m[5]>>4)&0xF, // 44 .. 47     user 6
			       // 47 .. 51     L hour
		(m[6]>>4)&0xF, // 52 .. 55     user 7
			       // 56 .. 57     H hour
                               // 58           reserved
                               // 59           group bit
		(m[7]>>4)&0xF, // 60 .. 63     user 8
			       // 64 .. 79     code words (16 bits)
		BIT(m,GR1BIT), // 43 - group bit
		BIT(m,GR2BIT), // 59 - group bit
		BIT(m,RESBIT)  // 58 - reserved
	);
	strncat(str2, buff, sizeof(str2)-1);

	if (!resultstr) {
		AddResultString( str );
		AddResultString( str2 );
	} else {
		strcpy(resultstr, str2);
	};
}

void SMPTEAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	Frame frame = GetFrame( frame_index );

	ClearResultStrings();
	fillSMPTE(frame,NULL);
}

void SMPTEAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "# " << TITLE << " / " << VERSION << std::endl;
	file_stream << "Time [s],SMPTE Timestamp HH:MM:SS.frame (flags)" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

		while(strlen(time_str)<9)
			strcat(time_str,"0");

		char str[128];
	        fillSMPTE(frame,str);

		file_stream << time_str << "," << str << std::endl;

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void SMPTEAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	Frame frame = GetFrame( frame_index );
	char str[128];
	fillSMPTE(frame,str);

	ClearResultStrings();
	AddResultString( str );
}

void SMPTEAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void SMPTEAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}
