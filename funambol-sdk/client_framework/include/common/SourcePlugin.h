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


#ifndef INCL_SOURCE_PLUGIN
#define INCL_SOURCE_PLUGIN

#include "base/adapter/PlatformAdapter.h"
#include "base/util/StringBuffer.h"
#include "base/util/ArrayList.h"
#include "MediaHub/MHItemsStore.h"
#include "MediaHub/MHLabelsStore.h"
#include "MediaHub/MHSyncSource.h"
#include "spds/SyncSource.h"
#include "SourcePluginConfig.h"

BEGIN_NAMESPACE

/**
 * A source plugin represents at high level a type of data that the application is able to handle.
 * For example pictures, videos and contacts are plugins.
 * A plugin defines all the properties for the application to handle it. In case a new type
 * of data needs to be added, the only requirement is to create a new plugin, without changing
 * anything else in the code.
 *
 * The current implementation is still limited, as at the moment the plugin contains only fixed
 * properties (such as the source name) but in the future it will be extended to be also a factory
 * for its components (e.g. the all sources source bar, the chronological view, the settings view
 * and so on).
 */

class SourcePlugin
{
public:
    typedef enum OrderField {
        orderByCreationDate = 0,
        orderByModificationDate
    } order_field_t;
    
protected:
    StringBuffer sourceName;
    StringBuffer sourceLabel;
    StringBuffer sourceTag;
    StringBuffer sapiUri;
    StringBuffer sapiArrayKey;
    OrderField   orderField; // token key for sapi requests and local storage queries
    
    
    
    bool active;
    bool media;
    
    MHItemsStore* cache;
    MHItemsStore* excludedCache;
    MHItemsStore* pendingDeletesCache;
    MHLabelsStore* labelsStore;
    
    SyncSource* pimSyncSource;
    MHSyncSource* mediaSyncSource;
    
    SourcePluginConfig* config;
    
    long maxOutgoingItemSize;
    long long quotaThreshold;
   
    bool selectiveUploadEnabled;
    bool fillExcludedItemsMetadata;
    bool requiresThumbnailRefreshOnUploadTermination;
    bool showItemsCount;
    
    // specify sort order for queries with orderField token
    bool orderFieldDescending;  
    // specify if the source does not detect deletes on startup but it requires later checks
    // if this property is true the application is responsible to invoke checkLocalDelete any
    // time it needs information that is affected by local deletion of the corresponding item
    bool hasDeferredDeleteDetection;
   
    // a list of default upload folder by source
    std::vector<std::string> sourceDefaultUploadFolders;
    
protected:
    virtual SyncSource* createPIMSyncSource();
    virtual MHSyncSource* createMHSyncSource();
    virtual void fillInSourceDefaultUploadFolders();

private:
    int sourceId;
    
public:
    SourcePlugin(const char* tag, const char* label, const char* name);
        
    const char* getTag() const;
    const char* getName() const;
    const char* getLabel() const;
    
    const char* getSapiUri() const;
    const char* getSapiArrayKey() const;
    OrderField getOrderField() const;
    const char* getOrderFieldAsString() const;
    
    int getId() const;
    void setId(int sourceId);
    
    virtual bool isMedia() const;
    
    void setActive(bool active);
    bool getActive() const;
    
    /*
     * Returns true if this plugin has a tabbed view (e.g. chronological view)
     */
    bool getHasTabbedView() const;
    
    /*
     * This method returns the cache for the items belonging to this plugin. Note that
     * it is not required that each plugin has an item cache. In such a case NULL is returned
     */
    MHItemsStore* getItemsCache() const;
    
    /*
     * This method returns the cache for the excluded items belonging to this plugin. Note that
     * it is not required that each plugin has an item cache. In such a case NULL is returned
     */
    MHItemsStore* getExcludedItemsCache() const;
    
    /*
     * This method returns the cache for pending deletes (deleted items whose remote operation
     * has not been completed yet)
     */
    MHItemsStore* getPendingDeletesCache() const;
    
    
    /*
     * This method returns the store with the labels associated to the items belonging to this plugin
     */
    MHLabelsStore* getLabelsStore() const;
    
    bool getSelectiveUploadEnabled() const;
    bool getFillExcludedItemsMetadata() const;
    bool getOrderFieldDescending() const;
    
    bool getRequiresThumbnailRefreshOnUploadTermination() const;

    
    virtual bool init();
    virtual ~SourcePlugin();
         
    SyncSource* getPIMSyncSource() { return pimSyncSource; }
    MHSyncSource* getMHSyncSource() { return mediaSyncSource; }
       
    virtual bool canDownloadItem(MHSyncItemInfo* itemInfo);
    
    virtual long getTotalNumberOfLocalItems();
    
    virtual long getMaxOutgoingItemSize();
    
    /**
     * Cleans up all spool folders and cache DBs.
     */
    void cleanup();
    
    long long disableOnAvailableQuotaThreshold();
       
    
    SourcePluginConfig* getConfig() { return config; }
   
    bool getShowItemsCount();
    
    virtual bool requiresDeepSync();
    
    bool getDeferredDeleteDetection();

    virtual const char* getDefaultSyncDirection() const;
    // This method checks the actual size of an item. This method is meaningful if
    // the source stores full items in an external storage (such as the gallery) that
    // can be externally modified and deletes are detected deferred.
    virtual bool updateDeletedStatus(unsigned long cacheId);
    
    //UI related methods
    virtual const char* getEnableSourceText() const;
    virtual const char* getUploadSourceText() const;
    virtual const char* getUploadSourceDetailsText() const;
    virtual const char* getUploadNoteText() const;
    virtual const char* getDownloadSourceText() const;
    virtual const char* getDownloadSourceDetailsText() const;
    virtual const char* getAutoDownloadSourceText() const;
    virtual const char* getSourceDestFolderText() const;
    virtual const char* getToolbarIdentifier() const;
    virtual const char* getToolbarIconPath() const;
    virtual const char* getToolbarWarningIconPath() const;
    virtual const char* getToolbarTitle() const;
    virtual const char* getSourceColumnHeaderTitle() const;
    const std::vector<std::string>& getSourceDefaultUploadFolders() const;
};


END_NAMESPACE
#endif
