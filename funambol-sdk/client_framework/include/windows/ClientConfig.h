/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2011 Funambol, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License version 3 as published by
 * the Free Software Foundation with the addition of the following permission
 * added to Section 15 as permitted in Section 7(a): FOR ANY PART OF THE COVERED
 * WORK IN WHICH THE COPYRIGHT IS OWNED BY FUNAMBOL, FUNAMBOL DISCLAIMS THE
 * WARRANTY OF NON INFRINGEMENT  OF THIRD PARTY RIGHTS.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
 * details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, see http://www.gnu.org/licenses or write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA.
 * 
 * You can contact Funambol, Inc. headquarters at 1065 East Hillsdale Blvd.,
 * Ste.400, Foster City, CA 94404 USA, or at email address info@funambol.com.
 * 
 * The interactive user interfaces in modified source and object code versions
 * of this program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU Affero General Public License version 3.
 * 
 * In accordance with Section 7(b) of the GNU Affero General Public License
 * version 3, these Appropriate Legal Notices must retain the display of the
 * "Powered by Funambol" logo. If the display of the logo is not reasonably
 * feasible for technical reasons, the Appropriate Legal Notices must display
 * the words "Powered by Funambol".
 */

#ifndef INCL_CLIENT_CONFIG
#define INCL_CLIENT_CONFIG

/** @cond DEV */

#include "base/fscapi.h"
#include "client/DMTClientConfig.h"
#include "spdm/ManagementNode.h"
#include "spdm/DMTreeFactory.h"
#include "spdm/DMTree.h"
#include "sapi/SapiConfig.h"
#include "MediaHub/MHConfig.h"
#include "MediaHub/MHSyncSource.h"


#include "spdm/DMTreeFactory.h"
#include "spds/DefaultConfigFactory.h"
#include <windows.h>

#include <string>
#include <list>


#include "WindowsDeviceConfig.h"
#include "DateFilter.h"

#define TIMEZONE_CONTEXT                       L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones"

/// Timezone informations.
/// This is a more complete structure than 'TIME_ZONE_INFORMATION' because we need
/// a unique key value to recognize the timezones ('keyName').
typedef struct TimeZone {
    int           index;                 // Unique index of timezone
    std::wstring  keyName;               // Unique name of timezone (english)
    std::wstring  displayName;           // The display name
    bool          isDaylight;            // 'true' if currently under Daylight Saving Time (DST).
    LONG          bias;                  // The current bias for local time translation on this computer, in minutes.
    std::wstring  standardName;          // A description for standard time.
    SYSTEMTIME    standardDate;          // A SYSTEMTIME structure that contains a date and local time when the transition from daylight saving time to standard time occurs on this operating system.
    LONG          standardBias;          // The bias value to be used during local time translations that occur during standard time
    std::wstring  daylightName;          // A description for daylight saving time.
    SYSTEMTIME    daylightDate;          // A SYSTEMTIME structure that contains a date and local time when the transition from standard time to daylight saving time occurs on this operating system.
    LONG          daylightBias;          // The bias value to be used during local time translations that occur during daylight saving time.
} TimeZoneInformation;


using namespace Funambol;
using namespace std;

/**
 * The class to manage the Client Configuration.
 */
class ClientConfig : public DMTClientConfig {

public:

    /// Pointer to CTPManager instance
    static ClientConfig* pinstance;
    
    
    // Constructor
    ClientConfig();
    
    // Method to get the sole instance of CTPManager
    static ClientConfig* getInstance();

    ~ClientConfig();

    void createConfig();

    /**
    * create the default AccessConfig
    */
    static Funambol::AccessConfig* getWinAccessConfig();

    /**
    * Create the default DeviceConfig
    */
    static Funambol::DeviceConfig* getWinDeviceConfig();

    /**
    * return the SyncSourceConfig given the sorce name
    */
    SyncSourceConfig* getWinSyncSourceConfig(const char* name);

    /**
     * Returns a default generated MHConfig for Mac client.
     * 
     * @return: MHConfig pointer allocated new, so it must
     *          be freed by the caller.
     */
    static Funambol::MHConfig* getMHConfig() {return Funambol::DefaultConfigFactory::getMHConfig();};

    /**
     * Returns a default generated SapiConfig for Mac client.
     * 
     * @return: SapiConfig pointer allocated new, so it must
     *          be freed by the caller.
     */
    static Funambol::SapiConfig* getSapiConfig();

    string getInstallDir();
    
    bool read();
    bool save();
    void init();

    void setFunambolSwv(const StringBuffer& v) { funambolSwv = v; }
    const StringBuffer& getFunambolSwv() { return funambolSwv; }

    void createOMHFolder();

    int updateConfig();

    // PIM 
    //WindowsDeviceConfig* winDC;
    TimeZoneInformation currentTimezone;
    /**
     * The date filtering for appointments source. It is in the main config but used only for calendar
     */
    DateFilter appointmentsDateFilter;
    DateFilter& getAppointmentsDateFilter() { return appointmentsDateFilter; }

    int readCurrentTimezone();
    void readPIMSourcesCTCap();
    //void saveDeviceConfig(ManagementNode& n, bool server = false);
    //bool readDeviceConfig(ManagementNode& n, bool server = false);
    const TimeZoneInformation* getCurrentTimezone() const;
    //WindowsDeviceConfig & getWindowsDeviceConfig();
    //WindowsDeviceConfig & getDeviceConfig();

    // read directly to the registry but don't create the key if not exists
    char* readPropertyValueIfExists(const char* context, const char* propertyName, HKEY rootKey, bool bit64 = false) {
        return readPropertyValuePrivate(context, propertyName, rootKey, bit64, true);
    }
    
    // read/save directly to the registry
    char* readPropertyValue(const char* context, const char* propertyName, HKEY rootKey, bool bit64 = false) {
        return readPropertyValuePrivate(context, propertyName, rootKey, bit64, false);
    }
    void savePropertyValue(const StringBuffer& context, const StringBuffer& name, const StringBuffer& value);

    // restore the settings that have been copied temporary from the folder
    void restorePIMSettingsBeforeV12();

private:

    char* readPropertyValuePrivate(const char* context, const char* propertyName, HKEY rootKey, bool bit64, bool justopen);
    
    StringBuffer funambolSwv;

    StringBuffer createUniqueDevID();
    char* ClientConfig::readSystemErrorMsg(DWORD errorCode);
    void ClientConfig::toWindows(char* str);

    // StringBuffer readFunswv(const char* registry); // to not use directly...
    StringBuffer readCurrentFunambolSwv();  // from the OnemediaHub registry
    StringBuffer readNewFunambolSwv();  // from OnemediaHub_inst registry
    char* readNewSwv();
    void saveFunambolSwv(); // save the funambol swv version 

    void initializeVersionsAndUserAgent();

    
    StringBuffer getDefaultSourceFolder(const char* name);
    void createMHFolderIco(StringBuffer folderPath);    

    /* 
     * just for testing pourpose: it is popuplate by nsis installer
     */
    void saveDefaultInstConfigRoot();

    // HRESULT CreateShortcut(StringBuffer shortcutFile, StringBuffer link, StringBuffer desc);
    // BOOL AddToDesktop(StringBuffer folder);

    int getBuildNumberFromVersion(const char* swv);
    int deleteProperty(const char* path, const char* propertyName) ;
};


/** @endcond */
#endif

