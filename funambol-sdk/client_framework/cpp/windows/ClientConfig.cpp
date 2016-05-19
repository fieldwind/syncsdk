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


#include "ClientConfig.h"
#include "base/util/StringBuffer.h"
#include "base/Log.h"
#include <cstdio>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <map>
#include "base/adapter/PlatformAdapter.h"
#include "base/stringUtils.h"
#include "spds/DataTransformer.h"
#include "spds/DataTransformerFactory.h"
#include "WinClientConstants.h"
#include "mediaHub/MHFileSyncSource.h"

#include "outlook/outlookDataTransformer.h"

using namespace std;
using namespace Funambol;

// Init static pointer.
ClientConfig* ClientConfig::pinstance = NULL;

/**
 * Method to create the sole instance of CTPManager
 */
ClientConfig* ClientConfig::getInstance() {

    if (pinstance == NULL) {
        pinstance = new ClientConfig;
        pinstance->init();
    }
    return pinstance;
}

ClientConfig::ClientConfig() {
    //winDC = NULL;

}

/**
 * Denstructor: stop any active thread, and close socket
 * connection if still active.
 */
ClientConfig::~ClientConfig() {

}

void ClientConfig::init() {
    
    setCredInfo(CREDINFO);
    setAccessConfigUserToken(ENCRYPT_ACCESS_CONFIG_PASSWORD);
    setSessionIdExpirationTime(SESSION_ID_EXP_TIME);
    setValidationKey("");
    setSSLVerifyServer(ENABLE_SSL_CERTIFICATE_CHECK);

    // Read the configuration
    if (!read()) {
        if (readNewFunambolSwv() == "") {
            saveDefaultInstConfigRoot();
        }
        // Config not found, generate a default one
        createConfig();
        
    } 
    const char* cred_info = getCredInfo();
    if (strcmp(cred_info, CREDINFO)) {
        setCredInfo(CREDINFO);
        save();
    }

    // set the filter for appointment source for outlook in any case
    SyncSourceConfig* ssc = getSyncSourceConfig(CALENDAR_SOURCE_NAME);
    if (ssc) {
        bool err = false;
        int val = ssc->getIntProperty(PROPERTY_FILTER_DATE_DIRECTION, &err);
        appointmentsDateFilter.setDirection((DateFilter::FilterDirection)val);

        val = ssc->getIntProperty(PROPERTY_FILTER_DATE_LOWER, &err);
        appointmentsDateFilter.setRelativeLowerDate((DateFilter::RelativeLowerDate)val);
    }
    
}


bool ClientConfig::read() {
    
    if (!DMTClientConfig::read()) {
        return false; // error in the common config read.
    }

    if (strcmp(DMTClientConfig::getAccessConfig().getUserAgent(), "") == 0) {
        return false;
    }

    // it is just part of the Windows configuration not the DMT
    funambolSwv = readCurrentFunambolSwv();

    return true;
}

/**
 * Overload the save() method, adding the code to store client-specific values.
 *
 * @return  true for success
 */
bool ClientConfig::save() {

    if ( !DMTClientConfig::save() ) {
        return false; // error in the common config save.
    }
    
    // Saves the Funambol sw version
    saveFunambolSwv();

    return true;
}

/**
 * Method to create a default config.
*/
void ClientConfig::createConfig() {

    AccessConfig* ac = getWinAccessConfig();
    ac->setMaxMsgSize(60000);
    ac->setUsername ("");
    ac->setPassword ("");
    ac->setSyncURL (DEFAULT_SYNC_URL);

    this->setAccessConfig(*ac);
    delete ac;

    DeviceConfig* dc = getWinDeviceConfig();  
    StringBuffer deviceID = createUniqueDevID();
    dc->setDevID(deviceID.c_str());
    this->setDeviceConfig(*dc);
    delete dc;

    MHConfig* mhConfig = getMHConfig();
    this->setMHConfig(*mhConfig);
    delete mhConfig;

    SyncSourceConfig* sc = getWinSyncSourceConfig(PICTURES_SOURCE_NAME);    
    this->setSyncSourceConfig(*sc);
    delete sc;

    sc = getWinSyncSourceConfig(VIDEOS_SOURCE_NAME);    
    this->setSyncSourceConfig(*sc);
    delete sc;

    sc = getWinSyncSourceConfig(FILES_SOURCE_NAME);    
    this->setSyncSourceConfig(*sc);
    delete sc;

    sc = getWinSyncSourceConfig(MUSIC_SOURCE_NAME);    
    this->setSyncSourceConfig(*sc);
    delete sc;

    sc = getWinSyncSourceConfig(CONTACTS_SOURCE_NAME);    
    this->setSyncSourceConfig(*sc);
    delete sc;

    sc = getWinSyncSourceConfig(CALENDAR_SOURCE_NAME);    
    this->setSyncSourceConfig(*sc);
    delete sc;
    
    setSourceOrder(SOURCE_ORDER);
    
    // set the default useragent and version
    initializeVersionsAndUserAgent();

    setCredInfo(CREDINFO);
    setAccessConfigUserToken(ENCRYPT_ACCESS_CONFIG_PASSWORD);
    setSessionIdExpirationTime(SESSION_ID_EXP_TIME);
    setValidationKey("");
    setSubscriptionsEnabled("");
    
    // save the configuration
    save();
}

AccessConfig* ClientConfig::getWinAccessConfig() {
    
    AccessConfig* ac = new AccessConfig();
    
   
    ac->setUsername             ("");
    ac->setPassword             ("");
    ac->setFirstTimeSyncMode    (SYNC_NONE);
    ac->setUseProxy             (true);
    ac->setProxyHost            ("");
    ac->setProxyPort            (8080);
    ac->setProxyUsername        ("");
    ac->setProxyPassword        ("");
    ac->setSyncURL              ("");
    ac->setBeginSync            (0);
    ac->setEndSync              (0);
    ac->setServerAuthRequired   (false);
    ac->setClientAuthType       ("syncml:auth-basic");      // may be changed at runtime is AuthenticationType = OAUTH2
    ac->setServerAuthType       ("syncml:auth-basic");
    ac->setServerPWD            ("funambol");
    ac->setServerID             ("funambol");
    ac->setServerNonce          ("");
    ac->setClientNonce          ("");
    ac->setMaxMsgSize           (256000);
    ac->setReadBufferSize       (0);
    ac->setUserAgent            ("");
    ac->setCheckConn            (true);
    ac->setResponseTimeout      (0);   
   
    return ac;
}

