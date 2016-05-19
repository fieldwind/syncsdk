/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2010 Funambol, Inc.
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


#include "base/fscapi.h"

#if FUN_TRANSPORT_AGENT == FUN_MAC_TRANSPORT_AGENT

#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>
#include "base/util/StringUtils.h"
#include "http/HttpConnectionHandler.h"
#include "http/AbstractHttpConnection.h"
#include "http/constants.h"
#include "base/util/utils.h"
#include "base/util/KeyValuePair.h"
#include "base/util/StringBuffer.h"
#include "event/FireEvent.h"
#include <pthread.h>
#include <inttypes.h>

BEGIN_FUNAMBOL_NAMESPACE

#define READ_MAX_BUFFER_SIZE    1024 * 4 // 4Kb
#define DEFAULT_REQUEST_TIMEOUT 120      // timeout in secs.

// main request thread cleanup handler
static void stopRequestThread(void *arg);
// reader thread cleanup handler
static void stopReaderThread(void *arg);
// timeout watchdog thread starter
static void* timeoutWatchdogThreadStartRoutine(void *);
// progress notifier thread starter 
static void* progressNotifierThreadStartRoutine(void *arg);
// progress notifier thread starter 
static void* uploadProgressNotifierThreadStartRoutine(void *arg);

static void unscheduleRequest(HttpConnectionHandler* streamHandle);

// reader thread fuctions
static void* readerThread(void *arg);
static void* readerStreamThread(void *arg);

// callback called by progress notification runloop
static void progressNotifier(CFRunLoopTimerRef timer, void *info);
// connection timeout callback to stop reader thread
static void timeoutHandler(CFRunLoopTimerRef timer, void *info);

// CFReadStream callbacks
static void readClientCallBack(CFReadStreamRef stream, CFStreamEventType type, void *clientCallBackInfo);
static void readStreamClientCallBack(CFReadStreamRef stream, CFStreamEventType type, void *clientCallBackInfo);

static void cancelRequestCallBack(CFNotificationCenterRef center, void *observer,
                        CFStringRef name, const void *object, CFDictionaryRef userInfo);

static const CFOptionFlags kNetworkEvents = kCFStreamEventOpenCompleted |kCFStreamEventHasBytesAvailable |
                                            kCFStreamEventEndEncountered | kCFStreamEventErrorOccurred;

CFRunLoopRef HttpConnectionHandler::networkRunLoop = nil;


HttpConnectionHandler::HttpConnectionHandler(unsigned int reqTimeout, size_t chunkSize,
                                             HttpConnectionUploadObserver* uObserver) : 
    stream(NULL), os(NULL), requestSize(0), readerThreadRunning(false), handle(NULL), 
    timeout(reqTimeout > 0 ? reqTimeout : DEFAULT_REQUEST_TIMEOUT),
    readChunkSize(chunkSize > 0 ? chunkSize : READ_MAX_BUFFER_SIZE),
    bytesUploaded(0), bytesRead(0), requestStatus(0), requestToken(-1), uploadObserver(uObserver)
{
    nCenter = CFNotificationCenterGetLocalCenter();
    dispatcher = NotificationDispatcher::getInstance();
    waitSem = dispatch_semaphore_create(0);
    //pthread_mutex_init(&timerMtx, NULL);
    //pthread_mutex_init(&notifierMtx, NULL);
    pthread_mutex_init(&cancelMtx, NULL);
}

HttpConnectionHandler::~HttpConnectionHandler() 
{
    //pthread_mutex_destroy(&timerMtx);
    //pthread_mutex_destroy(&notifierMtx);
    
    if (readerThreadRunning) {
        stopRequest();
    } else {
        dispatch_semaphore_signal(waitSem);
    }
    //pthread_mutex_unlock(&cancelMtx);
    
    pthread_mutex_destroy(&cancelMtx);
    
    dispatch_release(waitSem);
}

int HttpConnectionHandler::startConnectionHandler(CFReadStreamRef readStream, size_t reqSize)
{
    int status = 0;
    int ret = E_SUCCESS; 
    void* thread_ret_val = 0;
    pthread_t request_tid;
    
    CFMutableDictionaryRef notificationDict = NULL; 
    
    if (readerThreadRunning) {
        LOG.error("%s: reader thread already running", __FUNCTION__);
        
        return E_ALREADY_RUNNING;
    }

    readerThreadRunning = true;
    
    // set PTHREAD_CANCEL_DEFERRED to enable cleanup handlers
    // over pthread_cancel() calls
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    
    // clear out previous buffer
    responseBuffer.reset();

    stream = readStream;
    os = NULL;
    requestSize = reqSize;
    
    // set stopReaderThread as cleanup handler for pthread_cancel
    //pthread_cleanup_push(stopRequestThread, (void *) this);
    
    // create reader thread (this will open connection sending HTTP request
    // and will read the response).
    if ((status = pthread_create(&request_tid, NULL, runRequestHandler, 
            reinterpret_cast<void *>(this))) != 0) {
        LOG.error("%s: error creating reader thread: %s", __FUNCTION__,
                            strerror(status));
        readerThreadRunning = false;
        dispatcher->postNotification((CFStringRef)@NOTIFY_CONNECTION_HANDLER_ENDED, NULL);
        ret = E_THREAD_CREATE;
        
        return ret;
    }
    
    requestToken = (arc4random() % ((unsigned)RAND_MAX + 1));
    
    notificationDict = CFDictionaryCreateMutable(NULL, 1, &kCFCopyStringDictionaryKeyCallBacks, 
                                                    &kCFTypeDictionaryValueCallBacks);
    
    CFNumberRef requestTokenNum = CFNumberCreate(NULL, kCFNumberIntType, (int *)&requestToken);
    CFDictionaryAddValue(notificationDict, CFSTR(CONNECTION_HANDLER_REQUEST_TOKEN_KEY), requestTokenNum);
    
    dispatcher->postNotification((CFStringRef)@NOTIFY_CONNECTION_HANDLER_STARTED, notificationDict);

    CFRelease(requestTokenNum);
    CFRelease(notificationDict);
    
    CFNotificationCenterAddObserver(nCenter, reinterpret_cast<void *>(this),
                                        cancelRequestCallBack,
                                        (CFStringRef)@CONNECTION_HANDLER_CANCEL_REQUEST_NOTIFICATION,
                                        NULL,
                                        CFNotificationSuspensionBehaviorDeliverImmediately);
    
    if ((status = pthread_join(request_tid, &thread_ret_val)) != 0) {
        LOG.error("%s: error creating reader thread: %s", __FUNCTION__, strerror(status));
        readerThreadRunning = false;
        CFNotificationCenterRemoveObserver(nCenter, (void *)this, CFSTR(CONNECTION_HANDLER_CANCEL_REQUEST_NOTIFICATION),
                                                NULL);
        dispatcher->postNotification((CFStringRef)@CONNECTION_HANDLER_CANCEL_REQUEST_NOTIFICATION, NULL);
        ret = E_THREAD_JOIN;
        
        return ret;
    }
    
    CFNotificationCenterRemoveObserver(nCenter, (void *)this, CFSTR(CONNECTION_HANDLER_CANCEL_REQUEST_NOTIFICATION),
                                       NULL);
    readerThreadRunning = false;

    if (thread_ret_val == (void *)PTHREAD_CANCELED) {
        ret = E_REQUEST_CANCELLED;
        LOG.info("%s: cancelled on user request", __FUNCTION__);
    } else if (thread_ret_val == (void *)E_TIMEOUT_REACHED) {
        ret = ERR_CONNECTION_TIMEOUT;
    } else {
        ret = (intptr_t)thread_ret_val;
    }
         
    dispatcher->postNotification((CFStringRef)@NOTIFY_CONNECTION_HANDLER_ENDED, NULL);
    
    LOG.debug("%s: HTTP request status: %d", __FUNCTION__, ret);

    return ret;    
}

