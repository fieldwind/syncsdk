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

#include <string>
#include "MediaHub/MHItemsStore.h"
#include "base/Log.h"
#include "string.h"
#include "errno.h"
//#include "utils.h"

BEGIN_FUNAMBOL_NAMESPACE

const char* MHItemsStore::luid_field_name               = "luid";
const char* MHItemsStore::guid_field_name               = "guid";
const char* MHItemsStore::name_field_name               = "name";
const char* MHItemsStore::size_field_name               = "size";
const char* MHItemsStore::creation_date_field_name      = "creation_date";
const char* MHItemsStore::modification_date_name        = "modification_date";
const char* MHItemsStore::content_type_field_name       = "content_type";
const char* MHItemsStore::status_field_name             = "status";
const char* MHItemsStore::server_last_update_field_name = "server_last_update";
const char* MHItemsStore::server_url_field_name         = "url";
const char* MHItemsStore::remote_item_url_field_name    = "remote_item_url";
const char* MHItemsStore::remote_thumb_url_field_name   = "remote_thumb_url";
const char* MHItemsStore::remote_preview_url_field_name = "remote_preview_url";
const char* MHItemsStore::local_thumb_path_field_name   = "local_thumb_path";
const char* MHItemsStore::local_preview_path_field_name = "local_preview_path";
const char* MHItemsStore::local_item_path_field_name    = "local_item_path";
const char* MHItemsStore::remote_item_etag              = "remote_item_etag";
const char* MHItemsStore::remote_thumb_etag             = "remote_thumb_etag";
const char* MHItemsStore::remote_preview_etag           = "remote_preview_etag";
const char* MHItemsStore::local_item_etag               = "local_item_etag";
const char* MHItemsStore::local_thumb_etag              = "local_thumb_etag";
const char* MHItemsStore::local_preview_etag            = "local_preview_etag";
const char* MHItemsStore::validation_status             = "validation_status";

// exif fields
const char* MHItemsStore::exif_maker_field_name         = "exif_maker";
const char* MHItemsStore::exif_model_field_name         = "exif_model";
const char* MHItemsStore::exif_image_width_field_name   = "exif_image_width";
const char* MHItemsStore::exif_image_length_field_name  = "exif_image_length";
const char* MHItemsStore::exif_creation_date_field_name = "exif_creation_date";

// video source field
const char* MHItemsStore::video_codec_field_name        = "video_codec";
const char* MHItemsStore::video_duration_field_name     = "video_duration";
const char* MHItemsStore::video_bitrate_field_name      = "video_bitrate";
const char* MHItemsStore::video_height_field_name       = "video_height";
const char* MHItemsStore::video_width_field_name        = "video_width";

// shared services
const char* MHItemsStore::shared_services_field_name    = "shared_services";

// shared via email
const char* MHItemsStore::shared_via_email_field_name   = "shared_via_email";

// Keep track of how many upload failures to blacklist the item
const char* MHItemsStore::num_upload_failures_field_name= "num_upload_failures";

// Items-Labels bridging table
const char* MHItemsStore::itemsLabelsBridgeTableName        = "items_labels";
const char* MHItemsStore::itemsLabelsBridgeItemIdFieldName  = "item_id";
const char* MHItemsStore::itemsLabelsBridgeLabelIdFieldName = "label_id";


ColumnDescriptor* MHItemsStore::columns;
int MHItemsStore::numColumns;

ColumnDescriptor MHItemsStore::basicColumns1[] = {
    {"id", INTEGER, true, true},
    {MHItemsStore::luid_field_name, TEXT, false, false},
    {MHItemsStore::guid_field_name, TEXT, false, false},
    {MHItemsStore::name_field_name, TEXT, false, false},
    {MHItemsStore::size_field_name, UINTEGER64, false, false},
    {MHItemsStore::creation_date_field_name, INTEGER64, false, false},
    {MHItemsStore::modification_date_name, INTEGER64, false, false},
    {MHItemsStore::content_type_field_name, TEXT, false, false},
    {MHItemsStore::status_field_name, INTEGER, false, false},
    {MHItemsStore::server_last_update_field_name, INTEGER64, false, false},
    {MHItemsStore::server_url_field_name, TEXT, false, false},
    {MHItemsStore::remote_item_url_field_name, TEXT, false, false},
    {MHItemsStore::remote_thumb_url_field_name, TEXT, false, false},
    {MHItemsStore::remote_preview_url_field_name, TEXT, false, false},
    {MHItemsStore::local_thumb_path_field_name, TEXT, false, false},
    {MHItemsStore::local_preview_path_field_name, TEXT, false, false}
};

ColumnDescriptor MHItemsStore::basicColumns2[] = {
    {MHItemsStore::shared_services_field_name, TEXT, false, false},
    {MHItemsStore::shared_via_email_field_name, INTEGER, false, false},
    {MHItemsStore::num_upload_failures_field_name, INTEGER, false, false},
    {MHItemsStore::local_item_path_field_name, TEXT, false, false},
};

ColumnDescriptor MHItemsStore::basicColumns3[] = {
    {MHItemsStore::remote_item_etag, TEXT, false, false},
    {MHItemsStore::remote_thumb_etag, TEXT, false, false},
    {MHItemsStore::remote_preview_etag, TEXT, false, false},
    {MHItemsStore::local_item_etag, TEXT, false, false},
    {MHItemsStore::local_thumb_etag, TEXT, false, false},
    {MHItemsStore::local_preview_etag, TEXT, false, false},
    {MHItemsStore::validation_status, INTEGER, false, false}
};

ColumnDescriptor MHItemsStore::exifColumns[] = {
    {MHItemsStore::exif_maker_field_name, TEXT, false, false},
    {MHItemsStore::exif_model_field_name, TEXT, false, false},
    {MHItemsStore::exif_image_width_field_name, INTEGER, false, false},
    {MHItemsStore::exif_image_length_field_name, INTEGER, false, false},
    {MHItemsStore::exif_creation_date_field_name, INTEGER64, false, false}
};

ColumnDescriptor MHItemsStore::videoColumns[] = {
    {MHItemsStore::video_codec_field_name, TEXT, false, false},
    {MHItemsStore::video_duration_field_name, INTEGER, false, false},
    {MHItemsStore::video_bitrate_field_name, INTEGER, false, false},
    {MHItemsStore::video_height_field_name, INTEGER, false, false},
    {MHItemsStore::video_width_field_name, INTEGER, false, false}
};
    

