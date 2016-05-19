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

#include "SourcePlugin.h"
#include "base/adapter/PlatformAdapter.h"
#include "base/util/StringBuffer.h"


USE_FUNAMBOL_NAMESPACE

SourcePlugin::SourcePlugin(const char* tag, const char* label, const char* name)
                                             : sourceTag(tag),
                                               sourceLabel(label),
                                               sourceName(name),
                                               orderField(SourcePlugin::orderByCreationDate),
                                               cache(NULL),
                                               excludedCache(NULL),
                                               pendingDeletesCache(NULL),
                                               media(false), pimSyncSource(NULL),
                                               mediaSyncSource(NULL),
                                               maxOutgoingItemSize(0),
                                               quotaThreshold(0),
                                               selectiveUploadEnabled(true),
                                               fillExcludedItemsMetadata(false),
                                               requiresThumbnailRefreshOnUploadTermination(false),
                                               showItemsCount(true),
                                               orderFieldDescending(true)
{
}

SourcePlugin::~SourcePlugin()
{
}

const char* SourcePlugin::getTag() const {
    return sourceTag.c_str();
}

const char* SourcePlugin::getName() const {
    return sourceName.c_str();
}

const char* SourcePlugin::getLabel() const {
    return sourceLabel.c_str();
}

const char* SourcePlugin::getSapiUri() const {
    return sapiUri.c_str();
}

const char* SourcePlugin::getSapiArrayKey() const {
    return sapiArrayKey.c_str();
}

SourcePlugin::OrderField SourcePlugin::getOrderField() const {
    return orderField;
}

const char* SourcePlugin::getOrderFieldAsString() const {
    switch (orderField) {
        case orderByCreationDate:
            return MHMediaRequestManager::sortByCreationDateToken;

        case orderByModificationDate:
            return MHMediaRequestManager::sortByModificationDateToken;

        default:
            break;
    }

    return MHMediaRequestManager::sortByCreationDateToken;
}

bool SourcePlugin::getOrderFieldDescending() const
{
    return orderFieldDescending;
}

bool SourcePlugin::getActive() const {
    return active;
}

void SourcePlugin::setActive(bool active) {
    this->active = active;
}

bool SourcePlugin::isMedia() const {
    return media;
}

int SourcePlugin::getId() const {
    return sourceId;
}

void SourcePlugin::setId(int sourceId) {
    this->sourceId = sourceId;
}

MHItemsStore* SourcePlugin::getItemsCache() const {
    return cache;
}

/*
 * This method returns the cache for the excluded items belonging to this plugin. Note that
 * it is not required that each plugin has an item cache. In such a case NULL is returned
 */
MHItemsStore* SourcePlugin::getExcludedItemsCache() const {
    return excludedCache;
}

MHItemsStore* SourcePlugin::getPendingDeletesCache() const {
    return pendingDeletesCache;
}

MHLabelsStore* SourcePlugin::getLabelsStore() const {
    return labelsStore;
}

bool SourcePlugin::getSelectiveUploadEnabled() const {
    return selectiveUploadEnabled;
}

bool SourcePlugin::getFillExcludedItemsMetadata() const {
    return fillExcludedItemsMetadata;
}


bool SourcePlugin::init() {
    return false;
}

bool SourcePlugin::canDownloadItem(MHSyncItemInfo* itemInfo) {
    return true;
}

SyncSource* SourcePlugin::createPIMSyncSource() {
    return NULL;    
}

MHSyncSource* SourcePlugin::createMHSyncSource() {
    return NULL;    
}

void SourcePlugin::fillInSourceDefaultUploadFolders() {
    sourceDefaultUploadFolders.clear();
}

long SourcePlugin::getTotalNumberOfLocalItems() {
    return 0;
}

void SourcePlugin::cleanup()
{
    if (cache)               cache->removeAllEntries();
    if (excludedCache)       excludedCache->removeAllEntries();
    if (pendingDeletesCache) pendingDeletesCache->removeAllEntries();
    
    if (media && mediaSyncSource) {
        mediaSyncSource->cleanup();
    }
}

long SourcePlugin::getMaxOutgoingItemSize() {
    return maxOutgoingItemSize;
}

long long SourcePlugin::disableOnAvailableQuotaThreshold() {
    return quotaThreshold;
}

bool SourcePlugin::requiresDeepSync() {
    return true;
}

bool SourcePlugin::getShowItemsCount() {
    return showItemsCount;
}

bool SourcePlugin::getDeferredDeleteDetection() {
    return hasDeferredDeleteDetection;
}

bool SourcePlugin::updateDeletedStatus(unsigned long cacheId) {
    return false;
}

//UI related classes
const char* SourcePlugin::getEnableSourceText() const {
    return NULL;
}

const char* SourcePlugin::getUploadSourceText() const {
    return NULL;
}

const char* SourcePlugin::getUploadSourceDetailsText() const {
    return NULL;
}

const char* SourcePlugin::getUploadNoteText() const {
    return NULL;
}

const char* SourcePlugin::getDownloadSourceText() const {
    return NULL;
}

const char* SourcePlugin::getDownloadSourceDetailsText() const {
    return NULL;
}

const char* SourcePlugin::getAutoDownloadSourceText() const {
    return NULL;
}

const char* SourcePlugin::getSourceDestFolderText() const {
    return NULL;
}

const char* SourcePlugin::getToolbarIdentifier() const {
    return NULL;
}

const char* SourcePlugin::getToolbarIconPath() const {
    return NULL;
}

const char* SourcePlugin::getToolbarWarningIconPath() const {
    return NULL;
}

const char* SourcePlugin::getToolbarTitle() const {
    return NULL;
}

const char* SourcePlugin::getSourceColumnHeaderTitle() const {
    return NULL;
}

const std::vector<std::string>& SourcePlugin::getSourceDefaultUploadFolders() const {
    return sourceDefaultUploadFolders;
}

const char* SourcePlugin::getDefaultSyncDirection() const {
    return NULL;
}