void* HttpConnectionHandler::runRequestHandler(void* arg)
{
    int status = 0;
    HttpConnectionHandler* connectionHandler = reinterpret_cast<HttpConnectionHandler*>(arg); 
 
    status = connectionHandler->runRequest();
    
    return (void *)status;
}

class UploadProgressMonitor {
    
public:
    UploadProgressMonitor(HttpConnectionHandler* connHandler, dispatch_semaphore_t& sem,
                          HttpConnectionUploadObserver* uObserver);
    void stop();
    void run();
    void setUploadTimeoutSecs(dispatch_semaphore_t* sem, int32_t timeout) {
        timeoutSem = sem;
        uploadTimeoutSecs = timeout;
    }
    
    bool getDidTimeout() {
        return timedout;
    }
    
private:
    HttpConnectionHandler* connHandler;
    HttpConnectionUploadObserver* uploadObserver;
    bool done;
    dispatch_semaphore_t& sem;
    dispatch_semaphore_t* timeoutSem;
    int64_t lastBytesUploaded;
    int32_t uploadTimeoutSecs;
    bool timedout;
};

UploadProgressMonitor::UploadProgressMonitor(HttpConnectionHandler* handler,
                                             dispatch_semaphore_t& s,
                                             HttpConnectionUploadObserver* uObserver) :
              connHandler(handler),
              done(false),
              sem(s),
              uploadObserver(uObserver),
              lastBytesUploaded(-1),
              uploadTimeoutSecs(-1),
              timedout(false)
{
}

void UploadProgressMonitor::stop() {
    done = true;
}

void UploadProgressMonitor::run() {
    
    uint32_t idleTime = 0;
    while(!done) {
        
        CFReadStreamRef stream = connHandler->getStream();
        if (stream != NULL) {
            CFNumberRef bytesUploadedRef = (CFNumberRef)CFReadStreamCopyProperty(stream, kCFStreamPropertyHTTPRequestBytesWrittenCount);
            if (bytesUploadedRef) {
                int totalBytesUploaded;
                if (CFNumberGetValue(bytesUploadedRef,  kCFNumberIntType,
                                     (void *)&totalBytesUploaded))
                {
                    if (totalBytesUploaded > 0 && uploadObserver != NULL) {
                        LOG.debug("%s Uploaded %d bytes", __FUNCTION__, totalBytesUploaded);
                        uploadObserver->uploadProgress(totalBytesUploaded);
                        
                        if (uploadTimeoutSecs > 0) {
                            if (lastBytesUploaded != totalBytesUploaded) {
                                // The upload moved forward, reset the idle time counter
                                idleTime = 0;
                            } else {
                                // Increase the idle time
                                idleTime += 1;
                                if (idleTime > uploadTimeoutSecs) {
                                    LOG.info("%s: Upload request timedout after %d",__FUNCTION__,idleTime);
                                    done = true;
                                    timedout = true;
                                    dispatch_semaphore_signal(*timeoutSem);
                                }
                            }
                        }
                        
                        lastBytesUploaded = totalBytesUploaded;
                    }
                }
            }
        }
        
        // Check the status every two seconds
        sleep(1);
    }
    // Notify the handler that the monitoring is over
    dispatch_semaphore_signal(sem);
}

CFRunLoopRef HttpConnectionHandler::getNetworkRunLoop() {
    return networkRunLoop != nil ? networkRunLoop : CFRunLoopGetMain();
}