const char* MHItemsStore::select_all_ordered_stmt_fmt   = "SELECT * from %s ORDER BY %s %s";
const char* MHItemsStore::select_all_labels_filter_ordered_stmt_fmt   = "SELECT %s.* from %s,%s WHERE %s.id=%s AND \
 %s=%d ORDER BY %s %s";
const char* MHItemsStore::select_entry_id_stmt_fmt      = "SELECT * from %s WHERE id = %lu";
const char* MHItemsStore::delete_row_id_stmt_fmt        = "DELETE FROM %s WHERE id = %lu";
const char* MHItemsStore::select_count_with_status      = "SELECT COUNT(*) FROM %s WHERE status = %lu";

MHItemsStore::MHItemsStore(const char* storeName, const char* storePath,
                           int funambolSavedVersionNumber,
                           int funambolCurrentVersionNumber,
                           bool init) : MHStore(storeName, storePath), listener(NULL)
{
    if (init) {
        if (store_status == store_status_not_initialized) {
            
            initializeColumns();
            initializeStaticQueries();
            
            // db file was just created: init table
            LOG.debug("%s: initializing table for store %s", __FUNCTION__, store_name.c_str());
            if (!initializeTable()) {
                LOG.debug("%s: initializing store %s for db: %s", __FUNCTION__, store_name.c_str(), sqlite3_errmsg(db));
            }
            
            // Initialize the items-labels bridge table
            initializeItemsLabelsBridgeTable();
            
            // ignoring error in initialize_table (can't catch the 'error already exist')
            store_status = store_status_initialized;
            
            if (funambolSavedVersionNumber != funambolCurrentVersionNumber) {
                upgrade(funambolSavedVersionNumber, funambolCurrentVersionNumber);
            }
        }
    }
}

int MHItemsStore::getBasicColumns1Count() {
    int count = sizeof(basicColumns1) / sizeof(ColumnDescriptor);
    return count;
}

int MHItemsStore::getBasicColumns2Count() {
    int count = sizeof(basicColumns2) / sizeof(ColumnDescriptor);
    return count;
}

int MHItemsStore::getBasicColumns3Count() {
    int count = sizeof(basicColumns3) / sizeof(ColumnDescriptor);
    return count;
}

void MHItemsStore::initializeColumns() {
    // In the basic implementation we have basicColumns1 + exifColumns + videoColumns + basicColumns2
    int numBasic1 = getBasicColumns1Count();
    int numExif   = sizeof(exifColumns) / sizeof(ColumnDescriptor);
    int numVideos = sizeof(videoColumns) / sizeof(ColumnDescriptor);
    int numBasic2 = getBasicColumns2Count();
    int numBasic3 = getBasicColumns3Count();
    columns = new ColumnDescriptor[numBasic1 + numExif + numVideos + numBasic2 + numBasic3];
    
    int j = 0;
    for(int i=0;i<numBasic1;++i) {
        columns[j++] = basicColumns1[i];
    }
    for(int i=0;i<numExif;++i) {
        columns[j++] = exifColumns[i];
    }
    for(int i=0;i<numVideos;++i) {
        columns[j++] = videoColumns[i];
    }
    for(int i=0;i<numBasic2;++i) {
        columns[j++] = basicColumns2[i];
    }
    for(int i=0;i<numBasic3;++i) {
        columns[j++] = basicColumns3[i];
    }
    numColumns = j;
}

MHItemsStore::~MHItemsStore() {}

bool MHItemsStore::initializeTable()
{
    int ret = SQLITE_OK;
    ret = sqlite3_exec(db, createQuery.c_str(), NULL, NULL, NULL);
    return (ret == SQLITE_OK);
}


bool MHItemsStore::initializeItemsLabelsBridgeTable() {
    bridgeTableInsertQuery.append("INSERT INTO ").append(itemsLabelsBridgeTableName).append(" ")
    .append("(")
    .append(itemsLabelsBridgeItemIdFieldName)
    .append(",")
    .append(itemsLabelsBridgeLabelIdFieldName)
    .append(")")
    .append(" VALUES(")
    .append("%u")
    .append(",")
    .append("%u")
    .append(")");
    
    std::string createQuery;
    createQuery.append("CREATE TABLE ").append(itemsLabelsBridgeTableName).append(" (ID INTEGER PRIMARY KEY AUTOINCREMENT")
               .append(",")
               .append(itemsLabelsBridgeItemIdFieldName).append(" ").append("INTEGER")
               .append(",")
               .append(itemsLabelsBridgeLabelIdFieldName).append(" ").append("INTEGER")
               .append(")");
    int ret = sqlite3_exec(db, createQuery.c_str(), NULL, NULL, NULL);
    return (ret == SQLITE_OK);
}

void MHItemsStore::printColumnNameType(ColumnDescriptor& desc, std::string& sql) {
    sql = sql + desc.name + " ";
    const char* type;
    switch (desc.type) {
        case INTEGER:
        case UINTEGER:
        case INTEGER64:
        case UINTEGER64:
            type = "INTEGER";
            break;
        case TEXT:
            type = "TEXT";
            break;
        default:
            LOG.error("%s: Unsupported type %d",__FUNCTION__, desc.type);
    }
    sql += type;
}

