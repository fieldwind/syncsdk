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

#include "HttpConnection.h"
#include "HttpConnectionHandler.h"
#include "base/util/StringUtils.h"
#include "ioStream/AppleBufferInputStream.h"

#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>

BEGIN_FUNAMBOL_NAMESPACE

HttpConnectionType HttpConnection::connectionType = EUnknownConnection;

HttpConnection::HttpConnection(const char* user_agent) : AbstractHttpConnection(user_agent),
                                                         clientRequest(NULL),
                                                         responseStream(NULL), 
                                                         gotflags(true), 
                                                         isReachable(true),
                                                         noConnectionRequired(true),
                                                         http_verb(NULL),
                                                         contentLen(0),
                                                         isSecureConnection(false)
{}

HttpConnection::~HttpConnection()
{
    close();
}

int HttpConnection::open(const URL& url, RequestMethod method, bool log_request)
{
    CFStringRef CFurl = nil;
    CFURLRef requestURL = nil;
        
    if ((url.fullURL == NULL) || (strlen(url.fullURL) == 0)) {
        setErrorF(ERR_HTTP_INVALID_URL, "%s - can't open connection: invalid url.", __FUNCTION__);
        LOG.error("%s: can't open connection: invalid url.", __FUNCTION__);
        requestHeaders.clear();
    
        return 1;
    }

    if (url.isSecure()) {
        isSecureConnection = true;
    }
    
    switch (method) {
        case MethodGet:
            http_verb = CFSTR(METHOD_GET);
            break;
        case MethodPost:
            http_verb = CFSTR(METHOD_POST);
            break;
        default:
            requestHeaders.clear();

            // HTTP method not supported...
            LOG.error("%s: unsupported HTTP request type", __FUNCTION__);
            return 1;
    }
    
    this->url = url;

    // Construct URL
    CFurl =  CFStringCreateWithCString(NULL, url.fullURL, kCFStringEncodingUTF8);
    requestURL = CFURLCreateWithString(kCFAllocatorDefault, CFurl, NULL);
    clientRequest = CFHTTPMessageCreateRequest(kCFAllocatorDefault, http_verb, requestURL, kCFHTTPVersion1_1);

    if (CFurl) 
    { 
        CFRelease(CFurl);  
    }
    
    if (requestURL) {
        CFRelease(requestURL); 
    }

    if (!clientRequest){
        LOG.error("%s: error can't create HTTP request", __FUNCTION__);
        setErrorF(ERR_NETWORK_INIT, "%s: error: can't create HTTP request", __FUNCTION__);
        requestHeaders.clear();
    
        return 1;
    }
    
    connectionType = EUnknownConnection;

#if defined(FUN_IPHONE)    
    SCNetworkReachabilityFlags flags;
    SCNetworkReachabilityRef   scnReachRef = SCNetworkReachabilityCreateWithName(kCFAllocatorDefault, url.host);

    gotflags = SCNetworkReachabilityGetFlags(scnReachRef, &flags);
    isReachable = flags & kSCNetworkReachabilityFlagsReachable;
    noConnectionRequired = !(flags & kSCNetworkReachabilityFlagsConnectionRequired);
    
    if ((flags & kSCNetworkReachabilityFlagsIsWWAN)) {
        noConnectionRequired = true;
        //LOG.debug("%s: connection type is WWAN", __FUNCTION__);
        connectionType = EGPRS;
    }
    else {
        //LOG.debug("%s: connection type is not WWAN", __FUNCTION__);
        connectionType = EWifi;
    }

    CFRelease(scnReachRef);
#endif

    return 0;
}