int HttpConnectionHandler::runRequest()
{
    int ret = E_SUCCESS;
    CFStreamClientContext ctxt = { 0, (void*)this, NULL, NULL, NULL };
   
    //pthread_mutex_lock(&cancelMtx);
    
    // set the client notifier
    if (CFReadStreamSetClient(stream, kNetworkEvents, readClientCallBack, &ctxt) == false ) {
        LOG.error("%s: failed to create HTTP request...", __FUNCTION__);
        ret = E_INTERNAL_ERROR;
        return ret;
    }
   
    LOG.debug("%s: scheduling request thread", __FUNCTION__);
    
    // Schedule the stream
    
    CFRunLoopRef runLoop = this->getNetworkRunLoop();
    CFReadStreamScheduleWithRunLoop(stream, runLoop, kCFRunLoopCommonModes);
    //streamHandle->setRequestThreadScheduled(true);
    //pthread_mutex_unlock(&cancelMtx);
    
     // open connection and send request
    if (!CFReadStreamOpen(stream)) {
        //streamHandle->unscheduleTimeoutWatchdog();    
    
        CFReadStreamSetClient(stream, kCFStreamEventNone, NULL, NULL );
        CFReadStreamUnscheduleFromRunLoop(stream, runLoop, kCFRunLoopCommonModes );
    
        LOG.error("%s: failed to send HTTP request...", __FUNCTION__);
        ret = ERR_CONNECT;
        return ret;
    }
    
    LOG.debug("%s: waiting for request completion", __FUNCTION__);
    
    // This code enables timeout for SyncML. But it needs to be tested more carefully and also it would require longer
    // timeouts compared to SAPI operations. For this reason it is currently disabled
#if 0
    // wait for completion of readerThread. We perform an active wait to implement a timeout mechanism
    dispatch_time_t readTimeout = dispatch_time(DISPATCH_TIME_NOW, timeout * NSEC_PER_SEC);
    // While the reading moves forward we keep waiting. We check every timeout and if nothing was received meanwhile, then
    // we abort the request
    uint64_t lastBytesRead;
    int waitRes;
    do {
        lastBytesRead = getRequestTotalBytesRead();
        waitRes = dispatch_semaphore_wait(waitSem, readTimeout);
    } while(waitRes != 0 && getRequestTotalBytesRead() > lastBytesRead);
    
    if (waitRes != 0) {
        requestStatus = E_TIMEOUT_REACHED;
    }
#else
    // wait for request completion
    dispatch_semaphore_wait(waitSem, DISPATCH_TIME_FOREVER);
#endif
    
    LOG.debug("%s: unscheduling request thread", __FUNCTION__);
    
    CFReadStreamSetClient(stream, kCFStreamEventNone, NULL, NULL );
    CFReadStreamUnscheduleFromRunLoop(stream, runLoop, kCFRunLoopCommonModes);
    
    fireTransportEvent(bytesRead /*totalBytesRead*/, RECEIVE_DATA_END);
    ret =  requestStatus;
    
    LOG.debug("%s: data transfert completed", __FUNCTION__);
    
    // Signal the end of the operation
    return ret;
}

int HttpConnectionHandler::startConnectionHandler(CFReadStreamRef readStream, OutputStream& outputStream, size_t reqSize)
{
    int status = 0;
    int ret = E_SUCCESS; 
    void* thread_ret_val = 0;
    pthread_t request_tid;
    CFMutableDictionaryRef notificationDict = NULL; 
    
    if (readerThreadRunning) {
        LOG.error("%s: reader thread already running", __FUNCTION__);
        return E_ALREADY_RUNNING;
    }

    readerThreadRunning = true;
    // clear out previous buffer
    responseBuffer.reset();

    stream = readStream;
    os = &outputStream;
    requestSize = reqSize;
    
    // set PTHREAD_CANCEL_DEFERRED to enable cleanup handlers
    // over pthread_cancel() calls
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    // set stopReaderThread as cleanup handler for pthread_cancel
    pthread_cleanup_push(stopRequestThread, (void *) this);
    
    if ((status = pthread_create(&request_tid, NULL, runStreamedRequestHandler, 
            reinterpret_cast<void *>(this))) != 0) {
        LOG.error("%s: error creating reader thread: %s", __FUNCTION__,
                            strerror(status));
        readerThreadRunning = false;
        dispatcher->postNotification((CFStringRef)@NOTIFY_CONNECTION_HANDLER_ENDED, NULL);
        ret = E_THREAD_CREATE;
        
        return ret;
    }

    requestToken = (arc4random() % ((unsigned)RAND_MAX + 1));
    
    notificationDict = CFDictionaryCreateMutable(NULL, 1, &kCFCopyStringDictionaryKeyCallBacks, 
                                                    &kCFTypeDictionaryValueCallBacks);
    
    CFNumberRef requestTokenNum = CFNumberCreate(NULL, kCFNumberIntType, (int *)&requestToken);
    CFDictionaryAddValue(notificationDict, CFSTR(CONNECTION_HANDLER_REQUEST_TOKEN_KEY), requestTokenNum);
    
    dispatcher->postNotification((CFStringRef)@NOTIFY_CONNECTION_HANDLER_STARTED, notificationDict);
    
    CFRelease(requestTokenNum);
    CFRelease(notificationDict);
    
    CFNotificationCenterAddObserver(nCenter, reinterpret_cast<void *>(this),
                                        cancelRequestCallBack,
                                        (CFStringRef)@CONNECTION_HANDLER_CANCEL_REQUEST_NOTIFICATION,
                                        NULL,
                                        CFNotificationSuspensionBehaviorDeliverImmediately);
    
    // wait for completion of readerThread
    if ((status = pthread_join(request_tid, &thread_ret_val)) != 0) {
        readerThreadRunning = false;
        LOG.error("%s: error creating reader thread: %s", __FUNCTION__, strerror(status));
        CFNotificationCenterRemoveObserver(nCenter, (void *)this, CFSTR(CONNECTION_HANDLER_CANCEL_REQUEST_NOTIFICATION), 
                                                NULL);
   
        dispatcher->postNotification((CFStringRef)@NOTIFY_CONNECTION_HANDLER_ENDED, NULL);
        ret = E_THREAD_JOIN;
        
        return ret;
    } 

    CFNotificationCenterRemoveObserver(nCenter, (void *)this, CFSTR(CONNECTION_HANDLER_CANCEL_REQUEST_NOTIFICATION), 
                                                NULL);
   
    pthread_cleanup_pop(0);
    
    readerThreadRunning = false;  

    if (thread_ret_val == (void *)PTHREAD_CANCELED) {
        LOG.info("%s: cancelled on user request", __FUNCTION__);
        ret = E_REQUEST_CANCELLED;
    } else if (thread_ret_val == (void *)E_TIMEOUT_REACHED) {
        ret = ERR_CONNECTION_TIMEOUT;
    } else {
        ret = (intptr_t)thread_ret_val;
    }

    LOG.debug("%s: HTTP request status: %d", __FUNCTION__, ret);
    
    dispatcher->postNotification((CFStringRef)@NOTIFY_CONNECTION_HANDLER_ENDED, NULL);
    
    return ret;    
}

