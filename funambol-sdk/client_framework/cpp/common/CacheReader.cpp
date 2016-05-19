/*
 * Funambol is a mobile platform developed by Funambol, Inc.
 * Copyright (C) 2009 Funambol, Inc.
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

#include "CacheReader.h"
#include "base/adapter/PlatformAdapter.h"
#include "base/util/StringBuffer.h"
#include "SourcePluginManager.h"
#include "SourcePlugin.h"

namespace Funambol {

CacheReader* CacheReader::instance = NULL;


CacheReader::CacheReader()
{
}

CacheReader::~CacheReader()
{  
}

CacheReader* CacheReader::getInstance(DMTClientConfig* clientConfig)
{
    if (!instance) {        
        instance = new CacheReader();
        instance->init(clientConfig);
    }
    
    return instance;    
}

void CacheReader::init(DMTClientConfig* clientConfig)
{
    StringBuffer cache_path;
    const char* configFolder = PlatformAdapter::getConfigFolder();
    
    pluginManager = SourcePluginManager::getInstance(clientConfig);
    cache_path.reset();
    cache_path.sprintf("%s/%s.db", configFolder, COMMANDS_REPORT_CACHE_NAME);
    commandReportStore = new CommandReportStore(COMMANDS_REPORT_CACHE_NAME, cache_path.c_str());

}

bool CacheReader::getItemsFromCache(const char* syncSourceName, CacheItemsList &itemsList){
    if ((syncSourceName == NULL) || (strlen(syncSourceName) == 0)) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        
        return false;
    }
    
    SourcePlugin* source = pluginManager->getSourceByName(syncSourceName);
    return getItemsFromCache(source, itemsList);
}

bool CacheReader::getItemsFromCache(SourcePlugin* source, CacheItemsList& itemsList) {
    MHItemsStore* itemsStore = source->getItemsCache();
    if (itemsStore != NULL) {
        SourcePlugin::OrderField orderField = source->getOrderField();
        
        switch (orderField) {
            case SourcePlugin::orderByModificationDate:
                return itemsStore->getAllEntries(itemsList, 
                        MHItemsStore::modification_date_name, source->getOrderFieldDescending());
                
            case SourcePlugin::orderByCreationDate:
            default:        
                return itemsStore->getAllEntries(itemsList,
                        MHItemsStore::creation_date_field_name, source->getOrderFieldDescending());
        }
    }
    
    LOG.error("%s: unsupported source '%s'", __FUNCTION__, source->getName());
     
    return false;
}

MHSyncItemInfo* CacheReader::getItemFromCache(const char* syncSourceName, const unsigned long itemID) {
    if ((syncSourceName == NULL) || (strlen(syncSourceName) == 0)) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        
        return NULL;
    }
    
    SourcePlugin* source = pluginManager->getSourceByName(syncSourceName);
    return getItemFromCache(source, itemID);
}
    
MHSyncItemInfo* CacheReader::getItemFromCache(SourcePlugin* source, const unsigned long itemID) {
    MHItemsStore* itemsStore = source->getItemsCache();
    if (itemsStore != NULL) {
        return (MHSyncItemInfo*)itemsStore->getEntry(itemID);
    }
    LOG.error("%s: unsupported source '%s'", __FUNCTION__, source->getName());
        
    return NULL;
}

int CacheReader::getCountFromCacheForSource(const char* sourceName)
{
    if ((sourceName == NULL) || (strlen(sourceName) == 0)) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        
        return -1;
    }
    
    SourcePlugin* source = pluginManager->getSourceByName(sourceName);
    return getCountFromCacheForSource(source);
}
    
int CacheReader::getCountFromCacheForSource(SourcePlugin* source)
{
    if (source == NULL) {
        return -1;
    }
    
    MHItemsStore* itemsStore = source->getItemsCache();
    if (itemsStore != NULL) {
        return itemsStore->getCount();
    }
    
    LOG.error("%s: unsupported source '%s'", __FUNCTION__, source->getName());
    
    return -1;
}

MHItemsStore* CacheReader::getCache(const char* sourceName)
{
    if ((sourceName == NULL) || (strlen(sourceName) == 0)) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return NULL;
    }
    
    SourcePlugin* source = pluginManager->getSourceByName(sourceName);
    return getCache(source);
}

MHItemsStore* CacheReader::getCache(SourcePlugin* source)
{
    if (source == NULL) {
        return NULL;
    }
    
    return source->getItemsCache();
}


MHItemsStore* CacheReader::getExcludedItemsCache(const char* sourceName)
{
    if ((sourceName == NULL) || (strlen(sourceName) == 0)) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return NULL;
    }
    
    SourcePlugin* source = pluginManager->getSourceByName(sourceName);
    return getExcludedItemsCache(source);
}
 
MHItemsStore* CacheReader::getExcludedItemsCache(SourcePlugin* source)
{
    if (source == NULL) {
        return NULL;
    }
    
    return source->getExcludedItemsCache();
}

CommandReportStore* CacheReader::getCommandReportStore()
{
    return commandReportStore;
}

}