void MHItemsStore::initializeStaticQueries() 
{
    // First of all search for the key field
    int keyIdx = -1;
    for(int i=0;i<numColumns;++i) {
        ColumnDescriptor &desc = columns[i];
        if (desc.key) {
            keyIdx = i;
            break;
        }
    }
    
    if (keyIdx == -1) {
        LOG.error("%s: Missing key in table, the app cannot work",__FUNCTION__);
        return;
    }
    
    // Build the create stmt
    createQuery = "CREATE TABLE ";
    createQuery.append(store_name.c_str()).append(" (");
    printColumnNameType(columns[keyIdx], createQuery);
    createQuery.append(" PRIMARY KEY ");
    if (columns[keyIdx].autoincrement) {
        createQuery.append("AUTOINCREMENT");
    }
    createQuery.append(",");
    // Now dump all the fields
    for(int i=0;i<numColumns;++i) {
        ColumnDescriptor &desc = columns[i];
        if (!desc.key) {
            printColumnNameType(columns[i], createQuery);
            if (i < (numColumns - 1)) {
                createQuery.append(",");
            }
        }
    }
    createQuery.append(")");
    
    // Build the insert row stmt
    insertQuery = "INSERT INTO ";
    insertQuery.append(store_name.c_str());
    insertQuery.append(" (");
    for(int i=0;i<numColumns;++i) {
        ColumnDescriptor& desc = columns[i];
        
        if (!desc.autoincrement) {
            insertQuery.append(desc.name);
            if (i < (numColumns - 1)) {
                insertQuery.append(",");
            }
        }
    }
    insertQuery.append(") VALUES (");
    for(int i=0;i<numColumns;++i) {
        ColumnDescriptor& desc = columns[i];
        if (!desc.autoincrement) {
            switch(desc.type) {
                case TEXT:
                    insertQuery.append("'%q'");
                    break;
                case INTEGER:
                    insertQuery.append("%d");
                    break;
                case UINTEGER:
                    insertQuery.append("%u");
                    break;
                case INTEGER64:
                    insertQuery.append("%lld");
                    break;
                case UINTEGER64:
                    insertQuery.append("%llu");
                    break;
            }
            if (i < (numColumns - 1)) {
                insertQuery.append(",");
            }
        }
    }
    insertQuery.append(")");
    
    // Build the update row stmt
    updateQuery = "UPDATE ";
    updateQuery.append(store_name.c_str());
    updateQuery.append(" SET ");
    for(int i=0;i<numColumns;++i) {
        ColumnDescriptor& desc = columns[i];
        
        if (!desc.autoincrement) {
            updateQuery.append(desc.name).append("=");
            switch(desc.type) {
                case TEXT:
                    updateQuery.append("'%q'");
                    break;
                case INTEGER:
                    updateQuery.append("%d");
                    break;
                case UINTEGER:
                    updateQuery.append("%u");
                    break;
                case INTEGER64:
                    updateQuery.append("%lld");
                    break;
                case UINTEGER64:
                    updateQuery.append("%llu");
                    break;
            }
            if (i < (numColumns - 1)) {
                updateQuery.append(",");
            }
        }
    }
    updateQuery.append(" WHERE id=%lu");
    
    select_count_stmt.sprintf(select_count_stmt_fmt, "id", store_name.c_str());
    
    //select_all_stmt.sprintf(select_all_stmt_fmt, store_name.c_str());
}

bool MHItemsStore::addEntries(std::vector<MHStoreEntry*>& entries) {
    std::vector<uint64_t> entriesId;
    bool ret = MHStore::addEntries(entries, entriesId);
    
    if (entriesId.size() != entries.size()) {
        LOG.error("%s: Size mismatch, missing inserted entries id (%d,%d)",__FUNCTION__,entriesId.size(),entries.size());
    }
    
    std::vector<uint64_t>::iterator it = entriesId.begin();
    std::vector<MHStoreEntry*>::iterator entriesIt = entries.begin();
    for(;it != entriesId.end();++it,++entriesIt) {
        uint64_t entryId = *it;
        MHSyncItemInfo* itemInfo = (MHSyncItemInfo*)*entriesIt;
        itemInfo->setId(entryId);
    }
    if (listener != NULL) {
        listener->itemsAdded(entries);
    }
    return ret;
}

bool MHItemsStore::AddEntry(MHStoreEntry* entry)
{
    uint64_t entryId;
    bool ret = MHStore::AddEntry(entry, entryId);
    
    //
    // add the item ID
    //
    if (ret) {
        MHSyncItemInfo* itemInfo = dynamic_cast<MHSyncItemInfo*>(entry);
        if (itemInfo == NULL) {
            LOG.error("%s: invalid parameter", __FUNCTION__);
            return false;
        }
        itemInfo->setId(static_cast<uint32_t>(entryId));
    
        if (listener != NULL) {
            listener->itemAdded(entry);
        }
    }
    
    return ret;
}

bool MHItemsStore::UpdateEntry(MHStoreEntry* entry)
{
    MHSyncItemInfo* itemInfo = (MHSyncItemInfo*)dynamic_cast<MHSyncItemInfo*>(entry);
    if (itemInfo == NULL) {
        LOG.error("%s: invalid parameter", __FUNCTION__);
        return false;
    }
    
    bool res = MHStore::UpdateEntry(entry);

    if (res && listener != NULL) {
        listener->itemUpdated(itemInfo);
    }

    return res;
}

bool MHItemsStore::updateEntries(std::vector<MHStoreEntry*>& entries) {
    bool ret = MHStore::updateEntries(entries);
    if (listener != NULL) {
        listener->itemsUpdated(entries);
    }
    return ret;
}

bool MHItemsStore::RemoveEntry(MHStoreEntry* entry)
{
    if (entry == NULL) {
        LOG.error("%s: invalid parameter", __FUNCTION__);
        return false;
    }
    
    MHSyncItemInfo* itemInfo = (MHSyncItemInfo*)entry;
    if (itemInfo == NULL) {
        LOG.error("%s: invalid parameter", __FUNCTION__);
        return false;
    }

    if (store_status != store_status_initialized) {
        LOG.error("%s: can't add entry: cache is not initialized", __FUNCTION__);
        return false;
    }
    
    
    StringBuffer sql;
    sql.sprintf(delete_row_id_stmt_fmt, store_name.c_str(), itemInfo->getId());
    
    if (sql.empty()) {
        LOG.error("%s: error formatting delete-entry statement", __FUNCTION__);
        return false;
    }
    
    //LOG.debug("%s: > SQL exec: '%s'", __FUNCTION__, sql.c_str());
    pthread_mutex_lock(&store_access_mutex);
    int ret = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    int changes = sqlite3_changes(db);
    pthread_mutex_unlock(&store_access_mutex);
    
    if (ret != SQLITE_OK) {
        LOG.error("%s: error executing SQL statement: %s", __FUNCTION__, sqlite3_errmsg(db));
        return false;
    } else if (changes == 0) {
        LOG.info("%s: cannot remove item %u", __FUNCTION__, itemInfo->getId());
        return false;
    }
    else {
        cache_items_count = getCount();
        cache_items_count--;
        
        if (listener != NULL) {
            listener->itemDeleted(entry);
        }
        
        return true;
    }
}


bool MHItemsStore::getAllEntries(CacheItemsList& entryList)
{
    return getAllEntries(entryList, creation_date_field_name, true, (uint32_t)-1);
}

StringBuffer MHItemsStore::createSelectAllQuery(const char* orderedBy, bool descending, const char* filter,
                                                int32_t labelId, uint32_t limit)
{
    StringBuffer res;
    
    if (labelId < 0) {
        res.sprintf(select_all_ordered_stmt_fmt, store_name.c_str(), orderedBy, descending? "DESC":"ASC");
    } else {
        res.sprintf(select_all_labels_filter_ordered_stmt_fmt, store_name.c_str(), store_name.c_str(), 
                    itemsLabelsBridgeTableName, store_name.c_str(), itemsLabelsBridgeItemIdFieldName, itemsLabelsBridgeLabelIdFieldName,
                    labelId, orderedBy, descending ? "DESC":"ASC");
    }
    
    if (limit != (uint32_t)-1) {
        // We have to limit the number of items
        res.append(" LIMIT ").append(limit);
    }
    
    return res;
}

