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

#include "base/util/StringBuffer.h"
#include "MediaHub/MHContentTypes.h"
#include "MediaHub/MHSyncManager.h"
#include "MediaHub/MediaRequestManagerFactory.h"
#include "client/DMTClientConfig.h"
#include "event/FireEvent.h"
#include "base/util/WString.h"
#include "ioStream/DownloadProgressObserver.h"
#include "base/util/utils.h"

#include "MediaHub/MHFileSyncSource.h"
#include <map>

namespace Funambol {

const char* MHFileSyncSource::PROPERTY_EXCLUDED_FILE_NAMES                     = "excludedFileNames";
const char* MHFileSyncSource::PROPERTY_ITEMS_DOWNLOAD_DIRECTORY                = "itemsDownloadDirectory";
const char* MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_DIRECTORIES_LIST           = "itemsUploadDirectoriesList";
const char* MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_NETWORK_DIRECTORIES_LIST   = "itemsUploadNetworkDirectoriesList";
const char* MHFileSyncSource::PROPERTY_ITEMS_UPLOAD_REMOVABLE_DIRECTORIES_LIST = "itemsUploadRemovableDirectoriesList";
const char* MHFileSyncSource::PROPERTY_EXCLUDED_DIRECTORY_NAMES                = "excludedDirectoryNames";

MHFileSyncSource::MHFileSyncSource(const char* sapiSourceUri,
                                                 const char* sapiArrayKey,
                                                 const char* orderFieldValue,
                                                 SyncSourceConfig& sc, SyncSourceReport& rep, 
                                                 MHItemsStore* itemsStore,
                                                 MHLabelsStore* labelsStore,
                                                 size_t incomingFilterDate,
                                                 size_t outgoingFilterDate,
                                                 StringBuffer& tSpoolPath, DMTClientConfig* clientConfig_)
                                    : MHSyncSource(sapiSourceUri, sapiArrayKey, orderFieldValue, sc, rep,
                                                   itemsStore, labelsStore, incomingFilterDate, outgoingFilterDate, NULL), 
                                      clientConfig(clientConfig_)
{
    tmpSpoolPath = tSpoolPath;
    
    if (createFolder(tmpSpoolPath.c_str()) != 0) {
        LOG.error("[%s] cannot create the temporary files spool folder", __FUNCTION__);
    } else {
        tmpSpoolPath.append("/");
    }
   
    itemsDownloadPath = sc.getProperty(PROPERTY_ITEMS_DOWNLOAD_DIRECTORY);
    if (itemsDownloadPath.empty()) {
        LOG.error("%s: download path for items is unset", __FUNCTION__);
    } else {
        LOG.debug("%s: items download path: %s", __FUNCTION__, itemsDownloadPath.c_str());
    }

    const char* uploadFoldersList          = sc.getProperty(PROPERTY_ITEMS_UPLOAD_DIRECTORIES_LIST);
	const char* uploadNetworkFoldersList   = sc.getProperty(PROPERTY_ITEMS_UPLOAD_NETWORK_DIRECTORIES_LIST);
    const char* uploadRemovableFoldersList = sc.getProperty(PROPERTY_ITEMS_UPLOAD_REMOVABLE_DIRECTORIES_LIST);
	
	if (uploadFoldersList) {
        int num = splitString(uploadFoldersList, ',', itemsUploadPaths);
		LOG.debug("[%s]: %d upload folders to scan", __FUNCTION__, num);
    }
	if (uploadNetworkFoldersList) {
		int num = splitString(uploadNetworkFoldersList, ',', itemsUploadPaths);
		LOG.debug("[%s]: %d network upload folders to scan", __FUNCTION__, num);
	}
	if (uploadRemovableFoldersList) {
		int num = splitString(uploadRemovableFoldersList, ',', itemsUploadPaths);
		LOG.debug("[%s]: %d removable upload folders to scan", __FUNCTION__, num);
	}
	LOG.debug("[%s]: %d total upload folders to scan", __FUNCTION__, itemsUploadPaths.size());

    
    const char* ext = sc.getProperty(PROPERTY_EXTENSION);
    if (ext) {
        StringBuffer vals(ext);
        vals.split(extension, ",");
    }
    
    if (extension.size() == 0) {
        LOG.debug("[%s] No filter on extension: all allowed outgoing", __FUNCTION__);
    }
    
    const char* excluded_files = sc.getProperty(PROPERTY_EXCLUDED_FILE_NAMES);
    if (excluded_files) {
        StringBuffer vals(excluded_files);
        vals.split(excluded_file_names, ",");
    }
    
    if (extension.size() == 0) {
        LOG.debug("[%s] No filter on file names: all allowed outgoing", __FUNCTION__);
    }
   
    const char* excluded_dir_names = sc.getProperty(PROPERTY_EXCLUDED_DIRECTORY_NAMES);
    
    if (excluded_dir_names) {
        splitString(excluded_dir_names, ',', excludedDirectoryNames);
    }

    notifyIfNoLocalChanges = false;
}

MHFileSyncSource::~MHFileSyncSource() 
{
}

int MHFileSyncSource::startUploadItem(MHSyncItemInfo* itemInfo, AbstractSyncConfig& config)
{
    int uploadStatus = 0;
    
    if (itemInfo == NULL) {
        return ESSMGenericSyncError;
    }
	if (itemInfo->getLocalItemPath().empty()) {
		return ESSMGenericSyncError;
	}

	// Check before upload.
	if (!fileExists(itemInfo->getLocalItemPath())) {
		// skip upload if removable/network drive is no more available at this point
		if (removableDriveForUploadNotAvailable(itemInfo->getLocalItemPath())) {
			LOG.info("Skip upload %s '%s': removable drive no more available, disconnected?", 
				getConfig().getName(), itemInfo->getLocalItemPath().c_str());
			return ESSMSuccess;		// not an error (upload not even started)
		}
		else if (networkDriveForUploadNotAvailable(itemInfo->getLocalItemPath())) {
			LOG.info("Skip upload %s '%s': network drive no more available", 
				getConfig().getName(), itemInfo->getLocalItemPath().c_str());
			return ESSMSuccess;     // not an error (upload not even started)
		}
		else {
			LOG.error("%s: cannot upload %s '%s': file is no more available", 
				__FUNCTION__, getConfig().getName(), itemInfo->getLocalItemPath().c_str());
			return ESSMReadLocalItemsError;
		}
	}
    

	//
	// UPLOAD ITEM
	//
    StringBuffer itemName = itemInfo->getName();
    LOG.info("Starting upload of %s: %s (ID=%lu)", getConfig().getName(), itemName.c_str(), itemInfo->getId());
    int numFailedUpload = itemInfo->getNumUploadFailures();

    if ((uploadStatus = uploadItem(itemInfo, *clientConfig, NULL)) == 0) {
        LOG.debug("%s: Upload completed successfully",__FUNCTION__);
    } 
	else {
        if (uploadStatus == ESSMUnknownMediaException) {
            itemInfo->setNumUploadFailures(++numFailedUpload);
            if (numFailedUpload == 5) {
                itemInfo->setStatus(EStatusLocalOnly);
            }
            updateItemInCache(NULL, itemInfo, NULL);
        }
		else if (uploadStatus == ESSMReadLocalItemsError) {
			if (removableDriveForUploadNotAvailable(itemInfo->getLocalItemPath())) {
				// this may happen when a USB drive is disconnected while reading input stream
				LOG.error("Could not upload %s item '%s': removable drive disconnected?", 
					getConfig().getName(), itemInfo->getName().c_str());
			}
			// TODO: check for network drives? can we retry the upload here?
		}
		else {
			LOG.error("%s: error uploading item: %d", __FUNCTION__, uploadStatus);
		}
    }   

    return uploadStatus;
}

FileOutputStream* MHFileSyncSource::createOutputStream(MHSyncItemInfo& itemInfo)
{
    StringBuffer guid = itemInfo.getGuid();

    if (guid.empty()) {
        LOG.error("[%s] the guid from the server is empty", __FUNCTION__);
    }

    StringBuffer fileName = itemInfo.getName();

    unsigned int pos = fileName.rfind(".");

    if (pos != StringBuffer::npos) {
        StringBuffer extension = fileName.substr(pos);
        guid.append(extension);
        LOG.debug("[%s] the file to write is %s", __FUNCTION__, guid.c_str());
    }

    StringBuffer tmpFile = tmpSpoolPath;
    tmpFile.append(guid);

    FileOutputStream* ostream = new FileOutputStream(tmpFile, true); // move the offset if the file already exists

    return ostream;
}

int MHFileSyncSource::cleanTemporarySpoolItem(const StringBuffer& item) 
{
    StringBuffer tmpFile = tmpSpoolPath;    
    tmpFile.append(item.c_str());
    LOG.debug("%s: removing temporary spool item: %s", __FUNCTION__, tmpFile.c_str());
    return remove(tmpFile);
}

void MHFileSyncSource::cleanup()
{
    removeFileInDir(tmpSpoolPath.c_str());
}

int MHFileSyncSource::downloadRequest(MHSyncItemInfo* itemInfo, OutputStreamObserver* observer) 
{
    WString wguid;
    StringBuffer luid;
    DownloadMHSyncItem* serverItem = NULL;
    EMHMediaRequestStatus err;
    EMHSyncSourceError sourceErr = ESSSNoErr;
    bool removeTmpItem = false;
    bool    isResume = false;
    int downloadStatus = -1;
    int64_t itemSize = 0;
    
    if (itemInfo == NULL) {
        LOG.error("invalid parameter for item download request");
        downloadStatus = ESSMGenericSyncError;
        return downloadStatus;
    }
    
    StringBuffer guid = itemInfo->getGuid();
    if (guid.empty()) {
        LOG.error("Internal error: empty download item GUID");
        downloadStatus = ESSMGenericSyncError;
        goto finally;
    }
    
    wguid = guid;
    
    if ((itemSize = itemInfo->getSize()) > 0) {
        isResume = true;
    }
    
    // Check local quota (not in case of resume!)
    if (isResume == false) {
#if 0
        int requiredSpace = itemSize;
        int errorCode = 0;        
        // calculate required disk space for an update

        MHSyncItemInfo* localItem = getItemInfo(luid);
        if (localItem) {
            if (itemInfo->getSize() > localItem->getSize()) {
                requiredSpace = itemInfo->getSize() - localItem->getSize();
            }
        }
        
        if (isLocalStorageAvailable(requiredSpace, &errorCode) == false) {
            if (errorCode == ERR_FILE_SYSTEM) {
                LOG.info("Can't download item %s: cannot find the MediaHub path", itemInfo->getName().c_str());
                downloadStatus = ESSMMediaHubPathNotFound;  
            } else {
                LOG.info("Can't download item %s: local storage is full", itemInfo->getName().c_str());
                downloadStatus = ESSMClientQuotaExceeded;
            }
            goto finally;
        }
#endif
    }
    
    // Allocate space for the incoming item (creates the output stream)
    // In case of resume, opens the already existing item and links the output stream
    // at the end of the data already received
    serverItem = createDownloadItem(*itemInfo, observer);
    if (serverItem == NULL) {
        downloadStatus = ESSMSetItemError;
        goto finally;
    } else {
        
        updateItemInCache(itemInfo, NULL);
        
        //
        // *** Download data ***
        //
        URL url(clientConfig->getSyncURL());
        StringBuffer host = url.getHostURL();
        MediaRequestManagerFactory* reqManFactory = MediaRequestManagerFactory::getInstance();
        MHMediaRequestManager* mhMediaRequestManager = reqManFactory->getMediaRequestManager(host, 
                                                          getSapiUri(),
                                                          getSapiArrayKey(),
                                                          getOrderField(),
                                                          createItemJsonParser(),
                                                          clientConfig);
                                                  
        // set http params, read from config
        mhMediaRequestManager->setRequestTimeout   (clientConfig->getMHRequestTimeout());
        mhMediaRequestManager->setResponseTimeout  (clientConfig->getMHResponseTimeout());
        mhMediaRequestManager->setUploadChunkSize  (clientConfig->getMHUploadChunkSize());
        mhMediaRequestManager->setDownloadChunkSize(clientConfig->getMHDownloadChunkSize());
                
        LOG.info("Downloading item '%s'...", itemInfo->getName().c_str());
        err = mhMediaRequestManager->downloadItem(serverItem);
        
        if (err == ESMRNetworkError) {
            err = retryDownload(mhMediaRequestManager, serverItem);
        } 
        
        delete mhMediaRequestManager;
   
        if (err != ESMRSuccess) {
            if (err == ESMRNetworkError) {
                downloadStatus = ESSMNetworkError;
                goto finally;
            }
            if (err == ESMRInvalidParam        || err == ESMRConnectionSetupError ||
                err == ESMRAccessDenied        || err == ESMRHTTPFunctionalityNotSupported ||
                err == ESMRMHNotSupported    || err == ESMRInvalidContentRange || 
                err == ESMRErrorRetrievingMediaItem || err == ESMRForbidden) {
                // For these error codes, we DON'T resume http download
                removeTmpItem = true;
            }
            if (ESMRForbidden) {
                downloadStatus = ESSMForbidden;
            } else {
                downloadStatus = ESSMMHError;
            }
            goto finally;
        }
        
        //
        // Insert the item into the local storage
        //
        if (err == ESMRSuccess) {
            struct stat st;
            StringBuffer localItemPath;
            
            luid = "";
            luid = addItem(serverItem, &sourceErr);
            itemInfo->setLuid(luid.c_str());
            itemInfo->setSize(serverItem->getMHSyncItemInfo()->getSize());  // may change after add to local gallery
            itemInfo->setServerLastUpdate(serverItem->getMHSyncItemInfo()->getServerLastUpdate());
            localItemPath = serverItem->getMHSyncItemInfo()->getLocalItemPath();
            itemInfo->setLocalItemPath(localItemPath.c_str());
            itemInfo->setLocalItemETag(itemInfo->getRemoteItemETag().c_str());
            
            statFile(localItemPath.c_str(), &st);
            
            itemInfo->setCreationDateSecs(getFileCreationDate(st));
            itemInfo->setModificationDateSecs(st.st_mtime);
            
            LOG.info("%s: local item path: %s", __FUNCTION__, serverItem->getMHSyncItemInfo()->getLocalItemPath().c_str());
        }
        
        if (luid.empty()) {
            switch (sourceErr) {
                case ESSSItemNotSupported:
                    downloadStatus = ESSMItemNotSupportedBySource;
                    itemInfo->setStatus(EStatusRemoteOnly);
                    break;
                    
                default:
                    downloadStatus = ESSMSetItemError;
                    break;
            }
            goto finally;
        }
        
        // If we get here, the operation is successfull
        itemInfo->setStatus(EStatusRemote);
        downloadStatus = ESSMSuccess;   // success
    }
    
finally:
    // update cache
    updateItemInCache(itemInfo, NULL);
    
    delete serverItem;
    
    if (removeTmpItem) {
        StringBuffer fileName = itemInfo->getName();
        
        unsigned int pos = fileName.rfind(".");
        
        if (pos != StringBuffer::npos) {
            StringBuffer extension = fileName.substr(pos);
            guid.append(extension);
            LOG.debug("[%s] the file to delete is %s", __FUNCTION__, guid.c_str());
        }
        
        // clear tmp item in case of error
        cleanTemporarySpoolItem(guid);
    }
    
    return downloadStatus;
}


/**
 * Called by MHSyncManager, before downloading a new item. 
 * @param  itemInfo the item's metadata info
 * @return a new allocated SapiSyncItem
 */
DownloadMHSyncItem* MHFileSyncSource::createDownloadItem(MHSyncItemInfo& itemInfo, OutputStreamObserver* observer) {
    
    FileOutputStream* ostream = createOutputStream(itemInfo);
    
    if (ostream == NULL) {
        LOG.error("[%s] - Error creating a new DownloadSapiSyncItem", __FUNCTION__);
        return NULL;
    }
    ostream->setObserver(observer);
    
    MHSyncItemInfo* info = (MHSyncItemInfo*)new MHSyncItemInfo(itemInfo);
    DownloadMHSyncItem* item = new DownloadMHSyncItem(info, ostream);
    
    return item;
}

UploadMHSyncItem* MHFileSyncSource::createUploadItem(MHSyncItemInfo* itemInfo) 
{
    UploadMHSyncItem* item = NULL;
    
    if (itemInfo == NULL) {
        LOG.debug("[%s] - invalid parameter", __FUNCTION__);
        return item;
    }
    
    InputStream* istream = createInputStream(*itemInfo);
    
    if (istream == NULL) {
        LOG.error("[%s] - no InputStream retrieved with the current luid %s", __FUNCTION__, itemInfo->getLuid().c_str());
        return item;
    }
    
    item = new UploadMHSyncItem(itemInfo, istream);
    
    //
    // FIX: real size of item to upload may be different from local item storage size (i.e. iOS gallery)
    //      Note: set ONLY in the UploadMHSyncItem, not in the local itemInfo, so the Server
    //      will receive the correct real size, but locally the cache is coherent with the local size.
    //
    int64_t realSize = istream->getTotalSize();
    int64_t localSize = item->getMHSyncItemInfo()->getSize();
    if (localSize != realSize) {
        LOG.debug("%s: real size of the %s '%s' to upload is %lld bytes (locally is %lld bytes)", 
                  __FUNCTION__, config.getName(), itemInfo->getName().c_str(), realSize, localSize);
        item->getMHSyncItemInfo()->setSize(realSize);
    }
    
    return item;
}

ESMRStatus MHFileSyncSource::retryDownload(MHMediaRequestManager* mhMediaRequestManager,
                                             DownloadMHSyncItem* serverItem) 
{
    ESMRStatus err = ESMRNetworkError;
    int maxRetries = clientConfig->getSapiMaxRetriesOnError();
    if (maxRetries == 0) {
        return err;
    }
   
    MHSyncItemInfo* itemInfo = serverItem->getMHSyncItemInfo();
    
    int attempt = 0;
    while (attempt < maxRetries) 
    {
        LOG.info("Retry download of item '%s' (%d of %d)...", itemInfo->getName().c_str(), attempt+1, maxRetries);
        //fireSyncSourceEvent(sourceURI.c_str(), sourceName.c_str(), syncMode, attempt+1, SYNC_SOURCE_RETRY);
        
        long sleepMsec = clientConfig->getSapiSleepTimeOnRetry();
        if (sleepMsec) {
            LOG.debug("sleep %li msec", sleepMsec);
            sleepMilliSeconds(sleepMsec);
        }
        
        int oldPos = serverItem->getStream()->size();
        
        err = mhMediaRequestManager->downloadItem(serverItem);
        
        if (err == ESMRNetworkError) {
            // Network error: check the amount of data transferred
            int newPos = serverItem->getStream()->size();
            LOG.debug("Retry download failed: data transferred = %d bytes", newPos - oldPos);
            
            long minDataSize = clientConfig->getSapiMinDataSizeOnRetry();
            if ((minDataSize == 0) || (newPos - oldPos) <= minDataSize) {
                // transferred data is small (or none)
                attempt ++;
            } else {
                // transferred data is big: reset the retry mechanism
                attempt = 0;
            }
            
            continue;
        } else {
            // all other errors: out
            break;
        }
    }
    
    return err;
}

/**
 * Called by MHSyncManager after downloading an item to add (after call to createItem)
 * Stores the passed DownloadMHSyncItem to the local storage.
 * Updates the cache file.
 * @param syncItem the item to add
 * @return the LUID of the new item added. Empty string if there was an error
 */
StringBuffer MHFileSyncSource::addItem(DownloadMHSyncItem* syncItem,
                                         EMHSyncSourceError* errCode) 
{
    if (syncItem == NULL) {
        LOG.error("[%s] - The syncItem is NULL", __FUNCTION__);
        *errCode = ESSSInvalidItem;
        return "";
    }
    
    long modTime = 0;
    StringBuffer itemLuid = insertItem(syncItem, errCode, &modTime);
    if (itemLuid == "") {
        LOG.error("[%s] - error in adding item. The luid of the item to insert is empty", __FUNCTION__);
        
        // source status code has been updated by insertItem
        if (*errCode == ESSSItemNotSupported) {
            blackListItem(syncItem->getMHSyncItemInfo());
        }
        return "";
    }
    
    return itemLuid;
}


bool MHFileSyncSource::getLocalUpdates(std::vector<MHSyncItemInfo*>& newLocalItems, 
                                          std::vector<MHSyncItemInfo*>& updatedLocalItems)
{
    LOG.info("Checking local modifications on source %s...", getConfig().getName());

    std::vector<std::string> fileList;
    std::map<std::string, bool> readFoldersMap;
    std::vector<std::string>::const_iterator it = itemsUploadPaths.begin(),
                                                   end = itemsUploadPaths.end();
    struct stat st; 
       
    for (; it != end; it++) {
        std::string localItemsPath = *it;
        
        if (localItemsPath.empty()) {
            continue;
        }
        
        if (localItemsPath.find_first_of("|") == 0) {
            localItemsPath = localItemsPath.substr(1);
            LOG.debug("%s: folder cannot be unselected '%s'", __FUNCTION__, localItemsPath.c_str());
        }
        
        if (localItemsPath[0] == '!') {
            LOG.debug("%s: skipping folder '%s' unselected by user", __FUNCTION__, localItemsPath.c_str());
            
            continue;
        }
        
        memset(&st, 0, sizeof(struct stat));
        
        LOG.debug("%s: checking local items in directory '%s'...", __FUNCTION__, localItemsPath.c_str());
        if (statFile(localItemsPath.c_str(), &st) != 0) {

            if (errno == ENOENT) {
				// file not found
				if (removableDriveForUploadNotAvailable(localItemsPath.c_str())) {
					LOG.info("Skipping upload folder '%s': removable drive not available", localItemsPath.c_str());
					continue;
				}
				else if (networkDriveForUploadNotAvailable(localItemsPath.c_str())) {
					LOG.info("Skipping upload folder '%s': network drive not available", localItemsPath.c_str());
					continue;
				}
                else {
					LOG.error("%s: local upload folder '%s' not found (code %d: %s)", 
						__FUNCTION__, localItemsPath.c_str(), errno, strerror(errno));
					continue;
				}
            } else {
                LOG.error("%s: can't stat local items spool directory '%s': %s", 
					__FUNCTION__, localItemsPath.c_str(), strerror(errno));
                continue;      
            }
        }
      
        if (clientConfig->isToAbort()) {
            LOG.debug("%s: interrupting item scanning on user cancellation request", __FUNCTION__);
            return false;
        }
    
        std::map<std::string, bool>::iterator it = readFoldersMap.find(localItemsPath);
        if (it != readFoldersMap.end()) {
            LOG.debug("%s: folder '%s' already scanned - skipping...", __FUNCTION__, localItemsPath.c_str());
            
            continue;
        }
        
        if (readDir(localItemsPath.c_str(), fileList, true, readFoldersMap)) {
            LOG.error("%s: error reading local items spool directory '%s': %s", __FUNCTION__,
                localItemsPath.c_str(), strerror(errno));
                
            return false;
        }
    }
    
    int filesCount = fileList.size();
    
    if (filesCount == 0) {
        LOG.info("Checks for local modifications completed on source %s: no items found", getConfig().getName());
        return true;
    }
    
    it = fileList.begin(),
    end = fileList.end();
    
    LOG.debug("%s: checking local file list  [count %d]...", __FUNCTION__, filesCount);
    
    for (; it != end; it++) { 
        
        // check user aborted
        if (clientConfig->isToAbort()) {
            LOG.debug("%s: interrupting item scanning on user cancellation request", __FUNCTION__);
            return false;
        }

        std::string filePath = *it;
        if (filePath.empty()) {
            continue;
        }

        memset(&st, 0, sizeof(struct stat));
        
        if (statFile(filePath.c_str(), &st) < 0) {
            LOG.error("[%s] can't stat file '%s' (%s): skipping ", __FUNCTION__, 
                filePath.c_str(), strerror(errno));
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) {
            LOG.info("[%s] file '%s' is a directory: skipping", __FUNCTION__, filePath.c_str());
            continue;   // skip subfolders: TODO fix here for recursive reading
        }

        std::string fileName = baseName(filePath);
        if (fileName.empty()) {
            LOG.error("%s: can't get basename for file '%s'", __FUNCTION__, filePath.c_str());  
            continue;
        }
       
        if (st.st_size == 0) {
            LOG.info("%s: discarding file '%s' of size 0", __FUNCTION__, fileName.c_str());
            continue;
        }
        
		MHSyncItemInfo* info = new MHSyncItemInfo();
         
        info->setSize(st.st_size);       
        bool ignore = filterBySize(*info);
       
        if (ignore) {
            // ignore file exceeding the max allowed size
            LOG.debug("%s: ignoring file '%s' exceeding max allowed size (actual size %lld)", __FUNCTION__, filePath.c_str(), st.st_size);
            delete info;
			continue;
        }
        
        info->setName(fileName.c_str());
        info->setLocalItemPath(filePath.c_str());
     
        if ((ignore = filterByName(fileName.c_str())) == true) {
            LOG.debug("%s: ignoring file '%s': file name excluded by source", __FUNCTION__, filePath.c_str());
			delete info;
            continue;
        }

        if ((ignore = filterByExtension(*info)) == true) {
            LOG.debug("%s: ignoring file '%s': extension not supported by source", __FUNCTION__, filePath.c_str());
			delete info;
            continue;
        }
      
        unsigned long modificationDate = st.st_mtime;
        info->setModificationDateSecs(modificationDate);

        unsigned long creationDate = (unsigned long)getFileCreationDate(st);
        info->setCreationDateSecs(creationDate);
        
        // set the mime/type based on the extension        
        std::string mime("");
        std::string::size_type pos = fileName.rfind(".");
        if (pos != std::string::npos) {
            std::string extension = fileName.substr(pos);
            mime = MHContentType::getContentTypeByExtension(extension.c_str());
        } else {
            mime = MHContentType::getContentTypeByExtension("");
        }
        
        info->setContentType(mime.c_str());
        info->setStatus(EStatusLocal);
       
        MHSyncItemInfo* item = getItemFromCache(MHItemsStore::local_item_path_field_name, filePath.c_str());
        
        if (item) { // check for updated items
            unsigned long lastItemUpdate = item->getModificationDateSecs();

            if (modificationDate > lastItemUpdate) { // item was updated
                LOG.debug("%s: found updated item %d (local path: '%s')", __FUNCTION__, item->getId(), filePath.c_str());
                
                info->setId(item->getId());
                info->setLuid(item->getLuid());
                info->setGuid(item->getGuid());
                info->setRemoteItemUrl(item->getRemoteItemUrl());
                info->setServerLastUpdate(item->getServerLastUpdate());
                info->setModificationDateSecs(modificationDate);
               
                updatedLocalItems.push_back(info);
            } 
			else {
				// existing item, not modified
				if (item->getStatus() == EStatusLocallyRemoved) {
					//
					// Locally removed item has been added again: restore it (fix bug 13532)
					//
					if (item->getGuid().empty()) {
						// local item
						LOG.info("Restoring locally removed item '%s': new item to uplaod", info->getName().c_str());
						newLocalItems.push_back(info);
					}
					else {
						// remote item (or uploading)
						if (info->getSize() == item->getSize()) {
							LOG.info("Restoring locally removed item '%s' with GUID and same size: let's try resume upload", info->getName().c_str());
							info->setStatus(EStatusUploading);
						}
						else {
							LOG.info("Restoring locally removed item '%s' with GUID and different size: update item from scratch", info->getName().c_str());
							info->setStatus(EStatusLocal);
						}

						info->setId(item->getId());
						info->setLuid(item->getLuid());
						info->setGuid(item->getGuid());
						info->setRemoteItemUrl(item->getRemoteItemUrl());
						info->setServerLastUpdate(item->getServerLastUpdate());
						info->setModificationDateSecs(modificationDate);

						updatedLocalItems.push_back(info);
					}
				}
				else {
					// existing item, not modified, status ok in DB: nothing to do
					delete info;
				}
			}

        } else {
			// item not found in cache
            LOG.debug("%s: found new local item '%s' (path: '%s')", __FUNCTION__, info->getName().c_str(), filePath.c_str());
            newLocalItems.push_back(info);
        }
    }
    
    LOG.info("Checks for local modifications on source %s completed: %d new, %d updated items found.", 
        getConfig().getName(), newLocalItems.size(), updatedLocalItems.size());
    return true;
}


bool MHFileSyncSource::getMissingLocalItems(std::vector<MHSyncItemInfo*>& missingLocalItems)
{
    CacheItemsList cacheItemsList;
    struct stat st;
    int size = 0; 

    LOG.info("Checking for missing local items on source %s...", getConfig().getName());

    mhItemsStore->getAllEntries(cacheItemsList);
    std::vector<MHStoreEntry*> entries = cacheItemsList.getItems();
    
    size = entries.size();
    LOG.debug("%s: %d items in cache", __FUNCTION__, size);

    if (size == 0) {
        LOG.info("Checks for missing local items completed on source %s: no items in cache.", getConfig().getName());
        return true;
    }

    for (int i = 0; i < size; i++) {
        MHSyncItemInfo* itemInfo = (MHSyncItemInfo*)entries[i];
        const char *itemGuid = NULL, *itemLuid = NULL;
        int itemId = 0;

        if (clientConfig->isToAbort()) {
            LOG.debug("%s: interrupting operation on user cancel request", __FUNCTION__); 
            break;
        }
     
        if (!itemInfo) continue;
		if (itemInfo->getLuid().empty()) continue;	// empty LUID means remote-only item

        itemGuid = itemInfo->getGuid().c_str();
        itemLuid = itemInfo->getLuid().c_str();
        itemId = itemInfo->getId();
         
        LOG.debug("%s: #%03d: ID=%d, status=%d, %s", __FUNCTION__, i, 
                itemId, itemInfo->getStatus(), itemInfo->getName().c_str());

        const char* localItemPath = itemInfo->getLocalItemPath();

        if ((localItemPath == NULL) || (strlen(localItemPath) == 0)) {
            // The item is not locally available. This may due to these reasons:
            // 1) the item was not downloaded because of local quota error
            // 2) the item was not downloaded because of a network error
            // 3) the items download was disabled
            LOG.info("%s: can't find item %d local contents", __FUNCTION__, itemId);
            continue;  
        }

        memset(&st, 0, sizeof(struct stat));

        if (statFile(localItemPath, &st) != 0) {
            if (errno == ENOENT) { // missing item 

				// Skip if it's a removable drive and the drive is not available.
				// Otherwise the item is marked as deleted, and we could not recover it when drive is reconnected again.
				if (removableDriveForUploadNotAvailable(localItemPath)) {
					LOG.info("Skip missing file: removable drive is NOT available: %s", localItemPath);
					continue;
				}

				// TODO: check for network drives?

                LOG.debug("%s: adding item %s to missing items list", __FUNCTION__, localItemPath);
                MHSyncItemInfo* item = itemInfo->clone();
                missingLocalItems.push_back(item);
            } else {
                LOG.error("%s: can't stat local item %d at path '%s': %s", __FUNCTION__, itemId, strerror(errno));
                continue;
            }
        }                
    }

    LOG.info("Checks for missing local items completed on source %s: %d items found.", getConfig().getName(), missingLocalItems.size());
    return true;
}

bool MHFileSyncSource::getLocalItems()
{
    std::vector<MHSyncItemInfo*> newLocalItems; 
    std::vector<MHSyncItemInfo*> updatedLocalItems;
    std::vector<MHSyncItemInfo*> missingLocalItems;

	bool foundLocalChanges = false;
    bool cacheUpdated = false;
    const char* localItemPath = NULL;
    
    if (getLocalUpdates(newLocalItems, updatedLocalItems) == false) {
        LOG.error("%s: error getting local updates", __FUNCTION__);
        return false;
    }

    if (getMissingLocalItems(missingLocalItems) == false) {
        LOG.error("%s: error getting missing local items", __FUNCTION__);
        return false;
    }

	//
	// check local renames (new & del matching -> mod)
	//
    std::vector<MHSyncItemInfo *>::iterator it = newLocalItems.begin(), end = newLocalItems.end();

    while (it != end) {
        MHSyncItemInfo* newItemInfo = (MHSyncItemInfo *)*it;
        std::vector<MHSyncItemInfo *>::iterator missingItemsIt = missingLocalItems.begin(),
                                                missingItemsEnd = missingLocalItems.end();
        bool listUpdated = false;
        
        for (; missingItemsIt != missingItemsEnd; missingItemsIt++) {
            MHSyncItemInfo* missingItemInfo = (MHSyncItemInfo *)*missingItemsIt;
            
            LOG.debug("%s: checking for renamed item - size: %ld %ld - modification date: %ld %ld - creation date: %ld %ld",
                __FUNCTION__,
                newItemInfo->getSize(), missingItemInfo->getSize(),
                newItemInfo->getModificationDateSecs(), 
                missingItemInfo->getModificationDateSecs(),
                newItemInfo->getCreationDateSecs(),
                missingItemInfo->getCreationDateSecs());
                
            if ((newItemInfo->getSize() == missingItemInfo->getSize()) &&
                //(newItemInfo->getModificationDateSecs() == missingItemInfo->getModificationDateSecs()) &&
                (newItemInfo->getCreationDateSecs() == missingItemInfo->getCreationDateSecs())) {
                StringBuffer oldFileName = missingItemInfo->getName();
                StringBuffer newFileName =  newItemInfo->getName();
                
                LOG.info("Found renamed item %s (renamed from %s to %s)",
                   newItemInfo->getName().c_str(), missingItemInfo->getLocalItemPath().c_str(), 
                    newItemInfo->getLocalItemPath().c_str());
                
                // check if renaming was only a change of path
                missingItemInfo->setLocalItemPath(newItemInfo->getLocalItemPath());
                if (oldFileName != newFileName) {
                    LOG.debug("%s: setting local metadata changes status for renamed item", __FUNCTION__);
                    missingItemInfo->setName(newFileName);
                    missingItemInfo->setStatus(EStatusLocalMetaDataChanged);
                }
                
                updatedLocalItems.push_back(missingItemInfo);
                missingLocalItems.erase(missingItemsIt);
                
                it = newLocalItems.erase(it);
                end = newLocalItems.end();
                listUpdated = true;
                break;
            } 
        }
        
        if (!listUpdated) {
            ++it;
        }
    }

	//
	// add NEW local items to DB
	//
    it = newLocalItems.begin();
    end = newLocalItems.end();
    
    for (; it != end; it++) {
        MHSyncItemInfo* newItemInfo = (MHSyncItemInfo *)*it;
        localItemPath = newItemInfo->getLocalItemPath().c_str();
        
        LOG.debug("%s: adding new local item to DB, at path %s", __FUNCTION__, localItemPath);
    
        cacheUpdated |= addItemToCache(newItemInfo, NULL);
           
        if (cacheUpdated) {
            StringBuffer luid;
            luid.sprintf("%ld", newItemInfo->getId());

            newItemInfo->setLuid(luid);
            newItemInfo->setStatus(EStatusLocal);
            
            LOG.debug("%s: updating item in DB '%s': setting luid: %s",
                __FUNCTION__, localItemPath, luid.c_str());
            
            if (updateItemInCache(newItemInfo, NULL) == false) {
                LOG.error("%s: failed to update item in DB: '%s'", __FUNCTION__, localItemPath);
                continue;
            }
			foundLocalChanges = true;
        }
    }
    
	//
	// set MOD local items to DB
	//
    it = updatedLocalItems.begin();
    end = updatedLocalItems.end();

    for (; it != end; it++) {
        MHSyncItemInfo* modifiedItemInfo = (MHSyncItemInfo *)*it;
        localItemPath = modifiedItemInfo->getLocalItemPath().c_str(); 

        LOG.debug("%s: updating locally modified item in DB, at path: %s", __FUNCTION__, localItemPath);
    
        if (updateItemInCache(modifiedItemInfo, NULL) == false) {
            LOG.error("%s: failed to update item in DB: '%s'", __FUNCTION__, localItemPath);
            continue;
        }
		foundLocalChanges = true;
    }
    
	//
	// set DEL local items to DB
	//
    it = missingLocalItems.begin();
    end = missingLocalItems.end();

    for (; it != end; it++) {
        MHSyncItemInfo* deletedItemInfo = (MHSyncItemInfo *)*it;
		if (!deletedItemInfo) continue;

		// Optimization: update in cache only if status is not correct
		if (deletedItemInfo->getStatus() != EStatusLocallyRemoved) {

			localItemPath = deletedItemInfo->getLocalItemPath().c_str(); 
			LOG.debug("%s: updating locally removed item in DB, at path: %s", __FUNCTION__, localItemPath);
            
			deletedItemInfo->setStatus(EStatusLocallyRemoved);
    
			if (updateItemInCache(deletedItemInfo, NULL) == false) {
				LOG.error("%s: failed to update item in DB: '%s'", __FUNCTION__, localItemPath);
				continue;
			}
			cacheUpdated = true;
			foundLocalChanges = true;
		}
    }
    
	// cleanup
    unsigned itemsCount = newLocalItems.size(); 
    for (unsigned int i=0; i < itemsCount; ++i) {
        MHSyncItemInfo* entry = newLocalItems[i];
        delete entry;
    }
    newLocalItems.clear();
    
    itemsCount = updatedLocalItems.size();
    for (unsigned int i=0; i < itemsCount; ++i) {
        MHSyncItemInfo* entry = updatedLocalItems[i];
        delete entry;
    }
    updatedLocalItems.clear();
    
    itemsCount = missingLocalItems.size();
    for (unsigned int i=0; i < itemsCount; ++i) {
        MHSyncItemInfo* entry = missingLocalItems[i];
        delete entry;
    }
    missingLocalItems.clear();
    
	//  true, if detected at least 1 local change
    return foundLocalChanges;
}


int MHFileSyncSource::beginSync(bool /*incremental*/)
{
    bool localChanges = getLocalItems();

    if (!localChanges && notifyIfNoLocalChanges) {
        // it's not an error, but this will stop the sync immediately
		LOG.info("No local changes detected for source %s -> force to exit sync with code %d", 
			getConfig().getName(), ESSMNoLocalChangesDetected);
        getConfig().setLastSourceError(0);
        return ESSMNoLocalChangesDetected;
    }

    return ESSMSuccess;
}

void MHFileSyncSource::uploadPhaseStarted()
{
    SyncSourceConfig& ssconfig = getConfig();
    const char* sourceName = ssconfig.getName();
    const char* sourceURI  = ssconfig.getURI();
    
    MHSyncSource::uploadPhaseStarted();
    fireSyncSourceEvent(sourceURI, sourceName, SYNC_NONE, 0, SYNC_SOURCE_UPLOAD_PHASE_STARTED);
}

int MHFileSyncSource::uploadPhaseCompleted(int res)
{
    SyncSourceConfig& ssconfig = getConfig();
    const char* sourceName = ssconfig.getName();
    const char* sourceURI  = ssconfig.getURI();
    
    int ret = MHSyncSource::uploadPhaseCompleted(res);
    
    fireSyncSourceEvent(sourceURI, sourceName, SYNC_NONE, 0, SYNC_SOURCE_UPLOAD_PHASE_ENDED);
    return ret;
}

int MHFileSyncSource::downloadPhaseCompleted(int res) {
    
    int ret = 0;
    int quotaErrorCode = 0;
    ret = MHSyncSource::downloadPhaseCompleted(res);

    if (res != ESMRSuccess) {
        return res;
    }

    // Scan all the items in the cache and trigger the required uploads
    CacheItemsList cacheItemsList;
    mhItemsStore->getAllEntries(cacheItemsList);
    std::vector<MHStoreEntry*> entries = cacheItemsList.getItems();
    
    // check and launch pending tasks
    int size = entries.size();
    LOG.info("Checking for pending downloads for source %s (%d items in cache)", getConfig().getName(), size);
    
    // just for statistics
    int numRemote=0;
    int numRemoteOnly=0;
    int numSynced=0;

    SyncSourceConfig& ssconfig = getConfig();
    const char* sourceName = ssconfig.getName();
    const char* sourceURI  = ssconfig.getURI();
    
    fireSyncSourceEvent(sourceURI, sourceName, SYNC_NONE, 0, SYNC_SOURCE_DOWNLOAD_PHASE_STARTED);

    CacheItemsList localItems;

    LOG.debug("----- CACHE content for source %s -----", getConfig().getName());
    for (int i=0; i < size; i++) {
        MHSyncItemInfo* itemInfo = (MHSyncItemInfo*)entries[i];
        const char *itemGuid = NULL, *itemLuid = NULL;
        int itemId = 0;
        OutputStreamObserver* observer = NULL;
         
        if (itemInfo == NULL) {
            continue;
        }

        if (clientConfig->isToAbort()) {
            LOG.debug("%s: interrupting operation on user cancel request", __FUNCTION__);
            break;
        }
    
        itemGuid = itemInfo->getGuid().c_str();
        itemLuid = itemInfo->getLuid().c_str();
        itemId = itemInfo->getId();
         
        LOG.debug("  > #%03d: ID=%d, status=%d, %s", i, itemId, itemInfo->getStatus(), itemInfo->getName().c_str());
        
        if (!itemInfo->isLocallyAvailable()) {
            LOG.debug("%s: item is not locally available download it",__FUNCTION__);
            if (checkIfFitsOnLocalStorage(itemInfo) == ESSMSuccess) {
                observer = createDownloadProgressObserver(itemInfo, getConfig().getName());
                ret = downloadItem(itemInfo, observer);
                numRemote++;
            } else {
                if (quotaErrorCode == 0) {
                    quotaErrorCode = ESSMClientQuotaExceeded;
                }
            }
        } else if (itemInfo->isSynced()) {
            StringBuffer localItemPath = itemInfo->getLocalItemPath();
            numSynced++;
           
            LOG.debug("%s: local item path: '%s'", __FUNCTION__, localItemPath.c_str());
            LOG.debug("%s: RemoteItemETag: %s - LocalItemETag: %s", __FUNCTION__,
                itemInfo->getRemoteItemETag().c_str(), itemInfo->getLocalItemETag().c_str());
          
            if (localItemPath.empty() || itemInfo->getRemoteItemETag().compare(itemInfo->getLocalItemETag()) != 0) {
                LOG.info("%s: item '%s' is stored in cache as synched but it needs to be re-downloaded",
                        __FUNCTION__, itemInfo->getName().c_str());
                        
                if (checkIfFitsOnLocalStorage(itemInfo) == ESSMSuccess) {
                    observer = createDownloadProgressObserver(itemInfo, getConfig().getName());
                    ret = downloadItem(itemInfo, observer);
                } else {
                    if (quotaErrorCode == 0) {
                        quotaErrorCode = ESSMClientQuotaExceeded;
                    }
                }
            } else {
                const char* localItemName = NULL, *updatedItemName = NULL;
           
                std::string localItemNameStr = baseName(localItemPath.c_str());
                localItemName = localItemNameStr.c_str();
                updatedItemName = itemInfo->getName();
        
                LOG.info("%s: checking for renames (item in cache: %s - from fs: %s)",
                    __FUNCTION__, updatedItemName, localItemName);
                // update also renamed file
                if (strcmp(localItemName, updatedItemName)) {
                    StringBuffer destFile;
                    std::string dirPath;
                    
                    if (dirName(dirPath, localItemPath.c_str()) == false) {
                        LOG.error("%s: error getting dir name from item local path", __FUNCTION__);
                        delete observer;

                        continue;
                    }
                    
                    destFile.sprintf("%s/%s", dirPath.c_str(), itemInfo->getName().c_str());

                    if (!fileExists(destFile.c_str())) {
                        LOG.info("Renaming %s item %s to %s (names don't match, we keep remote name)", 
                            getConfig().getName(), localItemPath.c_str(), destFile.c_str());

                        if (renameFile(localItemPath, destFile.c_str()) != 0) {
                            LOG.error("%s: error renaming local item '%s': %s", __FUNCTION__,
                                localItemPath.c_str(), strerror(errno));
                        } else {
                            itemInfo->setLocalItemPath(destFile.c_str());
                            updateItemInCache(itemInfo, NULL);
                        }
                    }
                    else {
                        LOG.debug("%s: Not renaming %s item %s to %s (local file already exists)", __FUNCTION__,
                            getConfig().getName(), localItemPath.c_str(), destFile.c_str());
                    }
                }
            }
        }

        delete observer;

        // check if for some errors it is to exit.
        if (ret == ESSMNetworkError) {
            LOG.info("%s Network error: stop to download items...", __FUNCTION__);
            break;
        }
    }
    
    fireSyncSourceEvent(sourceURI, sourceName, SYNC_NONE, 0, SYNC_SOURCE_DOWNLOAD_PHASE_ENDED);
    // just for statistics
    LOG.info("\n========== download phase for %ss completed ==========\nDIGITAL LIFE: %d items (%d remote,\n"
             "%d remote only, %d synced)\n===================================================",
             getConfig().getName(), size, numRemote, numRemoteOnly, numSynced);
    
    if (ret == 0) { // consider more important the ret error instead the quota
        ret = quotaErrorCode;
    }

    return ret;
}

DownloadProgressObserver* MHFileSyncSource::createDownloadProgressObserver(MHSyncItemInfo *itemInfo, const char* sourceName)
{
    return NULL;
}

int MHFileSyncSource::downloadItem(MHSyncItemInfo *itemInfo, OutputStreamObserver* observer)
{
    int downloadStatus = 0;
    const char *serverUrl = NULL, *itemRemoteUrl = NULL;
    StringBuffer itemDownloadPath;
    StringBuffer itemRequestUrl;
    int res = 0;
    
    if (!itemInfo) {
        LOG.error("%s: invalid param: NULL itemInfo", __FUNCTION__);
        return 1;
    }

    StringBuffer itemName = itemInfo->getName();
    if (itemName.empty()) {
        LOG.error("%s: invalid param: empty item name (item ID=%d)", __FUNCTION__, itemInfo->getId());
        return 1;
    }

    MHSyncManager syncMan(*this, *clientConfig);
    CacheItemsList items;
    CacheLabelsMap labels;

    serverUrl = itemInfo->getServerUrl();
    itemRemoteUrl = itemInfo->getRemoteItemUrl();

    if (serverUrl == NULL || strlen(serverUrl) == 0 || itemRemoteUrl == NULL || strlen(itemRemoteUrl) == 0) {
        LOG.info("%s: invalid param: empty server url or itemRemoteUrl (item ID=%d). Download metadata...", __FUNCTION__, itemInfo->getId());
        int err = 0;
        
        syncMan.resetErrors();
        err = syncMan.getServerItemFromId(items, labels, itemInfo->getGuid());
        
        if (err == 0) {
            std::vector<MHStoreEntry*> entries = items.getItems();
            
            if (entries.size() > 0) {
                MHSyncItemInfo* metadata = (MHSyncItemInfo*)entries[0];
                
                if (metadata) {
                    itemInfo->setServerUrl(metadata->getServerUrl().c_str());
                    itemInfo->setRemoteItemUrl(metadata->getRemoteItemUrl().c_str());

                    serverUrl = itemInfo->getServerUrl();
                    itemRemoteUrl = itemInfo->getRemoteItemUrl();
                }
            }
        } else {
            switch (err) {
                case ESMROperationCanceled:
                    LOG.debug("%s: operation cancelled on user request", __FUNCTION__);
                    res = 1;

                    break;
                case ESMRRequestTimeout:
                case ESMRNetworkError:
                case ESMRConnectionSetupError:
                    LOG.info("%s: network error",__FUNCTION__);
                    res = 1;
                    break;

                default:
                    LOG.info("%s: operation failed with error code: %d",__FUNCTION__, err);
                    res = 1;
                    break;
            }
        }
    }

    // format url request for item
    if (serverUrl == NULL || strlen(serverUrl) == 0 || itemRemoteUrl == NULL || strlen(itemRemoteUrl) == 0) {
        LOG.info("%s: invalid param: empty server url or itemRemoteUrl after downlaod: exit task", __FUNCTION__);
        return 1;
    }

    LOG.info("Starting download of item: %s (ID=%lu)",itemName.c_str(), itemInfo->getId());

    if ((downloadStatus = downloadRequest(itemInfo, observer)) != 0) {
        if (downloadStatus == ESSMForbidden) {
            LOG.info("%s: Forbidden download:  %s urls are expired, try to update them first",
                     __FUNCTION__, itemInfo->getName().c_str());
            CacheItemsList items;
            int err = 0;
        
            syncMan.resetErrors();
            err = syncMan.getServerItemFromId(items, labels, itemInfo->getGuid());
        
            if (err == 0) {
                std::vector<MHStoreEntry*> entries = items.getItems();
            
                if (entries.size() > 0) {
                    MHSyncItemInfo* metadata = (MHSyncItemInfo*)entries[0];
                
                    if (metadata) {
                        itemInfo->setRemoteItemUrl(metadata->getRemoteItemUrl().c_str());
                    
                        downloadStatus = downloadRequest(itemInfo, observer);
                    }
                }
            } else {
                switch (err) {
                    case ESMROperationCanceled:
                        LOG.debug("%s: operation cancelled on user request", __FUNCTION__);
                        res = 1;

                        break;
                    case ESMRRequestTimeout:
                    case ESMRNetworkError:
                    case ESMRConnectionSetupError:
                        LOG.info("%s: network error",__FUNCTION__);
                        res = 1;
                        break;

                    default:
                        LOG.info("%s: operation failed with error code: %d",__FUNCTION__, err);
                        res = 1;
                        break;
                }
            }
        }
       
        if (downloadStatus != 0) {
            LOG.error("%s: request for download full item failed with code %d", __FUNCTION__, downloadStatus);
        }
    }
    
    return downloadStatus;
}

bool MHFileSyncSource::filterByName(const StringBuffer& name) {

    bool ret = false;
    int excluded_file_names_size = 0;
    
    if (name.empty()) {
        return ret;
    }
    
    if ((excluded_file_names_size = excluded_file_names.size()) == 0) {        
        return ret; // wee keep it
    }
    
    for (int i = 0; i < excluded_file_names_size; i++) {
        StringBuffer* excluded_name_pattern = (StringBuffer*)excluded_file_names.get(i);
        
        if ((excluded_name_pattern) && (excluded_name_pattern->empty() == false)) {
            if (excluded_name_pattern->endsWith('*')) {
                // this is a very limited 'globbing' support: get the pattern from
                // the start of the string till the '*' character and see if file 
                // name starts with the same pattern
                size_t glob_index = excluded_name_pattern->find("*");
                StringBuffer start_pattern = excluded_name_pattern->substr(0, glob_index);
                size_t pattern_size = start_pattern.length();
                
                if (pattern_size) {
                    if (strncmp(name.c_str(), start_pattern.c_str(), pattern_size) == 0) {
                        ret = true;
                        break;
                    }
                }
            } else {
                if (name == *excluded_name_pattern) {
                    ret = true;
                    break;
                }
                
                if(strncmp(name.substr(0,2), excluded_name_pattern->c_str(), 2) == 0){
                    ret = true;
                    break;
                }
            }
        }
    }

    if (ret) {
        LOG.debug("[%s] item %s filtered (file name discarded by source)", __FUNCTION__, name.c_str());
    }
    
    return ret;
}

void MHFileSyncSource::requestSourceOperation(unsigned componentType, MHSyncItemInfo* itemInfo)
{
}

int MHFileSyncSource::readNewItems(AbstractSyncConfig& config, const char* listType) {
    int numNewItems;
    return readNewItems(config, listType, numNewItems);
}

int MHFileSyncSource::readNewItems(AbstractSyncConfig& config, const char* listType,
                                          int &numNewItemsInDL)
{
    return 0;
}

bool MHFileSyncSource::localItemsFullScan(CacheItemsList& itemsInfoList) 
{
    std::vector<std::string> fileList;
    std::vector<std::string>::const_iterator it = itemsUploadPaths.begin(),
                                                   end = itemsUploadPaths.end();
    struct stat st;
    std::map<std::string, bool> readFoldersMap;
    
    LOG.debug("%s: reading local items for source %s...", __FUNCTION__, getConfig().getName());
       
    for (; it != end; it++) {
        std::string localItemsPath = *it;
        const char* localItemsPathStr = NULL;
        
        if (localItemsPath.empty()) {
            continue;
        }
        
        if (localItemsPath.find_first_of("|") == 0) {
            localItemsPath = localItemsPath.substr(1);
            LOG.debug("%s: folder not removible '%s'", __FUNCTION__, localItemsPath.c_str());
        }
        
        localItemsPathStr = localItemsPath.c_str();
        
        if (localItemsPathStr[0] == '!') {
            LOG.debug("%s: scan of folder '%s' contents unselected by user", __FUNCTION__, localItemsPath.c_str());
            localItemsPathStr++;
            
            continue;
        }
 
        memset(&st, 0, sizeof(struct stat));

        if (clientConfig->isToAbort()) {
            return false;
        }

        
        LOG.debug("%s: reading local items in directory '%s'...", __FUNCTION__, localItemsPathStr);
        if (statFile(localItemsPathStr, &st) != 0) {
            
			if (errno == ENOENT) {
				// file not found
				if (removableDriveForUploadNotAvailable(localItemsPath.c_str())) {
					LOG.info("%s: skipping upload folder '%s': removable drive not available", __FUNCTION__, localItemsPath.c_str());
					continue;
				}
				else if (networkDriveForUploadNotAvailable(localItemsPath.c_str())) {
					LOG.info("%s: skipping upload folder '%s': network drive not available", __FUNCTION__, localItemsPath.c_str());
					continue;
				}
                else {
					LOG.error("%s: local upload folder '%s' not found", __FUNCTION__, localItemsPath.c_str(), strerror(errno));
					continue;
				}
            } else {
                LOG.error("%s: can't stat local items spool directory '%s': %s", __FUNCTION__, localItemsPathStr, strerror(errno));
                continue;       
            }
        }
    
        std::map<std::string, bool>::const_iterator it = readFoldersMap.find(localItemsPath);
        if (it != readFoldersMap.end()) {
            LOG.debug("%s: folder '%s' already scanned - skipping...", __FUNCTION__, localItemsPath.c_str());
            
            continue;
        }
        
        if (readDir(localItemsPathStr, fileList, true, readFoldersMap)) {
            LOG.error("%s: error reading local items spool directory '%s': %s", __FUNCTION__,
                localItemsPathStr, strerror(errno));
                
            continue;
        }
    }
    
    int filesCount = fileList.size();
    it = fileList.begin(),
    end = fileList.end();

    LOG.debug("%s: creating local file list [count %d]...", __FUNCTION__, filesCount);
    
    for (; it != end; it++) { 
        // check user aborted
        if (clientConfig->isToAbort()) {
            return false;
        }

        std::string filePath = *it;
        if (filePath.empty()) {
            continue;
        }

        std::string fileName = baseName(filePath);
        if (fileName == "." || fileName == "..") {
            continue;
        }
        
        MHSyncItemInfo* info = new MHSyncItemInfo();

        info->setLocalItemPath(filePath.c_str());
       
        struct stat st;
        memset(&st, 0, sizeof(struct stat));

         if (statFile(filePath.c_str(), &st) < 0) {
            LOG.error("[%s] can't stat file '%s' [%d]", __FUNCTION__, filePath.c_str(), errno);
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) {
            LOG.info("[%s] file '%s' is a directory: skipping", __FUNCTION__, filePath.c_str());
            continue;   // skip subfolders here (subfolders are already scanned in readDir if recursive = true)
        }

        MHSyncItemInfo* item = getItemFromCache(MHItemsStore::local_item_path_field_name, filePath.c_str());
        
        if (item) {
            unsigned long itemId = item->getId();
            const char* luid = item->getLuid();
            const char* guid = item->getGuid();
            
            info->setId(itemId);
            
            if (guid) {
                info->setGuid(guid);
            }
            
            if (luid) {
                info->setLuid(luid);
            }
        } 
        
        info->setName(fileName.c_str());
        
        // set size (it's mandatory)
        info->setSize(st.st_size);

        unsigned long tstamp = st.st_mtime;
        info->setModificationDateSecs(tstamp);

        tstamp = (unsigned long)getFileCreationDate(st);
        info->setCreationDateSecs(tstamp);
    
        // set the mime/type based on the extension        
        std::string mime("");
        std::string::size_type pos = fileName.rfind(".");
        if (pos != std::string::npos) {
            std::string extension = fileName.substr(pos);
            mime = MHContentType::getContentTypeByExtension(extension.c_str());
        } else {
            mime = MHContentType::getContentTypeByExtension("");
        }
        
        info->setContentType(mime.c_str());

        itemsInfoList.addItem(info);
    }

    return true;        
}

int MHFileSyncSource::performTwinDetection(AbstractSyncConfig& mainConfig)
{
    CacheItemsList cacheItems;
    unsigned cacheSize = 0;
    
    // Read all the items in cache
    if (!getItemsFromCache(cacheItems)) {
        LOG.error("%s: error reading cache", __FUNCTION__);
        
        return 2;
    }
    std::vector<MHStoreEntry*> entries = cacheItems.getItems();
    
    cacheSize = entries.size();
    LOG.debug("%d items in cache", cacheSize);
    if (cacheSize == 0) {
        LOG.debug("%s: nothing to do", __FUNCTION__);
    
        return 0;
    }
    
    LOG.info("%s: reading all local items for source %s...", __FUNCTION__, getConfig().getName());
    
    CacheItemsList localItems;
    
    // reads ALL items from client sandbox
    if (localItemsFullScan(localItems) == false) {
        LOG.info("%s: error getting local items from client cache", __FUNCTION__);
        
        return 1;
    }

    std::vector<MHStoreEntry*> localEntries = localItems.getItems();
    
    // For each remote item in cache, check twin with the list of all local items 
    // (including out of digital life ones)
    // Start from the last one, as optimization (downloads already started from the 1st one)
    for (int i = (cacheSize - 1); i>=0; i--)  {
        MHSyncItemInfo* itemInCache = (MHSyncItemInfo*)entries[i];
        if (!itemInCache) continue;
        
        if (mainConfig.isToAbort()) {
            LOG.debug("%s: interrupting item scan on user cancel request", __FUNCTION__);
        
            return 1;
        }

        if (itemInCache->isLocallyAvailable()) {
            std::vector<MHStoreEntry*>::iterator itemIt;
            itemIt = getItemInfoIndexByName(itemInCache->getName(), localEntries);
            if (itemIt != localEntries.end()) {
                // found the local item in digital life in the file spool directory (allItemInfo)
                // remove it so the allItemInfo list will have only excluded items at the end
                localEntries.erase(itemIt);
            } else {
                // A local item is not found anymore in the spool directories (allItemInfo)
                // Its status is inconsistent: fix it                
                fixLocalItemRemoved(itemInCache);
            }
            
            continue;
        }
        
        // if here, itemInCache is a remote item already existing in cache
        MHSyncItemInfo* twin = twinDetection(*itemInCache, localEntries);
        if (twin) {
            LOG.info("Twin found! %s #%lu in cache: %s (GUID = %s)", 
                      getConfig().getName(), itemInCache->getId(), twin->getName().c_str(), itemInCache->getGuid().c_str());

            // update local item with server date
            twin->setStatus(EStatusRemote);
            twin->setGuid(itemInCache->getGuid());
            twin->setServerUrl(itemInCache->getServerUrl());
            twin->setServerLastUpdate(itemInCache->getServerLastUpdate());
            twin->setRemoteItemUrl(itemInCache->getRemoteItemUrl());
            
            twin->setLocalItemETag(itemInCache->getRemoteItemETag().c_str());
            twin->setLocalThumbETag(itemInCache->getRemoteThumbETag().c_str());
            twin->setLocalPreviewETag(itemInCache->getRemotePreviewETag().c_str());
            
            twin->setRemoteItemETag(itemInCache->getRemoteItemETag().c_str());
            twin->setRemoteThumbETag(itemInCache->getRemoteThumbETag().c_str());
            twin->setRemotePreviewETag(itemInCache->getRemotePreviewETag().c_str());
            
            updateItemInCache(twin, NULL);
            
            // remove server item from cache
            removeItemFromCache(itemInCache);
            delete twin;
        }
    }

    return 0;
}

MHSyncItemInfo* MHFileSyncSource::twinDetection(MHSyncItemInfo& serverItemInfo, std::vector<MHStoreEntry*> localEntries) {
    MHSyncItemInfo* ret = NULL;
    std::vector<MHStoreEntry*>::iterator it = localEntries.begin();
    std::vector<MHStoreEntry*>::iterator endIt = localEntries.end();
    while(it != endIt) {
        MHSyncItemInfo* itemInfo = (MHSyncItemInfo*)*it;
        if ((itemInfo->getSize() == serverItemInfo.getSize()) && 
            (strcmp(itemInfo->getName(), serverItemInfo.getName()) == 0)) {

            ret = (MHSyncItemInfo*)new MHSyncItemInfo(*itemInfo);
            it = localEntries.erase(it);
            endIt = localEntries.end();
            break;
        } else {
            ++it;
        }
    }
    return ret;
} 


int MHFileSyncSource::readAllItemsChanges(AbstractSyncConfig& mainConfig)
{
    return 0; 
}


StringBuffer MHFileSyncSource::insertItem(DownloadMHSyncItem* syncItem, EMHSyncSourceError* errCode, long* modificationDate)
{
    if (syncItem == NULL) {
        LOG.error("[%s] the passed syncItem is NULL", __FUNCTION__);
        return "";
    }

    FileOutputStream* stream = (FileOutputStream*)syncItem->getStream();
    if (stream == NULL) {
        LOG.error("[%s] The ostream of the syncItem is NULL", __FUNCTION__);
        return "";

    }

    stream->close();
    
    StringBuffer luid;
    MHSyncItemInfo *itemInfo = syncItem->getMHSyncItemInfo();
    if (itemInfo != NULL) {
        unsigned int cacheId = itemInfo->getId();
        luid.append(cacheId);
        *errCode = ESSSNoErr;
        
        // optimization: move the tmp downloaded file to the fullItems spool folder so
        // it's immediately available if this item is full screen. 
        // Also fixes refresh issues like bug 12524.
        itemInfo->setLuid(luid.c_str());
        if (moveTemporarySpoolItem(itemInfo) == false) {
            StringBuffer p = stream->getFilePath();
            remove(p);
        } else {
            LOG.info("%s: updating item after move...", __FUNCTION__);  
            if (!updateItemInCache(itemInfo, NULL)) {
                LOG.error("%s: error updating item in cache", __FUNCTION__);
            }
        }
    } else {
        LOG.error("%s: Cannot download item with no item info associated",__FUNCTION__);
        *errCode = ESSSInternalError;
    }

    return luid;
}

int MHFileSyncSource::deleteItem(MHSyncItemInfo* itemInfo) 
{
    const char* localPath = NULL;
    long int itemId      = itemInfo->getId();
    EItemInfoStatus  itemStatus = EStatusUndefined;
    
    if (!itemInfo) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        
        return 1;
    }
    