int HttpConnection::request(InputStream& stream, OutputStream& response, bool log_request)
{
    int ret = STATUS_OK;
    AppleInputStream* data = dynamic_cast<AppleInputStream *>(&stream);
    CFReadStreamRef dataStream = NULL;
    size_t requestPayloadSize = 0;
    StringBuffer contentLenStr;
    
    if (data == NULL) {
        LOG.error("Invalid input stream.");
        return StatusInternalError;
    }
    
    if (gotflags && isReachable && noConnectionRequired) {
        KeyValuePair headersKvPair;
        
        if (http_verb == NULL) {
            LOG.error("%s: can't create HTTP request: request method not defined");
            return StatusInternalError;
        }
        
        if (log_request) {
            LOG.debug("%s: requesting resource %s at %s:%d", __FUNCTION__, url.resource, url.host, url.port);
        }
        
        // check if we have a valid stream associated to the request: 
        // get data size from attached stream (considering the current stream offset)
        if ((requestPayloadSize = (data->getTotalSize() - data->getPosition())) > 0) {
            dataStream = data->getStream();
        } else {
            // get content length set in HTTP body
            requestPayloadSize = contentLen;
        }
        
        // set HTTP headers:
        setRequestHeader(HTTP_HEADER_USER_AGENT, userAgent);

        if (requestPayloadSize > 0) {
            LOG.debug("%s: setting HTTP content length header request (size: %d)", __FUNCTION__, requestPayloadSize);
            contentLenStr.sprintf("%ld", requestPayloadSize);
            setRequestHeader(HTTP_HEADER_CONTENT_LENGTH, contentLenStr.c_str());
        }
        
        if (log_request) {
            LOG.debug("Request header:");
        }
        
        for (headersKvPair = requestHeaders.front(); !headersKvPair.null(); headersKvPair = requestHeaders.next()){
            const char* key = headersKvPair.getKey().c_str();
            const char* val = headersKvPair.getValue().c_str();
            
            if ((key == NULL) || (val == NULL)) {
                LOG.info("%s: null value in KeyValuePair", __FUNCTION__);
                continue;
            }
            
            CFStringRef hdrKey = CFStringCreateWithCString(NULL, key, kCFStringEncodingUTF8);
            CFStringRef hdrVal = CFStringCreateWithCString(NULL, val, kCFStringEncodingUTF8);

            CFHTTPMessageSetHeaderFieldValue(clientRequest, hdrKey, hdrVal);
            
            if (log_request) {
                LOG.debug("    %s: %s", key, val);
            }
            
            CFRelease(hdrKey);
            CFRelease(hdrVal);
        }
    
        if (auth) {
            if (!addHttpAuthentication(&clientRequest)) {
                LOG.error("Failed to add HTTP authentication information...");
                CFRelease(clientRequest);
                clientRequest = NULL;
                return StatusInternalError;
            }
        }

        if ((ret = sendRequest(dataStream, response, requestPayloadSize)) != StatusNoError) {
            goto finally;
        }
        
        ret = getResponse();
    } else {
        setErrorF(ERR_CONNECT, "network error: can't connect to the server");
        LOG.error("%s", "network error: can't connect to the server");
        ret = StatusNetworkError;
    }

finally:
    if (responseStream != NULL) {
        CFReadStreamClose(responseStream);
        CFRelease(responseStream);
        responseStream = NULL;
    }

    return ret;
}

int HttpConnection::request(const char* data, OutputStream& response, bool log_request)
{         
    int requestStatus = 0;
    CFDataRef bodyData = NULL;
    AppleBufferInputStream inputStream("");
    
    if (data) {
        size_t dataLen = strlen(data);
        
        if (log_request) {
            LOG.debug("%s: request body: %s", __FUNCTION__, data);
        }
        
        bodyData = CFDataCreate(kCFAllocatorDefault, (const UInt8*)data, dataLen);
    
        if (!bodyData){
            LOG.error("%s: error in CFDataCreate", __FUNCTION__);
  
            return StatusInternalError;
        }
        
        contentLen = dataLen;
        CFHTTPMessageSetBody(clientRequest, bodyData);
    }
    
    requestStatus = request(inputStream, response, log_request);
  
    if (bodyData) {
        CFRelease(bodyData);
    }
    
    return requestStatus;
}

int HttpConnection::close()
{
    if (clientRequest) {
        LOG.debug("%s Releasing clientRequest %p",__FUNCTION__,clientRequest);
        CFRelease(clientRequest);
        clientRequest = NULL;
    }
    
    if (responseStream) {
        LOG.debug("%s: Closing response stream %p",__FUNCTION__,responseStream);
        CFRelease(responseStream);
        responseStream = NULL;
    }

    // reset connection data 
    contentLen = 0;
    requestHeaders.clear();
    responseHeaders.clear();
    
    return 0;
}

