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

#include "MediaHub/ExternalServicesStore.h"
#include "MediaHub/ExternalService.h"
#include "base/Log.h"
#include "string.h"
#include "errno.h"

BEGIN_FUNAMBOL_NAMESPACE

const char* ExternalServicesStore::serviceName_field_name           = "service_name";
const char* ExternalServicesStore::displayName_field_name           = "display_name";
const char* ExternalServicesStore::authorized_field_name            = "authorized";
const char* ExternalServicesStore::authUrl_field_name               = "auth_url";
const char* ExternalServicesStore::iconUrl_field_name               = "icon_url";
const char* ExternalServicesStore::iconPath_field_name              = "icon_path";
const char* ExternalServicesStore::accountName_field_name           = "account_name";
const char* ExternalServicesStore::sources_field_name               = "sources";
const char* ExternalServicesStore::apiKey_field_name                = "api_key";
const char* ExternalServicesStore::itemAttributes_field_name        = "item_attributes";
const char* ExternalServicesStore::itemPrivacy_field_name           = "item_privacy";
const char* ExternalServicesStore::lastUsedItemPrivacy_field_name   = "last_used_item_privacy";
const char* ExternalServicesStore::exportMultiple_field_name        = "export_multiple";
const char* ExternalServicesStore::hasAlbums_field_name             = "has_albums";
const char* ExternalServicesStore::lastUsedAlbum_field_name         = "last_used_album";
const char* ExternalServicesStore::albumPrivacy_field_name          = "album_privacy";
const char* ExternalServicesStore::albumAttributes_field_name       = "album_attributes";


const char* ExternalServicesStore::create_table_stmt_fmt = "CREATE TABLE %s (%s TEXT PRIMARY KEY, %s TEXT, "    \
                                                           "%s INTEGER, %s TEXT, %s TEXT, %s TEXT, %s TEXT, "   \
                                                           "%s TEXT, %s TEXT, %s TEXT, %s TEXT, %s TEXT, "      \
                                                           "%s INTEGER, %s INTEGER, %s TEXT, %s TEXT, %s TEXT)";

const char* ExternalServicesStore::insert_row_stmt_fmt   = "INSERT INTO %s (%s, %s, %s, %s, %s, %s, %s," \
                                                           "%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)";

const char* ExternalServicesStore::row_values_fmt        = "VALUES (\"%q\", \"%q\", %d, \"%q\", \"%q\", \"%q\", \"%q\","  \
                                                           " \"%q\", \"%q\", \"%q\", \"%q\", \"%q\", %d, %d, \"%q\","     \
                                                           " \"%q\", \"%q\")";

const char* ExternalServicesStore::update_row_stmt_fmt   = "UPDATE %s %s where %s = \"%s\"";
const char* ExternalServicesStore::update_set_stmt_fmt   = "SET %s=\"%q\", %s=%d, %s=\"%q\", %s=\"%q\", %s=\"%q\","  \
                                                           "%s=\"%q\", %s=\"%q\", %s=\"%q\", %s=\"%q\", %s=\"%q\", " \
                                                           "%s=\"%q\", %s=%d, %s=%d, %s=\"%q\", %s=\"%q\", %s=\"%q\"";

ExternalServicesStore::ExternalServicesStore(const char* storeName, const char* storePath) : MHStore(storeName, storePath) 
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

ExternalServicesStore::~ExternalServicesStore() {}

bool ExternalServicesStore::initialize_table()
{
    int ret = SQLITE_OK;
    StringBuffer sql;

    sql.sprintf(create_table_stmt_fmt,          store_name.c_str(),             serviceName_field_name, 
                displayName_field_name,         authorized_field_name,          authUrl_field_name, 
                iconUrl_field_name,             iconPath_field_name,            accountName_field_name,     
                sources_field_name,             apiKey_field_name,              itemAttributes_field_name,  
                itemPrivacy_field_name,         lastUsedItemPrivacy_field_name, exportMultiple_field_name,  
                hasAlbums_field_name,           lastUsedAlbum_field_name,       albumPrivacy_field_name,    
                albumAttributes_field_name);
    
    //LOG.debug("%s: > SQL exec: '%s'", __FUNCTION__, sql.c_str());
    ret = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    
    return (ret == SQLITE_OK);
}