bool MHItemsStore::getAllEntries(CacheItemsList& entryList, const char* orderedBy,
                                 bool descending, int32_t labelId,
                                 const char* filter, uint32_t limit)
{
    //LOG.debug("%s: entering", __FUNCTION__);
    
    int ret = SQLITE_OK;
    sqlite3_stmt *stmt = NULL;
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't get entries: cache is not initialized", __FUNCTION__);
        return false;
    }
    
    if (!orderedBy) {
        LOG.error("%s: empty 'ordered by' parameter", __FUNCTION__);
        return false;
    }

    // format the SQL query
    
    StringBuffer selectAllStmt = createSelectAllQuery(orderedBy, descending, filter, labelId, limit);
  
    pthread_mutex_lock(&store_access_mutex);
    
    ret = sqlite3_prepare_v2(db, selectAllStmt.c_str(), -1, &stmt, NULL );
    if (ret  != SQLITE_OK) {
        pthread_mutex_unlock(&store_access_mutex);
        
        LOG.error("%s: error preparing SQL query: %s", __FUNCTION__,
            sqlite3_errmsg(db));
            
        return false;
    }
    
    while ((sqlite3_step(stmt) == SQLITE_ROW))
    {        
        MHStoreEntry* entry = readEntry(stmt);
        if (entry) {
            entryList.addItem(entry);
        }
    }

    sqlite3_finalize( stmt );
    pthread_mutex_unlock(&store_access_mutex);
    
    return true;
}


MHStoreEntry* MHItemsStore::getEntry(const unsigned long itemID) const
{
    int res = SQLITE_OK;
    sqlite3_stmt *stmt = NULL;
    MHStoreEntry* entry = NULL;
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't get entries: cache is not initialized", __FUNCTION__);
        return NULL;
    }
    
    StringBuffer select_entry_stmt;
    select_entry_stmt.sprintf(select_entry_id_stmt_fmt, store_name.c_str(), itemID);
   
    pthread_mutex_lock(&store_access_mutex);
    res = sqlite3_prepare_v2(db, select_entry_stmt.c_str(), -1, &stmt, NULL );
    if (res  != SQLITE_OK) {
        pthread_mutex_unlock(&store_access_mutex);
        LOG.error("%s: error preparing SQL query: %s", __FUNCTION__, sqlite3_errmsg(db));
        return NULL;
    }
    
    while ((sqlite3_step(stmt) == SQLITE_ROW))  {
        entry = readEntry(stmt);
    }
    
    sqlite3_finalize( stmt );
    pthread_mutex_unlock(&store_access_mutex);
    
    return entry;
}


MHStoreEntry* MHItemsStore::readEntry(sqlite3_stmt *stmt) const
{
    if (!stmt) {
        LOG.error("%s: null sql statement", __FUNCTION__);
        return NULL;
    }
    
    MHSyncItemInfo* itemInfo = NULL;
    
    sqlite3_value *oid = NULL;
    unsigned long item_id = 0L;
    const char *luid = NULL, *guid = NULL, *name = NULL,
    *remoteItemUrl = NULL, *remoteThumbUrl = NULL,
    *remotePreviewUrl = NULL,*localThumbPath = NULL,
    *localPreviewPath = NULL, *contentType = NULL,
    *serverUrl = NULL, *exif_maker = NULL,
    *exif_model = NULL, *shared_services = NULL,
    *localItemPath = NULL, *remoteItemETag = NULL,
    *remoteThumbETag = NULL, *remotePreviewETag = NULL;
    
    int64_t size = 0;
    time_t serverLastUpdate = 0, exif_creation_date = 0;
    int exif_image_width = 0, exif_image_length = 0;
    int64_t modificationDate;
    int64_t creationDate;
    int32_t validationStatus;
    
    EItemInfoStatus status = EStatusUndefined;
    
    const char* video_codec = NULL;
    unsigned long video_duration = 0;
    unsigned int video_bitrate = 0;
    unsigned video_height = 0;
    unsigned video_width  = 0;
    int shared_via_email = 0;
    int num_upload_failures = 0;
    
    oid = sqlite3_column_value(stmt, 0);
    item_id = static_cast<unsigned long>(sqlite3_value_int64(oid));
    luid = (const char*)sqlite3_column_text(stmt, 1);
    guid = (const char*)sqlite3_column_text(stmt, 2);
    name = (const char*)sqlite3_column_text(stmt, 3);
    size = (int64_t)sqlite3_column_int64(stmt, 4);
    creationDate = (int64_t)sqlite3_column_int64(stmt, 5);
    modificationDate = (int64_t)sqlite3_column_int64(stmt, 6);
    contentType = (const char*)sqlite3_column_text(stmt, 7);
    status = (EItemInfoStatus)sqlite3_column_int(stmt, 8);
    serverLastUpdate = (unsigned long)sqlite3_column_int(stmt, 9);
    serverUrl = (const char*)sqlite3_column_text(stmt, 10);
    remoteItemUrl = (const char*)sqlite3_column_text(stmt, 11);
    remoteThumbUrl = (const char*)sqlite3_column_text(stmt, 12);
    remotePreviewUrl = (const char*)sqlite3_column_text(stmt, 13);
    localThumbPath = (const char*)sqlite3_column_text(stmt, 14);
    localPreviewPath = (const char*)sqlite3_column_text(stmt, 15);
    exif_maker = (const char*)sqlite3_column_text(stmt, 16);
    exif_model = (const char*)sqlite3_column_text(stmt, 17);
    exif_image_width = (int)sqlite3_column_int(stmt, 18);
    exif_image_length = (int)sqlite3_column_int(stmt, 19);
    exif_creation_date = (time_t)sqlite3_column_int(stmt, 20);
    
    video_codec = (const char*)sqlite3_column_text(stmt, 21);
    video_duration = (unsigned long)sqlite3_column_int(stmt, 22);
    video_bitrate = (int)sqlite3_column_int(stmt, 23);
    video_height = (int)sqlite3_column_int(stmt, 24);
    video_width  = (int)sqlite3_column_int(stmt, 25);
    
    shared_services = (const char*)sqlite3_column_text(stmt, 26);
    shared_via_email = (int)sqlite3_column_int(stmt, 27);
    num_upload_failures = (int)sqlite3_column_int(stmt, 28);
    localItemPath = (const char*)sqlite3_column_text(stmt, 29);
    remoteItemETag = (const char*)sqlite3_column_text(stmt, 30);
    remoteThumbETag = (const char*)sqlite3_column_text(stmt, 31);
    remotePreviewETag = (const char*)sqlite3_column_text(stmt, 32);
    const char* localItemETag = (const char*)sqlite3_column_text(stmt, 33);
    const char* localThumbETag = (const char*)sqlite3_column_text(stmt, 34);
    const char* localPreviewETag = (const char*)sqlite3_column_text(stmt, 35);
    validationStatus = (uint32_t)sqlite3_column_int(stmt, 36);
    
    itemInfo = (MHSyncItemInfo *)this->createEntry(item_id, guid, luid, name, size, serverUrl, contentType,
                                  creationDate, modificationDate, status, serverLastUpdate, 
                                  remoteItemUrl, remoteThumbUrl, remotePreviewUrl, localThumbPath, 
                                  localPreviewPath, localItemPath, remoteItemETag, remoteThumbETag,
                                  remotePreviewETag);
                                  
    itemInfo->setShared(shared_via_email != 0);
    itemInfo->setNumUploadFailures(num_upload_failures);
    itemInfo->setValidationStatus((EValidationStatus)validationStatus);
    itemInfo->setLocalItemETag(localItemETag != NULL ? localItemETag : "");
    itemInfo->setLocalThumbETag(localThumbETag != NULL ? localThumbETag : "");
    itemInfo->setLocalPreviewETag(localPreviewETag != NULL ? localPreviewETag : "");
    
    MHItemExifData itemExifData(exif_maker, exif_model, exif_image_width, exif_image_length, exif_creation_date);
    itemInfo->setItemExifData(itemExifData);
    
    MHItemVideoMetaData itemVideoMetaData(video_codec, video_duration, 
                                          video_bitrate, video_height, video_width);
    itemInfo->setItemVideoMetadata(itemVideoMetaData);
    
    if ((shared_services != NULL) && (strlen(shared_services) > 0)) {
        itemInfo->setExportedServicesFromString(shared_services);
    }
    
    return itemInfo;
}