void* HttpConnectionHandler::runStreamedRequestHandler(void* arg)
{
    int status = 0;
    HttpConnectionHandler* connectionHandler = reinterpret_cast<HttpConnectionHandler*>(arg); 
 
    status = connectionHandler->runStreamedRequest();
    
    return (void *)status;
}

int HttpConnectionHandler::runStreamedRequest()
{
    int ret = E_SUCCESS;
    CFStreamClientContext ctxt = { 0, (void*)this, NULL, NULL, NULL };
    
    //pthread_mutex_lock(&cancelMtx);
    
    // set the client notifier
    if (CFReadStreamSetClient(stream, kNetworkEvents, readStreamClientCallBack, &ctxt) == false ) {
        LOG.error("%s: failed to create HTTP request...", __FUNCTION__);
        ret = E_INTERNAL_ERROR;
        //pthread_mutex_unlock(&cancelMtx);
    
        return ret;        
    }
   
    LOG.debug("%s: scheduling request thread", __FUNCTION__);
    // Schedule the stream
    CFRunLoopRef runLoop = this->getNetworkRunLoop();
    CFReadStreamScheduleWithRunLoop(stream, runLoop, kCFRunLoopCommonModes);
    
    // open connection and send request (this will start upload of the associated stream)
    if (!CFReadStreamOpen(stream)) {
        LOG.error("%s: failed to send HTTP request...", __FUNCTION__);
        ret = ERR_CONNECT;
        //streamHandle->stopRequestProgressNotifier();
        //streamHandle->unscheduleTimeoutWatchdog();
        
        CFReadStreamSetClient(stream, kCFStreamEventNone, NULL, NULL);
        CFReadStreamUnscheduleFromRunLoop(stream, runLoop, kCFRunLoopCommonModes);
    
        return ret;
    }

    // Create the object monitoring the upload progress
    dispatch_semaphore_t uploadProgressSem = dispatch_semaphore_create(0);
    UploadProgressMonitor uplProgressMonitor(this, uploadProgressSem, uploadObserver);
    int uploadProgressThreadStatus = 0;
    pthread_t request_tid;
    if (requestSize > 0) {
        uplProgressMonitor.setUploadTimeoutSecs(&waitSem, timeout);
        if ((uploadProgressThreadStatus = pthread_create(&request_tid, NULL,uploadProgressNotifierThreadStartRoutine,
                                                         reinterpret_cast<void *>(&uplProgressMonitor))) != 0) {
            LOG.error("%s: error creating upload progress thread: %s", __FUNCTION__,
                      strerror(uploadProgressThreadStatus));
        }
    }
    
    LOG.debug("%s: waiting for completion", __FUNCTION__);
    // wait for completion of readerThread. We perform an active wait to implement a timeout mechanism
    dispatch_time_t readTimeout = dispatch_time(DISPATCH_TIME_NOW, (uint64_t)timeout* (uint64_t)NSEC_PER_SEC);
    // While the reading moves forward we keep waiting. We check every timeout and if nothing was received meanwhile, then
    // we abort the request. If we have a post request, then we do not trigger timeouts on receiving data
    uint64_t lastBytesRead;
    int waitRes;
    bool readingOk;
    do {
        lastBytesRead = getRequestTotalBytesRead();
        waitRes = dispatch_semaphore_wait(waitSem, readTimeout);
        // We consider readingOk if we received more data or we are performing a POST request (in which case we
        // are not downloading anything)
        readingOk = requestSize > 0 || getRequestTotalBytesRead() > lastBytesRead;
        // If waitRes is != 0, then we unblocked because of a timeout
    } while(waitRes != 0 && readingOk);
    
    if (waitRes != 0) {
        requestStatus = E_TIMEOUT_REACHED;
        LOG.debug("%s: The upload request terminated with a timeout",__FUNCTION__);
    }
    
    if (requestSize > 0) {
        // Stop the upload progress monitor
        uplProgressMonitor.stop();
        timeout = uplProgressMonitor.getDidTimeout();
        // Since the upload progress monitor executes in its own thread, we shall wait for its
        // termination to make sure it does not access "this" object which may be deallocated
        if (uploadProgressThreadStatus == 0) {
            dispatch_semaphore_wait(uploadProgressSem, DISPATCH_TIME_FOREVER);
        }
    }
    // At this point we are sure the monitoring thread has completed and therefore the upload
    // progress monitor can safely get out of scope
   
    //streamHandle->stopRequestProgressNotifier();
    //streamHandle->unscheduleTimeoutWatchdog();
    
    LOG.debug("%s: unscheduling request thread", __FUNCTION__);
    
    //pthread_mutex_lock(&cancelMtx);
    
    //streamHandle->setRequestThreadScheduled(false);
    CFReadStreamSetClient(stream, kCFStreamEventNone, NULL, NULL);
    CFReadStreamUnscheduleFromRunLoop(stream, runLoop, kCFRunLoopCommonModes);
    //CFReadStreamClose(stream);
    if (requestSize > 0) {
        pthread_detach(request_tid);
    }
    
    //pthread_mutex_unlock(&cancelMtx);
  
    LOG.debug("%s: data transfert completed", __FUNCTION__);
 
    fireTransportEvent(bytesRead, RECEIVE_DATA_END);
    
    ret = requestStatus; //streamHandle->getRequestStatus();
    return ret;
}

