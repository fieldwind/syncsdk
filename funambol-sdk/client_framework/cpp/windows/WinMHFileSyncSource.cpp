// 
// Funambol is a mobile platform developed by Funambol, Inc.
// Copyright (C) 2012 Funambol, Inc.
// 
// This program is a free software; you can redistribute it and/or modify it pursuant to
// the terms of the GNU Affero General Public License version 3 as published by
// the Free Software Foundation with the addition of the following provision
// added to Section 15 as permitted in Section 7(a): FOR ANY PART OF THE COVERED
// WORK IN WHICH THE COPYRIGHT IS OWNED BY FUNAMBOL, FUNAMBOL DISCLAIMS THE
// WARRANTY OF NON INFRINGEMENT OF THIRD PARTY RIGHTS.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; INCLUDING ANY WARRANTY OF MERCHANTABILITY OR FITNESS
// FOR A PARTICULAR PURPOSE, TITLE, INTERFERENCE WITH QUITE ENJOYMENT. THE PROGRAM
// IS PROVIDED “AS IS” WITH ALL FAULTS. Refer to the GNU General Public License for more
// details.
// 
// You should have received a copy of the GNU Affero General Public License
// along with this program; if not, see http://www.gnu.org/licenses or write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA 02110-1301 USA.
// 
// You can contact Funambol, Inc. headquarters at 1065 East Hillsdale Blvd., Suite
// 400, Foster City, CA 94404, USA, or at email address info@funambol.com.
// 
// The interactive user interfaces in modified source and object code versions
// of this program must display Appropriate Legal Notices, pursuant to
// Section 5 of the GNU Affero General Public License version 3.
// 
// In accordance with Section 7(b) of the GNU Affero General Public License
// version 3, these Appropriate Legal Notices must retain the display of the
// "Powered by Funambol" logo. If the display of the logo is not reasonably
// feasible for technical reasons, the Appropriate Legal Notices must display
// the words "Powered by Funambol".
// 

#include "WinMHFileSyncSource.h"
#include "base/util/StringBuffer.h"
#include "event/FireEvent.h"
#include "base/util/WString.h"
#include "ioStream/FileInputStream.h"
#include "ClientConfig.h"
#include "client/DMTClientConfig.h"



using namespace Funambol;

WinMHFileSyncSource::WinMHFileSyncSource(const char* sapiSourceUri,
                                                 const char* sapiArrayKey,
                                                 const char* orderFieldValue,
                                                 SyncSourceConfig& sc, SyncSourceReport& rep, 
                                                 MHItemsStore* itemsStore,
                                                 MHLabelsStore* labelsStore,
                                                 size_t incomingFilterDate,
                                                 size_t outgoingFilterDate,
                                                 StringBuffer& tSpoolPath,
                                                 DMTClientConfig* clientConfig_)
                                    : MHFileSyncSource(sapiSourceUri, sapiArrayKey, orderFieldValue, sc, rep,
                                                   itemsStore, labelsStore, incomingFilterDate, outgoingFilterDate, tSpoolPath,
                                                   clientConfig_)
{
}

WinMHFileSyncSource::~WinMHFileSyncSource() 
{
}

void WinMHFileSyncSource::requestSourceOperation(unsigned componentType, MHSyncItemInfo* itemInfo)
{
}

DownloadProgressObserver* WinMHFileSyncSource::createDowloadProgressObserver(MHSyncItemInfo *itemInfo, const char* sourceName)
{
    return NULL;
}


InputStream* WinMHFileSyncSource::createInputStream(MHSyncItemInfo& itemInfo)
{
    FileInputStream* inputStream = NULL;
    
    const char* luid = itemInfo.getLuid().c_str();
    const char* itemPath = itemInfo.getLocalItemPath().c_str();
    
    if (luid == NULL) {
        LOG.error("[%s] the luid is null", __FUNCTION__);
        
        return inputStream;
    }
   
    if ((itemPath == NULL) || (strlen(itemPath) == 0)) {
        LOG.error("%s: can't get local item path for input stream creation", 
            __FUNCTION__);
    
        return inputStream;
    }
    
    inputStream = new FileInputStream(itemPath); 
    
    return inputStream;
}


void WinMHFileSyncSource::sourceStatusUpdate(unsigned sourceStatus, MHSyncItemInfo* itemInfo)
{
    // write to queue
}