MHStoreEntry* MHItemsStore::createEntry(unsigned long id_,
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
                                        const char* remotePreviewETag) const
{
    return new MHSyncItemInfo(id_, guid, luid, name, size, serverUrl, contentType,
                              creationDate, modificationDate, status, serverLastUpdate,
                              remoteItemUrl, remoteThumbUrl, remotePreviewUrl, localThumbPath,
                              localPreviewPath, localItemPath_, remoteItemETag, remoteThumbETag,
                              remotePreviewETag);
}

StringBuffer MHItemsStore::formatInsertItemStmt(MHStoreEntry* entry)
{
    StringBuffer stmt("");
  
    if (entry == NULL) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return stmt;
    }
    
    MHSyncItemInfo* itemInfo = (MHSyncItemInfo*)entry;
    if (itemInfo == NULL) {
        LOG.error("%s: invalid parameter", __FUNCTION__);
        return stmt;
    }
    
    const char *luid = itemInfo->getLuid();
    const char *guid = itemInfo->getGuid();
    StringBuffer name = itemInfo->getName() != NULL ? itemInfo->getName() : "";
    
    int64_t size  = itemInfo->getSize();
    int64_t serverLastUpdate = itemInfo->getServerLastUpdate();
    int64_t modificationDate = itemInfo->getModificationDate();
    int64_t creationDate = itemInfo->getCreationDate();
    
    int status = itemInfo->getStatus();
    const char *serverUrl = itemInfo->getServerUrl(),
               *remoteItemUrl = itemInfo->getRemoteItemUrl(),
               *remoteThumbUrl = itemInfo->getRemoteThumbUrl(),
               *remotePreviewUrl = itemInfo->getRemotePreviewUrl(),
               *localThumbPath   = itemInfo->getLocalThumbPath(),
               *localPreviewPath = itemInfo->getLocalPreviewPath(),
               *contentType      = itemInfo->getContentType(),
               *localItemPath    = itemInfo->getLocalItemPath(),
               *remoteItemETag   = itemInfo->getRemoteItemETag().c_str(),
               *remoteThumbETag  = itemInfo->getRemoteThumbETag().c_str(),
               *remotePreviewETag= itemInfo->getRemotePreviewETag().c_str(),
               *localItemETag    = itemInfo->getLocalItemETag().c_str(),
               *localThumbETag   = itemInfo->getLocalThumbETag().c_str(),
               *localPreviewETag = itemInfo->getLocalPreviewETag().c_str();
    
    MHItemExifData exifData = itemInfo->getItemExifData();
    StringBuffer exifMaker = exifData.maker.c_str() != NULL ? exifData.maker.c_str() : "";
    
    StringBuffer exifModel = exifData.model.c_str() != NULL ? exifData.model.c_str() : "";
    
    int64_t exif_creation_date = exifData.creation_date;
    int exif_image_width  = exifData.image_width;
    int exif_image_length = exifData.image_length;
    
    MHItemVideoMetaData videoMetaData = itemInfo->getItemVideoMetadata();
    StringBuffer videoCodec = videoMetaData.codec.c_str() != NULL ? videoMetaData.codec.c_str() : "";
    unsigned long video_duration = videoMetaData.duration;
    unsigned int video_bitrate = videoMetaData.bitrate;
    unsigned video_height = videoMetaData.height;
    unsigned video_width  = videoMetaData.width;
    
    int shared_via_email = itemInfo->getShared() ? 1 : 0;
    int num_upload_failures = itemInfo->getNumUploadFailures();
    uint32_t validationStatus = itemInfo->getValidationStatus();

    StringBuffer shared_services = itemInfo->formatExportedServices();
    
    char* queryBuilt = sqlite3_mprintf(insertQuery.c_str(), luid ? luid : "",
                                   guid ? guid : "", 
                                   name.c_str(), 
                                   size, 
                                   creationDate, 
                                   modificationDate,
                                   contentType ? contentType : "",
                                   status, 
                                   serverLastUpdate,
                                   serverUrl ? serverUrl : "", 
                                   remoteItemUrl ? remoteItemUrl : "", 
                                   remoteThumbUrl ? remoteThumbUrl : "",
                                   remotePreviewUrl ? remotePreviewUrl : "", 
                                   localThumbPath ? localThumbPath : "", 
                                   localPreviewPath ? localPreviewPath : "",
                                   exifMaker.c_str(),
                                   exifModel.c_str(),
                                   exif_image_width, 
                                   exif_image_length,
                                   exif_creation_date,
                                   videoCodec.c_str(),
                                   video_duration, 
                                   video_bitrate, 
                                   video_height, 
                                   video_width,
                                   shared_services.empty() ? "" : shared_services.c_str(),
                                   shared_via_email, num_upload_failures,
                                   localItemPath ? localItemPath : "",
                                   remoteItemETag ? remoteItemETag : "",
                                   remoteThumbETag ? remoteThumbETag : "",
                                   remotePreviewETag ? remotePreviewETag : "",
                                   localItemETag ? localItemETag : "",
                                   localThumbETag ? localThumbETag : "",
                                   localPreviewETag ? localPreviewETag : "",
                                   validationStatus);
    stmt.sprintf("%s", queryBuilt); 
    sqlite3_free(queryBuilt);
    return stmt;
}