    if ((localPath = itemInfo->getLocalItemPath()) == NULL) {
        LOG.error("%s: local path not set for item", __FUNCTION__);
        
        return 1;
    }
    
    itemStatus = itemInfo->getStatus();
    
    if ((itemStatus == EStatusLocal) ||
        (itemStatus == EStatusLocalMetaDataChanged)) {
        LOG.info("%s: discarding delete for item %lu which has local modifications", __FUNCTION__, itemId);
        // reset guid and set item status to perform an upload
        itemInfo->setGuid("");
        itemInfo->setStatus(EStatusLocal);
        
        updateItemInCache(itemInfo, NULL);
        
        return 1;
    }
    
    // remove item from digital life
    LOG.info("Removing %s item %s from digital life (ID=%d, LUID=%s, GUID=%s file path: '%s')", config.getName(), 
              itemInfo->getName().c_str(), itemInfo->getId(), itemInfo->getLuid().c_str(), 
              itemInfo->getGuid().c_str(), localPath);

    if (localPath != NULL && strlen(localPath) > 0) {
        if (unlinkFile(localPath) != 0) {
            LOG.error("%s: error removing local item '%s': %s", __FUNCTION__, 
                localPath, strerror(errno)); 
        }
    }
    
    if (!removeItemFromCache(itemInfo)) {
        LOG.error("%s: error removing %s item %s (ID=%d) from items store", 
                  __FUNCTION__, getConfig().getName(), itemInfo->getName().c_str(), itemInfo->getId());
        return 2;
    }
    
