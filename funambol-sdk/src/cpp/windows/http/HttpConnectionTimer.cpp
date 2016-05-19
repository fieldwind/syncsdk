/*
* Funambol is a mobile platform developed by Funambol, Inc. 
* Copyright (C) 2003 - 2007 Funambol, Inc.
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


#include "http/HttpConnectionTimer.h"
#include "push/FThread.h"

#define SLEEP_INTERVAL 2    //secs

USE_FUNAMBOL_NAMESPACE

//////////////////////////////////////////////////////////////////////////////
// HttpConnectionTimeThread
//////////////////////////////////////////////////////////////////////////////

HttpConnectionTimer::HttpConnectionTimer(int* amout, HINTERNET req, unsigned int resTimeout) : FThread() {
    totalAmount = amout;
    reqHandle = req;
    currentAmount = 0;
    numberOfLoops = resTimeout / SLEEP_INTERVAL;
    InitializeCriticalSection(&_mutex);
   
}

HttpConnectionTimer::~HttpConnectionTimer() {
    terminate = true;
    DeleteCriticalSection(&_mutex);
}

void HttpConnectionTimer::softTerminate() {
    EnterCriticalSection(&_mutex);
    terminate = true;
    LeaveCriticalSection(&_mutex);
}


void HttpConnectionTimer::run() {
    LOG.debug("%s Starting HttpConnectionTimer", __FUNCTION__);
    errorCode = 0;    
    currentAmount = *totalAmount;
    int counter = 0;
    
    // Send 'ready' message to Server and sleep ctpReady seconds
    while (1) {   
        EnterCriticalSection(&_mutex);
        if (terminate == true) {
            break;
        }
        LeaveCriticalSection(&_mutex);

        FThread::sleep(SLEEP_INTERVAL * 1000);
        
        EnterCriticalSection(&_mutex);
        if (terminate == true) {
            break;
        }
        if (currentAmount != *totalAmount) {
            currentAmount = *totalAmount;
            counter = 0;
        } else {
            LOG.debug("%s: currentAmount: %i, totalAmount: %i, loop number %i, loops number %i", 
                            __FUNCTION__, currentAmount, *totalAmount, counter, numberOfLoops);                        

            if (counter == numberOfLoops) {
                LOG.debug("%s counter %i, loops %i", __FUNCTION__, counter, numberOfLoops);
                if (reqHandle) {
                    LOG.debug("%s Closing request handle", __FUNCTION__);
                    InternetCloseHandle(reqHandle);        
                    break;
                }
            } else {
                counter++;
            }
        }
        LeaveCriticalSection(&_mutex);
    }
    LeaveCriticalSection(&_mutex);
    
   
}