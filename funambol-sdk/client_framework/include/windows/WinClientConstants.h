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

#ifndef INCL_WIN_CLIENT_CONSTANTS
#define INCL_WIN_CLIENT_CONSTANTS

/** @cond DEV */

#include "vocl/appdefs.h"
#include <windows.h>
#include <string>

#define PUSH_APP_NAME        "pushfnbl.exe"   // this is used

// definition of push notification windows
#define PUSH_CLASS_NAME                    TEXT("ClientPushWindowPicture")
#define WM_FOLDER_CHANGE                   0x8000
#define WM_CLASS_PIM_LISTENER              0x8001
#define WM_CLASS_CTP_LISTENER              0x8002
#define WM_SUSPEND_LISTENER                0x8003
#define WM_RESUME_LISTENER                 0x8004
#define WM_WHOLE_SYNC_COMPLETED            0x8005
#define WM_REMOVE_SHORTCUT_LINK            0x8006
#define WM_EXIT_AND_REMOVE_SHORTCUT_LINK   0x8007

#define PUSH_MUTEX_FOR_POLLING             "pollingpushmutex"
#define PUSH_USE_COP                       true       // by default use the cop. If false use the light polling
#define PUSH_POLLING_TIMER_ID              5010        // identify the timer that start the polling
#define PUSH_POLLING_TIME                  900         // (15mins) in secs


#define SYNC_ENGINE_MAIN_CLASS             "syncenginemainclass"


//
// SOURCE NAMES
//
#define CONTACT_                           "contact"
#define APPOINTMENT_                       "appointment"
#define JOURNAL_                           "journal"
#define PICTURE_                           "picture"
#define VIDEO_                             "video"
#define FILES_                             "files"
#define POST_                              "post"
#define MUSIC_                             "audio"
#define DISTRIBUTION_LIST_                 "distribution list"

#define CONTACTS_SOURCE_NAME                "contact"
#define CALENDAR_SOURCE_NAME                "appointment"  /* "calendar" */
#define PICTURES_SOURCE_NAME                "picture"
#define VIDEOS_SOURCE_NAME                  "video"
#define FILES_SOURCE_NAME                   "file"
#define MUSIC_SOURCE_NAME                   "music"

#define WCONTACTS_SOURCE_NAME                L"contact"
#define WCALENDAR_SOURCE_NAME                L"appointment"  /* "calendar" */
#define WPICTURES_SOURCE_NAME                L"picture"
#define WVIDEOS_SOURCE_NAME                  L"video"
#define WFILES_SOURCE_NAME                   L"file"
#define WMUSIC_SOURCE_NAME                   L"music"


//
// PIM constants
// 
#define CONTACTS_DEVINFO_SYNC_MODES     SYNC_MODE_TWO_WAY  "," \
                                        SYNC_MODE_REFRESH_FROM_CLIENT "," \
                                        SYNC_MODE_REFRESH_FROM_SERVER "," \
                                        "slow"

#define CALENDAR_DEVINFO_SYNC_MODES     SYNC_MODE_TWO_WAY  "," \
                                        SYNC_MODE_REFRESH_FROM_CLIENT "," \
                                        SYNC_MODE_REFRESH_FROM_SERVER "," \
                                        "slow"

/// Filtering properties
#define PROPERTY_FILTER_DATE_LOWER              "filterDateLower"
#define PROPERTY_FILTER_DATE_UPPER              "filterDateUpper"
#define PROPERTY_FILTER_DATE_DIRECTION          "filterDateDirection"

#define PROPERTY_USE_SUBFOLDERS                 "useSubfolders"
#define PROPERTY_FOLDER_PATH                    "folderPath" 
#define PROPERTY_MEDIAHUB_PATH                  "mediaHubPath" 

#define OUTLOOK 1

// Program parameters:
#define SYNC_MUTEX_NAME                     "fol-SyncInProgress"
#define EMPTY_WSTRING                      L""
// Default remote names:
#define MAX_PATH_LENGTH                     512

/// If MS Outlook app is installed, this registry key exists and it's not empty (under HKLM)
#define OUTLOOK_EXE_REGKEY                 "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\OUTLOOK.exe"

/// If Redemption.dll is registered in the system, this registry key exists and it's not empty (under HKCR)
#define REDEMPTION_CLSID_REGKEY            "CLSID\\{03C4C5F4-1893-444C-B8D8-002F0034DA92}\\InprocServer32"

/// Name of file to store 'forced' modified appointments
#define APPOINTMENT_FORCED_MODIFIED        L"appointment_modified"