    return 0;
}

bool MHFileSyncSource::fixLocalItemRemoved(MHSyncItemInfo* itemInfo)
{
    if (!itemInfo) return false;
   
    if (itemInfo->getStatus() == EStatusLocal || itemInfo->getStatus() == EStatusLocalOnly ||
            itemInfo->getStatus() == EStatusUploading || itemInfo->getStatus() == EStatusLocalNotUploaded) {   
        // Local item -> cleanup it from store so it will disappear
        LOG.info("A local %s (%s) has been removed from file spool directory (ID=%d): clean up the store", 
            config.getName(), itemInfo->getName().c_str(), itemInfo->getId());
        return removeItemFromCache(itemInfo);
    } else if (itemInfo->isLocallyAvailable()) {
        // Synced item -> set as remote so it may be downloaded again
        LOG.info("A fully synced %s (%s) has been removed from file spool direcory (ID=%d): make iy", 
            config.getName(), itemInfo->getName().c_str(), itemInfo->getId());
        itemInfo->setStatus(EStatusRemote);
        return updateItemInCache(itemInfo, NULL);
    }
     
    // Remote item -> nothing to do
    return false;
}

std::vector<MHStoreEntry*>::iterator MHFileSyncSource::getItemInfoIndexByName(const StringBuffer& name,
                                                                                     std::vector<MHStoreEntry*>& list) {
    
    std::vector<MHStoreEntry*>::iterator it = list.begin();
    std::vector<MHStoreEntry*>::iterator end = list.end();
    for(;it != end;++it) {
        MHSyncItemInfo* itemInfo = (MHSyncItemInfo*)*it;
        if (!itemInfo) continue;
            
        if (itemInfo->getName() == name) {  
            // found!
            return it;
        } 
    }
    // not found
    return end;
}

