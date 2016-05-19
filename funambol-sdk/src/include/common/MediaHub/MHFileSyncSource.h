/*
 * Funambol is a mobile platform developed by Funambol, Inc.
 * Copyright (C) 2012 Funambol, Inc.
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
 
#ifndef __MH_FILE_SYNC_SOURCE_H__
#define __MH_FILE_SYNC_SOURCE_H__

#include "base/fscapi.h"
#include "spds/constants.h"
#include "spds/SyncSourceConfig.h"
#include "MediaHub/MHSyncItemInfo.h"
#include "MediaHub/MHLabelsStore.h"
#include "base/util/KeyValueStore.h"
#include "spds/SyncSourceReport.h"
#include "base/util/PropertyFile.h"
#include "spds/spdsutils.h"
#include "MediaHub/MHSyncSource.h"
#include <string>
#include <vector>

namespace Funambol {

class DMTClientConfig;
class DownloadProgressObserver;

class MHFileSyncSource: public MHSyncSource
{
    
public:
    
    MHFileSyncSource(const char* sapiSourceUri, const char* sapiArrayKey,
                            const char* orderFieldValue,
                            SyncSourceConfig& sc, SyncSourceReport& report, 
                            MHItemsStore* itemsStore, MHLabelsStore* labelsStore,
                            size_t incomingFilterDate, size_t outgoingFilterDate,
                            StringBuffer& tSpoolPath, DMTClientConfig* clientConfig_);
    
    
    virtual ~MHFileSyncSource();
	
	/**
     * This method is invoked at the beginning of the synchronization. The source can perform its initialization and
     * checks for local changes in the local folders (new/mod/del items).
     *
     * @param incremental true if this sync is an incremental (i.e. not the first one) - actually NOT USED
	 * @return  0 if no error occurred and the sync process should continue
	 *          code 20 (ESSMNoLocalChangesDetected) if no local change has been detected and 
     *            the flag notifyIfNoLocalChanges is true (NOTE: this will stop the sync immediately)
     */
    virtual int beginSync(bool /*incremental*/);

    virtual int performTwinDetection(AbstractSyncConfig& mainConfig);

    virtual void uploadPhaseStarted();
    virtual int uploadPhaseCompleted(int res);
    
    virtual int downloadPhaseCompleted(int res);
    
    virtual int downloadItem(MHSyncItemInfo* itemInfo, OutputStreamObserver* observer);
    virtual int startUploadItem(MHSyncItemInfo* itemInfo, AbstractSyncConfig& config);

    virtual int readNewItems(AbstractSyncConfig& mainConfig, const char* listType);                         // DEPRECATED
    virtual int readNewItems(AbstractSyncConfig& mainConfig, const char* listType, int &numNewItemsInDL);   // DEPRECATED
    virtual int readAllItemsChanges(AbstractSyncConfig& mainConfig);                                        // DEPRECATED
        
    virtual InputStream* createInputStream(MHSyncItemInfo& itemInfo) = 0;
    virtual FileOutputStream* createOutputStream(MHSyncItemInfo& itemInfo);
    
    virtual bool isLocalStorageAvailable(int64_t size, int* errorCode);

    virtual int checkIfFitsOnLocalStorage(MHSyncItemInfo* itemInfo);

	/// @return true if the path is a removable folder for uploads, and the root folder is NOT available
	///         false if not a removable drive, or root folder available
	///        (config param PROPERTY_ITEMS_UPLOAD_REMOVABLE_DIRECTORIES_LIST)
	bool removableDriveForUploadNotAvailable(const char* filePath);

	/// @return true if the path is a network folder for uploads, and the root folder is NOT available
	///         false if not a removable drive, or root folder available
	///        (config param PROPERTY_ITEMS_UPLOAD_NETWORK_DIRECTORIES_LIST)
	bool networkDriveForUploadNotAvailable(const char* filePath);


    /// @param item   the item's name (the GUID or LUID)
    virtual int cleanTemporarySpoolItem(const StringBuffer& item);


    // Cleanup spool folders
    virtual void cleanup();

    /**
     * Called by MHSyncManager after downloading an item that must be locally added. The default implementation takes care
     * of invoking insertItem to store the item in the local store and to add the proper entry into the itemsStore (cache).
     * The method can be redefined even if in general this is not required to have a functional source.
     * @param syncItem the item to add
     * @param errCode pointer to a EMHSyncSourceError status code
     * @return the LUID of the new item added if no errors happen
     * on error an empty string buffer is returned and errCode is 
     * set to appropiate error
     */
    virtual StringBuffer addItem(DownloadMHSyncItem* syncItem, EMHSyncSourceError* errCode);


    /**
     * It must store the outputstream in the proper way depending on the device.     
     * @param - OutputStream that the client must store in the proper way
     * @return - The key of the item stored in the device
     */    
    StringBuffer insertItem(DownloadMHSyncItem* syncItem, EMHSyncSourceError* errCode, long* modificationDate);
    
    /**
     * Deletes an item from the local device.
     * @return  0 if item removed succesfully
     *         >0 if errors removing the item
     *         -1 if not implemented (default)
     */
    virtual int deleteItem(MHSyncItemInfo* itemInfo);

    virtual bool addItemToCache(MHSyncItemInfo* itemInfo, std::vector<MHLabelInfo*>* labels);
    virtual bool updateItemInCache(MHSyncItemInfo* itemInfo, std::vector<MHLabelInfo*>* labels);
    virtual bool updateItemInCache(MHSyncItemInfo* oldItemInfo, MHSyncItemInfo* itemInfo, std::vector<MHLabelInfo*>* labels);
    
    void getAdditionalMetadata(MHSyncItemInfo* itemInfo); 
    
    /**
     * Called by MHSyncManager, before downloading a new item.
     * Creates a a new item in the local storage, given the
     * item's metadata. The returned DownloadMHSyncItem contains an
     * outputStream ready to be written with the downloaded data.
     * If something was already downloaded, 
     *
     * @param  itemInfo the item's metadata info
     * @return a new allocated SapiSyncItem
     */
    virtual DownloadMHSyncItem* createDownloadItem(MHSyncItemInfo& itemInfo, OutputStreamObserver* observer);
    virtual UploadMHSyncItem* createUploadItem(MHSyncItemInfo* itemInfo);

    /// Sets the flag @see notifyIfNoLocalChanges.
    void setNotifyIfNoLocalChanges(bool val) { notifyIfNoLocalChanges = val; }
    
 
