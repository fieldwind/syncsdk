/*
 * Funambol is a mobile platform developed by Funambol, Inc.
 * Copyright (C) 2003 - 2009 Funambol, Inc.
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

#ifndef MH_SYNC_SOURCE
#define MH_SYNC_SOURCE

#include <vector>

#include "base/fscapi.h"
#include "spds/constants.h"
#include "spds/SyncSourceConfig.h"
#include "MediaHub/MHSyncItemInfo.h"
#include "MediaHub/MHLabelInfo.h"
#include "MediaHub/MHLabelsStore.h"
#include "MediaHub/UploadMHSyncItem.h"
#include "MediaHub/DownloadMHSyncItem.h"
#include "MediaHub/MHItemsStore.h"
#include "MediaHub/MHMediaRequestManager.h"
#include "base/util/ArrayListEnumeration.h"
#include "spds/SyncSourceReport.h"
#include "base/util/PropertyFile.h"
#include "base/adapter/PlatformAdapter.h"
#include "spds/spdsutils.h"
#include "spds/AbstractSyncConfig.h"
#include "http/AbstractHttpConnection.h"

BEGIN_FUNAMBOL_NAMESPACE


// Entry keys for the resume map
#define RESUME_UPLOAD               "upload"
#define RESUME_DOWNLOAD             "download"
#define PROPERTY_EXTENSION          "extension"
#ifndef PROPERTY_FOLDER_PATH
#define PROPERTY_FOLDER_PATH        "folderPath"
#endif
#ifndef PROPERTY_MEDIAHUB_PATH
#define PROPERTY_MEDIAHUB_PATH      "mediaHubPath"
#endif

typedef enum EMHSyncSourceError
{
    ESSSNoErr = 0,
    ESSSOperationCancelled, // operation cancelled by user
    ESSSItemNotSupported,   // sync source can't add this type of item (not supported by source backend)
    ESSSNoSpaceLeft,        // no space available left during insert
    ESSSPermissionDenied,   // permission error (on source backend)
    ESSSInvalidItem,        // item can't be added because is not valid
    ESSSInternalError       // configuration/setup general errors
} ESSSError; // sss_err_t

typedef enum EMHSyncSourceConflictResolutionType {
    EKeepLastModified,
    EKeepFromFirstList,
    EKeepFromSecondList,
    ERemoveFromFirstFromCache
} ESSSConflictResType;


class MHOperationDescriptor {
public:
    typedef enum OperationType {
        ADD_OP,
        UPDATE_OP,
        DELETE_OP
    } OperationType;
    
private:
    
    OperationType operationType;
    MHStoreEntry* oldEntry;
    MHStoreEntry* newEntry;
    std::vector<MHLabelInfo*>* newLabels;
    bool success;
    
    MHOperationDescriptor(OperationType opType, MHStoreEntry* oEntry, MHStoreEntry* nEntry,std::vector<MHLabelInfo*>* labels) :
         operationType(opType),
         oldEntry(oEntry),
         newEntry(nEntry),
         newLabels(labels) {
    }
    
public:
    static MHOperationDescriptor* createAddOperation(MHStoreEntry* entry, std::vector<MHLabelInfo*>* labels) {
        return new MHOperationDescriptor(MHOperationDescriptor::ADD_OP, NULL, entry, labels);
    }
    
    static MHOperationDescriptor* createUpdateOperation(MHStoreEntry* oEntry, MHStoreEntry* nEntry,
                                                        std::vector<MHLabelInfo*>* labels) {
        return new MHOperationDescriptor(MHOperationDescriptor::UPDATE_OP, oEntry, nEntry, labels);
    }
    
    OperationType getType()                   { return operationType; }
    MHStoreEntry* getNewEntry()               { return newEntry;  }
    MHStoreEntry* getOldEntry()               { return oldEntry;  }
    std::vector<MHLabelInfo*>* getLabels()    { return newLabels; }
    
    void setSuccess(bool value)               { success = value; }
    bool getSuccess()                         { return success; }
};



/**
 * This class represents the base for all the sync sources used by MHSyncManager to
 * perform a synchronization.
 * A sync source is a sort of glue between a synchronization engine and the specific
 * data to be synchronized. Synchronization engines are as data agnostic as possible
 * and a sync source is responsible for providing access to data.
 * For example a synchronization engine fetches the list of changes available on the server
 * and on a client and after resolving conflict it applies the required changes on both
 * sides. This task can only be accomplished using a sync source which has full
 * knowledge of the data and of the local data storage. In general a sync source is
 * responsbile for:
 *
 * 1) retrieve the list of local changes
 * 2) create new items received from the server
 * 3) update items modified on the server
 * 4) delete items removed on the server
 * 5) upload items that need to be refreshed on the server
 *
 * A MHSyncSource has explicit methods to create/update/delete/upload items, while the list
 * of changes is exposed with a table of type MHItemsStore. Such a table is expected to be
 * completely updated after the beginSync method is invoked and the MHSyncManager will
 * scan this list to fetch the local changes. Once this information is read it is comparated
 * with the list of remote changes and the engine decides the modifications to apply on
 * both sides.
 *
 * This class provides a basic infrastructure that needs to be extended/customized to be complete and
 * working.
 */