StringBuffer MHItemsStore::formatUpdateItemStmt(MHStoreEntry* entry)
{
    StringBuffer update_row_stmt("");
    
    if (entry == NULL) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return update_row_stmt;
    }
    
    MHSyncItemInfo* itemInfo = dynamic_cast<MHSyncItemInfo*>(entry);
    if (itemInfo == NULL) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return update_row_stmt;
    }
    
    unsigned long itemId = itemInfo->getId();
    const char *luid = itemInfo->getLuid();
    const char *guid = itemInfo->getGuid();
    StringBuffer name = itemInfo->getName() != NULL ? itemInfo->getName() : "";
    
    int64_t size  = itemInfo->getSize();
    int64_t   serverLastUpdate = itemInfo->getServerLastUpdate();
    int64_t modificationDate = itemInfo->getModificationDate();
    int64_t creationDate = itemInfo->getCreationDate();
    int status = itemInfo->getStatus();
    const char *serverUrl = itemInfo->getServerUrl(),
               *remoteItemUrl = itemInfo->getRemoteItemUrl(),
               *remoteThumbUrl = itemInfo->getRemoteThumbUrl(),
               *remotePreviewUrl = itemInfo->getRemotePreviewUrl(),
               *localThumbPath   = itemInfo->getLocalThumbPath(),
               *localPreviewPath = itemInfo->getLocalPreviewPath(),
               *contentType      = itemInfo->getContentType(),
               *localItemPath    = itemInfo->getLocalItemPath(),
               *remoteItemETag   = itemInfo->getRemoteItemETag().c_str(),
               *remoteThumbETag  = itemInfo->getRemoteThumbETag().c_str(),
               *remotePreviewETag= itemInfo->getRemotePreviewETag().c_str(),
               *localItemETag    = itemInfo->getLocalItemETag().c_str(),
               *localThumbETag   = itemInfo->getLocalThumbETag().c_str(),
               *localPreviewETag = itemInfo->getLocalPreviewETag().c_str();
    
    MHItemExifData exifData = itemInfo->getItemExifData();
    
    StringBuffer exifMaker = exifData.maker.c_str() != NULL ? exifData.maker.c_str() : "";
    StringBuffer exifModel = exifData.model.c_str() != NULL ? exifData.model.c_str() : "";
    int64_t exif_creation_date = exifData.creation_date;
    int exif_image_width  = exifData.image_width;
    int exif_image_length = exifData.image_length;
    
    MHItemVideoMetaData videoMetaData = itemInfo->getItemVideoMetadata();
    StringBuffer videoCodec = videoMetaData.codec.c_str() != NULL ? videoMetaData.codec.c_str() : "";
    unsigned long video_duration = videoMetaData.duration;
    unsigned int video_bitrate = videoMetaData.bitrate;
    unsigned video_height = videoMetaData.height;
    unsigned video_width  = videoMetaData.width;

    StringBuffer shared_services = itemInfo->formatExportedServices();
    int shared_via_email = itemInfo->getShared() ? 1 : 0;
    int num_upload_failures = itemInfo->getNumUploadFailures();
    uint32_t validationStatus = itemInfo->getValidationStatus();
    
    // Log some info if some of the item 
    if (localPreviewETag == NULL || strlen(localPreviewETag) == 0) {
        LOG.info("%s: Setting local preview etag to empty value for %s,%d",__FUNCTION__,
                 name.c_str(),itemId);
    }
    if (localThumbETag == NULL || strlen(localThumbETag) == 0) {
        LOG.info("%s: Setting local thumb etag to empty value for %s,%d",__FUNCTION__,
                 name.c_str(),itemId);
    }
    if (localItemETag == NULL || strlen(localItemETag) == 0) {
        LOG.info("%s: Setting local item etag to empty value for %s,%d",__FUNCTION__,
                 name.c_str(),itemId);
    }
    
    char* queryBuilt = sqlite3_mprintf(updateQuery.c_str(),
                                   luid ? luid : "", 
                                   guid ? guid : "", 
                                   name.c_str(), 
                                   size, 
                                   creationDate, 
                                   modificationDate,
                                   contentType ? contentType : "",
                                   status, 
                                   serverLastUpdate,
                                   serverUrl ? serverUrl : "", 
                                   remoteItemUrl ? remoteItemUrl : "", 
                                   remoteThumbUrl ? remoteThumbUrl : "",
                                   remotePreviewUrl ? remotePreviewUrl : "", 
                                   localThumbPath ? localThumbPath : "", 
                                   localPreviewPath ? localPreviewPath : "",
                                   exifMaker.c_str(),
                                   exifModel.c_str(),
                                   exif_image_width,
                                   exif_image_length,
                                   exif_creation_date > 0 ? exif_creation_date : 0LL,
                                   videoCodec.c_str(),
                                   video_duration, 
                                   video_bitrate, 
                                   video_height, 
                                   video_width,
                                   shared_services.empty() ? "" : shared_services.c_str(),
                                   shared_via_email,
                                   num_upload_failures,
                                   localItemPath ? localItemPath : "",
                                   remoteItemETag ? remoteItemETag : "",
                                   remoteThumbETag ? remoteThumbETag : "",
                                   remotePreviewETag ? remotePreviewETag : "",
                                   localItemETag ? localItemETag : "",
                                   localThumbETag ? localThumbETag : "",
                                   localPreviewETag ? localPreviewETag : "",
                                   validationStatus,
                                       
                                   itemId);
    update_row_stmt.sprintf("%s", queryBuilt); 
    sqlite3_free(queryBuilt);

    
    return update_row_stmt;
}

