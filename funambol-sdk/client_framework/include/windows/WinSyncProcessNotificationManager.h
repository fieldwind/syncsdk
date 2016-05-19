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

#ifndef __WIN_SYNC_PROCESS_NOTIFICATION_MANAGER_H__
#define __WIN_SYNC_PROCESS_NOTIFICATION_MANAGER_H__

#include "SyncProcessNotificationManager.h"
#include <string>
#include "WinClientConstants.h"

using namespace std;

BEGIN_NAMESPACE


/**
 * This class implements IPC notification delivery on Windows
 */
class WinSyncProcessNotificationManager : public SyncProcessNotificationManager
{
public:
    
    HANDLE hFile;

    static WinSyncProcessNotificationManager* getInstance();
    
    /**
     * Singleton implementation: release the unique instance.
     */
    static void dispose();
    
    // obsolete: using the new ones with the start and end
    void postSyncProcessSourceNotification(const char* sourceName) {}

    void postSyncProcessSourceStartNotification(const char* sourceName);
    void postSyncProcessSourceEndNotification(const char* sourceName, int errorCode = 0);
    
    void postSyncProcessStartNotification();    
    void postSyncProcessEndNotification(int syncResult);
    void postSyncProcessCancelPendingNotification();

    void postSyncProcessSourceUploadPhaseStartedNotification(const char* sourceName);
    void postSyncProcessSourceUploadPhaseEndedNotification(const char* sourceName);

    void postSyncProcessSourceDownloadPhaseStartedNotification(const char* sourceName);
    void postSyncProcessSourceDownloadPhaseEndedNotification(const char* sourceName);

    /*
     * At the moment they are all performing in the same way
     */
    void postSyncItemAddedNotification(const char* sourceName, const WCHAR* itemID, int direction, 
                                                                      int progressive, int total, bool ignore = false);
    void postSyncItemUpdatedNotification(const char* sourceName, const WCHAR* itemID, int direction, 
                                                                      int progressive, int total, bool ignore = false);
    void postSyncItemDeletedNotification(const char* sourceName, const WCHAR* itemID, int direction, 
                                                                      int progressive, int total, bool ignore = false);

    inline const bool getIsInitialized() const { return isInitialized; }

    wstring formatSyncProcessStatus(int code);
    wstring formatSyncSourceProcessStatus(const char* sourceName, int code);

    /**
     * The itemID is the id of the item (for pim items it is the ID inside the MS Outlook)
     */
    wstring formatSyncSourceItemModification(const char* sourceName, const WCHAR* itemID, 
                                                int direction, int partial, int total, bool ignore);

    void setClientNumberOfChanges(int val) { clientNumberOfChanges = val; }
    int  getClientNumberOfChanges() { return clientNumberOfChanges; }

    void setServerNumberOfChanges(int val) { serverNumberOfChanges = val; }
    int  getServerNumberOfChanges() { return serverNumberOfChanges; }

    
private:
    
    CRITICAL_SECTION syncprocmutex;

    /**
     * Default constructor
     */
    WinSyncProcessNotificationManager();
    virtual ~WinSyncProcessNotificationManager();
    
    static WinSyncProcessNotificationManager* instance;
    
    bool isInitialized;

    void postOnPipe(wstring message);

    int clientNumberOfChanges; // currently used only by PIM
    int serverNumberOfChanges; // currently used only by PIM

    void renewFileHandle() ;
};

END_NAMESPACE
#endif