class MHSyncSource
{

public:

    /**
     * Constructs the MHSyncSource.
     * 
     * @param sapiSourceUri the server uri of the data being synchronized
     * @param sapiSourceArrayKey the server key in json arrays for the data being synchronized
     * @param orderField is the metadata field to be used to order data when requested to the server
     * @param sc    the syncSource's configuration
     * @param report the syncSource's report
     * @param itemsStore local cache for MH items (digital life). This table is exposed and used by the MHSyncManager
     *        to compute local changes.
     * @param incomingFilterDate a filter date from which to filter the item by date. 0 means no filter
     * @param outgoingFilterDate a filter date from which to filter the item by date. 0 means no filter
     * @param recursive specifies if the source shall recursively find objects in its store
     */
    MHSyncSource(const char* sapiSourceUri,
                 const char* sapiSourceArrayKey,
                 const char* orderField,
                 SyncSourceConfig& sc, 
                 SyncSourceReport& report,
                 MHItemsStore* itemsStore,
                 MHLabelsStore* labelsStore,
                 size_t incomingFilterDate, 
                 size_t outgoingFilterDate,
                 bool recursive_ = false);
                   

    virtual ~MHSyncSource();
    
    /////////////////////////////////////////////////////////
    ///////// PROPERTIES GETTERS/SETTERS ////////////////////
    /////////////////////////////////////////////////////////

    SyncSourceConfig& getConfig()     { return config;       }
    SyncSourceReport& getReport()     { return report;       }
    MHItemsStore* getMHItemsStore()   { return mhItemsStore; }
    
    const char* getSapiUri() const      { return sapiRemoteUri.c_str(); }
    const char* getSapiArrayKey() const { return sapiArrayKey.c_str(); }
    const char* getOrderField() const   { return orderField.c_str(); }
    
    void setRecursive(bool recur) { recursive = recur; }
    bool getRecursive()           { return recursive; }
    
    void setClientServerTimeDrift(time_t drift) { clientServerTimeDrift = drift; }
    time_t getClientServerTimeDrift() { return clientServerTimeDrift; }
    
    /////////////////////////////////////////////////////////
    ///////////// ITEMS FILTERING SECTION ///////////////////
    /////////////////////////////////////////////////////////
    

    /**
     * Checks if the given item shall be filtered and not sent to the server (i.e. ignored).
     * The provided implementation filters on item's size by invoking filterBySize.
     * May be overridden by clients for custom implementations.
     * @param clientItemInfo the metadata of client item to check
     * @return          true if the item has to be filtered out (skipped)
     *                  false if the item is ok 
     */
    virtual bool filterOutgoingItem(MHSyncItemInfo& itemInfo);
    
    /**
     * Checks if the given item received from the server shall be stored locally. The provided implementation
     * checks the item name extension by invoking filterByExtension.
     * May be overridden by clients for custom implementations.
     * @param serverItemInfo the metadata of server item to check
     * @param offsetTime difference between time on client and on server. It is
     *                   calculated clientTime - serverTime
     * @return true if the item must be filtered and not used by the manager
     */
    virtual bool filterIncomingItem(MHSyncItemInfo& serverItemInfo, time_t offsetTime);
    
    /**
     * Filters the incoming item based on the extension of the filename. This method is
     * invoked by the default filterIncomingItem implementation.
     * @return true if the item has to be filtered (skipped) false otherwise
     */
    bool filterByExtension(MHSyncItemInfo& clientItemInfo);
    
    /**
     * Filters the outgoing item based on the extension of the filename. This method
     * is invoked by the default filterOutgoingItem implementation.
     * @return true if the item has to be filtered (skipped) false otherwise
     */
    bool filterBySize(MHSyncItemInfo& clientItemInfo);
    