bool MHFileSyncSource::addItemToCache(MHSyncItemInfo* itemInfo, std::vector<MHLabelInfo*>* labels) 
{
    bool res = false;
    res = MHSyncSource::addItemToCache(itemInfo, labels);
    return res;
}

bool MHFileSyncSource::updateItemInCache(MHSyncItemInfo* itemInfo, std::vector<MHLabelInfo*>* labels) {
    return updateItemInCache(NULL, itemInfo, labels);
}


bool MHFileSyncSource::updateItemInCache(MHSyncItemInfo* oldItemInfo, MHSyncItemInfo* itemInfo,
                                         std::vector<MHLabelInfo*>* labels)
{
    bool downloadBinary = false;
    if (oldItemInfo != NULL && itemInfo->isLocallyAvailable()) {
        LOG.info("%s: The item is locally available and got updated on the server.",__FUNCTION__);
        // Check the binary ETag to see if the binary content needs to be re-downloaded
        if (oldItemInfo->getRemoteItemETag().length() == 0 ||
            oldItemInfo->getRemoteItemETag().compare(itemInfo->getRemoteItemETag()) != 0)
        {
            LOG.info("%s: The item changed ETag, re-download its binary content",__FUNCTION__);
            itemInfo->setLocalItemETag("");
            downloadBinary = true;
        }
    }
    
    bool res = MHSyncSource::updateItemInCache(oldItemInfo, itemInfo, labels);
    
    if (res && downloadBinary) {
        downloadRequest(itemInfo, NULL);
    }

    if (itemInfo->getStatus() == EStatusRemote) {
        sourceStatusUpdate(ITEM_UPDATED_BY_SERVER, itemInfo);
    }
    
    return res;
}