DeviceConfig* ClientConfig::getWinDeviceConfig() {
    
    DeviceConfig* dc = new DeviceConfig();
    
    StringBuffer deviceId = "fol-windows-client";  // will be replaced before saving

    dc->setMan                  ("Funambol Inc.");
    dc->setMod                  ("Windows PC App");
    dc->setOem                  ("");
    dc->setFwv                  ("");
    dc->setSwv                  ("");
    dc->setHwv                  ("");
    dc->setDevID                (deviceId.c_str());
    dc->setDevType              ("Windows");
    dc->setDsV                  ("1.2");
    dc->setUtc                  (true);
    dc->setLoSupport            (false);
    dc->setNocSupport           (true);
    dc->setLogLevel             (DEFAULT_LOG_LEVEL);
    dc->setMaxObjSize           (5000000);
    dc->setDevInfHash           ("");
    
    return dc;
}


StringBuffer ClientConfig::getDefaultSourceFolder(const char* name) {

    StringBuffer defaultFolder = "";
    HRESULT result = S_FALSE;
    WCHAR path [2048];
    
    if (strcmp(name, PICTURES_SOURCE_NAME) == 0) {
        result = SHGetFolderPath(NULL, CSIDL_MYPICTURES, NULL, SHGFP_TYPE_CURRENT, path);
    } 
    else if (strcmp(name, VIDEOS_SOURCE_NAME) == 0) {
        result = SHGetFolderPath(NULL, CSIDL_MYVIDEO, NULL, SHGFP_TYPE_CURRENT, path);
    } 
    else if (strcmp(name, MUSIC_SOURCE_NAME) == 0) {
        result = SHGetFolderPath(NULL, CSIDL_MYMUSIC, NULL, SHGFP_TYPE_CURRENT, path);
    }
    
    if (S_OK == result) {
        defaultFolder.convert(path);
    }

    if (defaultFolder.empty() == false) {
        defaultFolder.append("/");
        defaultFolder.replaceAll("\\", "/");
    }
    return defaultFolder;
}