    void setBulkOperationsSize(uint32_t size) { bulkOperationsSize = size; }
    
    /////////////////////////////////////////////////////////
    /////////////////// SYNC HOOKS //////////////////////////
    /////////////////////////////////////////////////////////
    
    /**
     * This method is invoked at the beginning of the synchronization. The source can perform its initialization and
     * in particular it is required that the items store is properly populated and updated at the end of this method.
     * NOTE: SYNCSOURCE CAN STOP THE SYNC PROCESS HERE, RETURNING AN ERROR CODE != 0.
     * 
     * @param incremental true if this sync is an incremental (i.e. not the first one)
	 * @return  0 if no error occurred and the sync process should continue
     *          an error code != 0 if there was an error and/or the sync process should NOT continue
     */
    virtual int beginSync(bool incremental);
    
    /**
     * Called by the sync engine at the end of sync.
     */
    virtual void endSync(int syncEndCode);
    
    /**
     * This method is invoked once the synchronization engine terminates the download phase.
     * @param res the download phase result. 0 means success.
     * @return the result of the download phase
     */
    virtual int downloadPhaseCompleted(int res);
   
    /**
     * This method is invoked optionally before the synchronization engine starts the upload phase.
     */
    virtual void uploadPhaseStarted();
    
    /**
     * This method is invoked once the synchronization engine terminates the upload phase.
     * @param res the upload phase result. 0 means success.
     * @return the result of the upload phase
     */
    virtual int uploadPhaseCompleted(int res);
    
    /////////////////////////////////////////////////////////
    /////////////// OPERATIONS IMPLEMENTATION ///////////////
    /////////////////////////////////////////////////////////
    
    /**
     * This method creates an UploadMHSyncItem which is used by the default uploadItem implementation to send the
     * provided item to the server. This method is required to have a complete implementation unless a source
     * also redefines the uploadItem method in a way that does not require it.
     * @param itemInfo the item to be sent to the server
     */
    virtual UploadMHSyncItem* createUploadItem(MHSyncItemInfo* itemInfo) = 0;
    
    /**
     * This method is invoked by the synchronization engine when an item is about to be sent to the server. The default
     * implementation simply invokes uploadItem.
     * @param itemInfo the item to be sent to the server
     * @param mainConfig the main configuration
	 * @return 0 if no error, a EMHSyncManagerError code otherwise (ESSM-x)
     */
    virtual int startUploadItem(MHSyncItemInfo* itemInfo, AbstractSyncConfig& mainConfig);
    
    /**
     * This method uploads an item to server. Redefining this method is possible but not required. Usually it can be
     * redefined to add a wrapper, but a complete redefinition is discouraged because the default implementation takes
     * care of uploading/resuming and handling all the possible upload errors.
     * @param itemInfo the item to be sent to the server
     * @param mainConfig the main configuration
     */
    virtual int uploadItem(MHSyncItemInfo* itemInfo, AbstractSyncConfig& mainConfig,
                           HttpConnectionUploadObserver* uploadObserver);

    /**
     * Searches for a twin item in the local items lists.
     * This method may be overridden by clients to customize the
     * twin detection algorithm.
     * The default implementation checks the fileName and the file size.
     * If the fileName is different but the size is the same the item
     * is considered the same but there is a log at info level.
     * This implementation is fine for media like pictures, video, music
     * It can be different for files.
     *
     * NOTE: if twin found, the item is removed from the corresponding
     *       list of local items, since there's no need to sync it.
     *
     * @param serverItemInfo the metadata of server item to search
     * @return the local twin item if found, as a new allocated copy (must be deleted by the caller)
     *         NULL if the twin item is not found
     */
    virtual MHSyncItemInfo* twinDetection(MHSyncItemInfo& serverItemInfo, std::vector<MHStoreEntry*> localItems);
    
    /**
     * Deletes an item from the local device and updates the local stores.
     * To be reimplemented by derived classes (default implementation returns -1)
     * @return  0 if item removed succesfully
     *         >0 if errors removing the item
     *         -1 if not implemented
     */
    virtual int deleteItem(MHSyncItemInfo* itemInfo);


    /////////////////////////////////////////////////////////
    //////////// CACHE ACCESS ///////////////////////////
    /////////////////////////////////////////////////////////
    
    /* These methods are provided for convenience to allow access to the source itemsStore (cache).
     * In general these methods do not require a reimplementation and are part of the basic provided
     * infrastructure.
     */
   
    /**
     * Gets all items keys from the cache.
     */
    virtual bool getItemsFromCache(CacheItemsList& itemsInfoList);
    