void MHFileSyncSource::sourceStatusUpdate(unsigned sourceStatus, MHSyncItemInfo* itemInfo)
{
}

void MHFileSyncSource::getAdditionalMetadata(MHSyncItemInfo* itemInfo)
{
}

bool MHFileSyncSource::moveTemporarySpoolItem(MHSyncItemInfo* itemInfo)
{
    if (!itemInfo)                   return false;
    if (itemInfo->getLuid().empty()) return false;
    if (itemInfo->getGuid().empty()) return false;
    
    StringBuffer srcFile = tmpSpoolPath;    
    srcFile.append(itemInfo->getGuid());
    
    StringBuffer srcFileName = itemInfo->getName();
    unsigned int pos = srcFileName.rfind(".");
    if (pos != StringBuffer::npos) {
        StringBuffer extension = srcFileName.substr(pos);
        srcFile.append(extension);
    }
    
    if (itemsDownloadPath.empty()) {
        LOG.error("%s: items download directory is unset", __FUNCTION__);
        
        return false;
    }
    
    StringBuffer destFile, fileName;
    const char* localItemPath = itemInfo->getLocalItemPath();
    struct stat st;
    
    if (!itemInfo->getName().empty()) {
        fileName = itemInfo->getName();
    } else {
        fileName = itemInfo->getLuid();
    }

    std::string dirPath; 
    
    if ((localItemPath == NULL) || (strlen(localItemPath) == 0)) {
        dirPath = itemsDownloadPath;
    } else {
        if (dirName(dirPath, localItemPath) == false) {
            LOG.error("%s: error getting dir name from local item path", __FUNCTION__);
            
            return false;
        }
    }
    destFile = dirPath.c_str(); 
	if (destFile.endsWith("/") || destFile.endsWith("\\")) {
		destFile.append(fileName.c_str());        
    } else {
		destFile.append("/"); destFile.append(fileName.c_str());
	}
    // destFile.sprintf("%s/%s", dirPath.c_str(), fileName.c_str());
    memset(&st, 0, sizeof(struct stat));
        
    LOG.debug("%s: checking file path '%s' for spooling", __FUNCTION__, destFile.c_str());
    // Check if the file already exists
    if (statFile(destFile.c_str(), &st) == 0) {
        LOG.debug("%s: file '%s' already exists on filesystem: checking for corresponding item", 
            __FUNCTION__, destFile.c_str());
        MHSyncItemInfo* otherItem = getItemFromCache(MHItemsStore::local_item_path_field_name, destFile);
        
        if (otherItem != NULL) {
            LOG.debug("%s: found item with same local path: checking if items GUIDs match", __FUNCTION__);
        
            if (!(itemInfo->getGuid() == otherItem->getGuid())) {
                StringBuffer newFileName;
                LOG.info("%s: GUIDs don't match (new item with same name): generating new path for item with GUID '%s'",
                    __FUNCTION__, itemInfo->getGuid().c_str());
                
                newFileName = generateLocalFileName(fileName, itemsDownloadPath.c_str());
                
                if (newFileName.empty()) {
                    LOG.error("%s: error generating unique local file name", __FUNCTION__);
                    delete otherItem;
                    
                    return false;
                }
               
                LOG.debug("%s: new local item path: '%s'", __FUNCTION__, newFileName.c_str()); 
                
                destFile = newFileName;
            }
        }
        
        delete otherItem;
    }
  
  
    LOG.debug("%s: moving file '%s' to '%s'", __FUNCTION__, srcFile.c_str(), destFile.c_str());
    
    if (renameFile(srcFile.c_str(), destFile.c_str()) != 0) {
        LOG.error("%s: error moving file '%s' to '%s': %s", __FUNCTION__, 
            srcFile.c_str(), destFile.c_str(), strerror(errno));
        
        // try to remove failed copy destination
        unlinkFile(destFile.c_str());
      
        return false;   
    }
   
    memset(&st, 0, sizeof(struct stat));
        
    if (statFile(destFile.c_str(), &st) != 0) {
        itemInfo->setLocalItemPath(destFile.c_str());
    
        LOG.error("%s: error stating file '%s': %s", __FUNCTION__,
            destFile.c_str(), strerror(errno));
        
        return false;
    }
  
    // Set the local item path and other info in cache
    itemInfo->setLocalItemPath(destFile.c_str());
    itemInfo->setModificationDateSecs(st.st_mtime);
    itemInfo->setCreationDateSecs(getFileCreationDate(st));
    itemInfo->setSize(st.st_size);
    
    return true;    
} 

