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

#include "MediaHub/ExternalServicesAlbumStore.h"
#include "MediaHub/ExternalServiceAlbum.h"
#include "base/Log.h"
#include "string.h"
#include "errno.h"

BEGIN_FUNAMBOL_NAMESPACE

const char* ExternalServicesAlbumStore::serviceName_field_name         = "service_name";
const char* ExternalServicesAlbumStore::albumId_field_name             = "album_id";
const char* ExternalServicesAlbumStore::albumName_field_name           = "album_name";
const char* ExternalServicesAlbumStore::albumPrivacy_field_name        = "album_privacy";

const char* ExternalServicesAlbumStore::create_table_stmt_fmt = "CREATE TABLE %s (%s TEXT, %s TEXT, "  \
                                                           " %s TEXT, %s TEXT, PRIMARY KEY (%s, %s))";

const char* ExternalServicesAlbumStore::insert_row_stmt_fmt   = "INSERT INTO %s (%s, %s, %s, %s)";

const char* ExternalServicesAlbumStore::row_values_fmt        = "VALUES (\"%q\", \"%q\", \"%q\", \"%q\")";

const char* ExternalServicesAlbumStore::select_albums_fmt     = "SELECT * from %s where service_name = \"%s\"";

const char* ExternalServicesAlbumStore::delete_albums_fmt     = "DELETE from %s where service_name = \"%s\"";

//const char* ExternalServicesStore::update_set_stmt_fmt   = "SET %s=\"%s\", %s=\"%s\", %s=%d, %s=\"%s\", %s=\"%s\", " \
//                                                           "%s=\"%s\", %s=\"%s\", %s=\"%s\", %s=\"%s\", %s=\"%s\", " \
//                                                           "%s=\"%s\", %s=%d, %s=%d, %s=\"%s\", %s=\"%s\", %s=\"%s\"";



ExternalServicesAlbumStore::ExternalServicesAlbumStore(const char* storeName, const char* storePath) : MHStore(storeName, storePath) 
{
    if (store_status == store_status_not_initialized) {
        // db file was just created: init table
        LOG.debug("%s: initializing table for store %s", __FUNCTION__, store_name.c_str());
        if (!initialize_table()) {
            LOG.debug("%s: error initializing table for db: %s", __FUNCTION__, sqlite3_errmsg(db));
        }
        // ignoring error in initialize_table (can't catch the 'error already exist')
        store_status = store_status_initialized;
    }
    
    if (store_status == store_status_initialized) {
        initialize_static_queries();
    }
}

ExternalServicesAlbumStore::~ExternalServicesAlbumStore() {}

bool ExternalServicesAlbumStore::initialize_table()
{
    int ret = SQLITE_OK;
    StringBuffer sql;
  
    sql.sprintf(create_table_stmt_fmt,          store_name.c_str(),         serviceName_field_name, 
                albumId_field_name,             albumName_field_name,       albumPrivacy_field_name,
                serviceName_field_name,         albumId_field_name);
    LOG.debug("table create album: %s", sql.c_str());
    ret = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    
    return (ret == SQLITE_OK);
}

void ExternalServicesAlbumStore::initialize_static_queries() 
{
    insert_row_stmt.sprintf(insert_row_stmt_fmt,            store_name.c_str(),         serviceName_field_name, 
                            albumId_field_name,             albumName_field_name,       albumPrivacy_field_name);
    LOG.debug("query 1: %s", insert_row_stmt.c_str());
    select_count_stmt.sprintf(select_count_stmt_fmt, serviceName_field_name, store_name.c_str());
    LOG.debug("query 2: %s", select_count_stmt.c_str());
    
    select_all_stmt.sprintf(select_all_stmt_fmt, store_name.c_str());
    LOG.debug("query 1: %s", select_all_stmt.c_str());
        
    
}

StringBuffer ExternalServicesAlbumStore::formatUpdateItemStmt(MHStoreEntry* entry) {
    LOG.error("%s: not yet implemented!", __FUNCTION__);
    return "";
}


bool ExternalServicesAlbumStore::RemoveEntry(MHStoreEntry* entry)
{
    LOG.error("%s: not yet implemented!", __FUNCTION__);
    return false;
}