void ExternalServicesStore::initialize_static_queries() 
{
    insert_row_stmt.sprintf(insert_row_stmt_fmt,            store_name.c_str(),             serviceName_field_name, 
                            displayName_field_name,         authorized_field_name,          authUrl_field_name, 
                            iconUrl_field_name,             iconPath_field_name,            accountName_field_name,     
                            sources_field_name,             apiKey_field_name,              itemAttributes_field_name,  
                            itemPrivacy_field_name,         lastUsedItemPrivacy_field_name, exportMultiple_field_name,  
                            hasAlbums_field_name,           lastUsedAlbum_field_name,       albumPrivacy_field_name,    
                            albumAttributes_field_name);
    
    select_count_stmt.sprintf(select_count_stmt_fmt, serviceName_field_name, store_name.c_str());
    
    select_all_stmt.sprintf(select_all_stmt_fmt, store_name.c_str());
}


bool ExternalServicesStore::UpdateEntry(MHStoreEntry* entry)
{
    int ret = SQLITE_OK;
    ExternalService* externalService = NULL;
    
    if (entry == NULL) {
        LOG.error("%s: invalid parameter", __FUNCTION__);
        return false;
    }
    
    externalService = (ExternalService *)entry;
    
    if (externalService == NULL) {
        LOG.error("%s: invalid parameter", __FUNCTION__);
        return false;
    }

    if (store_status != store_status_initialized) {
        LOG.error("%s: can't add entry: cache is not initialized", __FUNCTION__);
        
        return false;
    }

    StringBuffer sql = formatUpdateItemStmt(entry);
    
    if (sql.empty()) {
        LOG.error("%s: error formatting cache insertion statement", __FUNCTION__);
        
        return false;
    }
    
    //LOG.debug("%s: > SQL exec: '%s'", __FUNCTION__, sql.c_str());
    pthread_mutex_lock(&store_access_mutex);
    ret = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    pthread_mutex_unlock(&store_access_mutex);
    
    if (ret != SQLITE_OK) {
        LOG.error("%s: error executing SQL statement: %s", __FUNCTION__, sqlite3_errmsg(db));
        
        return false;
    }

    return true;
}

bool ExternalServicesStore::RemoveEntry(MHStoreEntry* entry)
{
    LOG.error("%s: not yet implemented!", __FUNCTION__);
    return false;
}

bool ExternalServicesStore::getAllEntries(CacheItemsList& entryList)
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


MHStoreEntry* ExternalServicesStore::getEntry(const char* serviceName) const
{
    return getEntry(serviceName_field_name, serviceName);
}


MHStoreEntry* ExternalServicesStore::readEntry(sqlite3_stmt *stmt) const
{
    if (!stmt) {
        LOG.error("%s: null sql statement", __FUNCTION__);
        return NULL;
    }
    
    const char* serviceName         = (const char*)sqlite3_column_text(stmt, 0);
    const char* displayName         = (const char*)sqlite3_column_text(stmt, 1);
    bool        authorized          = (bool)       sqlite3_column_int (stmt, 2);
    const char* authUrl             = (const char*)sqlite3_column_text(stmt, 3);
    const char* iconUrl             = (const char*)sqlite3_column_text(stmt, 4);
    const char* iconPath            = (const char*)sqlite3_column_text(stmt, 5);
    const char* accountName         = (const char*)sqlite3_column_text(stmt, 6);
    const char* sources             = (const char*)sqlite3_column_text(stmt, 7);
    const char* apiKey              = (const char*)sqlite3_column_text(stmt, 8);
    const char* itemAttributes      = (const char*)sqlite3_column_text(stmt, 9);
    const char* itemPrivacy         = (const char*)sqlite3_column_text(stmt, 10);
    const char* lastUsedItemPrivacy = (const char*)sqlite3_column_text(stmt, 11);
    bool        exportMultiple      = (bool)       sqlite3_column_int (stmt, 12);
    bool        hasAlbums           = (bool)       sqlite3_column_int (stmt, 13);
    const char* lastUsedAlbum       = (const char*)sqlite3_column_text(stmt, 14);
    const char* albumPrivacy        = (const char*)sqlite3_column_text(stmt, 15);
    const char* albumAttributes     = (const char*)sqlite3_column_text(stmt, 16);
    
    if (!serviceName || !strlen(serviceName)) {
        LOG.error("%s: key field %s is empty or NULL", __FUNCTION__, serviceName_field_name);
        return NULL;
    }
    
    ExternalService* service = new ExternalService(serviceName);
    
    service->setDisplayName         (displayName);
    service->setAuthorized          (authorized);
    service->setAuthUrl             (authUrl);
    service->setIconUrl             (iconUrl);
    service->setIconPath            (iconPath);
    service->setAccountName         (accountName);
    service->setSources             (sources);
    service->setApiKey              (apiKey);
    service->setItemAttributes      (itemAttributes);
    service->setItemPrivacy         (itemPrivacy);
    service->setLastUsedItemPrivacy (lastUsedItemPrivacy);
    service->setExportMultiple      (exportMultiple);
    service->setHasAlbums           (hasAlbums);
    service->setLastUsedAlbum       (lastUsedAlbum);
    service->setAlbumPrivacy        (albumPrivacy);
    service->setAlbumAttributes     (albumAttributes);
    
    return service;
}


