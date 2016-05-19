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

#ifndef __MH_ITEMS_STORE_H__
#define __MH_ITEMS_STORE_H__

#include <algorithm>
#include <string>

#include "base/util/StringBuffer.h"
#include "MediaHub/MHStore.h"
#include "MediaHub/MHStoreEntry.h"
#include "MediaHub/MHSyncItemInfo.h"

BEGIN_FUNAMBOL_NAMESPACE

class MHSyncItemInfo;

typedef enum ColumnType {
    TEXT,
    INTEGER,
    UINTEGER,
    INTEGER64,
    UINTEGER64
} ColumnType;

typedef struct ColumnDescriptor {
    const char* name;
    ColumnType type;
    bool key;
    bool autoincrement;
} ColumnDescriptor;

class MHItemsStoreListener {
public:
    virtual void itemAdded(MHStoreEntry*) {}
    virtual void itemsAdded(std::vector<MHStoreEntry*>& entries) {}
    virtual void itemUpdated(MHStoreEntry*) {}
    virtual void itemsUpdated(std::vector<MHStoreEntry*>& entries) {}
    virtual void itemDeleted(MHStoreEntry*) {}
};

class MHItemsStore : public MHStore
{

    
protected:
    bool addLocalItemPathOnUpgrade();
    bool addRemoteItemETagOnUpgrade();
    bool addRemoteThumbETagOnUpgrade();
    bool addRemotePreviewETagOnUpgrade();
    bool addLocalItemETagOnUpgrade();
    bool addLocalThumbETagOnUpgrade();
    bool addLocalPreviewETagOnUpgrade();
    bool addValidationStatusOnUpgrade();
    bool migrateItemStatus();
    bool addColumnIfMissing(const char* newColName, const char* newColType, const char* defaultValue);

protected: 
    
    static const char* create_table_stmt_fmt;
    static const char* insert_row_stmt_fmt;
    static const char* row_values_fmt;
    static const char* update_row_stmt_fmt;
    static const char* update_set_stmt_fmt;
    static const char* select_all_ordered_stmt_fmt;
    static const char* select_all_labels_filter_ordered_stmt_fmt;
    
    static const char* select_entry_id_stmt_fmt;
    static const char* delete_row_id_stmt_fmt;
    static const char* select_count_with_status;
    
    static ColumnDescriptor basicColumns1[];
    static ColumnDescriptor basicColumns2[];
    static ColumnDescriptor basicColumns3[];
    static ColumnDescriptor exifColumns[];
    static ColumnDescriptor videoColumns[];
    static ColumnDescriptor *columns;
    static int numColumns;
    
    std::string insertQuery;
    std::string createQuery;
    std::string updateQuery;
    
    std::string bridgeTableInsertQuery;
    
    MHItemsStoreListener* listener;
    
    virtual void initializeColumns();
    virtual bool initializeTable();
    virtual void initializeStaticQueries(); 
    virtual bool initializeItemsLabelsBridgeTable();

    
    virtual StringBuffer formatInsertItemStmt(MHStoreEntry* entry);
    virtual StringBuffer formatUpdateItemStmt(MHStoreEntry* itemInfo);

    
    /// Returns new allocated MHStoreEntry (a MHSyncItemInfo*) from a a sqlite statement object
    virtual MHStoreEntry* readEntry(sqlite3_stmt *stmt) const;
    
    void printColumnNameType(ColumnDescriptor& desc, std::string& sql);
    
    int getBasicColumns1Count();
    int getBasicColumns2Count();
    int getBasicColumns3Count();
    
    virtual MHStoreEntry* createEntry(unsigned long id_,
                                      const char* guid,
                                      const char* luid,
                                      const char* name,
                                      int64_t size,
                                      const char* serverUrl,
                                      const char* contentType,
                                      int64_t creationDate,
                                      int64_t modificationDate,
                                      EItemInfoStatus status,
                                      time_t serverLastUpdate,
                                      const char* remoteItemUrl,
                                      const char* remoteThumbUrl,
                                      const char* remotePreviewUrl,
                                      const char* localThumbPath,
                                      const char* localPreviewPath,
                                      const char* localItemPath_,
                                      const char* remoteItemETag,
                                      const char* remoteThumbETag,
                                      const char* remotePreviewETag) const;
    
public:
    
