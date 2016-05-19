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

#ifndef __SYNC_PROCESS_NOTIFICATION_MANAGER_H__
#define __SYNC_PROCESS_NOTIFICATION_MANAGER_H__

#include "SyncProcessNotificationManagerConstants.h"
#include "base/fscapi.h"

namespace Funambol 
{

/**
 * This pure abstract class defines IPC notification delivery interface
 */
class SyncProcessNotificationManager
{
    
public:
    
    static SyncProcessNotificationManager* getInstance();
   
    virtual void postSyncProcessStartNotification() = 0;
    virtual void postSyncProcessEndNotification(int syncStatusCode) = 0;
      
    virtual void postSyncProcessSourceStartNotification(const char* sourceName) = 0;
    virtual void postSyncProcessSourceEndNotification(const char* sourceName, int errorCode = 0) = 0;
    
    virtual void postSyncProcessSourceUploadPhaseStartedNotification(const char* sourceName) = 0;
    virtual void postSyncProcessSourceUploadPhaseEndedNotification(const char* sourceName) = 0;
    
    virtual void postSyncProcessSourceDownloadPhaseStartedNotification(const char* sourceName) = 0;
    virtual void postSyncProcessSourceDownloadPhaseEndedNotification(const char* sourceName) = 0;

    // obsolete: using the new ones with the start and end
    virtual void postSyncProcessSourceNotification(const char* sourceName) = 0;    
    
    inline const bool getIsInitialized() const { return isInitialized; }    
    
protected:
    
    /**
     * Default constructor
     */
    SyncProcessNotificationManager();
        
private:

    static SyncProcessNotificationManager* instance;
    
    bool isInitialized;

};

} // end Funambol namespace

#endif
