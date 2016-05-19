/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2003 - 2011 Funambol, Inc.
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
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
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

#ifndef MH_SYNC_MANAGER
#define MH_SYNC_MANAGER

/** @cond API */
/** @addtogroup Client */
/** @{ */

#include <vector>

#include "base/globalsdef.h"
#include "base/util/ArrayList.h"
#include "spds/constants.h"
#include "spds/AbstractSyncConfig.h"
#include "spds/SyncReport.h"

#include "MediaHub/MHSyncSource.h"
#include "MediaHub/MHMediaRequestManager.h"

BEGIN_FUNAMBOL_NAMESPACE


/// Max number of items metadata to retrieve for each MH "getAll" call.
#define MH_PAGING_LIMIT       50

/// Max number of items metadata to retrieve for each MH "get IDs" call.
#define MH_PAGING_LIMIT_IDS   50


/**
 * Enumeration of possible error codes for MHSyncManager.
 */
typedef enum EMHSyncManagerError {
    ESSMSuccess = 0,
    ESSMCanceled,
    ESSMMHNotSupported,
    ESSMConfigError,
    ESSMMHError,
    ESSMBeginSyncError,
    ESSMEndSyncError,
    ESSMGetItemError,
    ESSMSetItemError,
    ESSMItemNotSupportedBySource,
    ESSMNetworkError,
    ESSMAuthenticationError,
    ESSMServerQuotaExceeded,
    ESSMClientQuotaExceeded,
    ESSMMediaHubPathNotFound,
    ESSMGenericSyncError,
    ESSMReadLocalItemsError,
    ESSMForbidden,
    ESSMPaymentRequired,
    ESSMUnknownMediaException,
	ESSMNoLocalChangesDetected,
    ESSMProxyAuthenticationError,

    ESSMErrorCodeLoginFailure750 = 750,
    ESSMErrorCodeLoginFailure751 = 751,
    ESSMErrorCodeLoginFailure752 = 752,
    ESSMErrorCodeLoginFailure753 = 753,
    ESSMErrorCodeLoginFailure754 = 754,
    ESSMErrorCodeLoginFailure755 = 755,
    ESSMErrorCodeLoginFailure756 = 756,
    ESSMErrorCodeLoginFailure757 = 757
    
} ESSMError;

typedef enum SyncDirection {
    TWO_WAYS,
    CLIENT_TO_SERVER,
    SERVER_TO_CLIENT
} SyncDirection;

/**
 * 
 */
class MHSyncManager {

public:

    /**
     * Initialize a new MH sync manager. Parameters provided to it
     * have to remain valid while this sync manager exists.
     *
     * @param s  the MH syncsource to sync
     * @param c  required configuration
     */
    MHSyncManager(MHSyncSource& s, AbstractSyncConfig& c);

    /// Destructor
    virtual ~MHSyncManager();
    
    bool initialize();

    int getAllServerMetadata();
    int getServerMetadataChanges();

    int getServerItemFromId(CacheItemsList& itemsList, CacheLabelsMap& labels, StringBuffer guid);
    
    void resetErrors() { setSyncError(ESSMSuccess); }
    
    ESMRStatus deleteRemoteItem(const char* guid);
    
	/**
	 * Starts the sync process.
	 */
    int sync(MHSyncSource* source, SyncSourceConfig* srcConf, SyncDirection direction);

protected:

    /**
     * Gets changed item info (metadata) from the Server, from a given array of items IDs.
     * Called by getServerChanges() for NEW and MOD Server items lists.
     * Paging the requests (see MH_PAGING_LIMIT_IDS for the limit): Server changes 
     * may be a lot, the getItemsFromId request can't be longer than 2K.
     * 
     * @param items    [IN-OUT] the list of items metadata returned by the Server
     * @param itemsIDs the Server items IDs to ask the metadata
     * @return         0 if no error
     */
    int getServerItemsFromIds(CacheItemsList& items, CacheLabelsMap& labels, const ArrayList& itemsIDs);
    
    int filterIncomingItems(CacheItemsList* serverItems);
    
    int performTwinDetection(int syncRes);

    
    //
    // ------------------------ retry mechanism ---------------------------
    //
    /**
     * Retries the MHMediaRequestManager::getItemsChanges(), called in case of network error.
     * It will retry the MH call until the error is not a network error.
     * The max number of retries is defined by the config param 'maxRetriesOnError'.
     * If the config param 'sleepTimeOnRetry' is set, a sleep will be applied
     * before each attempt (event SYNC_SOURCE_RETRY is fired).
     */
    ESMRStatus retryGetItemsChanges(ArrayList& newIDs, ArrayList& modIDs, ArrayList& delIDs, 
                                    const StringBuffer& fromDate);

    /**
     * Retries the MHMediaRequestManager::getItemsFromId(), called in case of network error.
     * It will retry the MH call until the error is not a network error.
     * The max number of retries is defined by the config param 'maxRetriesOnError'.
     * If the config param 'sleepTimeOnRetry' is set, a sleep will be applied
     * before each attempt (event SYNC_SOURCE_RETRY is fired).
     */
    ESMRStatus retryGetItemsFromId(CacheItemsList& items, CacheLabelsMap& labels, const ArrayList& itemsIDs);

