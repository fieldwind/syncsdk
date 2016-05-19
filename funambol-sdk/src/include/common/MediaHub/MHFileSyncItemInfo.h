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
#ifndef INCL_MH_FILE_SYNC_ITEM_INFO
#define INCL_MH_FILE_SYNC_ITEM_INFO

#include <iostream>
#include "MediaHub/MHSyncItemInfo.h"

BEGIN_FUNAMBOL_NAMESPACE

class MHFileSyncItemInfo : public MHSyncItemInfo {

public:
    MHFileSyncItemInfo(unsigned long id_,
                       const char* guid,
                       const char* luid,
                       const char* name,
                       size_t size,
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
                       const char* remotePreviewETag);
    
    bool isLocallyAvailable();
    
};


END_FUNAMBOL_NAMESPACE

#endif