    virtual MHSyncItemInfo* getItemFromCache(const unsigned long itemID);
    
    /**
     * Gets from the cache the item with the given fieldValue stored in the fieldName. This method is safe
     * only if the item is unique. If multiple items satisfy the condition, then only the first one is returned.
     */
    virtual MHSyncItemInfo* getItemFromCache(const char* fieldName, const char* fieldValue);
    
    /**
     * Gets the number of items from the cache
     */
    virtual int getCountFromCache();
    
    /**
     * insert item into MH local store
     */
    virtual bool addItemToCache(MHSyncItemInfo* itemInfo, std::vector<MHLabelInfo*>* labels);

    /**
     * insert items into MH local store
     */
    virtual bool addItemsToCache(std::vector<MHOperationDescriptor*>& operations);
    
    /**
     * Updates an item into MH local store.
     * If labels is NULL then existing labels are left unchanged. Otherwise the new set of labels is applied. If
     * an empty set of labels is provided, then the item's labels are cleared.
     */    
    virtual bool updateItemInCache(MHSyncItemInfo* itemInfo, std::vector<MHLabelInfo*>* labels);
    virtual bool updateItemInCache(MHSyncItemInfo* oldItemInfo, MHSyncItemInfo* itemInfo, std::vector<MHLabelInfo*>* labels);
    
    virtual bool updateItemsInCache(std::vector<MHOperationDescriptor*>& operations);
    
    /**
     * Removes an item from MH local store. Returns true if item found and succesfully removed
     */
    bool removeItemFromCache(MHSyncItemInfo* itemInfo);
    
    /**
     * Removes all entries from the cache.
     */    
    bool resetCache();
    
    void setBackwardCompatibilityMode(bool mode) { backwardCompatibilityMode = mode; }

    /////////////////////////////////////////////////
    /////////////////// MISC ////////////////////////
    /////////////////////////////////////////////////

    /**
     * Called by the constructors to create and initialize the KeyValueStore base on a storage location that 
     * can be passed by the client or left to the sync source to be created in a default position.
     * @param sc     the syncSource's configuration
     */
    virtual bool init(const char* sapiSourceUri, const char* sapiArrayKey,
                      const char* sourceOrderField,
                      SyncSourceConfig& sc);
    
    /**
     * Sets the (required for uploads!) content type of an item, from the name extension. The default implementation
     * chooses a mime type based on item's name extension. Maybe overridden with more specific implemenation.
     */
    virtual void setContentTypeByExtension(MHSyncItemInfo* itemInfo);
    
    /**
     * Removes temporary data (usually stored into files) used during item's upload.
     * It is called by the MHSyncManager when it doesn't want to keep this data.
     *
     * @param item   the identifier 
     * @return 0 if OK, a value otherwise, -2 if not implemented
     */
    virtual int cleanTemporarySpoolItem(const StringBuffer& item);    
    
    virtual MHItemJsonParser* createItemJsonParser();

    /**
     * Cleans up any local file/DB owned by this syncsource
     */
    virtual void cleanup() {}

    // TODO: this is just temporary until we re-enable the findTwin mechanism
    virtual int performTwinDetection(AbstractSyncConfig& config);

    /**
     * called by the MHSyncManager when a quota error occurs. The default implementation just
     * set the status as in quota error and save into db
     */
    virtual void handleServerQuota(MHSyncItemInfo* itemInfo);

protected:
    /**
     * Normalize the \\ into /
     */
    static void normalizeSlash(StringBuffer& s);
   
    /**
     * Return the installation timestamp passed in the constructor
     */
    size_t getOutgoingFilterDate() { return outgoingFilterDate; }
    
    /**
     * Return the installation timestamp passed in the constructor
     */
    size_t getIncomingFilterDate() { return incomingFilterDate; }

    /**
     * It should return if there is enough room to store the size passed as argument.
     * Default implmenation return always true.
     * @param the size to check if is is possible to be stored
     * @param [OUT] errorCode that can be returned by the check method
     * @return true if there is room, false otherwise
     */
    virtual bool isLocalStorageAvailable(int64_t size, int* errorCode) {
        return true;
    }
    
    int resumeUpload(UploadMHSyncItem& resumeItem, AbstractSyncConfig& mainConfig,
        MHMediaRequestManager& mhMediaRequestManager, int64_t& firstByteToSend);

protected:
    
