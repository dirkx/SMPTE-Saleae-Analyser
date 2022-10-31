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
 * $Id: SMPTEAnalyzerSettings.cpp 4383 2018-10-02 08:30:40Z dirkx $
 */
#include "SMPTEAnalyzer.h"
#include "SMPTEAnalyzerSettings.h"
#include "AnalyzerHelpers.h"


SMPTEAnalyzerSettings::SMPTEAnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL )
{
	mInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mInputChannelInterface->SetTitleAndTooltip( NAME, TITLE );
	mInputChannelInterface->SetChannel( mInputChannel );

	AddInterface( mInputChannelInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	// AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mInputChannel, NAME, false );
}

SMPTEAnalyzerSettings::~SMPTEAnalyzerSettings()
{
}

bool SMPTEAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface->GetChannel();

	ClearChannels();
	AddChannel( mInputChannel, TITLE, true);

	return true;
}

void SMPTEAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface->SetChannel( mInputChannel );
}

void SMPTEAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;

	ClearChannels();
	AddChannel( mInputChannel, TITLE, true);

	UpdateInterfacesFromSettings();
}

const char* SMPTEAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannel;

	return SetReturnString( text_archive.GetString() );
}