void ClientConfig::createMHFolderIco(StringBuffer folderPath) {

    StringBuffer file(folderPath);
    if (!folderPath.endsWith("/")) {
        file.append("/");
    }
    file.append("Desktop.ini");
    WCHAR* wfile = toWideChar(file.c_str());
    
    string filePath = getInstallDir();
    if (filePath.empty()) {
        return;
    }
    filePath.append("\\images\\MediaHubFolder.ico");

    FILE* f = fileOpen(file.c_str(), "w+");
    if (f) {
        StringBuffer s;
        s = "[.ShellClassInfo]\r\n";
        s.append("IconFile=");
        s.append(filePath.c_str());
        s.append("\r\n");
        s.append("IconIndex=0\r\n");
        s.append("InfoTip=");
        s.append(APP_NAME);

        fwrite(s.c_str(), 1, s.length(), f);
        fclose(f);
        SetFileAttributes(wfile, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    }
    SHChangeNotify(SHCNE_UPDATEIMAGE, SHCNF_FLUSHNOWAIT, NULL, NULL);
}

void ClientConfig::createOMHFolder() {

    StringBuffer itemsDownloadDefaultPath; 
    itemsDownloadDefaultPath.sprintf("%s/%s/", PlatformAdapter::getHomeFolder().c_str(), 
                                MEDIA_HUB_DEFAULT_FOLDER_NAME);
    createFolder(itemsDownloadDefaultPath.c_str());

    createMHFolderIco(itemsDownloadDefaultPath);
    
}

SyncSourceConfig* ClientConfig::getWinSyncSourceConfig(const char* name) {
    
    SyncSourceConfig* sc = new SyncSourceConfig();
    
    sc->setName                 (name);
    StringBuffer itemsDownloadDefaultPath, itemsUploadDefaultPath;    
    itemsDownloadDefaultPath.sprintf("%s/%s/", PlatformAdapter::getHomeFolder().c_str(), 
                                MEDIA_HUB_DEFAULT_FOLDER_NAME);
    itemsUploadDefaultPath.sprintf("|%s/%s/", PlatformAdapter::getHomeFolder().c_str(), 
                                MEDIA_HUB_DEFAULT_FOLDER_NAME);

    string defaultFolder = "";
    
    sc->setSync                 ("two-way");
    sc->setSyncModes            ("slow,two-way,one-way-from-server,one-way-from-client");
    sc->setLast                 (0);
    sc->setEncryption           ("");
    
   
    if (!strcmp(name, CONTACTS_SOURCE_NAME)){
        
        sc->setEncoding         ("bin");
        sc->setURI              ("card");
        sc->setSupportedTypes   ("text/x-vcard");
        sc->setType             ("text/x-vcard");
        sc->setVersion          ("2.1");
        sc->setSyncMode         (CONTACTS_SOURCE_SYNC_MODE);
        sc->setSyncModes        (CONTACTS_DEVINFO_SYNC_MODES);
        sc->setBoolProperty     (PROPERTY_USE_SUBFOLDERS, true);
        sc->setProperty         (PROPERTY_FOLDER_PATH, "");  
    }
    else if (!strcmp(name, CALENDAR_SOURCE_NAME)){
        sc->setEncoding         ("bin");
        sc->setURI              ("event");
        sc->setSupportedTypes   ("text/x-vcalendar");
        sc->setType             ("text/x-vcalendar");
        sc->setVersion          ("1.0");
        sc->setSyncMode         (CALENDAR_SOURCE_SYNC_MODE);
        sc->setSyncModes        (CALENDAR_DEVINFO_SYNC_MODES);
        sc->setBoolProperty     (PROPERTY_USE_SUBFOLDERS, true);
        sc->setProperty         (PROPERTY_FOLDER_PATH, "");
        // date filtering
        sc->setIntProperty      (PROPERTY_FILTER_DATE_DIRECTION, DateFilter::DIR_OUT);
        sc->setIntProperty      (PROPERTY_FILTER_DATE_LOWER,     DateFilter::LAST_MONTH);        
    }    
    else if (!strcmp(name, PICTURES_SOURCE_NAME)) {
        StringBuffer fullFolders = itemsUploadDefaultPath;
        StringBuffer folder = getDefaultSourceFolder(PICTURES_SOURCE_NAME);
        defaultFolder = folder.c_str();
        if (defaultFolder.empty() == false) {
            defaultFolder = defaultFolder.insert(0, "|!");
            fullFolders.append(",");
            fullFolders.append(defaultFolder.c_str());            
        }

        sc->setSync             ("one-way-from-client");
        sc->setEncoding         ("bin");
        sc->setURI              ("picture");
        sc->setSupportedTypes   ("image/*");
        sc->setType             ("image/*");
        sc->setSyncMode         (PICTURES_SOURCE_SYNC_MODE);
        sc->setProperty         (PROPERTY_DOWNLOAD_LAST_TIME_STAMP,     "0");
        sc->setIntProperty      (PROPERTY_SYNC_ITEM_NUMBER_FROM_CLIENT, -1);
        sc->setIntProperty      (PROPERTY_SYNC_ITEM_NUMBER_FROM_SERVER, -1);
        sc->setProperty         (PROPERTY_EXTENSION,                    PICT_EXTENSION);
        sc->setProperty         (MHFileSyncSource::PROPERTY_EXCLUDED_FILE_NAMES,      PICTURE_NAMES_EXCLUDED);
        sc->setProperty         (MHFileSyncSource::PROPERTY_EXCLUDED_DIRECTORY_NAMES, PICTURE_DIR_NAMES_EXCLUDED);
        sc->setProperty         (MHFileSyncSource::PROPERTY_ITEMS_DOWNLOAD_DIRECTORY, itemsDownloadDefaultPath.c_str());
        sc->setProperty         (MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_DIRECTORIES_LIST, fullFolders.c_str());
		sc->setProperty         (MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_NETWORK_DIRECTORIES_LIST, "");
		sc->setProperty         (MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_REMOVABLE_DIRECTORIES_LIST, "");
        
        sc->setProperty         (PROPERTY_LOCAL_QUOTA_STORAGE,          SAPI_LOCAL_QUOTA_STORAGE);
        sc->setLongLongProperty      (PROPERTY_SYNC_ITEM_MAX_SIZE,           (int64_t)SAPI_MAX_PICTURE_SIZE);   // 0 = unlimited; no more used in settings, just in Win<SourceName>SourcePlugin
    }
    else if (!strcmp(name, VIDEOS_SOURCE_NAME)) {
        StringBuffer fullFolders = itemsUploadDefaultPath;
        StringBuffer folder = getDefaultSourceFolder(VIDEOS_SOURCE_NAME);
        defaultFolder = folder.c_str(); 
        if (defaultFolder.empty() == false) {
            defaultFolder = defaultFolder.insert(0, "|!");
            fullFolders.append(",");
            fullFolders.append(defaultFolder.c_str());            
        }

        sc->setSync             ("one-way-from-client");
        sc->setEncoding         ("bin");
        sc->setURI              ("video");
        sc->setSupportedTypes   ("video/*");
        sc->setType             ("video/*");
        sc->setSyncMode         (VIDEOS_SOURCE_SYNC_MODE);
        sc->setProperty         (PROPERTY_DOWNLOAD_LAST_TIME_STAMP,     "0");
        sc->setIntProperty      (PROPERTY_SYNC_ITEM_NUMBER_FROM_CLIENT, -1);
        sc->setIntProperty      (PROPERTY_SYNC_ITEM_NUMBER_FROM_SERVER, -1);
        sc->setProperty         (PROPERTY_EXTENSION,                    VIDEO_EXTENSION);
        sc->setProperty         (MHFileSyncSource::PROPERTY_EXCLUDED_FILE_NAMES,      VIDEO_NAMES_EXCLUDED);
        sc->setProperty         (MHFileSyncSource::PROPERTY_EXCLUDED_DIRECTORY_NAMES, VIDEO_DIR_NAMES_EXCLUDED);
        sc->setProperty         (MHFileSyncSource::PROPERTY_ITEMS_DOWNLOAD_DIRECTORY, itemsDownloadDefaultPath.c_str());
        sc->setProperty         (MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_DIRECTORIES_LIST, fullFolders.c_str());
		sc->setProperty         (MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_NETWORK_DIRECTORIES_LIST, "");
		sc->setProperty         (MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_REMOVABLE_DIRECTORIES_LIST, "");

        sc->setProperty         (PROPERTY_LOCAL_QUOTA_STORAGE,          SAPI_LOCAL_QUOTA_STORAGE);
        sc->setLongLongProperty      (PROPERTY_SYNC_ITEM_MAX_SIZE,           (int64_t)SAPI_MAX_VIDEO_SIZE); // no more used in settings, just in Win<SourceName>SourcePlugin
    }
    else if (!strcmp(name, FILES_SOURCE_NAME)) {
        sc->setSync             ("one-way-from-client");
        sc->setEncoding         ("bin");
        sc->setURI              ("files");
        sc->setSupportedTypes   ("application/*");
        sc->setType             ("application/*");
        sc->setSyncMode         (FILES_SOURCE_SYNC_MODE);
        sc->setProperty         (PROPERTY_DOWNLOAD_LAST_TIME_STAMP,     "0");
        sc->setIntProperty      (PROPERTY_SYNC_ITEM_NUMBER_FROM_CLIENT, -1);
        sc->setIntProperty      (PROPERTY_SYNC_ITEM_NUMBER_FROM_SERVER, -1);
        sc->setProperty         (PROPERTY_EXTENSION,                    FILE_EXTENSION);
        sc->setProperty         (MHFileSyncSource::PROPERTY_EXCLUDED_FILE_NAMES, FILE_NAMES_EXCLUDED);
        sc->setProperty         (MHFileSyncSource::PROPERTY_EXCLUDED_DIRECTORY_NAMES, FILE_DIR_NAMES_EXCLUDED);
        sc->setProperty         (MHFileSyncSource::PROPERTY_ITEMS_DOWNLOAD_DIRECTORY, itemsDownloadDefaultPath.c_str());
        sc->setProperty         (MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_DIRECTORIES_LIST, itemsUploadDefaultPath.c_str());  
		sc->setProperty         (MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_NETWORK_DIRECTORIES_LIST, "");
		sc->setProperty         (MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_REMOVABLE_DIRECTORIES_LIST, "");

        sc->setProperty         (PROPERTY_LOCAL_QUOTA_STORAGE,          SAPI_LOCAL_QUOTA_STORAGE);
        sc->setLongLongProperty      (PROPERTY_SYNC_ITEM_MAX_SIZE,          (int64_t) SAPI_MAX_FILE_SIZE); // no more used in settings, just in Win<SourceName>SourcePlugin
    }
    else if (!strcmp(name, MUSIC_SOURCE_NAME)) {
        StringBuffer fullFolders = itemsUploadDefaultPath;
        StringBuffer folder = getDefaultSourceFolder(MUSIC_SOURCE_NAME);
        defaultFolder = folder.c_str();         
        if (defaultFolder.empty() == false) {
            defaultFolder = defaultFolder.insert(0, "|!");
            fullFolders.append(",");
            fullFolders.append(defaultFolder.c_str());            
        }

        sc->setSync             ("one-way-from-client");
        sc->setEncoding         ("bin");
        sc->setURI              ("audio");
        sc->setSupportedTypes   ("application/*");
        sc->setType             ("audio/*");
        sc->setSyncMode         (MUSIC_SOURCE_SYNC_MODE);
        sc->setProperty         (PROPERTY_DOWNLOAD_LAST_TIME_STAMP,     "0");
        sc->setIntProperty      (PROPERTY_SYNC_ITEM_NUMBER_FROM_CLIENT, -1);
        sc->setIntProperty      (PROPERTY_SYNC_ITEM_NUMBER_FROM_SERVER, -1);
        sc->setProperty         (PROPERTY_EXTENSION,                    MUSIC_EXTENSION);
        sc->setProperty         (MHFileSyncSource::PROPERTY_EXCLUDED_FILE_NAMES,      MUSIC_NAMES_EXCLUDED);
        sc->setProperty         (MHFileSyncSource::PROPERTY_EXCLUDED_DIRECTORY_NAMES, MUSIC_DIR_NAMES_EXCLUDED);
        sc->setProperty         (MHFileSyncSource::PROPERTY_ITEMS_DOWNLOAD_DIRECTORY, itemsDownloadDefaultPath.c_str());
        sc->setProperty         (MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_DIRECTORIES_LIST, fullFolders.c_str());  
		sc->setProperty         (MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_NETWORK_DIRECTORIES_LIST, "");
		sc->setProperty         (MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_REMOVABLE_DIRECTORIES_LIST, "");

        sc->setProperty         (PROPERTY_LOCAL_QUOTA_STORAGE,          SAPI_LOCAL_QUOTA_STORAGE);
        sc->setLongLongProperty      (PROPERTY_SYNC_ITEM_MAX_SIZE,           (int64_t)SAPI_MAX_MUSIC_SIZE); // no more used in settings, just in Win<SourceName>SourcePlugin
    }
    return sc;
}

SapiConfig* ClientConfig::getSapiConfig() {
    
    SapiConfig* c = new SapiConfig();
    
    c->setRequestTimeout        (SAPI_HTTP_REQUEST_TIMEOUT);      // 30 sec
    c->setResponseTimeout       (SAPI_HTTP_RESPONSE_TIMEOUT);     // 30 sec
    c->setUploadChunkSize       (SAPI_HTTP_UPLOAD_CHUNK_SIZE);    // 30 KByte
    c->setDownloadChunkSize     (SAPI_HTTP_DOWNLOAD_CHUNK_SIZE);  // 30 KByte
    c->setMaxRetriesOnError     (SAPI_MAX_RETRY_ON_ERROR);        // retry 2 times if network error
    c->setSleepTimeOnRetry      (SAPI_SLEEP_TIME_ON_RETRY);       // wait 500 millisec before retry
    c->setMinDataSizeOnRetry    (SAPI_MIN_DATA_SIZE_ON_RETRY);    // 10 KBytes
    
    return c;
}


int ClientConfig::getBuildNumberFromVersion(const char* swv) {

    int major=0, minor=0, build=0;
    if (!swv) {
        return 0;
    }
    int res = sscanf(swv, "%d.%d.%d", &major, &minor, &build);
    
    if (build > 1000) {
        // Fix for build numbers like "20091022" = date of today :)
        build = 0;
    }
    return (major*10000 + minor*100 + build);
}

StringBuffer ClientConfig::readCurrentFunambolSwv() {

    StringBuffer ret;
    const char* value = NULL;

    StringBuffer context;
    context.sprintf("Software/%s%s%s", APPLICATION_ROOT_CONTEXT, CONTEXT_SPDS_SYNCML, CONTEXT_DEV_DETAIL);
    value = readPropertyValue(context, PROPERTY_FUNAMBOL_SWV, HKEY_CURRENT_USER);
    if (!value || strlen(value)==0) {
        // funambol_swv not found
        // @TODO: understand what needed to do...

    }
   
    if (value && strlen(value)>0) {
        ret = value;
    }
    delete [] value;
    return ret;
}


StringBuffer ClientConfig::readNewFunambolSwv() {

    StringBuffer context("Software\\");
    context.append(APPLICATION_ROOT_CONTEXT_INST);
    StringBuffer ret = readPropertyValue(context.c_str(), PROPERTY_FUNAMBOL_SWV, HKEY_CURRENT_USER);
    return ret;
}

// current swv of the just installed client. These parameters should be changed also by the UI
char* ClientConfig::readNewSwv() {
    StringBuffer context("Software\\");
    context.append(APPLICATION_ROOT_CONTEXT_INST);
    return readPropertyValue(context.c_str(), PROPERTY_SOFTWARE_VERSION, HKEY_CURRENT_USER);
}


void ClientConfig::initializeVersionsAndUserAgent() {

    // Backup old Swv and save the new one (read in _inst registry)
    int oldSwv = getBuildNumberFromVersion(getClientConfig().getSwv());
    const char* newSwv = readNewSwv();
    getClientConfig().setSwv(newSwv);

    // Backup old Funambol product Swv and save the new one (read in _inst registry)
    int oldFunambolSwv = getBuildNumberFromVersion(getFunambolSwv().c_str());
    StringBuffer funambolNewSwv = readNewFunambolSwv();
    setFunambolSwv(funambolNewSwv);

    // Set the new User Agent = "Funambol Windows Sync Client v. x.y.z"
    // From 2012-10-26: Set the new User Agent = "omh windows client x.y.z"

    // It is a fixed value even for branded builds.
    StringBuffer ua(FUNAMBOL_USER_AGENT);
    ua += " ";
    ua += funambolNewSwv;
    accessConfig.setUserAgent(ua.c_str());

    delete [] newSwv;
}


int ClientConfig::updateConfig() {

    int oldSwv = getBuildNumberFromVersion(getClientConfig().getSwv());

    int oldFunambolSwv = getBuildNumberFromVersion(getFunambolSwv().c_str());

    // 11.2.2
    if (oldFunambolSwv < 110202) {
        // add new source params for removable/network folders list
		SyncSourceConfig* sc = getSyncSourceConfig(PICTURES_SOURCE_NAME);
		if (sc) {
			sc->setProperty(MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_NETWORK_DIRECTORIES_LIST,   "");
			sc->setProperty(MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_REMOVABLE_DIRECTORIES_LIST, "");
		}
		sc = getSyncSourceConfig(VIDEOS_SOURCE_NAME);
		if (sc) {
			sc->setProperty(MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_NETWORK_DIRECTORIES_LIST,   "");
			sc->setProperty(MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_REMOVABLE_DIRECTORIES_LIST, "");
		}
		sc = getSyncSourceConfig(MUSIC_SOURCE_NAME);
		if (sc) {
			sc->setProperty(MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_NETWORK_DIRECTORIES_LIST,   "");
			sc->setProperty(MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_REMOVABLE_DIRECTORIES_LIST, "");
		}
		sc = getSyncSourceConfig(FILES_SOURCE_NAME);
		if (sc) {
			sc->setProperty(MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_NETWORK_DIRECTORIES_LIST,   "");
			sc->setProperty(MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_REMOVABLE_DIRECTORIES_LIST, "");
		}

        // from now on, the user agent will be upgraded to the new default one.
		
    }

    // 11.2.3: added contacts and calendar
    //         added lnk extension in the list of extensions to be skipped
    if (oldFunambolSwv < 110203) { 

        // - added contact and calendar sources
        SyncSourceConfig* sc = getWinSyncSourceConfig(CONTACTS_SOURCE_NAME);    
        this->setSyncSourceConfig(*sc);
        delete sc;

        sc = getWinSyncSourceConfig(CALENDAR_SOURCE_NAME);    
        this->setSyncSourceConfig(*sc);
        delete sc;

        // - added .lnk removal
        sc = getSyncSourceConfig(FILES_SOURCE_NAME);
		if (sc) {
			 sc->setProperty(PROPERTY_EXTENSION, FILE_EXTENSION);
		}
       
        
    }
    // 12.0.4 Maverick
    
    // 12.0.5  
    //* fixed /sync support for PIM sync
    if (oldFunambolSwv < 120005) { 
        StringBuffer url = getAccessConfig().getSyncURL();
        if (url.endsWith("sync") == false) {
            if (url.endsWith("/") == true) {
                url.append("sync");
            } else {
                url.append("/sync");
            }
            getAccessConfig().setSyncURL(url.c_str());
        }
    }

    // in 12.1 * added proxy handling
    if (oldFunambolSwv < 120101) { 
        getAccessConfig().setUseProxy(true); 
    }

    //
    // v.12.1.3
    //
    // - bug 14833: special folders (leading "|") with accented chars are corrupted in registry,
    //              try to detect them for all media sources and regenerate.
    if (oldFunambolSwv < 120103) { 
        for (unsigned int i=0; i<getSyncSourceConfigsCount(); i++)
        {
            SyncSourceConfig* sc = getSyncSourceConfig(i);
            if (sc /* && isMedia(sc) */) {
                const char* uploadFoldersList = sc->getProperty(MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_DIRECTORIES_LIST);
                if (uploadFoldersList) {

                    std::vector<std::string> oldFolders;
                    int num = splitString(uploadFoldersList, ',', oldFolders);
                    std::vector<std::string>::const_iterator it = oldFolders.begin(), end = oldFolders.end();

                    std::string newFolders;

                    for (; it != end; it++) {
                        std::string oldFolder = *it;
                        if (oldFolder.empty()) continue;

                        if (!newFolders.empty()) {
                            newFolders.append(",");
                        }

                        // looking for special folders
                        if (oldFolder[0] == '|') {
                            string folderToCheck = oldFolder.substr(1);
                            if (folderToCheck[0] == '!') {
                                folderToCheck = folderToCheck.substr(1);
                            }
                            struct stat st;
                            memset(&st, 0, sizeof(struct stat));
                            if (statFile(folderToCheck.c_str(), &st) != 0) {
                                // corrupt special folder (bug 14833): regenerate it and set as disabled
                                std::string specialFolder = getDefaultSourceFolder(sc->getName());
                                specialFolder = specialFolder.insert(0, "|!");
                                newFolders += specialFolder;
                            }
                            else {
                                newFolders += oldFolder;
                            }
                        }
                        else {
                            newFolders += oldFolder;
                        }
                    }

                    sc->setProperty(MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_DIRECTORIES_LIST, newFolders.c_str());
                }
            }
        }
    }
    
    // 
    // set the max size of the file for each sync source. The value is taken from the from the WinClientConstants.h
    // and it is always overwritten with this value. Changing this value needs only for UI that has to show the max size.
    // The MHSyncSource get the max size from the Plugin initialization without using the registry value
    SyncSourceConfig* sc = getSyncSourceConfig(PICTURES_SOURCE_NAME);
	if (sc) {
		sc->setLongLongProperty      (PROPERTY_SYNC_ITEM_MAX_SIZE,           (int64_t)SAPI_MAX_PICTURE_SIZE);
	}
	sc = getSyncSourceConfig(VIDEOS_SOURCE_NAME);
	if (sc) {
		sc->setLongLongProperty      (PROPERTY_SYNC_ITEM_MAX_SIZE,           (int64_t)SAPI_MAX_VIDEO_SIZE);		
	}
	sc = getSyncSourceConfig(MUSIC_SOURCE_NAME);
	if (sc) {
        sc->setLongLongProperty      (PROPERTY_SYNC_ITEM_MAX_SIZE,           (int64_t)SAPI_MAX_MUSIC_SIZE);
	}
	sc = getSyncSourceConfig(FILES_SOURCE_NAME);
	if (sc) {
        sc->setLongLongProperty      (PROPERTY_SYNC_ITEM_MAX_SIZE,           (int64_t)SAPI_MAX_FILE_SIZE);
	}


    // reset the new values
    initializeVersionsAndUserAgent();

    // modify also the Uninstall registry
    StringBuffer appname(APP_NAME);
    StringBuffer context("\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\");
    context.append(appname);
    StringBuffer version(""); version.append(getClientConfig().getSwv());
    savePropertyValue(context, "DisplayVersion", version.c_str());
    appname.append(" "); appname.append(version);
    savePropertyValue(context, "DisplayName", appname.c_str());
    
    save();
    return oldFunambolSwv;
}


void ClientConfig::saveFunambolSwv() {

    StringBuffer context;
    context.sprintf("%s%s%s", APPLICATION_ROOT_CONTEXT, CONTEXT_SPDS_SYNCML, CONTEXT_DEV_DETAIL);
    savePropertyValue(context, PROPERTY_FUNAMBOL_SWV, funambolSwv);
}


char* ClientConfig::readSystemErrorMsg(DWORD errorCode) {

    if (!errorCode) {
        errorCode = GetLastError();
    }

    char* errorMessage = new char[512];
    memset(errorMessage, 0, 512);

    FormatMessageA(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                errorCode,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                errorMessage,
                512,
                NULL);
    
    return errorMessage;
}


StringBuffer ClientConfig::createUniqueDevID() {
    
    int ret = 0, len = 0;
    DWORD code = 0;
    char* msg  = NULL;
    DWORD bufSize = 128;
    char computerName[128], userName[128];
    wstring wprofileName;

    //
    // NetBIOS name of the local computer.
    //
    if (!GetComputerNameA(computerName, &bufSize)) {
        code = GetLastError();
        msg = readSystemErrorMsg(code);
        setErrorF(getLastErrorCode(), "GetComputerName error: code %d (%s)", code, msg);
        delete [] msg;
        return "";
    }
    len += bufSize;

    //
    // UserName of the owner of current thread.
    //
    bufSize = 128;
    if (!GetUserNameA(userName, &bufSize)) {
        code = GetLastError();
        msg = readSystemErrorMsg(code);
        setErrorF(getLastErrorCode(), "GetUserName error: code %d (%s)", code, msg);
        delete [] msg;
        return "";
    }
    len += bufSize;
    
    // get the unique GUID of the machine from HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Cryptography  
    // http://stackoverflow.com/questions/99880/generating-a-unique-machine-id
    char* value = readPropertyValue("SOFTWARE/Microsoft/Cryptography", "MachineGuid", HKEY_LOCAL_MACHINE);
    if (value && strcmp(value, "") == 0) {
        // check in the 64big node
        value = readPropertyValue("SOFTWARE/Microsoft/Cryptography", "MachineGuid", HKEY_LOCAL_MACHINE, true);
    }        
    // if value is not set, generate a new one based on timestamp
    if (value && strcmp(value, "") == 0) {
        value = new char[64];
        sprintf(value, "%i", time(NULL));
    }
    len += strlen(value);
    
    //
    // compose devID property and encrypt using MD5 algo as to have a shorter id 
    //    
    char* enc = MD5CredentialData(computerName, userName, value);
    
    StringBuffer devID(DEVICE_ID_PREFIX);
    devID.append("-");
    devID.append(enc);            

    if (enc)          delete [] enc;
    if (value)        delete [] value;

    return devID;
}

void ClientConfig::toWindows(char* str) {
    int i=0;
    while (str[i]) {
        if (str[i] == '/') {
            str[i] = '\\';
        }
        i++;
    }
}

char* ClientConfig::readPropertyValuePrivate(const char* contextA, const char* propertyNameA, HKEY rootKey, bool bit64, bool justOpen) {
    
    DWORD res = 0;  	
    long  err = 0;
    ULONG dim = 0;
    HKEY  key = NULL;
    char* ret = NULL;
    
    REGSAM mask = KEY_READ;
    if (bit64) {
        mask = mask | KEY_WOW64_64KEY;
    }
    // Need to convert all '/' into '\'.
    char* fullContextA = stringdup(contextA);
    toWindows(fullContextA);

    WCHAR* fullContext = toWideChar(fullContextA);
    WCHAR* propertyName = toWideChar(propertyNameA);
    
    if (justOpen == false) {
    err = RegCreateKeyEx(
            rootKey,
            fullContext,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            mask,                           // Read only: could be from a limited rights user.
            NULL,
            &key,
            &res
            );
    } else {
        err = RegOpenKeyEx(
            rootKey,
            fullContext,
            0,
            mask,
            &key
      );
    }
    if (key == 0) {
        setErrorF(ERR_INVALID_CONTEXT, "Invalid windows registry path: %s.", fullContextA);
        goto finally;
    }

    // Get value length
    err = RegQueryValueEx(
            key,
            propertyName,
            NULL,
            NULL,  // we currently support only strings
            NULL,
            &dim
            );

    if (err == ERROR_SUCCESS) {
		if (dim > 0) {
            TCHAR* buf = new TCHAR[dim + 1];

			err = RegQueryValueEx(
					key,
					propertyName,
					NULL,
					NULL,  // we currently support only strings
					(UCHAR*)buf,
					&dim 
                    );
            if (err == ERROR_SUCCESS) {
                ret = toMultibyte(buf);                
            }
            delete [] buf;
		}
    }

finally:

    if (!ret) {
        // Always return an empty string if key not found!
        ret = stringdup(EMPTY_STRING);
    }
    if (fullContext) {
        delete [] fullContext;
    }
    if (fullContextA) {
        delete [] fullContextA;
    }
    if (propertyName) {
        delete [] propertyName;
    }
    if (key != 0) {
        RegCloseKey(key);
    }
    return ret;
}

void ClientConfig::savePropertyValue(const StringBuffer& context, const StringBuffer& name, const StringBuffer& value) {

    ConfigurationNode* node = NULL;
    DMTree* dmt = DMTreeFactory::getDMTree(context.c_str());
    if (!dmt) goto finally;

    node = dmt->readManagementNode(context.c_str());
    if (!node) goto finally;

    node->setPropertyValue(name.c_str(), value.c_str());

finally:
    if (dmt)   delete dmt;
    if (node)  delete node;
    return;
}

void ClientConfig::saveDefaultInstConfigRoot() {
    
    StringBuffer context(APPLICATION_ROOT_CONTEXT_INST);
    savePropertyValue(context, PROPERTY_FUNAMBOL_SWV, "11.2.0");
    savePropertyValue(context, "swv", "11.2.0");
    savePropertyValue(context, "Customer", "");
    savePropertyValue(context, "Description", APP_NAME " v. 11.2.0");
    savePropertyValue(context, "InstallDir", "");
}

////////////////////////////////////////////
// Inherit from the old OutlookConfig.cpp
////////////////////////////////////////////

#define PROPERTY_ATTACH                "attach"
#define PROPERTY_LOG_NUM               "logNum"
#define PROPERTY_LOG_SIZE              "logSize"
#define DIM_MANAGEMENT_PATH            2048

void ClientConfig::readPIMSourcesCTCap() {

    SyncSourceConfig* ssc = getSyncSourceConfig(CONTACT_);
    if (ssc) {
        ArrayList* p = getVCardProperties();
        ssc->addCtCap(p, "text/x-vcard", "2.1");
        delete p;
    }

    ssc = getSyncSourceConfig(APPOINTMENT_);
    if (ssc) {
        ArrayList* p = getVCalendarProperties();
        ssc->addCtCap(p, "text/x-vcalendar", "1.0");
        delete p;
    }

}


/**
 * Populate 'currentTimezone' structure, reading values from TIME_ZONE_INFORMATION
 * and also directly from Windows Registry.
 * @note  some mandatory informations cannot be retrieved from Win32 API calls,
 *        so we have to get them from HKLM keys.
 * @return 0 if no errors
 */
int ClientConfig::readCurrentTimezone() {

    //
    // Get all known info from TIME_ZONE_INFORMATION.
    //
    TIME_ZONE_INFORMATION tzInfo;
    DWORD tzID = GetTimeZoneInformation(&tzInfo);
    if (tzID == TIME_ZONE_ID_DAYLIGHT) {
        currentTimezone.isDaylight = true;
    }
    else {
        currentTimezone.isDaylight = false;
    }
    currentTimezone.bias         = tzInfo.Bias;
    currentTimezone.daylightBias = tzInfo.DaylightBias;
    currentTimezone.daylightDate = tzInfo.DaylightDate;
    currentTimezone.daylightName = tzInfo.DaylightName;
    currentTimezone.standardBias = tzInfo.StandardBias;
    currentTimezone.standardDate = tzInfo.StandardDate;
    currentTimezone.standardName = tzInfo.StandardName;

    //
    // Now go directly to Win Registry keys and get the 
    // other mandatory informations.
    //
    bool found = false;
    HKEY hkTimeZones;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TIMEZONE_CONTEXT, 0, KEY_READ, &hkTimeZones) == ERROR_SUCCESS) {
        HKEY  hkTimeZone;
        DWORD dwIndex = 0;
        WCHAR keyName[DIM_MANAGEMENT_PATH];
        DWORD keyNameLenght = DIM_MANAGEMENT_PATH;

        // Scan all timezones, searching for the current one.
        while (RegEnumKey(hkTimeZones, dwIndex++, keyName, keyNameLenght) != ERROR_NO_MORE_ITEMS) {
            if (RegOpenKeyEx(hkTimeZones, keyName, 0, KEY_READ, &hkTimeZone) == ERROR_SUCCESS) {

                WCHAR stdName[DIM_MANAGEMENT_PATH];
                DWORD dwDataSize = DIM_MANAGEMENT_PATH * sizeof(WCHAR);
                RegQueryValueEx(hkTimeZone, L"Std", NULL, NULL, (BYTE*)&stdName, &dwDataSize);
                if (!wcscmp(stdName, currentTimezone.standardName.c_str())) {
                    found = true;

                    // Get Index
                    DWORD dwTimeZoneIndex;
                    dwDataSize = sizeof(DWORD);
                    RegQueryValueEx(hkTimeZone, L"Index", NULL, NULL, (BYTE*)&dwTimeZoneIndex, &dwDataSize);

                    // Get Display name
                    WCHAR displayName[DIM_MANAGEMENT_PATH];
                    dwDataSize = DIM_MANAGEMENT_PATH * sizeof(WCHAR);
                    RegQueryValueEx(hkTimeZone, L"Display", NULL, NULL, (BYTE*)&displayName, &dwDataSize);

                    // Set properties to currentTimezone struct.
                    currentTimezone.index       = dwTimeZoneIndex;
                    currentTimezone.displayName = displayName;
                    currentTimezone.keyName     = keyName;

                    RegCloseKey(hkTimeZone);
                    break;
                }
            }
            keyNameLenght = DIM_MANAGEMENT_PATH;
            RegCloseKey(hkTimeZone);
        }
        RegCloseKey(hkTimeZones);
    }
    else {
        return 1;
    }

    if (!found) {
        LOG.info("Error reading the timezone info from Win Registry");
        return 1;
    }

    LOG.debug("Current Timezone = %ls", currentTimezone.displayName.c_str());
    return 0;
}