int MHFileSyncSource::readDir(const char* dirpath, std::vector<std::string>& fileList, bool recursive, 
            std::map<std::string, bool>& readFoldersMap)
{
    ArrayList files = readFilesInDirRecursive(dirpath, recursive, excludedDirectoryNames, readFoldersMap);
    StringBuffer* fileName = NULL;
    
    while ((fileName = (StringBuffer*)files.next()) != NULL) {
        if (fileName->empty()) {
            continue;
        }
        
        fileList.push_back(fileName->c_str());
        
        if (clientConfig->isToAbort()) {
            return false;
        }
    }
    
    return 0;
}

std::string MHFileSyncSource::baseName(const std::string& fullName) 
{        
    std::string fileName("");
        
    std::string::size_type pos = fullName.rfind("/");
    if (pos == std::string::npos) {
        return fullName;
    }
    
    // Move to the first char of the filename
    pos += 1;

    fileName = fullName.substr(pos, fullName.size() - pos);
    
    return fileName;
}

bool MHFileSyncSource::dirName(std::string& dirPath, const std::string& fullName) 
{        
    std::string::size_type index = 0;
    
    if (fullName.empty()) {
        LOG.error("%s: invalid argument - no path specified", __FUNCTION__);
        
        return false;
    }
    
    index = fullName.rfind("/");
    if (index == std::string::npos) {
        LOG.error("%s: invalid argument - path don't have separators", __FUNCTION__);
        
        return false;
    }
    
    dirPath = fullName.substr(0, index);
    
    return true;
}

