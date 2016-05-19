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

#include "WinSyncProcessNotificationManager.h"
#include "WinClientConstants.h"

#include "base/Log.h"
#include "base/util/utils.h"
#include <string>

USE_NAMESPACE
using namespace std;


WinSyncProcessNotificationManager *WinSyncProcessNotificationManager::instance = NULL;

/**
 * Default constructor
 */
WinSyncProcessNotificationManager::WinSyncProcessNotificationManager(): SyncProcessNotificationManager()
{
    isInitialized = false;
    hFile = CreateFile(SYNC_PIPE_NAME, GENERIC_WRITE,
                             0, NULL, OPEN_EXISTING,
                             0, NULL);
    clientNumberOfChanges = -1;
    serverNumberOfChanges = -1;
    InitializeCriticalSection(&syncprocmutex);
    
}

/**
 * Singleton implementation: get the unique instance.
 *
 * @return  the singleton object
 */
WinSyncProcessNotificationManager *WinSyncProcessNotificationManager::getInstance()
{
    if (!instance) {
        instance = new WinSyncProcessNotificationManager();
    }
    
    return instance;
}

/**
 * Singleton implementation: release the unique instance.
 */
void WinSyncProcessNotificationManager::dispose() {
    delete instance;
    instance = NULL;
    
    
}

WinSyncProcessNotificationManager::~WinSyncProcessNotificationManager() {
    DeleteCriticalSection(&syncprocmutex);
}

void WinSyncProcessNotificationManager::renewFileHandle() {

    if (hFile) {
        CloseHandle(hFile);
    }
    hFile = CreateFile(SYNC_PIPE_NAME, GENERIC_WRITE,
                             0, NULL, OPEN_EXISTING,
                             0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        LOG.error("The handle of the sync pipe is wrong: %d", GetLastError());        
    }

}

/**
* Format message about the start/end/error of notification
*/
wstring WinSyncProcessNotificationManager::formatSyncProcessStatus(int code) {

    // push json message
    WCHAR wcode[128];
    wsprintf(wcode, L"%i", code);

    wstring json;
    json.append(L"{\"status\": "); json.append(wcode); 
    json.append(L"}");

    // HEADER message
    wstring message;
    message.append(L"x-version: 1.0\r\n");
    message.append(L"x-session: \r\n");
    message.append(L"x-category: SYNC_PROCESS_STATUS\r\n");
    message.append(L"x-body: "); message.append(json); message.append(L"\r\n");

    return message;
}

wstring WinSyncProcessNotificationManager::formatSyncSourceProcessStatus(const char* sourceName, int code) { 

    // push json message
    WCHAR* wsource = toWideChar(sourceName);
    WCHAR wcode[128];
    wsprintf(wcode, L"%i", code);

    wstring json;
    json.append(L"{\"name\":");
    json.append(L"\""); json.append(wsource); json.append(L"\","); 
    json.append(L"\"status\": ");
    json.append(wcode);
    json.append(L"}");

    wstring message;
    message.append(L"x-version: 1.0\r\n");
    message.append(L"x-session: \r\n");
    message.append(L"x-category: REFRESH_SOURCE_STATUS\r\n");
    message.append(L"x-body: "); message.append(json); message.append(L"\r\n");

    delete [] wsource;
    return message;

}

wstring WinSyncProcessNotificationManager::formatSyncSourceItemModification(const char* sourceName, const WCHAR* itemID, 
                                                                            int direction, int partial, int total, bool ignore) { 

    // push json message. currently used only for outlook element
    WCHAR* wsource = toWideChar(sourceName);
    
    WCHAR wdirection[128];
    wsprintf(wdirection, L"%i", direction);

    WCHAR wpartial[128];
    wsprintf(wpartial, L"%i", partial);

    WCHAR wtotal[128];
    wsprintf(wtotal, L"%i", total);

    wstring json;
    json.append(L"{\"name\":");
    json.append(L"\""); json.append(wsource); json.append(L"\","); 
    json.append(L"\"ID\":");
    json.append(L"\""); json.append(itemID); json.append(L"\",");
    json.append(L"\"direction\": "); 
    json.append(wdirection); json.append(L",");
    json.append(L"\"partial\": ");
    json.append(wpartial); json.append(L",");
    json.append(L"\"total\": ");
    json.append(wtotal); json.append(L",");
    json.append(L"\"ignore\": "); 
    ignore == false ? json.append(L"false") :  json.append(L"true");
    json.append(L"}");

    wstring message;
    message.append(L"x-version: 1.0\r\n");
    message.append(L"x-session: \r\n");
    message.append(L"x-category: SOURCE_ITEM_MODIFICATION\r\n");
    message.append(L"x-body: "); message.append(json); message.append(L"\r\n");

    delete [] wsource;
    return message;

}