int MHItemsStore::getCountOfItemsWithStatus(int status) {
    
    int res = SQLITE_OK;
    int count = 0;
    sqlite3_stmt *stmt = NULL;
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't get entries: cache is not initialized", __FUNCTION__);
        return 0;
    }
    
    StringBuffer query;
    query.sprintf(select_count_with_status, store_name.c_str(), status);
    
    pthread_mutex_lock(&store_access_mutex);
    res = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL );
    if (res  != SQLITE_OK) {
        pthread_mutex_unlock(&store_access_mutex);
        LOG.error("%s: error preparing SQL query: %s", __FUNCTION__, sqlite3_errmsg(db));
        return 0;
    }
    
    while ((sqlite3_step(stmt) == SQLITE_ROW))  {
        count = (int)sqlite3_column_int (stmt, 0);
    }
    
    sqlite3_finalize( stmt );
    pthread_mutex_unlock(&store_access_mutex);
    
    return count;
}

bool MHItemsStore::upgrade(int from, int to) {
    if (from == 110002) {
        addLocalItemPathOnUpgrade();
        from = 110003;
    }
    
    migrateItemStatus();

    // The upgrade procedure is only invoked when the application is run for the first time
    // after the upgrade. So there is no much performance penalty by always checking if this column exists
    addRemoteItemETagOnUpgrade();
    addRemoteThumbETagOnUpgrade();
    addRemotePreviewETagOnUpgrade();
    addLocalItemETagOnUpgrade();
    addLocalThumbETagOnUpgrade();
    addLocalPreviewETagOnUpgrade();
    addValidationStatusOnUpgrade();
    
    return true;
}

bool MHItemsStore::migrateItemStatus() {
    
    // RemoteThumbnail, RemotePreview and Synced become Remote
    
    // Begin a transaction
    sqlite3_exec(db, "BEGIN", 0, 0, 0);
    std::string update1 = "UPDATE ";
    update1.append(store_name.c_str());
    update1.append(" SET ")
    .append(status_field_name).append("=").append("1")
    .append(" WHERE (")
    .append(status_field_name).append("=").append("2")
    .append(" OR ")
    .append(status_field_name).append("=").append("3")
    .append(" OR ")
    .append(status_field_name).append("=").append("7")
    .append(")");
    int resUpdate = sqlite3_exec(db, update1.c_str(), NULL, NULL, NULL);
    if (resUpdate != SQLITE_OK) {
        LOG.error("%s: Cannot migrate status from RemoteThumbnail to Remote %s", __FUNCTION__, sqlite3_errmsg(db));
    }
    
    // Commit the transaction        
    sqlite3_exec(db, "COMMIT", 0, 0, 0);    
    return true;
}


bool MHItemsStore::addLocalItemPathOnUpgrade() {
    sqlite3_stmt *statement;
    // Check if the DB needs to be upgraded. Get the list of available columns
    const char* getColsSQL = "PRAGMA table_info(%s)";
    StringBuffer getColsQuery;
    getColsQuery.sprintf(getColsSQL, store_name.c_str());
    int ret = sqlite3_prepare_v2(db, getColsQuery.c_str(), -1, &statement, NULL );
    if (ret  != SQLITE_OK) {
        return false;
    }
    
    bool upgraded = false;
    while ((sqlite3_step(statement) == SQLITE_ROW)) {
        // The field name is in the first position
        const char* name = (const char*)sqlite3_column_text(statement, 1);
        if (strcmp(name,"local_item_path") == 0) {
            // The field is already available
            upgraded = true;
        }
    }
    sqlite3_finalize(statement);
    
    bool resAlter = true;
    bool resUpdate = true;
    if (!upgraded) {
        // Begin a transaction
        sqlite3_exec(db, "BEGIN", 0, 0, 0);
        
        const char* updateSQL = "ALTER TABLE %s ADD COLUMN local_item_path TEXT";
        StringBuffer alterSQL;
        alterSQL.sprintf(updateSQL, store_name.c_str());
        sqlite3_prepare_v2(db, alterSQL.c_str(), -1, &statement, NULL);
        resAlter = sqlite3_step(statement)==SQLITE_DONE;
        if (resAlter != SQLITE_OK) {
            LOG.error("%s: Cannot add column for local_item_path %s", __FUNCTION__, sqlite3_errmsg(db));
        }
        // Release the compiled statement from memory
        sqlite3_finalize(statement);
        
        // creation date and modification date are now in milliseconds, we need to convert the existing values
        std::string update = "UPDATE ";
        update.append(store_name.c_str());
        update.append(" SET ")
        .append(creation_date_field_name).append("=").append(creation_date_field_name).append(" * 1000")
        .append(",")
        .append(modification_date_name).append("=").append(modification_date_name).append(" * 1000");
        resUpdate = sqlite3_exec(db, update.c_str(), NULL, NULL, NULL);
        if (resUpdate != SQLITE_OK) {
            LOG.error("%s: Cannot migrate timestamps to milliseconds %s", __FUNCTION__, sqlite3_errmsg(db));
        }
        
        // Commit the transaction
        sqlite3_exec(db, "COMMIT", 0, 0, 0);
    }
    return resAlter && resUpdate;
}

bool MHItemsStore::addRemoteItemETagOnUpgrade() {
    return addColumnIfMissing(remote_item_etag, "TEXT", "-1");
}

bool MHItemsStore::addRemoteThumbETagOnUpgrade() {
    return addColumnIfMissing(remote_thumb_etag, "TEXT", "-1");
}

bool MHItemsStore::addRemotePreviewETagOnUpgrade() {
    return addColumnIfMissing(remote_preview_etag, "TEXT", "-1");
}


bool MHItemsStore::addLocalItemETagOnUpgrade() {
    return addColumnIfMissing(local_item_etag, "TEXT", "-1");
}

bool MHItemsStore::addLocalThumbETagOnUpgrade() {
    return addColumnIfMissing(local_thumb_etag, "TEXT", "-1");
}

bool MHItemsStore::addLocalPreviewETagOnUpgrade() {
    return addColumnIfMissing(local_preview_etag, "TEXT", "-1");
}

bool MHItemsStore::addValidationStatusOnUpgrade() {
    StringBuffer defaultValue;
    defaultValue.sprintf("%d", EValidationStatusValid);
    return addColumnIfMissing(validation_status, "INTEGER", defaultValue.c_str());
}