/// Default folder of items in Outlook (replaced by correct path)
#define DEFAULT_FOLDER                     L"DEFAULT_FOLDER"

/// Sapi cache dir name, under appdata dir
#define SAPI_STORAGE_FOLDER                 "sapi_media_storage"        // obsolete

// LOG properties (for rotation: to be checked)
#define MIN_LOG_FILE_SIZE               1
#define MAX_LOG_FILE_SIZE               20

#define MIN_LOG_FILE_NUM                2
#define MAX_LOG_FILE_NUM                20

// Enable the certificate check if valid on SSL server 
#define ENABLE_SSL_CERTIFICATE_CHECK   true






#define PUSH_LOG_NAME                       "pushlog.txt"
#define MAX_LOG_SIZE                        5000000                     /**< 5 MB          */
#define DEFAULT_LOG_LEVEL                   LOG_LEVEL_DEBUG

#define CTP_READY_INTERVAL_VAL              600  /** Interval (in seconds) for the CTP [ready] msg */
#define CTP_CONN_TIMEOUT_VAL                0
#define CTP_CMD_TIMEOUT_VAL                 60
#define CTP_NOTIFY_TIMEOUT_VAL              120
#define CTP_RETRY_INTERVAL_VAL              60
#define CTP_MAX_RETRY_VAL                   1200
#define CTP_PORT_VAL                        4745


//
// Customizable parameters
//
#define PRODUCT_PUBLISHER                   "Funambol"
#define APP_NAME                            "OneMediaHub"
#define APPLICATION_ROOT_CONTEXT            PRODUCT_PUBLISHER "/" APP_NAME      // "Funambol/OneMediaHub"
#define APPLICATION_ROOT_CONTEXT_INST       APPLICATION_ROOT_CONTEXT "_inst"    // "Funambol/OneMediaHub_inst" // created by NSIS to store info of the installation version

#define SOURCE_ORDER                        "picture,video,music,file,contact,appointment"
#define DEFAULT_SYNC_URL                    "https://onemediahub.com/sync"

//
// If the visible source should be enabled or disabled by default
//
#define CONTACTS_SOURCE_SYNC_MODE           SYNC_MODE_AUTO  
#define CALENDAR_SOURCE_SYNC_MODE           SYNC_MODE_AUTO  
#define PICTURES_SOURCE_SYNC_MODE           SYNC_MODE_AUTO
#define VIDEOS_SOURCE_SYNC_MODE             SYNC_MODE_AUTO
#define FILES_SOURCE_SYNC_MODE              SYNC_MODE_AUTO
#define MUSIC_SOURCE_SYNC_MODE              SYNC_MODE_AUTO

//
// Name of the pipe to communicate with the UI. REMEMBER that if the APP_NAME contains spaces they must be removed. The branding already does it.
//
#define PUSH_PIPE_NAME                       TEXT("\\\\.\\pipe\\") TEXT(APP_NAME) TEXT("\\push")  
#define SYNC_PIPE_NAME                       TEXT("\\\\.\\pipe\\") TEXT(APP_NAME) TEXT("\\sync") 

#define OUTLOOK_PIPE_NAME                    TEXT("\\\\.\\pipe\\") TEXT(APP_NAME) TEXT("\\outlook") 


enum { CONTACTS_SOURCE_VISIBLE                    = 1 };  // not used by windows client
enum { CALENDAR_SOURCE_VISIBLE                    = 1 };  // not used by windows client
enum { PICTURES_SOURCE_VISIBLE                    = 1 };  // not used by windows client
enum { VIDEOS_SOURCE_VISIBLE                      = 1 };  // not used by windows client
enum { FILES_SOURCE_VISIBLE                       = 1 };  // not used by windows client
enum { MUSIC_SOURCE_VISIBLE                       = 1 };  // not used by windows client