    // store entry name mappings
    static const char* luid_field_name;
    static const char* guid_field_name;
    static const char* name_field_name;
    static const char* size_field_name;
    static const char* creation_date_field_name;
    static const char* modification_date_name;           // local mod date
    static const char* modification_date_msec_name;      // local mod date
    static const char* content_type_field_name;
    static const char* status_field_name;
    static const char* server_last_update_field_name;    // remote mod date
    static const char* server_url_field_name;
    static const char* remote_item_url_field_name;       // full item url
    static const char* remote_thumb_url_field_name;      // thumb 176 url
    static const char* remote_preview_url_field_name;    // thumb 504 url
    static const char* local_thumb_path_field_name;      // could be fixed to <name>_thumb ?
    static const char* local_preview_path_field_name;    // could be fix
    static const char* local_item_path_field_name;
    static const char* remote_item_etag;                 // the etag of the remote item (computed by the server)
    static const char* remote_thumb_etag;                // the etag of the remote thumb (computed by the server)
    static const char* remote_preview_etag;              // the etag of the remote preview (computed by the server)
    static const char* local_item_etag;                  // the etag of the local item (computed by the server)
    static const char* local_thumb_etag;                 // the etag of the local thumb (computed by the server)
    static const char* local_preview_etag;               // the etag of the local preview (computed by the server)
    static const char* validation_status;                // item validation status
    
    // exif values
    static const char* exif_maker_field_name;
    static const char* exif_model_field_name;
    static const char* exif_image_width_field_name;
    static const char* exif_image_length_field_name;
    static const char* exif_creation_date_field_name;
    
    // video properties
    static const char* video_codec_field_name;
    static const char* video_duration_field_name;
    static const char* video_bitrate_field_name;
    static const char* video_height_field_name;
    static const char* video_width_field_name;
    
    // shared services
    static const char* shared_services_field_name;
    static const char* shared_via_email_field_name;
    
    // Keep track of how many upload failures to blacklist the item
    static const char* num_upload_failures_field_name;
    
    // Items-Labels bridging table
    static const char* itemsLabelsBridgeTableName;
    static const char* itemsLabelsBridgeItemIdFieldName;
    static const char* itemsLabelsBridgeLabelIdFieldName;

    
    explicit MHItemsStore(const char* cacheName, const char* cachePath,
                          int funambolSavedVersionNumber,
                          int funambolCurrentVersionNumber,
                          bool init=true);
    virtual ~MHItemsStore();
    
    void setListener(MHItemsStoreListener* listener) {
        this->listener = listener;
    }
    
    virtual bool AddEntry(MHStoreEntry* entry);
    virtual bool addEntries(std::vector<MHStoreEntry*>& entries);
    virtual bool UpdateEntry(MHStoreEntry* entry);
    virtual bool updateEntries(std::vector<MHStoreEntry*>& entries);
    bool RemoveEntry(MHStoreEntry* entry);
    
    /**
     * Returns all cache entries in the given list, ordered by a given parameter.
     * The entryList contains pointers to MHItemStoreEntry which are
     * automatically deallocated once the entryList gets deallocated.
     *
     * @param MHItemInfoList  [IN-OUT] the list returned with all entries
     * @param orderedBy         the field name to order the returned array
     * @param descending        if true, the items are sorted descending
     * @param labelId           the label to be used when filtering items (-1 to skip filtering)
     * @param filter            a custom filter condition to match items (this is a single value)
     * @param limit             max number of items returned. Pass (uint32_t)-1 to avoid limiting
     */
    virtual bool getAllEntries(CacheItemsList& entryList, const char* orderedBy, bool descending,
                               int32_t labelId = -1,
                               const char* filter = NULL, uint32_t limit = (uint32_t)-1);
    
    /**
     * Returns all cache entries in the given list.
     * The entryList contains pointers to MHItemStoreEntry which are
     * automatically deallocated once the entryList gets deallocated.
     *
     * @param MHItemInfoList  [IN-OUT] the list returned with all entries
     */
    bool getAllEntries(CacheItemsList& entryList);
    
    /// Returns a new allocated MHStoreEntry* (a MHSyncItemInfo*) given its ID, NULL if not found.
    MHStoreEntry* getEntry(const unsigned long itemID) const;
    
    /// Returns a new allocated MHStoreEntry* (a MHSyncItemInfo*) given a custom field, NULL if not found.
    virtual MHStoreEntry* getEntry(const char* fieldName, const char* fieldValue) const {
        return MHStore::getEntry(fieldName, fieldValue);
    }
    
    int getCountOfItemsWithStatus(int status);
    
    virtual int32_t addLabelsToItem(MHSyncItemInfo* itemInfo, std::vector<MHLabelInfo*>* labels);
    virtual bool getItemLabels(MHSyncItemInfo* itemInfo, std::vector<uint32_t>& labelsId);
    virtual bool getAllLabelsForItems(std::vector<std::pair<uint32_t,uint32_t> >& labelsId);

    
    /**
     * Removes all the labels associated to the given item
     * @return the number of items removed or -1 if the operation failed
     */
    virtual int32_t clearItemLabels(MHSyncItemInfo* itemInfo);
    
protected:
    virtual bool upgrade(int from, int to);
    virtual StringBuffer createSelectAllQuery(const char* orderedBy, bool descending, const char* filter,
                                              int labelId,uint32_t limit);
};

END_FUNAMBOL_NAMESPACE

#endif