    /**
     * Retries the MHMediaRequestManager::getAllItems(), called in case of network error.
     * It will retry the MH call until the error is not a network error.
     * The max number of retries is defined by the config param 'maxRetriesOnError'.
     * If the config param 'sleepTimeOnRetry' is set, a sleep will be applied
     * before each attempt (event SYNC_SOURCE_RETRY is fired).
     */
    ESMRStatus retryGetAllItems(CacheItemsList& items, CacheLabelsMap& labelsMap, int limit, int offset);

     /**
     * Retries to download again the server item, called in case of network error.
     * It will retry the action until the error is not a network error.
     * The max number of retries is defined by the config param 'maxRetriesOnError'.
     * If the config param 'sleepTimeOnRetry' is set, a sleep will be applied
     * before each attempt (event SYNC_SOURCE_RETRY is fired).
     *
     * @param serverItem  the item to download
     */
    ESMRStatus retryDownload(DownloadMHSyncItem* serverItem); 

    /**
     * Reads the client filter-by-number value from the config, and returns it.
     * If error, returns 0
     */
    int readClientFilterNumber();

    /**
     * Reads the server filter-by-number value from the config, and returns it.
     * If error, returns 0
     */
    int readServerFilterNumber();

    /**
     * Reads the server last timestamp (download) from the config, and returns it.
     * If error, returns 0
     */
    unsigned long readServerLastTimestamp();


    /**
     * Defines if we are syncing ALL items or CHANGES since a defined date.
     * It is called by beginSync() after the source's filters has been set.
     *
     *   - if is 1st sync on an active direction (download or upload), we always sync ALL items 
     *     (need twin detection at least the first time)
     *   - if filter-by-number is set for an active direction, we sync ALL
     *     (we can filter a number of items only on the ALL lists)
     *   - in none of the above, we can sync CHANGES
     *
     * @return  true if we have to sync item CHANGES since a date
     *          false if we have to sync ALL items
     */
    bool defineSyncBehavior();


    /**
     * Checks if the current Server Url and the username are the same as last sync.
     * If not, the cache/mappings/timestamps are no more valid, so they're 
     * cleaned up.
     * @return true if the cache/mappings/tstamps have been cleaned up, false if not
     */
    bool checkLastUserAndServer();

    /**
     * Called in case of sync errors.
     * Updates the report and the ssReport.
     * @param errorCode  one of ESSMError error codes
     * @param data       optionally, additional data
     */
    void setSyncError(const ESSMError errorCode, const int data = 0);
    
    int performUploads(MHSyncSource* source);

    /// The MHSyncSource under sync.
    MHSyncSource& source;

    /// The configuration object, containing all sync settings.
    AbstractSyncConfig& config;

    /// The sync source report, used to return sync results and errors.
    SyncSourceReport& report;
    

    /**
     * The interface for all MH calls.
     * It's initialized in the constructor, and it uses platform
     * specific HttpConnection to perform HTTP requests to the Server.
     */
    MHMediaRequestManager* mhMediaRequestManager;


    /// The name of MHSyncSource under sync, set in constructor for easy access.
    StringBuffer sourceName;

    /// The remote name of MHSyncSource under sync, set in constructor for easy access.
    StringBuffer sourceURI;

    /// The syncmode associated to this source. It's set in beginSync().
    SyncMode syncMode;

    /**
     * If false, it means we're synchronizing ALL items list (so 1st sync, twin detection,...)
     * If true, it means we're synchronizin NEW/MOD/DEL items changes from a specific date.
     * It's set during beginSync(), and it MUST be the same for upload and download.
     */
    bool isSyncingItemChanges;

    /**
     * This is the exact time when the current sync session started.
     * It is used as the last sync timestamp, for the downloads.
     * It is set in the config at the end of download() method, if all downloads ok.
     *
     * TODO: It should be created and returned by the Server during getAll/getChanges,
     *       now it's created by the client when sync session starts.
     */
    time_t downloadTimestamp;

    /**
     * It is the offset between the current client time (time(NULL)) and the server
     * time (requested in the MH). It is calculated with clientTime - serverTime
     */
    time_t offsetClientServer;
    
    /**
     * sapi session id (HTTP request cookie)
     */
    StringBuffer requestSessionId;

    /**
     * Says the remaining free space of the user quota
     */ 
    int64_t freeQuota;

    /**
     * Says the current total quota space of the user
     */
    uint64_t userQuota;

    /**
     * Return true if the current ItemInfo can be uploaded because there is enough space
     * on the cloud
     */
    bool fitsInTheCloud(MHSyncItemInfo* itemInfo);
    
    void releaseSourceOperations(std::vector<MHOperationDescriptor*>& operations);

};


END_FUNAMBOL_NAMESPACE

/** @} */
/** @endcond */
#endif