bool ExternalServicesAlbumStore::getAllEntries(CacheItemsList& entryList)
{
    //LOG.debug("%s: entering", __FUNCTION__);
    
    int ret = SQLITE_OK;
    sqlite3_stmt *stmt = NULL;
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't get entries: cache is not initialized", __FUNCTION__);
        return false;
    }
    
    pthread_mutex_lock(&store_access_mutex);
    
    ret = sqlite3_prepare_v2(db, select_all_stmt.c_str(), -1, &stmt, NULL );
    if (ret  != SQLITE_OK) {
        pthread_mutex_unlock(&store_access_mutex);
        LOG.error("%s: error preparing SQL query: %s", __FUNCTION__, sqlite3_errmsg(db));
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


CacheItemsList* ExternalServicesAlbumStore::getAlbums(const char* serviceName) const
{
    int ret = SQLITE_OK;
    sqlite3_stmt *stmt = NULL;
    CacheItemsList* albums = NULL;
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't get entries: cache is not initialized", __FUNCTION__);
        return NULL;
    }
    
    pthread_mutex_lock(&store_access_mutex);
    
    StringBuffer select_albums_stmt;
    select_albums_stmt.sprintf(select_albums_fmt, store_name.c_str(), serviceName); 
    LOG.debug("query %s", select_albums_stmt.c_str());
    ret = sqlite3_prepare_v2(db, select_albums_stmt.c_str(), -1, &stmt, NULL );
    if (ret  != SQLITE_OK) {
        pthread_mutex_unlock(&store_access_mutex);
        LOG.error("%s: error preparing SQL query: %s", __FUNCTION__, sqlite3_errmsg(db));
        return NULL;
    }
    
    while ((sqlite3_step(stmt) == SQLITE_ROW)) 
    {        
        MHStoreEntry* entry = readEntry(stmt);
        if (entry) {
            if (albums == NULL) { albums = new CacheItemsList(); }
            albums->addItem(entry);
        }
    }
    
    sqlite3_finalize( stmt );
    pthread_mutex_unlock(&store_access_mutex);
    
    return albums;
}


bool ExternalServicesAlbumStore::removeAlbums(const char* serviceName) {

    int ret = SQLITE_OK;
      
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't get entries: cache is not initialized", __FUNCTION__);
        return false;
    }

    StringBuffer delete_albums_stmt;
    delete_albums_stmt.sprintf(delete_albums_fmt, store_name.c_str(), serviceName); 
    LOG.debug("Delete query: %s", delete_albums_stmt.c_str());
    
    pthread_mutex_lock(&store_access_mutex);
     ret = sqlite3_exec(db, delete_albums_stmt.c_str(), NULL, NULL, NULL );
    pthread_mutex_unlock(&store_access_mutex);
    
    if (ret != SQLITE_OK) {
        LOG.error("%s: error preparing SQL query: %s", __FUNCTION__, sqlite3_errmsg(db));
        return false;
    }
    return true;
    
}

MHStoreEntry* ExternalServicesAlbumStore::readEntry(sqlite3_stmt *stmt) const
{
    if (!stmt) {
        LOG.error("%s: null sql statement", __FUNCTION__);
        return NULL;
    }
    
    const char* serviceName         = (const char*)sqlite3_column_text(stmt, 0);
    const char* albumId             = (const char*)sqlite3_column_text(stmt, 1);
    const char* albumName           = (const char*)sqlite3_column_text(stmt, 2);
    const char* albumPrivacy        = (const char*)sqlite3_column_text(stmt, 3);
        
    if (!serviceName || !strlen(serviceName)) {
        LOG.error("%s: key field %s is empty or NULL", __FUNCTION__, serviceName_field_name);
        return NULL;
    }
    
    if (!albumId || !strlen(albumId)) {
        LOG.error("%s: key field %s is empty or NULL", __FUNCTION__, albumId_field_name);
        return NULL;
    }
    
    ExternalServiceAlbum* service = new ExternalServiceAlbum(serviceName, albumId);
    
    service->setAlbumName(albumName);
    service->setPrivacy(albumPrivacy);
         
    return service;
}


StringBuffer ExternalServicesAlbumStore::formatInsertItemStmt(MHStoreEntry* entry)
{
    StringBuffer stmt("");
    
    if (entry == NULL) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return stmt;
    }
    ExternalServiceAlbum* service = (ExternalServiceAlbum*)entry;
    if (service == NULL) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return stmt;
    }
    
    char* queryBuilt = sqlite3_mprintf(row_values_fmt, 
                   service->getServiceName()   ? service->getServiceName() : "",
                   service->getAlbumId()       ? service->getAlbumId()     : "",
                   service->getAlbumName()     ? service->getAlbumName()   : "",
                   service->getPrivacy()       ? service->getPrivacy()     : "");
    stmt.sprintf("%s %s", insert_row_stmt.c_str(), queryBuilt); 
    sqlite3_free(queryBuilt);
    
    LOG.debug("%s: insert statement = '%s'", __FUNCTION__, stmt.c_str());
    return stmt;
}

END_FUNAMBOL_NAMESPACE