    /**
     * Looks for the MHSyncItemInfo corresponding to this luid in this list.
     * If found, its position index is returned, -1 if not found.
     * @param luid the luid to look for
     * @param list the items list to look into
     * @return     the item position index, -1 if not found
     */
    int getItemInfoIndex(const StringBuffer& luid, std::vector<MHStoreEntry*> list);
    
    /**
     * Filters the item searching its guid in blacklist. 
     * @return true if the item has to be filtered (skipped) false otherwise
     */
    bool filterFromBlackList(MHSyncItemInfo& itemInfo);

    /**
     * Called by the setItemStatus to decide if the code is an error code or not.
     * Based on the result, is udpates the cache or not.
     *
     * @code the code to be analyzed
     * @return true if it is an error code, false otherwise
     */
    virtual bool isErrorCode(int code);

    /**
     * Used to set errors for this SyncSource.
     * Will set the source state (error state), the source's last error
     * code and source's last error message.
     * @param  errorCode  the error code
     */
    void setSourceError(const int errorCode);


    /** 
     * add item to black list
     */
    int blackListItem(MHSyncItemInfo* itemInfo);
    
    int getUploadSpecificError(int err);

    /**
     * Labels management utilities
     */
    bool addMissingLabels(std::vector<MHLabelInfo*>* labels);
    bool addLabelsToItem(MHSyncItemInfo* itemInfo, std::vector<MHLabelInfo*>* labels);
    bool clearLabelsForItem(MHSyncItemInfo* itemInfo);
    
    /**
      * The syncsource's configuration, including custom 
      * params like filters and folder location to sync.
      */
    SyncSourceConfig& config;
    
    /**
     * The syncsource's report (owned by SyncReport)
     */
    SyncSourceReport& report;

    
    /// local storage for MH items (digital life)
    MHItemsStore* mhItemsStore;
    
    /// local storage for labels to which this source items can be associated
    MHLabelsStore* mhLabelsStore;
    
    /**
     * Enumeration of the new ItemInfo       
     */
    ArrayListEnumeration*  newItemInfo;

    /**
     * Enumeration of the updated ItemInfo  
     */
    ArrayListEnumeration*  updatedItemInfo;

    /**
     * Enumeration of the deleted ItemInfo  
     */
    ArrayListEnumeration*  deletedItemInfo;       
    
    /**
     * If false, it means we're synchronizing ALL items list (so 1st sync, twin detection,...)
     * If true, it means we're synchronizin NEW/MOD/DEL items changes from a specific date.
     * It's set during beginSync(), defined and passed by MHSyncManager.
     */
    bool isSyncingItemChanges;
    
    /**
     * List of extensions as filter in output. If the first element is a !, then the values are the ones 
     * not allowed. Otherwise they are allowed.
     * ex: !,.jpg,.tiff -> only the .jpg and .tiff are not allowed and are removed
     * ex: .jpg,.tiff -> only the .jpg and .tiff are synced
     */
    ArrayList extension;    

    /**
     *  flag to set recursive scan of item directory (used in populateAllItemInfoList())
     */
    bool recursive;
    
    /**
     * Remote URI for this source in the SAPI domain
     */
    StringBuffer sapiRemoteUri;
    
    /**
     * Array key for the source in the SAPI domanin
     */
    StringBuffer sapiArrayKey;
    
    /**
     * The field to be used when requring ordered items to the server. The allowed values depend on the server,
     * at the moment we have creationdate, date and id, but this list may change in the future.
     */
    StringBuffer orderField;
    
    
    /**
     * A filter date from which to filter the item by date in the incoming process
     */
    size_t incomingFilterDate;
    
    /**
     * A filter date from which to filter the item by date in the outgoing process
     */
    size_t outgoingFilterDate;
    
    /**
     * time drift from client to server 
     */
    time_t clientServerTimeDrift;

    /**
     * In backward compatibility mode the batch operations (addItemsToCache and updateItemsInCache) will
     * call the corresponding single item operation (addItemToCache and updateItemInCache)
     */
    bool backwardCompatibilityMode;
    
    uint32_t bulkOperationsSize;
    
    /**
     * Looks for the MHSyncItemInfo corresponding to this guid in this list.
     * If found, a pointer to the item is just returned (no copy, no remove).
     * @param luid the luid to look for
     * @param list the items list to look into
     * @return     pointer to MHSyncItemInfo corresponding to the luid, NULL if not found
     */
    MHSyncItemInfo* getItemInfo(const StringBuffer& luid, ArrayListEnumeration* list);
};

END_FUNAMBOL_NAMESPACE

#endif 