// SAPI media props
#define MIN_SAPI_VERSION                    100500                   /**< the min SAPI version supported (10.5.0) */    
#define SAPI_LOCAL_QUOTA_STORAGE            "98%"                    /**< the max local storage for all media sources */
#define SAPI_HTTP_REQUEST_TIMEOUT           30                       /**< 30 sec    */
#define SAPI_HTTP_RESPONSE_TIMEOUT          30                       /**< 30 sec    */
#define SAPI_HTTP_UPLOAD_CHUNK_SIZE         30000                    /**< 30 KByte  */
#define SAPI_HTTP_DOWNLOAD_CHUNK_SIZE       30000                    /**< 30 KByte  */
#define SAPI_MAX_RETRY_ON_ERROR             2                        /**< 2 retries */
#define SAPI_SLEEP_TIME_ON_RETRY            500                      /**< 0.5 sec   */
#define SAPI_MIN_DATA_SIZE_ON_RETRY         10000                    /**< 10 KBytes */
#define SAPI_MAX_PICTURE_SIZE               0                        /**< max size of pictures [bytes]. 0 means unlimited. */
#define SAPI_MAX_VIDEO_SIZE                 250 * 1024 * 1024LL        /**< max size of videos   [bytes]. 100 MB. */
#define SAPI_MAX_FILE_SIZE                  250 * 1024 * 1024LL        /**< max size of files    [bytes].  25 MB. */
#define SAPI_MAX_MUSIC_SIZE                 0                        /**< max size of music    [bytes]. 0 means unlimited. */
#define PICT_EXTENSION                      ".jpg,.jpeg,.jpe,.gif,.png,.jfif,.jif"
#define VIDEO_EXTENSION                     ".wmv,.mp4,.mov,.3g2,.3gp,.3gpp,.mpeg,.mpg,.mpe,.asf,.movie,.avi,.mpa,.mp2,.m4u,.m4v,.flv"
#define MUSIC_EXTENSION                     ".mp3,.aac,.m4a"         // to be added(?): .flac,.ogg,.oga,.wav,.f4a
#define FILE_EXTENSION                      "!," PICT_EXTENSION "," VIDEO_EXTENSION  "," MUSIC_EXTENSION ",.tmp,.lnk,.partial,.part,.crdownload"  /**< everything else */

#define PICTURE_NAMES_EXCLUDED              ".*"
#define VIDEO_NAMES_EXCLUDED                ".*"
#define FILE_NAMES_EXCLUDED                 ".*"
#define MUSIC_NAMES_EXCLUDED                ".*"

#define PICTURE_DIR_NAMES_EXCLUDED          ".*"
#define VIDEO_DIR_NAMES_EXCLUDED            ".*"
#define FILE_DIR_NAMES_EXCLUDED             ".*"
#define MUSIC_DIR_NAMES_EXCLUDED            ".*"

/// If true, check the online storage after 1st login and disable video source if storage < MIN_STORAGE_FOR_VIDEO
#define DISABLE_VIDEO_IF_LOW_STORAGE        true
#define MIN_STORAGE_FOR_VIDEO               250   // MegaBytes


// #define PICTURE_DEFAULT_PATH        "OneMediaHub"   // no more used
// #define VIDEO_DEFAULT_PATH          "OneMediaHub"   // no more used
// #define FILES_DEFAULT_PATH          "OneMediaHub"   // no more used
// #define MUSIC_DEFAULT_PATH          "OneMediaHub"   // no more used

// media hub default folder name for upload/download items: this name
// will be appended to the user home to have an absolute path
#define MEDIA_HUB_DEFAULT_FOLDER_NAME    "OneMediaHub" 

#define PICTURE_QUOTA_THRESHOLD            250 * 1024 * 1024
#define PICTURE_MANAGER_CACHE_NAME         "picture_items"
#define PICTURE_EXCLUDED_ITEMS_CACHE_NAME  "picture_excluded_items"
#define PICTURE_FULLITEM_SPOOL_MAX_NUM     10                   // > 3 for acceptable results
#define PICTURE_FULLITEM_SPOOL_MAX_SIZE    100 * 1000 * 1000    // 100 MB

#define VIDEO_QUOTA_THRESHOLD            250 * 1024 * 1024
#define VIDEO_MANAGER_CACHE_NAME         "video_items"
#define VIDEO_EXCLUDED_ITEMS_CACHE_NAME  "video_excluded_items"
#define VIDEOS_FULLITEM_SPOOL_MAX_NUM    10                   // > 3 for acceptable results
#define VIDEOS_FULLITEM_SPOOL_MAX_SIZE   100 * 1000 * 1000    // 100 MB


#define MAX_PICTURE_ITEM_SIZE           0                 // no filter for pictures
#define MAX_VIDEO_ITEM_SIZE             250 * 1024 * 1024 // 250 Mb 
#define VIDEO_QUOTA_THRESHOLD           250 * 1024 * 1024

#define MAX_FILE_ITEM_SIZE              250 * 1024 * 1024 // 250 Mb 
#define FILE_QUOTA_THRESHOLD            250 * 1024 * 1024
#define FILE_MANAGER_CACHE_NAME         "file_items"
#define FILE_EXCLUDED_ITEMS_CACHE_NAME  "file_excluded_items"
#define FILES_FULLITEM_SPOOL_MAX_NUM    10                   // > 3 for acceptable results
#define FILES_FULLITEM_SPOOL_MAX_SIZE   100 * 1000 * 1000    // 100 MB
#define FILES_FULLITEM_SPOOL           "full_item_files"
#define FILES_LOCAL_ITEM_SPOOL         "Inbox"
#define FILES_TEMPORARY_SPOOL          "mediahub_spool_file"
#define FILES_THUMBNAILS_PATH          "files_thumbnails"