void HttpConnectionHandler::stopRequest()
{
    LOG.debug("%s: cancelling request...", __FUNCTION__);
    
    //pthread_mutex_lock(&cancelMtx);
    requestStatus = E_REQUEST_CANCELLED;  
    
    /*
    LOG.debug("%s: stopping run loop timer", __FUNCTION__);
    streamHandle->unscheduleTimeoutWatchdog();
    
    LOG.debug("%s: stopping progress notification timer", __FUNCTION__);
    streamHandle->stopRequestProgressNotifier();
    */
    
   
    // remove client to unregister events on stream
    //CFReadStreamSetClient(stream, kCFStreamEventNone, NULL, NULL);
    dispatch_semaphore_signal(waitSem);
}

const char* HttpConnectionHandler::getRequestReponse() const
{
    return responseBuffer.c_str();
}

//
// StreamDataHandle class
//
StreamDataHandle::StreamDataHandle(CFReadStreamRef respStream, OutputStream* oStream, 
                    size_t chunkSize, unsigned int timeout, size_t reqSize) : 
        responseStream(respStream), os(oStream), readChunkSize(chunkSize), 
        requestSize(reqSize), bytesRead(0), timeout(timeout), timer(NULL), requestNotificationTimer(NULL), requestTimeout(false), timeoutWatchdogRunning(false),
        progressNotifierRunning(false), writeToStreamCompleted(false), stopStreamReading(false),
        bytesUploaded(0), requestStatus(0), requestThreadScheduled(false), streamOpened(false)
{
    mainThreadId = pthread_self();
    waitSem = dispatch_semaphore_create(0);
    pthread_mutex_init(&timerMtx, NULL);
    pthread_mutex_init(&notifierMtx, NULL);
    pthread_mutex_init(&cancelMtx, NULL);
}
  
StreamDataHandle::~StreamDataHandle()
{
    LOG.debug("%s: deleting object", __FUNCTION__);
    stopRequestProgressNotifier();
    unscheduleTimeoutWatchdog();

    pthread_mutex_destroy(&timerMtx);
    pthread_mutex_destroy(&notifierMtx);
    pthread_mutex_destroy(&cancelMtx);
    
    dispatch_release(waitSem);
    LOG.debug("%s: deleted...", __FUNCTION__);
}

int StreamDataHandle::scheduleTimeoutWatchdog()
{
    int timeoutWatchdogThreadStatus = 0;
#ifdef __ENABLE_HTTP_HANDLER_TIMEOUT__
    pthread_t timeoutWatchdogTid;
    pthread_attr_t timeoutWatchdogThreadAttrs;
 
    pthread_attr_init(&timeoutWatchdogThreadAttrs);
    pthread_attr_setdetachstate(&timeoutWatchdogThreadAttrs, PTHREAD_CREATE_DETACHED);

    if ((timeoutWatchdogThreadStatus = pthread_create(&timeoutWatchdogTid, &timeoutWatchdogThreadAttrs, 
            timeoutWatchdogThreadStartRoutine, reinterpret_cast<void *>(this))) != 0) {
        LOG.error("%s: error creating watchdog timer thread: %s", __FUNCTION__,
            strerror(timeoutWatchdogThreadStatus));
    }
#endif

    return timeoutWatchdogThreadStatus;
}

void StreamDataHandle::scheduleTimeoutWatchdogTimer()
{
#ifdef __ENABLE_HTTP_HANDLER_TIMEOUT__
    CFAllocatorRef allocator = kCFAllocatorDefault;
    CFRunLoopTimerContext context = { 0, (void *)this, NULL, NULL, NULL };
    CFTimeInterval interval = 0;
    CFOptionFlags flags = 0;
    CFIndex order = 0;

    pthread_mutex_lock(&timerMtx);
    
    if (timeoutWatchdogRunning == false) {
        CFAbsoluteTime fireDate = CFAbsoluteTimeGetCurrent() + timeout;
        CFRunLoopTimerCallBack callback = (CFRunLoopTimerCallBack)timeoutHandler;

        if (timer) {
            CFRunLoopTimerInvalidate(timer);
        }

        // create timer
        timer = CFRunLoopTimerCreate(allocator, fireDate, interval, flags, order, callback, &context);
        
        // star timer with notification callback
        CFRunLoopAddTimer(CFRunLoopGetMain(), timer, kCFRunLoopDefaultMode);
        timeoutWatchdogRunning = true;
    }
    
    pthread_mutex_unlock(&timerMtx);
#endif
}

void StreamDataHandle::unscheduleTimeoutWatchdog()
{
#ifdef __ENABLE_HTTP_HANDLER_TIMEOUT__
    pthread_mutex_lock(&timerMtx);
    
    if (timeoutWatchdogRunning) {
        timeoutWatchdogRunning = false;
        
        if (timer) {    
            CFRunLoopTimerInvalidate(timer);
            timer = NULL;
        }
    }
    
    pthread_mutex_unlock(&timerMtx);
#endif
}

int StreamDataHandle::startRequestProgressNotifier()
{
    int progressNotifierThreadStatus = 0;

#ifdef __ENABLE_HTTP_PROGRESS_NOTIFIER__
    pthread_t progressNotifierTid;
    pthread_attr_t progressNotifierThreadAttrs;
 
    pthread_attr_init(&progressNotifierThreadAttrs);
    pthread_attr_setdetachstate(&progressNotifierThreadAttrs, PTHREAD_CREATE_DETACHED);

    if ((progressNotifierThreadStatus = pthread_create(&progressNotifierTid, &progressNotifierThreadAttrs, 
            progressNotifierThreadStartRoutine, reinterpret_cast<void *>(this))) != 0) {
        LOG.error("%s: error creating progress notifier timer thread: %s", __FUNCTION__,
            strerror(progressNotifierThreadStatus));
    }
#endif

    return progressNotifierThreadStatus;
}