int HttpConnection::sendRequest(CFReadStreamRef requestBodyStream, OutputStream& os, size_t requestPayloadSize)
{
    int status = StatusNoError;
    
    HttpConnectionHandler* handler = new HttpConnectionHandler(requestTimeout,responseChunkSize,
                                                               uploadObserver);
        
    if (requestBodyStream) {
        responseStream = CFReadStreamCreateForStreamedHTTPRequest(kCFAllocatorDefault, clientRequest, requestBodyStream);
    } else {
        responseStream = CFReadStreamCreateForHTTPRequest(kCFAllocatorDefault, clientRequest);
    }
    
    if (responseStream == NULL) {
        LOG.error("%s: error creating HTTP data stream for upload", __FUNCTION__);
        delete handler;
        
        return StatusInternalError;
    }
    
    if (isSecureConnection && (SSLVerifyServer == false)) {
        // accept self signed and expired SSL certificates
        NSDictionary *sslProperties = [[NSDictionary alloc] initWithObjectsAndKeys:
                                      [NSNumber numberWithBool:YES], kCFStreamSSLAllowsExpiredCertificates,
                                      [NSNumber numberWithBool:YES], kCFStreamSSLAllowsAnyRoot,
                                      [NSNumber numberWithBool:NO],  kCFStreamSSLValidatesCertificateChain,
                                      kCFNull,kCFStreamSSLPeerName,
                                      nil];

        CFReadStreamSetProperty((CFReadStreamRef)responseStream,
                                    kCFStreamPropertySSLSettings,
                                    (CFTypeRef)sslProperties);
        [sslProperties release];
    }
    
    if ((status = handler->startConnectionHandler(responseStream, os, requestPayloadSize)) != 0) {
        if (status == ERR_CONNECT) {
            LOG.error("%s: connection failed", __FUNCTION__);
            status = StatusNetworkError;
        } else if (status == ERR_CONNECTION_TIMEOUT) {
            LOG.error("%s: connection timeout", __FUNCTION__);
            status = StatusTimeoutError;
        } else if (status == E_NET_READING) {
            LOG.error("%s: error reading http response", __FUNCTION__);
            status = StatusReadingError;
        } else if (status == E_DATA_WRITING) {
            LOG.error("%s: error writing http response to disk", __FUNCTION__);
            status = StatusStreamWritingError;
        } else if (status == E_REQUEST_CANCELLED) {
            LOG.info("%s: request cancelled", __FUNCTION__);
            status = StatusCancelledByUser;
        } else {
            LOG.error("%s: fatal error connecting to remote host", __FUNCTION__);
            status = StatusInternalError;
        }
    } else {
        status = StatusNoError;
    }
   
    delete handler;


    // the response body is in the outputStream "os"
    return status;
}

int HttpConnection::getResponse()
{
    CFHTTPMessageRef serverReply = NULL;
    int statusCode = STATUS_OK;
   
    serverReply = (CFHTTPMessageRef) CFReadStreamCopyProperty(responseStream, kCFStreamPropertyHTTPResponseHeader);

    // Pull the status code from the headers
    if (serverReply == NULL) {
        LOG.error("%s: errore getting server reply", __FUNCTION__);

        return StatusReadingError;
    }

    // Read the HTTP response status code
    statusCode = CFHTTPMessageGetResponseStatusCode(serverReply);
    // Read all the HTTP response headers
    readResponseHeaders(serverReply);

    if (serverReply != NULL) {
        CFRelease(serverReply);
    }

    LOG.debug("%s: HTTP message status code: %d", __FUNCTION__, statusCode);

    switch (statusCode) {
        case -1: {                    // connection error -> out code 2001
            setErrorF(ERR_CONNECT, "Network error in server receiving data");
            LOG.info("%s: Network error in server receiving data",__FUNCTION__);
            break;
        }
        case 400: { 
            setErrorF(ERR_SERVER_ERROR, "HTTP server error: %d. Server failure.", statusCode);
            LOG.info("%s: HTTP server error: %d. Server failure.", __FUNCTION__, statusCode);

            break;
        }
        case 500: {
            setErrorF(ERR_SERVER_ERROR, "HTTP server error: %d. Server failure.", statusCode);
            LOG.info("%s: HTTP server error: %d. Server failure.", __FUNCTION__, statusCode);
            break;
        }
        case 501: {
            setErrorF(HTTP_FUNCTIONALITY_NOT_SUPPORTED, 
                      "HTTP server error: %d. The server does not support the functionality needed to fulfill this request ().", 
                      statusCode);
            LOG.info("%s: HTTP server error: %d. Server failure.", __FUNCTION__, statusCode);
            break;
        }
        case 404: {
            setErrorF(ERR_HTTP_NOT_FOUND, "HTTP request error: resource not found (status %d)", statusCode);
            LOG.info("%s: HTTP request error: resource not found (status %d)", __FUNCTION__, statusCode);

            break;
        }
        case 408: {
            setErrorF(ERR_HTTP_REQUEST_TIMEOUT, "HTTP request error: server timed out waiting for request (status %d)", statusCode);
            LOG.info("%s: HTTP request error: server timed out waiting for request (status %d)", __FUNCTION__, statusCode);

            break;
        }
        case 401: {   // Authentication failed
            setErrorF(401, "Authentication failed");
            LOG.info("%s: Authentication failed",__FUNCTION__);
            break;
        }
        case 403: {   // Http operation forbidden
            setErrorF(403, "Request forbidden: media url expired?");
            LOG.info("%s: Http forbidden",__FUNCTION__);
            break;
        }
        case 402: {  // Http payment required
            setErrorF(402, "Payment required");
            LOG.info("%s: Payment required",__FUNCTION__);
            break;
        }
        default: {
            if (isErrorStatus(statusCode)) {
                statusCode = StatusNetworkError;
                setErrorF(statusCode, "HTTP request error: status received = %d", statusCode);
                LOG.info("%s: HTTP request error: status received = %d", __FUNCTION__, statusCode);
            } else {
                LOG.debug("HTTP request successful: code %d", statusCode);
            }
            
            break;
        }
    }

    return statusCode;
}