#define MAX_MUSIC_ITEM_SIZE            250 * 1024 * 1024
#define MUSIC_QUOTA_THRESHOLD          250 * 1024 * 1024
#define MUSIC_MANAGER_CACHE_NAME       "music_items"
#define MUSIC_EXCLUDED_ITEMS_CACHE_NAME  "music_excluded_items"
#define MUSIC_FULLITEM_SPOOL_MAX_NUM    10                   // > 3 for acceptable results
#define MUSIC_FULLITEM_SPOOL_MAX_SIZE   100 * 1000 * 1000    // 100 MB
#define MUSIC_TEMPORARY_SPOOL          "mediahub_spool_music"
#define MUSIC_FULLITEM_SPOOL           "full_item_music"

#define  CACHE_FOLDER_NAME             "cache"

// SAPI media sync paths
#define PICTURES_TEMPORARY_SPOOL        "mediahub_spool_picture"
#define VIDEOS_TEMPORARY_SPOOL          "mediahub_spool_video"
#define PICTURES_THUMBNAILS_PATH        "pictures_thumbnails"
#define VIDEOS_THUMBNAILS_PATH          "videos_thumbnails"
#define PICTURES_FULLITEM_SPOOL         "full_item_pictures"
#define VIDEOS_FULLITEM_SPOOL           "full_item_videos"
#define GLOBAL_LABELS_CACHE_NAME        "global_labels"

#define ENCRYPT_ACCESS_CONFIG_PASSWORD  "SettimioSevero"
#define CREDINFO                        ""


// The deviceId will be like: "fol-b64(%1:%2)" - see OutlookConfig::setUniqueDevID()
// where %1 is the local machine name, %2 is the Windows current user name (encoded in base64)
#define DEVICE_ID_PREFIX                    "fol"

// The user-agent is this fixed value + the funambol sw version.
// For example: "Funambol Windows Sync Client v. 10.0.0". It doesn't change in the OneMediaHub migration
// From 2012-10-26: "omh windows client 11.5.0". 

#define FUNAMBOL_USER_AGENT                 "omh windows client"  

// properties used in the upgrade
#define PROPERTY_FUNAMBOL_SWV                   "funambol_swv"
#define PROPERTY_CUSTOMER                       "Customer"

// default session id duration 24h 
#define SESSION_ID_EXP_TIME                  24 * 60 * 60

#define WIN_ERR_NONE                        0
#define WIN_ERR_SYNC_CANCELED               2
#define WIN_ERR_THREAD_TERMINATED           4
#define WIN_ERR_SERVER_QUOTA_EXCEEDED       8
#define WIN_ERR_LOCAL_STORAGE_FULL          9
#define WIN_ERR_DROPPED_ITEMS               10
#define WIN_ERR_DROPPED_ITEMS_SERVER        11
#define WIN_ERR_NO_SOURCES                  12
#define WIN_ERR_SAPI_NOT_SUPPORTED          13
#define WIN_ERR_ITEM_PERMANENTLY_REFUSED    14
#define WIN_ERR_OUTLOOK_DISCONNECTED        15              // Outlook session disconnected (COM error) -> retry sync
#define WIN_ERR_INVALID_CREDENTIALS         401
#define WIN_ERR_REMOTE_NAME_NOT_FOUND       404
#define WIN_ERR_PROXY_AUTH_REQUIRED         407
#define WIN_ERR_GENERIC	                    500				// auto-sync is false
#define WIN_ERR_WRONG_HOST_NAME             2001
#define WIN_ERR_NETWORK_ERROR               2050
#define WIN_ERR_PAYMENT_REQUIRED            402 // SAPI respond in a "Alert" Status that the user must pay for that service 
#define WIN_ERR_CRASH                       710 
#define WIN_ERR_UNSUPPORTED_COMMAND_LINE    711 
#define ERR_SERVER_QUOTA_EXCEEDED_UPGRADE   426 // just for windows

#define WIN_ERR_LOGIN_FAILURE_750           750
#define WIN_ERR_LOGIN_FAILURE_751           751
#define WIN_ERR_LOGIN_FAILURE_752           752
#define WIN_ERR_LOGIN_FAILURE_753           753
#define WIN_ERR_LOGIN_FAILURE_754           754
#define WIN_ERR_LOGIN_FAILURE_755           755
#define WIN_ERR_LOGIN_FAILURE_756           756
#define WIN_ERR_LOGIN_FAILURE_757           757