void StreamDataHandle::startRequestProgressNotifierTimer()
{
#ifdef __ENABLE_HTTP_PROGRESS_NOTIFIER__
    CFAllocatorRef allocator = kCFAllocatorDefault;
    CFRunLoopTimerContext context = { 0, (void *)this, NULL, NULL, NULL };
    CFTimeInterval interval = 1;
    CFOptionFlags flags = 0;
    CFIndex order = 0;

    pthread_mutex_lock(&notifierMtx);
    
    if (progressNotifierRunning == false) {
        CFAbsoluteTime fireDate = CFAbsoluteTimeGetCurrent() + 1;
        CFRunLoopTimerCallBack callback = (CFRunLoopTimerCallBack)progressNotifier;

        if (requestNotificationTimer) {
            CFRunLoopTimerInvalidate(requestNotificationTimer);
        }

        // create timer
        requestNotificationTimer = CFRunLoopTimerCreate(allocator, fireDate, interval, flags, order, callback, &context);
        
        // star timer with notification callback
        CFRunLoopAddTimer(CFRunLoopGetMain(), requestNotificationTimer, kCFRunLoopDefaultMode);
        progressNotifierRunning = true;
    }
    
    pthread_mutex_unlock(&notifierMtx);
#endif

}

void StreamDataHandle::stopRequestProgressNotifier()
{
#ifdef __ENABLE_HTTP_PROGRESS_NOTIFIER__
    pthread_mutex_lock(&notifierMtx);
  
    if (progressNotifierRunning) {
        if (requestNotificationTimer) {
            CFRunLoopTimerInvalidate(requestNotificationTimer);
            requestNotificationTimer = NULL;
        }
        progressNotifierRunning = false;
    }
    
    pthread_mutex_unlock(&notifierMtx);
#endif

}

StringMap* StreamDataHandle::parseHeaders()
{
    CFHTTPMessageRef serverReply = NULL;
    StringMap* responseHeaders = NULL;      // associative array for server response headers
    
    serverReply = (CFHTTPMessageRef) CFReadStreamCopyProperty(responseStream, kCFStreamPropertyHTTPResponseHeader);

    // Pull the status code from the headers
    if (serverReply == NULL) {
        LOG.error("%s: error getting server reply from response stream", __FUNCTION__);

        return responseHeaders;
    }
    
    CFDictionaryRef headers = CFHTTPMessageCopyAllHeaderFields(serverReply);
    
    if (!headers) {
        LOG.info("%s: no HTTP  headers in server response");
        return responseHeaders;
    }
 
    responseHeaders = new StringMap();
    responseHeaders->clear();
 
    int count = CFDictionaryGetCount(headers);
    if (count == 0) {
        // nothing to do
        return responseHeaders;
    }
 
    // Allocate the arrays of keys and values
    CFStringRef keys[count];
    CFStringRef values[count];
    for (int i=0; i<count; i++) {
        keys[i] = CFSTR("");
        values[i] = CFSTR("");
    }
    
    // Get the headers pairs and fill the stringMap
    CFDictionaryGetKeysAndValues(headers, (const void**)keys, (const void**)values);
    for (int i=0; i<count; i++) 
    {
        CFStringRef hdrKey = keys[i];
        CFStringRef hdrVal = values[i]; 
        if (!hdrKey || !hdrVal) continue;
        
        StringBuffer k = CFString2StringBuffer(hdrKey);
        StringBuffer v = CFString2StringBuffer(hdrVal);
        
        responseHeaders->put(k.c_str(), v.c_str());
        
        CFRelease(hdrKey);
        CFRelease(hdrVal);
    }

    return responseHeaders;
}


static void readClientCallBack(CFReadStreamRef stream, CFStreamEventType type, void *clientCallBackInfo)
{
    //StreamDataHandle* streamHandle = (StreamDataHandle *)clientCallBackInfo;
    HttpConnectionHandler* connectionHandler = reinterpret_cast<HttpConnectionHandler*>(clientCallBackInfo); 
 
    unsigned int read_size = 0;  
    CFIndex bytesRead = 0;
    size_t totalBytesRead = 0;
    StringBuffer* responseBuffer = NULL;
    dispatch_semaphore_t waitSem; 
    pthread_mutex_t* cancelMtx = NULL; 
  
    if (connectionHandler == NULL) {
        LOG.error("%s: invalid callback info parameter", __FUNCTION__);
        
        return;
    }
    
    cancelMtx = connectionHandler->getCancelLock();
    
    //pthread_mutex_lock(cancelMtx);
    read_size = connectionHandler->getReadChunkSize();
    responseBuffer = connectionHandler->getResponseBuffer();
    waitSem = connectionHandler->getWaitSem();
    
    switch (type) {
        case kCFStreamEventOpenCompleted: {
            //streamHandle->unscheduleTimeoutWatchdog();
            //streamHandle->stopRequestProgressNotifier();
            connectionHandler->setStreamOpened(true);
            //streamHandle->setStreamConnected(true);
            //streamHandle->setStopStreamReading(false);
            //streamHandle->setWriteToStreamCompleted(true);
            //pthread_mutex_unlock(cancelMtx);
    
            LOG.debug("%s: HTTP request sent", __FUNCTION__);
            
            break;
        }
        
        case kCFStreamEventHasBytesAvailable: {
            UInt8   buffer[read_size];
            memset(buffer, 0, read_size);
            
            LOG.debug("%s: Reading from stream %p",__FUNCTION__,stream);
            if ((bytesRead = CFReadStreamRead(stream, buffer, read_size - 1)) > 0) {
                responseBuffer->append((const char*)buffer);
                
                fireTransportEvent((int)bytesRead, DATA_RECEIVED);
                totalBytesRead += (int)bytesRead;
                connectionHandler->setRequestTotalBytesRead(totalBytesRead);
                //streamHandle->scheduleTimeoutWatchdog();
            }
            
            //pthread_mutex_unlock(cancelMtx);
    
            break;
        }
            
        case kCFStreamEventEndEncountered: {
            LOG.debug("%s: request read completed", __FUNCTION__);
            //streamHandle->unscheduleTimeoutWatchdog();
            //streamHandle->stopRequestProgressNotifier();
            
            connectionHandler->setRequestStatus(E_SUCCESS);
            //pthread_mutex_unlock(cancelMtx);
    
            dispatch_semaphore_signal(waitSem);
            
            break;
        }
            
        case kCFStreamEventErrorOccurred:
        {
            CFErrorRef streamError = CFReadStreamCopyError(stream);
            StringBuffer desc;
           
            //streamHandle->unscheduleTimeoutWatchdog();
            //streamHandle->stopRequestProgressNotifier();
            
            if (streamError) {
                CFStringRef errorDesc = CFErrorCopyDescription(streamError);
                
                if (errorDesc) {
                    desc = CFString2StringBuffer(errorDesc);
                }
            }
            
            if (desc.empty()) {
                LOG.error("%s: error reading from HTTP stream", __FUNCTION__);
            } else {
                LOG.error("%s: error reading from HTTP stream (%s)", __FUNCTION__, desc.c_str());
            }
            
            connectionHandler->setRequestStatus(E_NET_READING);
            //pthread_mutex_unlock(cancelMtx);
    
            dispatch_semaphore_signal(waitSem);
            break;
        }
        
        default: {
            //pthread_mutex_unlock(cancelMtx);
    
            break;
        }
    }
}

