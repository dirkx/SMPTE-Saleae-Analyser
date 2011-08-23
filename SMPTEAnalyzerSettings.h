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
 * $Id: SMPTEAnalyzerSettings.h 996 2011-08-23 16:47:34Z dirkx $
 */
#ifndef SMPTE_ANALYZER_SETTINGS
#define SMPTE_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class SMPTEAnalyzerSettings : public AnalyzerSettings
{
public:
	SMPTEAnalyzerSettings();
	virtual ~SMPTEAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();
	
	Channel mInputChannel;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mInputChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceInteger >	mBitRateInterface;
};

#endif //SMPTE_ANALYZER_SETTINGS