bool HttpConnection::addHttpAuthentication(CFHTTPMessageRef* request)
{
    if (!auth) {
        return false;
    }
    
    StringBuffer key(HTTP_HEADER_AUTHORIZATION);
    StringBuffer val;
    if (auth->getType() == HttpAuthentication::Basic) {
        val.sprintf("Basic %s", auth->getAuthenticationHeaders().c_str());
    }
    else {
        LOG.error("Digest authentication not yet supported - please use Basic auth");
        return false;
    }
    
    CFStringRef hdrKey = CFStringCreateWithCString(NULL, key, kCFStringEncodingUTF8);
    CFStringRef hdrVal = CFStringCreateWithCString(NULL, val, kCFStringEncodingUTF8);
    
    CFHTTPMessageSetHeaderFieldValue(clientRequest, hdrKey, hdrVal);
    LOG.debug("    %s: %s", key.c_str(), val.c_str());
    
    CFRelease(hdrKey);
    CFRelease(hdrVal);
    
    return true;
}

int HttpConnection::readResponseHeaders(CFHTTPMessageRef response)
{
    if (!response) {
        return -1;
    }

    responseHeaders.clear();
    
    CFDictionaryRef headers = CFHTTPMessageCopyAllHeaderFields(response);
    if (!headers) {
        LOG.error("Could not read the HTTP response headers");
        return -2;
    }
    
    int count = CFDictionaryGetCount(headers);
    if (count == 0) {
        // nothing to do
        return 0;
    }
    
    // Allocate the arrays of keys and values
    CFStringRef keys[count];
    CFStringRef values[count];
    for (int i=0; i<count; i++) {
        keys[i] = CFSTR("");
        values[i] = CFSTR("");
    }
    
    LOG.debug("%s:", __FUNCTION__);
    // Get the headers pairs and fill the stringMap
    CFDictionaryGetKeysAndValues(headers, (const void**)keys, (const void**)values);
    for (int i=0; i<count; i++) 
    {
        CFStringRef hdrKey = keys[i];
        CFStringRef hdrVal = values[i]; 
        if (!hdrKey || !hdrVal) continue;

        StringBuffer k = CFString2StringBuffer(hdrKey);
        StringBuffer v = CFString2StringBuffer(hdrVal);
        
        responseHeaders.put(k.c_str(), v.c_str());
        
        LOG.debug("\t%s: %s", k.c_str(), v.c_str());
        CFRelease(hdrKey);
        CFRelease(hdrVal);
    }

    return count;
}

bool HttpConnection::isWifiConnection() 
{
    return (connectionType == EWifi);
}

bool HttpConnection::isWifiConnection(const URL& url)
{
    bool wifiAvailable = false;
    
#if defined(FUN_IPHONE)    
    SCNetworkReachabilityFlags flags;
    SCNetworkReachabilityRef   scnReachRef = SCNetworkReachabilityCreateWithName(kCFAllocatorDefault, url.host);
   
    SCNetworkReachabilityGetFlags(scnReachRef, &flags);
    
    if ((int)flags != 0) {
        if (!(flags & kSCNetworkReachabilityFlagsIsWWAN)) {
            wifiAvailable = true;
        }
    }

    CFRelease(scnReachRef);
#endif

    return wifiAvailable;
}

bool HttpConnection::existsConnection(const URL& url)
{
    bool connAvailable = false;
    
#if defined(FUN_IPHONE)
    SCNetworkReachabilityFlags flags;
    SCNetworkReachabilityRef   scnReachRef = SCNetworkReachabilityCreateWithName(kCFAllocatorDefault, url.host);
    
    SCNetworkReachabilityGetFlags(scnReachRef, &flags); 
    
    if (!(flags & kSCNetworkReachabilityFlagsConnectionRequired)) {
        connAvailable = true;
    }
    
    CFRelease(scnReachRef);
#endif
    
    return connAvailable;
}

bool HttpConnection::onlyGPRSConnection(const URL& url)
{
    bool connAvailable = false;
    
#if defined(FUN_IPHONE)
    SCNetworkReachabilityFlags flags;
    SCNetworkReachabilityRef   scnReachRef = SCNetworkReachabilityCreateWithName(kCFAllocatorDefault, url.host);
    
    SCNetworkReachabilityGetFlags(scnReachRef, &flags);
    if ((int)flags != 0 && (int)flags != (int)kSCNetworkReachabilityFlagsReachable) {
       connAvailable = true;
    }
    
    CFRelease(scnReachRef);
#endif
    
    return connAvailable;
}

END_FUNAMBOL_NAMESPACE