static void readStreamClientCallBack(CFReadStreamRef stream, CFStreamEventType type, void *clientCallBackInfo)
{
    //StreamDataHandle* streamHandle = (StreamDataHandle *)clientCallBackInfo;
    HttpConnectionHandler* connectionHandler = reinterpret_cast<HttpConnectionHandler*>(clientCallBackInfo); 
 
    unsigned int read_size = 0;  
    CFIndex bytesRead = 0;
    size_t totalBytesRead = 0;
    OutputStream* os = NULL;
    dispatch_semaphore_t waitSem; 
    pthread_mutex_t* cancelMtx = NULL; 
  
    if (connectionHandler == NULL) {
        LOG.error("%s: invalid callback info parameter", __FUNCTION__);
        
        return;
    }
    
    cancelMtx = connectionHandler->getCancelLock();
    
    //pthread_mutex_lock(cancelMtx);
    read_size = connectionHandler->getReadChunkSize();
    os = connectionHandler->getOutputStream(); 
    waitSem = connectionHandler->getWaitSem();
    
    switch (type) {
        case kCFStreamEventOpenCompleted: {
            //streamHandle->setStreamOpened(true);
            //streamHandle->unscheduleTimeoutWatchdog();
            //streamHandle->stopRequestProgressNotifier();
            //streamHandle->setStreamConnected(true);
            //streamHandle->setStopStreamReading(false);
            //streamHandle->setWriteToStreamCompleted(true);
            //pthread_mutex_unlock(cancelMtx);
   
            LOG.debug("%s: HTTP request sent", __FUNCTION__);
            
            break;
        }
        
        case kCFStreamEventHasBytesAvailable: {
            
            if (stream != NULL) {
                UInt8   buffer[read_size];
                
                LOG.debug("%s: Reading from stream %p",__FUNCTION__,stream);
                
                if ((bytesRead = CFReadStreamRead(stream, buffer, read_size - 1)) > 0) {
                    size_t written = os->write((const void*)buffer, (int)bytesRead);
                    
                    fireTransportEvent((int)bytesRead, DATA_RECEIVED);
                    totalBytesRead += (int)bytesRead;
                    connectionHandler->setRequestTotalBytesRead(totalBytesRead);
                    //streamHandle->scheduleTimeoutWatchdog();
                    if (written == -1) {
                        LOG.info("%s: Cannot write received bytes, abort request (%p)",__FUNCTION__,stream);
                        connectionHandler->setRequestStatus(E_DATA_WRITING);
                        dispatch_semaphore_signal(waitSem);
                    }
                }
            }
            
            break;
        }
            
        case kCFStreamEventEndEncountered: {
            LOG.debug("%s: request read completed", __FUNCTION__);
            //streamHandle->unscheduleTimeoutWatchdog();
            //streamHandle->stopRequestProgressNotifier();
            
            connectionHandler->setRequestStatus(E_SUCCESS);
            pthread_mutex_unlock(cancelMtx);
   
            dispatch_semaphore_signal(waitSem);
    
            break;
        }
            
        case kCFStreamEventErrorOccurred:
        {
            CFErrorRef streamError = CFReadStreamCopyError(stream);
            StringBuffer desc;
            
            //streamHandle->unscheduleTimeoutWatchdog();
            //streamHandle->stopRequestProgressNotifier();
            
            if (streamError) {
                CFStringRef errorDesc = CFErrorCopyDescription(streamError);
                
                if (errorDesc) {
                    desc = CFString2StringBuffer(errorDesc);
                }
            }
            
            if (desc.empty()) {
                LOG.error("%s: error reading from HTTP stream", __FUNCTION__);
            } else {
                LOG.error("%s: error reading from HTTP stream (%s)", __FUNCTION__, desc.c_str());
            }
            
            connectionHandler->setRequestStatus(E_NET_READING);
            //pthread_mutex_unlock(cancelMtx);
            
            dispatch_semaphore_signal(waitSem);
            
            break;
        }
        
        default: {
            //pthread_mutex_unlock(cancelMtx);
   
            break;
        }
    }
}

void stopRequestThread(void *arg)
{
    HttpConnectionHandler* connectionHandler = (HttpConnectionHandler *)arg;
    
    connectionHandler->stopRequest();
}

/**
 * unschedule from main loop the network read/write callback
 * called by thread cleanup and timeout handlers
 */