/**
 * Returns a pointer to the currentTimezone internal structure.
 */
const TimeZoneInformation* ClientConfig::getCurrentTimezone() const {
    return &currentTimezone;
}

void ClientConfig::restorePIMSettingsBeforeV12() {
 
    StringBuffer context("Software/");
    context.append(APPLICATION_ROOT_CONTEXT_INST);

    // restore registries for contacts
    SyncSourceConfig* sc = getSyncSourceConfig(CONTACTS_SOURCE_NAME);
    StringBuffer ret = readPropertyValue(context.c_str(), "last_contact", HKEY_CURRENT_USER);
    if (!ret.empty()) {
        sc->setLast((unsigned long)atol(ret.c_str()));
    }

    ret = readPropertyValue(context.c_str(), "folderPath_contact", HKEY_CURRENT_USER);
    if (!ret.empty()) {
        sc->setProperty(PROPERTY_FOLDER_PATH, ret.c_str());
    }

    ret = readPropertyValue(context.c_str(), "useSubfolders_contact", HKEY_CURRENT_USER);
    if (!ret.empty()) {
        sc->setBoolProperty     (PROPERTY_USE_SUBFOLDERS, ret == "1" ? true : false);
    }

    ret = readPropertyValue(context.c_str(), "sourceSyncMode_contact", HKEY_CURRENT_USER);
    if (!ret.empty()) {        
        sc->setSyncMode(ret == "auto" ? SYNC_MODE_AUTO : SYNC_MODE_DISABLED);        
    }    

    deleteProperty(context, "last_contact");
    deleteProperty(context, "folderPath_contact");
    deleteProperty(context, "useSubfolders_contact");
    deleteProperty(context, "sourceSyncMode_contact");


    // restore registries for appointment
    sc = getSyncSourceConfig(CALENDAR_SOURCE_NAME);
    ret = readPropertyValue(context.c_str(), "last_appointment", HKEY_CURRENT_USER);
    if (!ret.empty()) {
        sc->setLast((unsigned long)atol(ret.c_str()));
    }

    ret = readPropertyValue(context.c_str(), "folderPath_appointment", HKEY_CURRENT_USER);
    if (!ret.empty()) {
        sc->setProperty(PROPERTY_FOLDER_PATH, ret.c_str());
    }

    ret = readPropertyValue(context.c_str(), "useSubfolders_appointment", HKEY_CURRENT_USER);
    if (!ret.empty()) {
        sc->setBoolProperty     (PROPERTY_USE_SUBFOLDERS, ret == "1" ? true : false);
    }

    ret = readPropertyValue(context.c_str(), "sourceSyncMode_appointment", HKEY_CURRENT_USER);
    if (!ret.empty()) {
        
        sc->setSyncMode(ret == "auto" ? SYNC_MODE_AUTO : SYNC_MODE_DISABLED);        
    }
    
    ret = readPropertyValue(context.c_str(), "filterDateLower_appointment", HKEY_CURRENT_USER);
    if (!ret.empty()) {
        int filtered = atoi(ret.c_str());
        sc->setIntProperty      (PROPERTY_FILTER_DATE_LOWER, (DateFilter::RelativeLowerDate)filtered);                 
    }
    
    deleteProperty(context, "last_appointment");
    deleteProperty(context, "folderPath_appointment");
    deleteProperty(context, "useSubfolders_appointment");
    deleteProperty(context, "sourceSyncMode_appointment");
    deleteProperty(context, "filterDateLower_appointment");

    // restore other settings like username/password/devid/syncurl
    ret = readPropertyValue(context.c_str(), "prev_username", HKEY_CURRENT_USER);
    if (!ret.empty()) {   
        std::string res = decodeAfterReading(ret.c_str());
        getAccessConfig().setUsername(res.c_str());
    }

    ret = readPropertyValue(context.c_str(), "prev_password", HKEY_CURRENT_USER);
    if (!ret.empty()) {
        std::string res = decodeAfterReading(ret.c_str());
        getAccessConfig().setPassword(res.c_str());
    }

    ret = readPropertyValue(context.c_str(), "prev_syncurl", HKEY_CURRENT_USER);
    if (!ret.empty()) {    
        getAccessConfig().setSyncURL(ret.c_str());
    }

    ret = readPropertyValue(context.c_str(), "prev_devid", HKEY_CURRENT_USER);
    if (!ret.empty()) {    
        getDeviceConfig().setDevID(ret.c_str());
    }
 
    deleteProperty(context, "prev_username");
    deleteProperty(context, "prev_password");
    deleteProperty(context, "prev_syncurl");
    deleteProperty(context, "prev_devid");

    save();

    if (strcmp(getAccessConfig().getUsername(), "") != 0 &&  
        strcmp(getAccessConfig().getPassword(), "") != 0) {
            // user is already logged in. Used by UI context
            StringBuffer intcontext("Software/");
            intcontext.append(APPLICATION_ROOT_CONTEXT);
            intcontext.append("/spds/syncml/Auth");            
            savePropertyValue(intcontext, "isLoggedIn", "1");
    }

}