StringBuffer MHFileSyncSource::generateLocalFileName(const StringBuffer& fileName, const StringBuffer& dirPath)
{
    StringBuffer localFileName("");
    struct stat st;
    int maxRetriesNum = 50;
    
    for (int i = 0; i < maxRetriesNum; i++) {
        StringBuffer tmp;
        StringBuffer targetFilePath;
        
        memset(&st, 0, sizeof(struct stat));
        
        int pos = fileName.rfind(".");
        
        if (pos != StringBuffer::npos) {
            StringBuffer extension;
            
            tmp = fileName.substr(0, pos);
            extension = fileName.substr(pos);
            tmp.append(StringBuffer().sprintf("_%02i", i));
            tmp.append(extension);
        } else {
            tmp.sprintf("%s_%02i", fileName.c_str(), i);
        }
        
        if (dirPath.endsWith('/')) {
            targetFilePath.sprintf("%s%s", dirPath.c_str(), tmp.c_str());
        } else {
            targetFilePath.sprintf("%s/%s", dirPath.c_str(), tmp.c_str());
        }
        
        if (statFile(targetFilePath.c_str(), &st) == 0) {
            continue;
        } else {
            localFileName = targetFilePath;
            break;
        }
    }

    return localFileName;
}

int MHFileSyncSource::checkIfFitsOnLocalStorage(MHSyncItemInfo* itemInfo) {
    
    int res = 0, errorCode = 0;
    int64_t itemSize = itemInfo->getSize();

    if (isLocalStorageAvailable(itemSize, &errorCode) == false) {
        if (errorCode == ERR_FILE_SYSTEM) {
            LOG.info("Can't download item %s: cannot find the download items path", itemInfo->getName().c_str());
            // res = ESSMMediaHubPathNotFound; // return quotaExceeded anyway
            res = ESSMClientQuotaExceeded;
        } else {
            LOG.info("Can't download item %s: local storage is full", itemInfo->getName().c_str());
            res = ESSMClientQuotaExceeded;
        }
    }
    return res;
}