bool MHItemsStore::addColumnIfMissing(const char* newColName, const char* newColType, const char* defaultValue) {
    sqlite3_stmt *statement;
    // Check if the DB needs to be upgraded. Get the list of available columns
    const char* getColsSQL = "PRAGMA table_info(%s)";
    StringBuffer getColsQuery;
    getColsQuery.sprintf(getColsSQL, store_name.c_str());
    int ret = sqlite3_prepare_v2(db, getColsQuery.c_str(), -1, &statement, NULL );
    if (ret  != SQLITE_OK) {
        return false;
    }

    bool upgraded = false;
    while ((sqlite3_step(statement) == SQLITE_ROW)) {
        // The field name is in the first position
        const char* name = (const char*)sqlite3_column_text(statement, 1);
        if (strcmp(name,newColName) == 0) {
            // The field is already available
            upgraded = true;
        }
    }
    sqlite3_finalize(statement);
    
    bool resAlter = true;
    if (!upgraded) {
        // Begin a transaction
        sqlite3_exec(db, "BEGIN", 0, 0, 0);
        
        StringBuffer alterSQL;
        alterSQL.sprintf("ALTER TABLE %s ADD COLUMN %s %s",store_name.c_str(),newColName,newColType);
        if (defaultValue != NULL) {
            if (strcmp(newColType,"TEXT") == 0) {
                alterSQL.append(" DEFAULT '").append(defaultValue).append("'");
            } else {
                alterSQL.append(" DEFAULT ").append(defaultValue);
            }
        }
        
        sqlite3_prepare_v2(db, alterSQL.c_str(), -1, &statement, NULL);
        resAlter = sqlite3_step(statement)==SQLITE_DONE;
        if (!resAlter) {
            LOG.error("%s: Cannot add column %s because %s", __FUNCTION__, newColName, sqlite3_errmsg(db));
        }
        // Release the compiled statement from memory
        sqlite3_finalize(statement);

        // Commit the transaction
        sqlite3_exec(db, "COMMIT", 0, 0, 0);
    }
    return resAlter;
}

int32_t MHItemsStore::addLabelsToItem(MHSyncItemInfo* itemInfo, std::vector<MHLabelInfo*>* labels) {
    // Add the given entries into the bridge table
    if (labels == NULL) {
        return 0;
    }
    int count = 0;
    pthread_mutex_lock(&store_access_mutex);
    sqlite3_exec(db, "BEGIN", 0, 0, 0);
    std::vector<MHLabelInfo*>::iterator labelsIt = labels->begin();
    
    for(;labelsIt != labels->end();++labelsIt) {
        MHLabelInfo* label = *labelsIt;
        StringBuffer query; 
        query.sprintf(bridgeTableInsertQuery.c_str(), itemInfo->getId(),label->getLuid());
        // char query[1024];
        // snprintf(query,1024,bridgeTableInsertQuery.c_str(),itemInfo->getId(),label->getLuid());
        int resInsert = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
        if (resInsert != SQLITE_OK) {
            LOG.error("%s: Cannot add item-label association", __FUNCTION__, sqlite3_errmsg(db));
        } else {
            count++;
        }
    }
    sqlite3_exec(db, "COMMIT", 0, 0, 0);
    pthread_mutex_unlock(&store_access_mutex);
    return count;
}

bool MHItemsStore::getItemLabels(MHSyncItemInfo* itemInfo, std::vector<uint32_t>& labelsId) {
    std::string bridgeTableSelectQuery;
    bridgeTableSelectQuery.append("SELECT * FROM ").append(itemsLabelsBridgeTableName)
                          .append(" WHERE ").append(itemsLabelsBridgeItemIdFieldName).append("=").append("%lu");
    
    pthread_mutex_lock(&store_access_mutex);
    
    StringBuffer query; query.sprintf(bridgeTableSelectQuery.c_str(),itemInfo->getId());
    // char query[1024];
    // snprintf(query,1024,bridgeTableSelectQuery.c_str(),itemInfo->getId());
    
    sqlite3_stmt *stmt = NULL;
    int ret = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL );
    if (ret  != SQLITE_OK) {
        pthread_mutex_unlock(&store_access_mutex);
        LOG.error("%s: error preparing SQL query: %s", __FUNCTION__, sqlite3_errmsg(db));
        return false;
    }
    
    while ((sqlite3_step(stmt) == SQLITE_ROW)) {
        uint32_t labelId = sqlite3_column_int(stmt, 2);
        labelsId.push_back(labelId);
    }
    
    sqlite3_finalize( stmt );
    pthread_mutex_unlock(&store_access_mutex);
    return true;
}

bool MHItemsStore::getAllLabelsForItems(std::vector<std::pair<uint32_t,uint32_t> >& labelsId) {
    
    std::string bridgeTableSelectQuery;
    bridgeTableSelectQuery.append("SELECT COUNT(*), ").append(itemsLabelsBridgeLabelIdFieldName).append(" FROM ")
                          .append(itemsLabelsBridgeTableName).append(",").append(store_name.c_str())
                          .append(" WHERE ")
                          .append(itemsLabelsBridgeItemIdFieldName).append("=")
                          .append(store_name.c_str()).append(".").append("id")
                          .append(" GROUP BY ").append(itemsLabelsBridgeLabelIdFieldName);
    
    pthread_mutex_lock(&store_access_mutex);
    
    sqlite3_stmt *stmt = NULL;
    int ret = sqlite3_prepare_v2(db, bridgeTableSelectQuery.c_str(), -1, &stmt, NULL );
    if (ret  != SQLITE_OK) {
        pthread_mutex_unlock(&store_access_mutex);
        LOG.error("%s: error preparing SQL query: %s", __FUNCTION__, sqlite3_errmsg(db));
        return false;
    }
    
    while ((sqlite3_step(stmt) == SQLITE_ROW)) {
        uint32_t count = sqlite3_column_int(stmt, 0);
        uint32_t labelId = sqlite3_column_int(stmt, 1);
        std::pair<uint32_t,uint32_t> v;
        v.first = labelId;
        v.second = count;
        labelsId.push_back(v);
    }
    
    sqlite3_finalize( stmt );
    pthread_mutex_unlock(&store_access_mutex);
    return true;    
}

int32_t MHItemsStore::clearItemLabels(MHSyncItemInfo* itemInfo) {
    StringBuffer sql;
    sql.sprintf("DELETE FROM %s WHERE %s=%ld", itemsLabelsBridgeTableName, itemsLabelsBridgeItemIdFieldName,
                itemInfo->getId());
    
    pthread_mutex_lock(&store_access_mutex);
    int ret = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    int changes = sqlite3_changes(db);
    pthread_mutex_unlock(&store_access_mutex);
    
    if (ret != SQLITE_OK) {
        LOG.error("%s: error executing SQL statement: %s", __FUNCTION__, sqlite3_errmsg(db));
        return -1;
    }
    return changes;
}


END_FUNAMBOL_NAMESPACE