void unscheduleRequest(StreamDataHandle* streamHandle)
{
    if (streamHandle) {
        CFReadStreamRef responseStream = NULL;
        pthread_mutex_t* cancelMtx = streamHandle->getCancelLock();
  
  
        LOG.debug("%s: stopping run loop timer", __FUNCTION__);
        streamHandle->unscheduleTimeoutWatchdog();
        
        LOG.debug("%s: stopping progress notification timer", __FUNCTION__);
        streamHandle->stopRequestProgressNotifier();
  
        pthread_mutex_lock(cancelMtx);
        
        if ((responseStream = streamHandle->getResponseStream())) {
            bool requestScheduled = streamHandle->getRequestThreadScheduled();
       
            // remove client to unregister events on stream
            //CFReadStreamSetClient(responseStream, kCFStreamEventNone, NULL, NULL);
            
            if (requestScheduled) {
                dispatch_semaphore_t waitSem = streamHandle->getWaitSem();
                
                // unschedule stream from runloop
                //CFReadStreamUnscheduleFromRunLoop(responseStream, CFRunLoopGetMain(), kCFRunLoopCommonModes);
                pthread_mutex_unlock(cancelMtx);
        
                dispatch_semaphore_signal(waitSem);
                
                return;
            }
        }
        
        pthread_mutex_unlock(cancelMtx);
    }
}

/**
 * thread cleanup handler (called on cancellation request 
 * sent to main thread)
 */ 
void stopReaderThread(void *arg)
{
    StreamDataHandle* streamHandle = (StreamDataHandle *)arg;
    
    if (streamHandle == NULL) {
        LOG.error("%s: invalid data handle", __FUNCTION__);
        
        return;
    }
    
    streamHandle->setRequestStatus(E_REQUEST_CANCELLED);
    LOG.debug("%s: cancelling request...", __FUNCTION__);
    unscheduleRequest(streamHandle);
}

/**
 * timeout hanlder
 */
void timeoutHandler(CFRunLoopTimerRef timer, void *info)
{
#ifdef __ENABLE_HTTP_HANDLER_TIMEOUT__
    StreamDataHandle* streamHandle = (StreamDataHandle *)info;
    
    if (streamHandle == NULL) {
        LOG.error("%s: invalid data handle", __FUNCTION__);
        
        return;
    }
    
    streamHandle->setRequestStatus(E_TIMEOUT_REACHED);
    
    LOG.debug("%s: unscheduling request thread", __FUNCTION__);
    unscheduleRequest(streamHandle);
#endif
}

void progressNotifier(CFRunLoopTimerRef timer, void *info)
{
    StreamDataHandle* streamHandle = (StreamDataHandle *)info;
    size_t bytesUploaded = streamHandle->getBytesUploaded();
    
    if (streamHandle) {
        int totalBytesUploaded = 0;
        CFReadStreamRef responseStream = streamHandle->getResponseStream();
        
        if (streamHandle->isTimeoutWatchdogRunning() == false) {
            streamHandle->scheduleTimeoutWatchdog();
        } 
        
        CFNumberRef bytesUploadedRef = (CFNumberRef)CFReadStreamCopyProperty(responseStream, kCFStreamPropertyHTTPRequestBytesWrittenCount);
        
        // The above method may return NULL if it is invoked too early. In that case it can just return
        if (bytesUploadedRef) {
            if (CFNumberGetValue(bytesUploadedRef,  kCFNumberIntType, (void *)&totalBytesUploaded)) {            
                if (bytesUploaded != totalBytesUploaded) {
                    streamHandle->unscheduleTimeoutWatchdog();
                    // fire transport notification: get bytes uploaded from last notification 
                    // subtracting streamHandle->bytesUploaded from bytesUploaded counter (total 
                    // bytes count from kCFStreamPropertyHTTPRequestBytesWrittenCount)
                    fireTransportEvent((unsigned long)(totalBytesUploaded - bytesUploaded), DATA_SENT);
                    streamHandle->setBytesUploaded(totalBytesUploaded);
                }
            }
        }
    }
}

void* timeoutWatchdogThreadStartRoutine(void *arg)
{
#ifdef __ENABLE_HTTP_HANDLER_TIMEOUT__
    StreamDataHandle* dataHandle = reinterpret_cast<StreamDataHandle *>(arg);
    
    if (dataHandle) {
        dataHandle->scheduleTimeoutWatchdogTimer();
    }
#endif
    return NULL;
}

void* uploadProgressNotifierThreadStartRoutine(void *arg)
{
    UploadProgressMonitor* monitor = reinterpret_cast<UploadProgressMonitor*>(arg);
    if (monitor) {
        monitor->run();
    }
    return NULL;
}

void* progressNotifierThreadStartRoutine(void *arg)
{
#ifdef __ENABLE_HTTP_PROGRESS_NOTIFIER__
    StreamDataHandle* dataHandle = reinterpret_cast<StreamDataHandle *>(arg);
    
    if (dataHandle) {
        dataHandle->startRequestProgressNotifierTimer();
    }
#endif
    return NULL;
}

void cancelRequestCallBack(CFNotificationCenterRef center, void *observer,
                        CFStringRef name, const void *object, CFDictionaryRef userInfo)
{
    HttpConnectionHandler* connectionHandler = reinterpret_cast<HttpConnectionHandler *>(observer);
       
    if (connectionHandler) {
        if (userInfo) {
            int request_token = 0;
            CFNumberRef requestToken = (CFNumberRef)CFDictionaryGetValue(userInfo, CFSTR(CONNECTION_HANDLER_REQUEST_TOKEN_KEY));

            if (CFNumberGetValue(requestToken, kCFNumberIntType, &request_token)) {
                LOG.debug("%s: request token from notification info: %d", __FUNCTION__, request_token);
                if (request_token == connectionHandler->getRequestToken()) {
                    connectionHandler->stopRequest();
                }
            }
        }
    }
}


END_FUNAMBOL_NAMESPACE

#endif