bool MHFileSyncSource::isLocalStorageAvailable(int64_t size, int* errorCode) { 
    
    StringBuffer val = config.getProperty(PROPERTY_LOCAL_QUOTA_STORAGE);
    if (val.null() || val.length() == 0) {
        // storage limit disabled
        return true;
    }

    unsigned long long totalBytes = 0, freeBytes = 0;
    if (int res = getFreeDiskSpace(itemsDownloadPath.c_str(), &totalBytes, &freeBytes)) {
        LOG.info("[%s] can't get the free space on disk, items are rejected. Code %i", __FUNCTION__, res);
        *errorCode = ERR_FILE_SYSTEM;
        return false;
    }

    unsigned long long used = totalBytes - freeBytes + size;  

    if (val.endsWith("%")) {
        // It's a percentage value [0-100]
        int perc = 0;
        sscanf(val.c_str(), "%d%%", &perc);
        if (perc > 100) { perc = 100; }
        if (perc < 0)   { perc = 0;   }
        
        unsigned long long limit = totalBytes * perc/100;         
        if (used >= limit) {
            LOG.error("Local storage full: used = %llu, limit = %llu [bytes] (%d%%)", used, limit, perc);
            return false;
        }
    }
    else {

        // It's an absolute value [MBytes]
        unsigned long limit = atol(val.c_str());        
        if (used >= limit*1024*1024) {
            LOG.error("Local storage full: used = %llu, limit = %ld [bytes]", used, limit*1024*1024);
            return false;
        }
    }

    return true;
}

bool MHFileSyncSource::removableDriveForUploadNotAvailable(const char* filePath) 
{
	const char* uploadRemovableFoldersList = getConfig().getProperty(PROPERTY_ITEMS_UPLOAD_REMOVABLE_DIRECTORIES_LIST);
	std::string rootFolder = findPathOnDirectoriesList(filePath, uploadRemovableFoldersList);

	if (rootFolder.empty()) return false;// it's not a removable drive

	struct stat st;
	memset(&st, 0, sizeof(struct stat));
	if (statFile(rootFolder.c_str(), &st) && errno == ENOENT) {
		// it's a removable drive, and root folder is not available
		return true;
	}

	return false;  // it's a removable drive, but root folder is available
}

bool MHFileSyncSource::networkDriveForUploadNotAvailable(const char* filePath) 
{
	const char* uploadNetworkFoldersList = getConfig().getProperty(PROPERTY_ITEMS_UPLOAD_NETWORK_DIRECTORIES_LIST);
	std::string rootFolder = findPathOnDirectoriesList(filePath, uploadNetworkFoldersList);

	if (rootFolder.empty()) return false;// it's not a network drive

	struct stat st;
	memset(&st, 0, sizeof(struct stat));
	if (statFile(rootFolder.c_str(), &st) && errno == ENOENT) {
		// it's a network drive, and root folder is not available
		return true;
	}

	return false;  // it's a network drive, but root folder is available
}

std::string MHFileSyncSource::findPathOnDirectoriesList(const char* filePath, const char* foldersList) 
{
	std::string path = filePath;
    if (path.empty()) return "";

	if (!foldersList) return "";

	std::vector<std::string> folders;
	splitString(foldersList, ',', folders);

	// check if at least 1 folder matches this file path
    std::vector<std::string>::const_iterator it = folders.begin(), end = folders.end();  
    for (; it != end; it++) {
        std::string folder = *it;
		if (path.find(folder) != std::string::npos) {
			return folder;
		}
	}
	return "";
}

} // end Funambol namespace