#define WIN_ERR_FATAL_OL_EXCEPTION          3   // deprecated
#define WIN_ERR_FULL_SYNC_CANCELED          5   // deprecated
#define WIN_ERR_UNEXPECTED_EXCEPTION        6
#define WIN_ERR_UNEXPECTED_STL_EXCEPTION    7


// status used for the sync process notification to the new UI
#undef SYNC_PROCESS_START_NOTIFICATION                   // already defined in SyncProcessNotificationManagerConstants.h
#define SYNC_PROCESS_START_NOTIFICATION                  1
#define SYNC_PROCESS_END_NOTIFICATION_OK                 WIN_ERR_NONE
#define SYNC_PROCESS_CANCELED_NOTIFICATION               WIN_ERR_SYNC_CANCELED
#define SYNC_PROCESS_CANCELING_NOTIFICATION              3


// status used to map the current phase of the refresh
#define SYNC_SOURCE_START_REFRESH                        1
#define SYNC_SOURCE_END_REFRESH                          0
#define SYNC_SOURCE_START_UPLOAD_PHASE                   2
#define SYNC_SOURCE_END_UPLOAD_PHASE                     3
#define SYNC_SOURCE_START_DOWNLOAD_PHASE                  4
#define SYNC_SOURCE_END_DOWNLOAD_PHASE                    5


// to be reviewed

class DLLCustomization {
public:
    static const bool defaultUseSubfolders              = true;     /**< include subfolders check (PIM) */
    static const bool removeFilteredDataOnCleanup       = true;     /**< send filtered out items as Delete items, in case of refresh sync */
    static const bool sendMovedAsNew                    = false;    /**< send items as new instead of updated, if moved to another folder */
    static const bool dontSendFilteredItemsAsDeleted    = true;     /**< don't send items out of the calendar filter as Deleted items */
    static const bool neverSendPhotos                   = false;    /**< don't send contact's photo */
    static const bool saveFileAs                        = false;    /**< save the contact's "fileAs" field, otherwise it's auto generated by Outlook */
    static const bool syncAttendees                     = false;     /**< to sync calendar's attendees */
    static const bool sendTimezone                      = true;     /**< to avoid sending events tz */
    static const bool continueOnSlowWithOneWay          = false;    /**< will not auto-continue a slow if sync direction is set to one-way */
    static const bool warnOnLargeDelete                 = false;    /**< If 50% of items or more are deleted, displays a warning */

    /// to validate custom X-foo properties
    static bool validateExtraProperty(const std::wstring & name) {
        const wchar_t * n = name.c_str();
        return
            (!wcsncmp(n,X_PREFIX,wcslen(X_PREFIX)) && // any X- Properties
            wcsncmp(n,X_FUNAMBOL_PREFIX,wcslen(X_FUNAMBOL_PREFIX)) && // any non-funambol X- Properties
            wcsncmp(n,X_MICROSOFT_PREFIX,wcslen(X_MICROSOFT_PREFIX)) && // any non-microsoft properties
            wcscmp(n,X_WM_CLIENT_CONTAINER_ID) && wcscmp(n,X_WM_CLIENT_CONTAINER_NAME)) // not certain WM props
            ;
    }

    // Source defaults
    static const char *  sourceDefaultEncoding;         /**< the default encoding for synsources */
    static const bool    sourceNotesDefaultSif;         /**< default notes data format (true = SIF, false = vnote) */
    static const char *  sourceNotesSifUri;             /**< the default sources URI for SIF notes */
    static const char *  sourceNotesVnoteUri;           /**< the default sources URI for vNote */
    static const char *  sourceTasksVcalUri;            /**< the default sources URI for vEvent */
    static const char *  sourceCalendarVcalUri;         /**< the default sources URI for vTodo */
    static const char *  sourceContactsVcardUri;        /**< the default sources URI for vCard */
    static const char *  sourcePicturesUri;             /**< the default sources URI for pictures */
    static const char *  sourceVideosUri;               /**< the default sources URI for videos */
    static const char *  sourceFilesUri;                /**< the default sources URI for files */
    static const char *  sourceMusicUri;                /**< the default sources URI for music */

    // For upgrades
    static const bool shouldFakeOldFunambolSwv = false; /**< if true, the installed version is replaced by a custom value */
    static const int fakeOldFunambolSwv = 80100;        /**< the custom value (see shouldFakeOldFunambolSwv) */

};

/** @endcond */
#endif