string ClientConfig::getInstallDir() {

    StringBuffer context("Software\\");
    context.append(APPLICATION_ROOT_CONTEXT_INST);
    string s = readPropertyValue(context.c_str(), "installDir", HKEY_CURRENT_USER);
    return s;
}

#include "Shlwapi.h"
int ClientConfig::deleteProperty(const char* path, const char* propertyName) { 

    int err = 0;
    if (propertyName) {        
        StringBuffer winPath(path);
        winPath.replaceAll("/", "\\");  // must be in the windows form
        
        const WCHAR* wname = toWideChar(propertyName);
        const WCHAR* wpath = toWideChar(winPath.c_str());

        err = SHDeleteValue(HKEY_CURRENT_USER, wpath, wname);
        if (err != ERROR_SUCCESS) {
            setErrorF(ERR_DM_TREE_NOT_AVAILABLE, "Could not delete property: %s", propertyName);
        }
        delete [] wname;
        delete [] wpath;
    }
    return err;
}

/* NOT USED AT THE MOMENT. LOOK AT THE C# PROJ
BOOL ClientConfig::AddToDesktop(StringBuffer folderPath)
{
    
    WCHAR desktopPath[MAX_PATH];
    SHGetSpecialFolderPath(0,  desktopPath,  CSIDL_DESKTOPDIRECTORY, FALSE ); 
    StringBuffer linkName; linkName.convert(desktopPath);
    linkName.append("\\");
    linkName.append(APP_NAME);
    //linkName.append(".lnk");

    // and create the shortcut
    CreateShortcut(folderPath, linkName, APP_NAME);

return TRUE;
}

#include <shlobj.h>
#include <atlbase.h>
HRESULT ClientConfig::CreateShortcut(StringBuffer shortcutFile, StringBuffer link, StringBuffer desc)
{

    HRESULT hres;

    StringBuffer sh = shortcutFile;
    if (sh.endsWith("/")) {
        sh = sh.substr(0, sh.length() - 1);        
    }
    sh.replaceAll("/", "\\"); // otherwise it doesn't work :((

    WCHAR* wshortcutFile = toWideChar(sh.c_str());
    WCHAR* wdesc = toWideChar(desc.c_str());
    WCHAR* wicon = NULL;
    
    StringBuffer currentInstallationDir("");
    currentInstallationDir.sprintf("Software/%s", APPLICATION_ROOT_CONTEXT_INST);
    char* value = readPropertyValue(currentInstallationDir, "installDir", HKEY_CURRENT_USER);
    if (value && strlen(value) >= 0) {
        StringBuffer iconPath(value);
        iconPath.append("\\images\\MediaHubFolder.ico");
        wicon = toWideChar(iconPath.c_str());  
    }

    IShellLink* psl;
    CoInitialize(NULL);
    // hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl); 
    hres = CoCreateInstance(CLSID_FolderShortcut, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl); 
    if (SUCCEEDED(hres)) 
    { 
        IPersistFile* ppf; 
 
        // Set the path to the shortcut target and add the description. 
        psl->SetPath(wshortcutFile); 
        psl->SetDescription(wdesc); 
        if (wicon) {
            psl->SetIconLocation(wicon, 0);
        }
        // Query IShellLink for the IPersistFile interface, used for saving the 
        // shortcut in persistent storage. 
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf); 
 
        if (SUCCEEDED(hres)) 
        { 
            WCHAR wsz[MAX_PATH]; 
 
            // Ensure that the string is Unicode. 
            MultiByteToWideChar(CP_ACP, 0, link.c_str(), -1, wsz, MAX_PATH);             
            
            // Save the link by calling IPersistFile::Save. 
            hres = ppf->Save(wsz, TRUE); 

            ppf->Release(); 
        } 
        psl->Release(); 
    } 
    delete [] wshortcutFile;
    delete [] wdesc;
    delete [] value;
    delete [] wicon;
    ::CoUninitialize();  


    return hres; 
 
} 
*/