protected:
    virtual ESMRStatus retryDownload(MHMediaRequestManager* mhMediaRequestManager, DownloadMHSyncItem* itemInfo);
    virtual int downloadRequest(MHSyncItemInfo* itemInfo, OutputStreamObserver* observer);
    virtual void requestSourceOperation(unsigned componentType, MHSyncItemInfo* itemInfo);
    virtual void sourceStatusUpdate(unsigned sourceStatus, MHSyncItemInfo* itemInfo);
    virtual bool moveTemporarySpoolItem(MHSyncItemInfo* itemInfo);
    virtual MHSyncItemInfo* twinDetection(MHSyncItemInfo& serverItemInfo, std::vector<MHStoreEntry*> localEntries);

    virtual bool filterByName(const StringBuffer& name);
    
    bool fixLocalItemRemoved(MHSyncItemInfo* itemInfo);
    std::vector<MHStoreEntry*>::iterator getItemInfoIndexByName(const StringBuffer& name, std::vector<MHStoreEntry*>& list);

    int readDir(const char* path, std::vector<std::string>& fileList, bool recursive, 
                std::map<std::string, bool>& readFoldersMap);
    StringBuffer generateLocalFileName(const StringBuffer& fileName, const StringBuffer& dirPath);
    std::string baseName(const std::string& fullName); 
    bool dirName(std::string& dirPath, const std::string& fullName);
    
    // local item management methods

	/**
     * Called by beginSync, checks for local changes in the local folders (new/mod/del items).
	 * Once detected, local changes are stored in the Digital Life items store so the next phases of synchronization
	 * will directly read the items store and perform data transfers if required (uploads).
	 * @return true if some local change has been detected (new/mod/del items), false if no local change is detected.
     */
    virtual bool getLocalItems();

    virtual bool getLocalUpdates(std::vector<MHSyncItemInfo*>& newLocalItems, std::vector<MHSyncItemInfo*>& updatedLocalItems);
    virtual bool getMissingLocalItems(std::vector<MHSyncItemInfo*>& missingLocalItems);
    virtual bool localItemsFullScan(CacheItemsList& itemsInfoList);
    virtual DownloadProgressObserver* createDownloadProgressObserver(MHSyncItemInfo *itemInfo, const char* sourceName);

	/**
	 * Checks if the filePath belongs to one of the folders in the directoryList (comma separated values).
	 * @return the folder this filePath belongs to, if found - empty string if not found. 
	 */
	std::string findPathOnDirectoriesList(const char* filePath, const char* directoriesList);
    

    /**
     * List of extensions as filter in output. If the first element is a !, then the values are the ones 
     * not allowed. Otherwise they are allowed.
     * ex: !,.jpg,.tiff -> only the .jpg and .tiff are not allowed and are removed
     * ex: .jpg,.tiff -> only the .jpg and .tiff are synced
     */
    ArrayList extension;
    
    /**
     * List of file names to be excluded as filter in output 
     */
    ArrayList excluded_file_names;

    // the dir where the temp file are stored
    StringBuffer tmpSpoolPath;

    std::string itemsDownloadPath;
    std::vector<std::string> itemsUploadPaths;
    std::vector<std::string> excludedDirectoryNames;
    
    DMTClientConfig* clientConfig;

    /**
     * If true, beginSync will return code 20 (ESSMNoLocalChangesDetected) in case there's no local change detected
     * (it will cause the sync to stop immediately).
     * Default = false. Used only for local-check-only syncs.
     */
    bool notifyIfNoLocalChanges;
    
public:
    static const char* PROPERTY_EXCLUDED_FILE_NAMES;
    static const char* PROPERTY_EXCLUDED_DIRECTORY_NAMES;
    static const char* PROPERTY_ITEMS_DOWNLOAD_DIRECTORY;
    static const char* PROPERTY_ITEMS_UPLOAD_DIRECTORIES_LIST;
	static const char* PROPERTY_ITEMS_UPLOAD_NETWORK_DIRECTORIES_LIST;
	static const char* PROPERTY_ITEMS_UPLOAD_REMOVABLE_DIRECTORIES_LIST;
};

}

#endif