StringBuffer ExternalServicesStore::formatUpdateItemStmt(MHStoreEntry* entry)
{
    StringBuffer stmt("");
    
    if (entry == NULL) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return stmt;
    }
    
    ExternalService* service = (ExternalService*)entry;
    if (service == NULL) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return stmt;
    }
    
    char* queryBuilt = sqlite3_mprintf(update_set_stmt_fmt, 
                   displayName_field_name, service->getDisplayName() ? service->getDisplayName() : "",
                   authorized_field_name,  service->getAuthorized()  ? 1 : 0,
                   authUrl_field_name,     service->getAuthUrl()     ? service->getAuthUrl() : "",
                   iconUrl_field_name,     service->getIconUrl()            ? service->getIconUrl() : "",
                   iconPath_field_name, service->getIconPath()           ? service->getIconPath() : "",
                   accountName_field_name, service->getAccountName()        ? service->getAccountName() : "",
                   sources_field_name, service->getSources()            ? service->getSources() : "",
                   apiKey_field_name, service->getApiKey()             ? service->getApiKey() : "",
                   itemAttributes_field_name,  service->getItemAttributes()     ? service->getItemAttributes() : "",
                   itemPrivacy_field_name, service->getItemPrivacy()        ? service->getItemPrivacy() : "",
                   lastUsedItemPrivacy_field_name, service->getLastUsedItemPrivacy()? service->getLastUsedItemPrivacy() : "",
                   exportMultiple_field_name, service->getExportMultiple()     ? 1 : 0,
                   hasAlbums_field_name, service->getHasAlbums()          ? 1 : 0,
                   lastUsedAlbum_field_name, service->getLastUsedAlbum()      ? service->getLastUsedAlbum() : "",
                   albumPrivacy_field_name, service->getAlbumPrivacy()       ? service->getAlbumPrivacy() : "",
                   albumAttributes_field_name, service->getAlbumAttributes()    ? service->getAlbumAttributes() : "");
    
    stmt.sprintf(update_row_stmt_fmt, store_name.c_str(), queryBuilt, serviceName_field_name, service->getServiceName()); 
    sqlite3_free(queryBuilt);
    
    return stmt;
}

StringBuffer ExternalServicesStore::formatInsertItemStmt(MHStoreEntry* entry)
{
    StringBuffer stmt("");
    
    if (entry == NULL) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return stmt;
    }
    
    ExternalService* service = (ExternalService*)entry;
    if (service == NULL) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return stmt;
    }
    
    char* queryBuilt = sqlite3_mprintf(row_values_fmt, 
                   service->getServiceName()        ? service->getServiceName() : "",
                   service->getDisplayName()        ? service->getDisplayName() : "",
                   service->getAuthorized()         ? 1 : 0,
                   service->getAuthUrl()            ? service->getAuthUrl() : "",
                   service->getIconUrl()            ? service->getIconUrl() : "",
                   service->getIconPath()           ? service->getIconPath() : "",
                   service->getAccountName()        ? service->getAccountName() : "",
                   service->getSources()            ? service->getSources() : "",
                   service->getApiKey()             ? service->getApiKey() : "",
                   service->getItemAttributes()     ? service->getItemAttributes() : "",
                   service->getItemPrivacy()        ? service->getItemPrivacy() : "",
                   service->getLastUsedItemPrivacy()? service->getLastUsedItemPrivacy() : "",
                   service->getExportMultiple()     ? 1 : 0,
                   service->getHasAlbums()          ? 1 : 0,
                   service->getLastUsedAlbum()      ? service->getLastUsedAlbum() : "",
                   service->getAlbumPrivacy()       ? service->getAlbumPrivacy() : "",
                   service->getAlbumAttributes()    ? service->getAlbumAttributes() : "");
    stmt.sprintf("%s %s", insert_row_stmt.c_str(), queryBuilt); 
    sqlite3_free(queryBuilt);
    
    return stmt;
}

END_FUNAMBOL_NAMESPACE