void WinSyncProcessNotificationManager::postOnPipe(wstring message) {
    
    EnterCriticalSection(&syncprocmutex);
    DWORD dwWrite;
    SetLastError(0);
    if (INVALID_HANDLE_VALUE == hFile) {
        renewFileHandle();
    }
    DWORD flg = WriteFile(hFile, message.c_str(), message.length() * sizeof(wchar_t), &dwWrite, NULL);    
    if (FALSE == flg) {        
        LOG.error("WriteFile failed for sync pipe: %d", GetLastError());
        renewFileHandle();
        SetLastError(0);
        flg = WriteFile(hFile, message.c_str(), message.length() * sizeof(wchar_t), &dwWrite, NULL);
        if (FALSE == flg) {
             LOG.error("WriteFile failed for sync pipe second temptative: %d", GetLastError());
        }
    }    
    LeaveCriticalSection(&syncprocmutex);
}

void WinSyncProcessNotificationManager::postSyncProcessStartNotification()
{

    wstring message = formatSyncProcessStatus(SYNC_PROCESS_START_NOTIFICATION);
    LOG.debug("%S - %S",  __FUNCTIONW__, message.c_str());
    postOnPipe(message);
    
}

void WinSyncProcessNotificationManager::postSyncProcessEndNotification(int syncStatusCode)
{
    wstring message = formatSyncProcessStatus(syncStatusCode);
    LOG.debug("%S - %S",  __FUNCTIONW__, message.c_str());
    postOnPipe(message);
    
}

void WinSyncProcessNotificationManager::postSyncProcessCancelPendingNotification()
{
    
    wstring message = formatSyncProcessStatus(SYNC_PROCESS_CANCELING_NOTIFICATION);
    LOG.debug("%S - %S",  __FUNCTIONW__, message.c_str());
    postOnPipe(message);
    
}

void WinSyncProcessNotificationManager::postSyncProcessSourceStartNotification(const char* sourceName)
{

    wstring message = formatSyncSourceProcessStatus(sourceName, SYNC_SOURCE_START_REFRESH);
    LOG.debug("%S - %S",  __FUNCTIONW__, message.c_str());
    postOnPipe(message);
   
}


void WinSyncProcessNotificationManager::postSyncProcessSourceEndNotification(const char* sourceName, int errorCode)
{

    wstring message = formatSyncSourceProcessStatus(sourceName, SYNC_SOURCE_END_REFRESH);
    
    LOG.debug("%S - %S",  __FUNCTIONW__, message.c_str());
    postOnPipe(message);
   
}


void WinSyncProcessNotificationManager::postSyncProcessSourceUploadPhaseStartedNotification(const char* sourceName) {

    wstring message = formatSyncSourceProcessStatus(sourceName, SYNC_SOURCE_START_UPLOAD_PHASE);
    LOG.debug("%S - %S",  __FUNCTIONW__, message.c_str());
    postOnPipe(message);
  
}
void WinSyncProcessNotificationManager::postSyncProcessSourceUploadPhaseEndedNotification(const char* sourceName) {
    
    wstring message = formatSyncSourceProcessStatus(sourceName, SYNC_SOURCE_END_UPLOAD_PHASE);
    LOG.debug("%S - %S",  __FUNCTIONW__, message.c_str());
    postOnPipe(message);

}

void WinSyncProcessNotificationManager::postSyncProcessSourceDownloadPhaseStartedNotification(const char* sourceName)  {
    wstring message = formatSyncSourceProcessStatus(sourceName, SYNC_SOURCE_START_DOWNLOAD_PHASE);
    LOG.debug("%S - %S",  __FUNCTIONW__, message.c_str());
    postOnPipe(message);
}

void WinSyncProcessNotificationManager::postSyncProcessSourceDownloadPhaseEndedNotification(const char* sourceName) {

    wstring message = formatSyncSourceProcessStatus(sourceName, SYNC_SOURCE_END_DOWNLOAD_PHASE);
    LOG.debug("%S - %S",  __FUNCTIONW__, message.c_str());
    postOnPipe(message);
}

void WinSyncProcessNotificationManager::postSyncItemAddedNotification(const char* sourceName, const WCHAR* itemID, int direction, 
                                                                      int partial, int total, bool ignore) {

    wstring message = formatSyncSourceItemModification(sourceName, itemID, direction, partial, total, ignore);
    LOG.debug("%S - %S",  __FUNCTIONW__, message.c_str());
    postOnPipe(message);
}

void WinSyncProcessNotificationManager::postSyncItemUpdatedNotification(const char* sourceName, const WCHAR* itemID, int direction, 
                                                                      int partial, int total, bool ignore) {

    wstring message = formatSyncSourceItemModification(sourceName, itemID, direction, partial, total, ignore);
    LOG.debug("%S - %S",  __FUNCTIONW__, message.c_str());
    postOnPipe(message);
}

void WinSyncProcessNotificationManager::postSyncItemDeletedNotification(const char* sourceName, const WCHAR* itemID, int direction, 
                                                                      int partial, int total, bool ignore) {

    wstring message = formatSyncSourceItemModification(sourceName, itemID, direction, partial, total, ignore);
    LOG.debug("%S - %S",  __FUNCTIONW__, message.c_str());
    postOnPipe(message);
}