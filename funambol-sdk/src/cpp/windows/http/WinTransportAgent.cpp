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

/*
 How to test SSL connections
 ----------------------------

 On the server:
 1) create the keystore:
    %JAVA_HOME%\bin\keytool -genkey -alias tomcat -keyalg RSA
 2) In $CATALINA_HOME/conf/server.xml uncomment the lines:
    <Connector className="org.apache.catalina.connector.http.HttpConnector"
               port="8443" minProcessors="5" maxProcessors="75"
               enableLookups="true"
               acceptCount="10" debug="0" scheme="https" secure="true">
      <Factory className="org.apache.catalina.net.SSLServerSocketFactory" clientAuth="false" protocol="TLS"/>
    </Connector>
 2) Export the certificate from the key store:
    %JAVA_HOME%\bin\keytool -export -alias tomcat -file myroot.cer

 On the client:
  [for _WIN32_WCE]
   1)  Copy myroot.cer in a device/emulator directory
   2) Click on it to import the certificate as a trusted CA
  [for WIN32]
   1) Connect (via https) to the server using a web-browser (type "https://<server_address>:8443)
   2) Accept and install the certificate sent from the server
*/

#include "base/Log.h"
#include "base/messages.h"
#include "base/util/utils.h"
#include "base/util/StringBuffer.h"
#include "base/util/WString.h"
#include "base/util/KeyValuePair.h"
#include "http/constants.h"
#include "http/errors.h"
#include "http/WinTransportAgent.h"
#include "event/FireEvent.h"
#include "http/WinDigestAuthHashProvider.h"
#include <Wincrypt.h>

#ifdef _WIN32_WCE
#include "http/GPRSConnection.h"
#endif

#define MAX_PROXY_RETRY 1
#define ENTERING(func) // LOG.debug("Entering %ls", func);
#define EXITING(func)  // LOG.debug("Exiting %ls", func);

#ifdef USE_ZLIB
#include "zlib.h"
#include "base/globalsdef.h"
#endif

USE_FUNAMBOL_NAMESPACE

// static handles, used only to break internet connection
static HINTERNET lastInet = NULL;
static HINTERNET lastConnection = NULL;
static HINTERNET lastReq = NULL;

/**
 * Constructor.
 * In this implementation newProxy is ignored, since proxy configuration
 * is taken from the WinInet subsystem.
 *
 * @param url    the url where messages will be sent with sendMessage()
 * @param proxy  proxy information or NULL if no proxy should be used
 */
WinTransportAgent::WinTransportAgent(const URL& newURL, Proxy& newProxy,
                                     unsigned int maxResponseTimeout,
                                     unsigned int maxmsgsize)
                                     // Use base class constructor to initialize common attributes
                                     : TransportAgent(newURL,
                                     newProxy,
                                     maxResponseTimeout,
                                     maxmsgsize) {

    if (maxResponseTimeout == 0) {
        setTimeout(DEFAULT_MAX_TIMEOUT);
    } else {
        setTimeout(maxResponseTimeout);
    }

    isToDeflate    = false;
    isFirstMessage = true;
    isToInflate    = false;
    this->auth = NULL;
    setHttpVerb(HTTP_POST);
    numberOfRetryForProxy = 1;
#ifdef _WIN32_WCE
    keepalive = false;
    // used by default. check connection before...
    if (!EstablishConnection()) {

#  ifdef WIN32_PLATFORM_PSPC
        setErrorF(ERR_INTERNET_CONNECTION_MISSING, "%s: %d",
                "Internet Connection Missing",
                ERR_INTERNET_CONNECTION_MISSING);
#  else
        LOG.error("Warning: internet connection missing.");
#  endif  // #ifdef WIN32_PLATFORM_PSPC
    }
#endif  // #ifdef _WIN32_WCE

}

WinTransportAgent::~WinTransportAgent()
{
#ifdef _WIN32_WCE
    if (!keepalive)
        DropConnection();
    keepalive = false;
#endif
}

int WinTransportAgent::closeConnection() {

    if (lastInet) {
        InternetCloseHandle (lastInet);
        lastInet = NULL;
    }

    if (lastConnection) {
        InternetCloseHandle (lastConnection);
        lastConnection = NULL;
    }

    if (lastReq) {
        InternetCloseHandle (lastReq);
        lastReq = NULL;
    }

    return 0;
}



/*
 * Sends the given SyncML message to the server specified
 * by the install property 'url'. Returns the server's response.
 * The response string has to be freed with delete [].
 * In case of an error, NULL is returned and lastErrorCode/Msg
 * is set.
 */
char* WinTransportAgent::sendMessage(const char* msg) {
        return sendMessage(msg, strlen(msg));
}


char* WinTransportAgent::sendMessage(const char* msg, const unsigned int contentLength)
{


    std::string message = "";
    if (getTimeout() == 99) {
        LOG.debug("Send message (logged in clear mode):\n%s", msg);
    } else {
        message = removeSensibleData(msg);
        LOG.debug("Send message:\n%s", message.c_str());
    }
    
    ENTERING(L"TransportAgent::sendMessage");

    // A non-null UserAgent is required: otherwise HttpSendRequest() 
    // will fail with code 12150 (The requested header was not found).
    const char* uagent = getUserAgent();
    if (!uagent || !strlen(uagent)) {
        setUserAgent(USER_AGENT);
    }

#ifdef USE_ZLIB
    // This is the locally allocated buffer for the compressed message.
    // Must be deleted after send.
    Bytef* compr   = NULL;
    WCHAR* wbuffer = NULL;
    WCHAR* buffer  = NULL;
#endif

    char* bufferA = new char[readBufferSize+1];
    int status = -1;
    WCHAR* wurlHost      = NULL;
    WCHAR* wurlResource  = NULL;
    char*  p             = NULL;
    char*  response      = NULL;
    DWORD  realContentLength = contentLength;
    WString headers;
        
    HINTERNET inet       = NULL,
              connection = NULL,
              request    = NULL;


    // Check sending msg and host.
    if (!msg) {
        setError(ERR_NETWORK_INIT, "TransportAgent::sendMessage error: NULL message.");
        goto exit;
    }
    if (!(url.host) || strlen(url.host) == 0) {
        setErrorF(ERR_HOST_NOT_FOUND, "TransportAgent::sendMessage error: %s.", ERRMSG_HOST_NOT_FOUND);
        goto exit;
    }

    DWORD size  = 0,
          read  = 0,
          flags = INTERNET_FLAG_RELOAD |
                  INTERNET_FLAG_NO_CACHE_WRITE |
                  INTERNET_FLAG_KEEP_CONNECTION |           // This is necessary if authentication is required.
                  INTERNET_FLAG_NO_COOKIES;                 // This is used to avoid possible server errors on successive sessions.

        LPCWSTR acceptTypes[2] = {TEXT("*/*"), NULL};


    // Set flags for secure connection (https).
    if (url.isSecure()) {
        if (getSSLVerifyServer() == false) {
            flags = flags
                  | INTERNET_FLAG_SECURE
                  | INTERNET_FLAG_IGNORE_CERT_CN_INVALID
                  | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID
                  ;
        } else {
            flags = flags
                  | INTERNET_FLAG_SECURE;
        }
    }
    
    //
    // Open Internet connection.
    //
    WCHAR* ua = toWideChar(userAgent);
    inet = InternetOpen (ua, INTERNET_OPEN_TYPE_PRECONFIG, NULL, 0, 0);
    if (ua) {delete [] ua; ua = NULL; }

    if (!inet) {
        DWORD code = GetLastError();
        char* tmp = createHttpErrorMessage(code);
        setErrorF(ERR_NETWORK_INIT, "InternetOpen Error: %d - %s", code, tmp);
        delete [] tmp;
        goto exit;
    }
    LOG.debug("Connecting to %s:%d", url.host, url.port);


    //
    // Open an HTTP session for a specified site by using lpszServer.
    //
    wurlHost = toWideChar(url.host);
    if (!(connection = InternetConnect (inet,
                                        wurlHost,
                                        url.port,
                                        NULL, // username
                                        NULL, // password
                                        INTERNET_SERVICE_HTTP,
                                        0,
                                        0))) {
        DWORD code = GetLastError();
        char* tmp = createHttpErrorMessage(code);
        setErrorF(ERR_CONNECT, "InternetConnect Error: %d - %s", code, tmp);
        delete [] tmp;
        goto exit;
    }
    LOG.debug("Requesting resource %s", url.resource);

    //
    // Open an HTTP request handle. By default it uses the POST method (see constructor).
    // it could be set the GET if someone would use it. The api doesn't use
    // GET at all.
    //    
    const WCHAR* http_verb = METHOD_POST;
	if (getHttpVerb() == HTTP_GET) {
        http_verb = METHOD_GET;
    }
    wurlResource = toWideChar(url.resource);
    if (!(request = HttpOpenRequest (connection,
                                     http_verb,
                                     wurlResource,
                                     HTTP_VERSION,
                                     NULL,
                                     acceptTypes,
                                     flags, 0))) {
        DWORD code = GetLastError();
        char* tmp = createHttpErrorMessage(code);
        setErrorF(ERR_CONNECT, "HttpOpenRequest Error: %d - %s", code, tmp);
        delete [] tmp;
        goto exit;
    }

    // save static handles
    lastInet = inet;
    lastConnection = connection;
    lastReq = request;


    //
    // Prepares headers
    //
        
    // Msg to send is the original msg by default.
    // If compression is enabled, it will be switched to
    // compr. We don't want to touch this pointer, so
    // it's const (msg is also const).
    const void* msgToSend = (const void*)msg;

    // For user agent, content length and accept encoding, override property
    // values, even if set by the caller.
    setProperty(TA_PropertyUserAgent, getUserAgent());
    setProperty(TA_PropertyContentLength, StringBuffer().append(realContentLength));

#ifdef USE_ZLIB

    if(compression){

        //
        // Say the client can accept the zipped content but the first message is clear
        //
        if (isFirstMessage || !isToDeflate) {

            // append Accept-Encoding property
            setProperty(TA_PropertyAcceptEncoding, "deflate");
            isFirstMessage = false;
        }
        else if (isToDeflate) {
            //
            // DEFLATE (compress data)
            //
            uLong comprLen = realContentLength;
            compr = new Bytef[realContentLength];

            // Compresses the source buffer into the destination buffer.
            int err = compress(compr, &comprLen, (Bytef*)msg, realContentLength);
            if (err != Z_OK) {
                setError(ERR_HTTP_DEFLATE, "ZLIB: error occurred compressing data.");
                delete [] compr;
                compr = NULL;
                goto exit;
            }

            // Msg to send is the compressed data.
            msgToSend = (const void*)compr;
            int uncomprLenght = realContentLength;
            realContentLength = comprLen;

            setProperty(TA_PropertyContentLength, StringBuffer().append(realContentLength));
            setProperty(TA_PropertyAcceptEncoding, "deflate");
            setProperty(TA_PropertyUncompressedContentLength, StringBuffer().append(uncomprLenght));
            setProperty(TA_PropertyContentEncoding, "deflate");
        }
    } //end if compression

#endif

    // Timeout to receive a rensponse from server (default = 5 min).
    DWORD timeoutMsec = timeout*1000;
    InternetSetOption(request, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeoutMsec, sizeof(DWORD));

    if (auth) {
        addAuthenticationHeaders(request);
    }

    writeHttpHeaders(headers);
    LOG.debug("Request header:\n\n ***");
    // LOG.debug("Request header:\n\n%ls", headers.c_str());
    responseProperties.clear();

    // if the client allows to sync over https even if the server
    // has an invalid certificate, the flag is false. By default it is true
    /*if (getSSLVerifyServer() == false) {
        DWORD dwFlags, dwBuffLen = sizeof(dwFlags);
        InternetQueryOption (request, INTERNET_OPTION_SECURITY_FLAGS,
                                                         (LPVOID)&dwFlags, &dwBuffLen);    
        dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
        InternetSetOption (request, INTERNET_OPTION_SECURITY_FLAGS,
                                                   &dwFlags, sizeof (dwFlags));        
    }*/

    if (url.isSecure()) {
        DWORD dwFlags, dwBuffLen = sizeof(dwFlags);
        InternetQueryOption (request, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID)&dwFlags, &dwBuffLen);    
        if (getSSLVerifyServer() == false) {            
            dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_REVOCATION | SECURITY_FLAG_IGNORE_WRONG_USAGE 
                | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
            
        } else {
             dwFlags = dwFlags| INTERNET_FLAG_SECURE;
        }
        InternetSetOption (request, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof (dwFlags));    
    }
      
    //
    // Try MAX_RETRIES times to send http request, in case of network errors
    //
    DWORD errorCode = 0;
    int numretries;
    for (numretries=0; numretries < MAX_RETRIES; numretries++) {

        //
        // Send a request to the HTTP server.
        //
        if (!HttpSendRequest(request, headers.c_str(), headers.length(), (LPVOID)msgToSend, realContentLength)) {
                        
            errorCode = GetLastError();
            if (errorCode == ERROR_INTERNET_SEC_CERT_REV_FAILED) {
                LOG.info("%s: error ERROR_INTERNET_SEC_CERT_REV_FAILED: retry once a time", __FUNCTION__);
                DWORD dwFlags, dwBuffLen = sizeof(dwFlags);
                InternetQueryOption (request, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID)&dwFlags, &dwBuffLen);    
                dwFlags |= SECURITY_FLAG_IGNORE_REVOCATION;
                InternetSetOption (request, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof (dwFlags));                    
                continue;
            }
            char* tmp = createHttpErrorMessage(errorCode);
            setErrorF(GetLastError(), "HttpSendRequest error %d: %s", errorCode, tmp);
            LOG.info("%s", getLastErrorMsg());
            delete [] tmp; tmp = NULL;
            //
            // The certificate is not trusted. Send the right error code to the
            // client
            //
            if (errorCode == ERROR_INTERNET_INVALID_CA) {
                
                setError(ERR_HTTPS_INVALID_CA, "The certificate is invalid");
                LOG.error("%s", getLastErrorMsg());
                                                
                // try to understand a bit more on the certificate
                INTERNET_CERTIFICATE_INFO   certificateInfo;
                DWORD                       certInfoLength = sizeof(INTERNET_CERTIFICATE_INFO);                
                if (TRUE == InternetQueryOption(request, 
                                                INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT, 
                                                &certificateInfo, &certInfoLength)) {
                    
                    char* subj   = (char*)certificateInfo.lpszSubjectInfo;
                    char* issuer = (char*)certificateInfo.lpszIssuerInfo;    
                    LOG.debug("Cert Subject %s", subj);
                    LOG.debug("Cert Issuer %s",  issuer);

                } else {                        
                    LOG.debug("Cannot retrieve info about the certificate");
                }     
                goto exit;                
            }
            
            if (errorCode == ERROR_INTERNET_OFFLINE_MODE) {                     // 00002 -> retry
                LOG.debug("Offline mode detected: go-online and retry...");
                WCHAR* wurl = toWideChar(url.fullURL);
                InternetGoOnline(wurl, NULL, NULL);
                delete [] wurl;
                resetError();
                continue;
            }
            else if (errorCode == ERROR_INTERNET_TIMEOUT ||                     // 12002 -> out code 2007
                     errorCode == ERROR_INTERNET_INCORRECT_HANDLE_STATE) {      // 12019 -> out code 2007
                //lastErrorCode = ERR_HTTP_TIME_OUT;
                //sprintf(lastErrorMsg, "Network error: the request has timed out -> exit.");
                setError(ERR_HTTP_TIME_OUT, "Network error: the request has timed out -> exit.");
                LOG.debug("%s", getLastErrorMsg());
                goto exit;
            }
            else if (errorCode == ERROR_INTERNET_CANNOT_CONNECT) {              // 12029 -> out code 2001
                //lastErrorCode = ERR_CONNECT;
                //sprintf(lastErrorMsg, "Network error: the attempt to connect to the server failed -> exit");
                setError(ERR_CONNECT, "Network error: the attempt to connect to the server failed -> exit"); 
                LOG.debug("%s", getLastErrorMsg());
                goto exit;
            }
            // Other network error: retry.
            LOG.info("Network error writing data from client: retry %i time...", numretries + 1);
            resetError();
            continue;
        }

        LOG.debug("%s", MESSAGE_SENT);
                
                
        //
        // Check the status code.
        //
        size = sizeof(status);
        HttpQueryInfo (request,
                       HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                       (LPDWORD)&status,
                       (LPDWORD)&size,
                       NULL);
        
        // OK: status 200
        if (status == HTTP_STATUS_OK) {
                LOG.debug("Data sent successfully to server. Server responds OK");
            break;
        }
                
#if defined(WIN32) && !defined(_WIN32_WCE)
        //
        // Proxy Authentication Required (407) / Server Authentication Required (401).
        // Need to set username/password.
        //
        else if (status == HTTP_STATUS_DENIED) {
            LOG.info("%s: received status 401 from server", __FUNCTION__);
            // status = HTTP_UNAUTHORIZED
            setError(HTTP_UNAUTHORIZED, "Authentication required.");
            goto exit;
            break;            
        }
        else if (status == HTTP_STATUS_PROXY_AUTH_REQ) {
            LOG.debug("%s: HTTP Proxy Authentication required.", __FUNCTION__);
            DWORD dwError = HTTP_PROXY_UNAUTHORIZED;

            // Automatic authentication (user/pass stored in win reg key).
            if (strcmp(proxy.user, "") && strcmp(proxy.password, "") && !auth && (numberOfRetryForProxy == MAX_PROXY_RETRY)) {
                WCHAR* wUser = toWideChar(proxy.user);
                WCHAR* wPwd  = toWideChar(proxy.password);

                InternetSetOption(request, INTERNET_OPTION_PROXY_USERNAME, wUser, wcslen(wUser)+1);
                InternetSetOption(request, INTERNET_OPTION_PROXY_PASSWORD, wPwd,  wcslen(wPwd)+1);

                delete [] wUser;
                delete [] wPwd;
                dwError = ERROR_INTERNET_FORCE_RETRY;
                numberOfRetryForProxy++;
                continue;
            }

            // Prompt dialog box.
            /* else if (!auth) {
                dwError = InternetErrorDlg(GetDesktopWindow(), request, NULL,
                                           FLAGS_ERROR_UI_FILTER_FOR_ERRORS |
                                           FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS |
                                           FLAGS_ERROR_UI_FLAGS_GENERATE_DATA,
                                           NULL);
            } 
            if (dwError == ERROR_INTERNET_FORCE_RETRY) {
                continue;
            }
            */
            else {
                LOG.error("HTTP Proxy authentication required.");
                // status = HTTP_PROXY_UNAUTHORIZED;
                setError(HTTP_PROXY_UNAUTHORIZED, "Proxy authentication required.");
                numberOfRetryForProxy = 1; // reset the value
                goto exit;                
            }
        }
#endif  // #if defined(WIN32) && !defined(_WIN32_WCE)
        
        else if (status == HTTP_ERROR) {                    // 400 bad request error. retry to send the message
            LOG.info("Network error in server receiving data. "
                     "Server responds 400: retry %i time...", numretries + 1);
            continue;
        }
        else if (status == HTTP_STATUS_SERVER_ERROR ) {     // 500 -> out code 2052
            setErrorF(ERR_SERVER_ERROR, "HTTP server error: %d. Server failure.", status);
            LOG.debug("%s", getLastErrorMsg());
            goto exit;
        }
                
#ifdef _WIN32_WCE
        // To handle the http error code for the tcp/ip notification with wrong credential
        else if (status == ERR_CREDENTIAL) {                // 401 -> out code 401
            setErrorF(ERR_CREDENTIAL, "HTTP server error: %d. Wrong credential.", status);
            LOG.debug("%s", getLastErrorMsg());
            goto exit;
        }
        // To handle the http error code for the tcp/ip notification when payment required
        else if (status == PAYMENT_REQUIRED) {              // 402 -> out code 402
            setErrorF(PAYMENT_REQUIRED, "HTTP server error: %d. Client not authenticated.", status);
            LOG.debug("%s", getLastErrorMsg());
            goto exit;
        }
        // To handle the http error code for the tcp/ip notification when client not activated (forbidden)
        else if (status == FORBIDDEN) {                     // 403 -> out code 403
            setErrorF(FORBIDDEN, "HTTP server error: %d. Connection forbidden, client not activated.", status);
            LOG.debug("%s", getLastErrorMsg());
            goto exit;
        }
        // to handle the http error code for the tcp/ip notification and client not notifiable
        else if (status == ERR_CLIENT_NOT_NOTIFIABLE) {     // 420 -> out code 420
            setErrorF(ERR_CLIENT_NOT_NOTIFIABLE, "HTTP server error: %d. Client not notifiable.", status);
            LOG.debug("%s", getLastErrorMsg());
            goto exit;
        }
        // to handle the http error code for the tcp/ip notification and client not notifiable
        // code 421 is returned by newer Funambol Server to say "you're allowed to start CTP"
        else if (status == ERR_CTP_ALLOWED) {               // 421 -> out code 421
            setErrorF(ERR_CTP_ALLOWED, "HTTP server error: %d. Client not notifiable and CTP Server is available.", status);
            LOG.debug("%s", getLastErrorMsg());
            goto exit;
        }
#endif
        
        else if (status == HTTP_STATUS_NOT_FOUND) {         // 404 -> out code 2060
            setErrorF(ERR_HTTP_NOT_FOUND, "HTTP request error: resource not found (status %d)", status);
            LOG.debug("%s", getLastErrorMsg());
            goto exit;
        }
        else if (status == HTTP_STATUS_REQUEST_TIMEOUT) {   // 408 -> out code 2061
            setErrorF(ERR_HTTP_REQUEST_TIMEOUT, "HTTP request error: server timed out waiting for request (status %d)", status);
            LOG.debug("%s", getLastErrorMsg());
            goto exit;
        }
        else {
            // Other HTTP errors -> OUT
            //lastErrorCode = ERR_HTTP_STATUS_NOT_OK;         // else -> out code 2053
            DWORD code = GetLastError();
            char* tmp = createHttpErrorMessage(code);
            setErrorF(ERR_HTTP_STATUS_NOT_OK, "HTTP request error: status received = %d): %s (code %d)", status, tmp, code);
                    LOG.debug("%s", getLastErrorMsg());
                    delete [] tmp;
                    goto exit;
        }
    } // for(numretries = 0; numretries < MAX_RETRIES; numretries++)
    
    setResponseCode(status);
        
    // Too much retries -> exit
    if (numretries == MAX_RETRIES) {                        // Network error -> out code 2001
        setErrorF(ERR_CONNECT, "HTTP request error: %d attempts failed.", numretries); 
        LOG.error("%s", getLastErrorMsg());
        goto exit;
    }
        
    //Initialize response
        unsigned int responseLength = 0;
    HttpQueryInfo (request,
                   HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
                   (LPDWORD)&responseLength,
                   (LPDWORD)&size,
                   NULL);



#ifdef USE_ZLIB
    int uncompressedContentLenght = 0;

    // Release the send buffer (also set msgToSend to NULL, to
    // avoid leaving a dangling pointer around.
    if (compr) {
        delete [] compr; compr = NULL;
        msgToSend = NULL;
    }

    //
    // Read headers: get contentLenght/Uncompressed-Content-Length.
    //
    readHttpHeader(request);
        
    // isToDeflate to be set
    DWORD dwSize = 512;
    buffer = new WCHAR[dwSize];
    memset(buffer, 0, dwSize*sizeof(WCHAR));

    wcscpy(buffer, TEXT("Accept-Encoding"));
    HttpQueryInfo(request, HTTP_QUERY_CUSTOM, (LPVOID)buffer, &dwSize, NULL);
    if (GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND) {
        isToDeflate = false;
    } else {
        isToDeflate = true;
    }

    SetLastError(0);
    dwSize = 512;
    memset(buffer, 0, dwSize*sizeof(WCHAR));
    wcscpy(buffer, TEXT("Content-Encoding"));
    HttpQueryInfo(request, HTTP_QUERY_CUSTOM, (LPVOID)buffer, &dwSize, NULL);
    if (GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND) {
        LOG.debug("'Content-Encoding' header NOT found: response is not compressed");
        isToInflate = false;
    } else {
        if (wcscmp(buffer, TEXT("deflate")) == 0) {
            LOG.debug("'Content-Encoding: deflate' header found: response is compressed");
            isToInflate = true;
        } else {
            LOG.error("'Content-Encoding: %ls' is not supported", buffer);
            isToInflate = false;
        }
    }

    SetLastError(0);
    if(isToInflate) {
        dwSize = 512;
        memset(buffer, 0, dwSize*sizeof(WCHAR));
        wcscpy(buffer, TEXT("Uncompressed-Content-Length"));

        HttpQueryInfo(request, HTTP_QUERY_CUSTOM, (LPVOID)buffer, &dwSize, NULL);
        if (GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND) {
            LOG.error("Error reading 'Uncompressed-Content-Length' header.");
            uncompressedContentLenght = -1;
        }
        else {
            uncompressedContentLenght = wcstol(buffer, NULL, 10);
            LOG.debug("Uncompressed-Content-Length: %ld", uncompressedContentLenght);
        }

        // Check header value, use MAX_MSG_SIZE if not valid.
        if(uncompressedContentLenght <= 0) {
            LOG.error("Invalid value, using max message size.");
            uncompressedContentLenght = maxmsgsize * 2;
        }
    }

    delete [] buffer;
    buffer = NULL;

#endif
        
        
    //
    // ====================================== Reading Response ======================================
    //
    LOG.debug("%s", READING_RESPONSE);
    LOG.debug("Content-length: %u", responseLength);
        
    if (responseLength <= 0) {
        LOG.debug("Undefined content-length = %u. Using the maxMsgSize = %u.", responseLength, maxmsgsize);
        responseLength = maxmsgsize;
    }
        
    // Allocate a block of memory for response read.
    response = new char[responseLength+1];
    if (response == NULL) {
        //lastErrorCode = ERR_NOT_ENOUGH_MEMORY;
        //sprintf(lastErrorMsg, "Not enough memory to allocate a buffer for the server response: %d required.", responseLength);
        setErrorF(ERR_NOT_ENOUGH_MEMORY, "Not enough memory to allocate a buffer for the server response: %d required.", responseLength);
        LOG.error("%s", getLastErrorMsg());
        goto exit;
    }
    memset(response, 0, responseLength);
    p = response;
    int realResponseLenght = 0;
           
    do {
        if (!InternetReadFile(request, (LPVOID)bufferA, readBufferSize, &read)) {
            DWORD code = GetLastError();
            //lastErrorCode = ERR_READING_CONTENT;
            char* tmp = createHttpErrorMessage(code);
            //sprintf(lastErrorMsg, "InternetReadFile Error: %d - %s", code, tmp);
            setErrorF(ERR_READING_CONTENT, "InternetReadFile Error: %d - %s", code, tmp);
            delete [] tmp;
            goto exit;
        }

        // Sanity check: some proxy could send additional bytes.
        // Correct 'read' value to be sure we won't overflow the 'response' buffer.
        if ((realResponseLenght + read) > responseLength) {
            LOG.debug("Warning! %d bytes read -> truncating data to content-lenght = %d.", (realResponseLenght + read), responseLength);
            read = responseLength - realResponseLenght;
        }
                
        if (read > 0) {
            memcpy(p, bufferA, read);               // Note: memcopy exactly the bytes read (could be no readable chars...)
            p += read;
            realResponseLenght += read;

            // Fire Data Received Transport Event
            fireTransportEvent(read, DATA_RECEIVED);
        }

    } while (read);

    // free read buffer
    delete [] bufferA; bufferA = NULL;

    if (realResponseLenght <= 0) {
        setErrorF(ERR_READING_CONTENT, "Error reading HTTP response from Server: received data of size = %d.", realResponseLenght);
        goto exit;
    }

    // Log bytes read if different from content length
    // (should be already the same...)
    if (realResponseLenght != responseLength) {
        responseLength = realResponseLenght;
    }
    response[responseLength] = 0;
    responseSize = responseLength;
        
    //------------------------------------------------------------- Response read

#ifdef USE_ZLIB

    if (isToInflate) {
        //
        // INFLATE (decompress data)
        //
        uLong uncomprLen = uncompressedContentLenght;
        Bytef* uncompr = new Bytef[uncomprLen + 1];

        // Decompresses the source buffer into the destination buffer.
        int err = uncompress(uncompr, &uncomprLen, (Bytef*)response, responseLength);
                
        if (err == Z_OK) {
            delete [] response;
            response = (char*)uncompr;
            response[uncompressedContentLenght] = 0;
        }
        else if (err < 0) {
            // Save the msg to file, for debugging...
            dumpMessage(response, responseLength);
                        
            LOG.error("Error from zlib: %s", zError(err));
            delete [] response;
            response = NULL;
            status = ERR_HTTP_INFLATE;
            setError(ERR_HTTP_INFLATE, "ZLIB: error occurred decompressing data from Server.");

            goto exit;
        }
    }

#endif

    // LOG.debug("Response read:\n%s", response);
    if (getTimeout() == 99) {
        LOG.debug("Response read (logged in clear mode):\n%s", response);
    } else {
        message = removeSensibleData(response);
        LOG.debug("Response read:\n%s", message.c_str());
    }
   

exit:
    // Close the Internet handles.
    if (inet) {
        InternetCloseHandle (inet);
    }
    if (connection) {
        InternetCloseHandle (connection);
    }
    if (request) {
        InternetCloseHandle (request);
    }

    if ((status != STATUS_OK) && (response !=NULL)) {
        delete [] response; response = NULL;
    }
    if (wurlHost)     delete [] wurlHost;
    if (wurlResource) delete [] wurlResource;
    if (bufferA)      delete [] bufferA;

#ifdef USE_ZLIB
    if (compr)        delete [] compr;
    if (buffer)       delete [] buffer;
    if (wbuffer)      delete [] wbuffer;
#endif
    EXITING(L"TransportAgent::sendMessage");

    return response;
}


/**
 * Utility function to retrieve the correspondant message for the Wininet error code passed.
 * Pointer returned is allocated new, must be freed by caller.
 * @param errorCode  the code of the last error
 * @return           the error message for the passed code, new allocated buffer
 */
char* WinTransportAgent::createHttpErrorMessage(DWORD errorCode) {

    WCHAR* errorMessage = new WCHAR[512];
    memset(errorMessage, 0, 512);

    FormatMessage(
                FORMAT_MESSAGE_FROM_HMODULE,
                GetModuleHandle(L"wininet.dll"),
                errorCode,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
                errorMessage,
                512,
                NULL);

    if (!errorMessage || wcslen(errorMessage) == 0) {
        wsprintf(errorMessage, L"Unknown error.");
    }

    char* ret = toMultibyte(errorMessage);
    if (errorMessage) delete [] errorMessage;
    return ret;
}



/** 
 * Saves the msg passed into a file under "\dump" for debugging purpose.
 * The message is saved as a binary file.
 */
void WinTransportAgent::dumpMessage(const char* msg, const int msgLen) {

    if (msgLen == 0) {
        return;
    }
    char fileName[100], now[30];
    SYSTEMTIME sys_time;
    GetLocalTime(&sys_time);
    sprintf(now, "%04d-%02d-%02d_%02d.%02d.%02d", sys_time.wYear, sys_time.wMonth, sys_time.wDay,
                                                  sys_time.wHour, sys_time.wMinute, sys_time.wSecond);

    // TODO: store dump files under installDir instead of root folder?
    CreateDirectory(L"\\dump", NULL);
    sprintf(fileName, "\\dump\\%s.dmp", now);
    saveFile(fileName, msg, msgLen, true);
}

/**
 * Add authentication headers generated by the authentication object to the request.
 * Headers are added with the setProperty method.
 *
 * @param hRequest The request to add authenticatino headers to
 */
void WinTransportAgent::addAuthenticationHeaders(HINTERNET hRequest) {
    DWORD dwStatus;
    DWORD cbStatus = sizeof(dwStatus);
    BOOL fRet;
    WCHAR szScheme[256];
    DWORD dwIndex = 0;
    DWORD cbScheme = sizeof(szScheme);
    DWORD dwFlags;
    StringBuffer authresponse;

    HttpSendRequest(hRequest, L"", 0, "", 0);
    HttpQueryInfo
    (
        hRequest,
        HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE,
        &dwStatus,
        &cbStatus,
        NULL
    );

    switch (dwStatus) {
        case HTTP_STATUS_DENIED:
            dwFlags = HTTP_QUERY_WWW_AUTHENTICATE;
            break;          
        default:
            return;
    }

    fRet = HttpQueryInfo(hRequest, dwFlags, szScheme, &cbScheme, &dwIndex);
    if (fRet) {
        HashProvider *hashProvider = new WinDigestAuthHashProvider();
        authresponse = auth->getAuthenticationHeaders(toMultibyte(szScheme), url, hashProvider);
        setProperty("Authorization", authresponse);
    }
}

void WinTransportAgent::readHttpHeader(HINTERNET hRequest)
{
    WCHAR *wbuffer = new WCHAR[1024];
    DWORD  ddsize = 1024;
	StringBuffer headerString;

    if (HttpQueryInfo(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF ,(LPVOID)wbuffer, &ddsize, NULL)) {
		headerString.convert(wbuffer);
		LOG.debug("Response Headers:");

		ArrayList headers;
		headerString.split(headers, "\r\n");

		StringBuffer *prop;

		for(ArrayElement* e=headers.front(); e; e=headers.next()) {
			prop = dynamic_cast<StringBuffer *>(e);
			if(prop->empty()) continue;

			size_t colon = prop->find(":");
			if (colon != StringBuffer::npos) {
				StringBuffer key = prop->substr(0, colon);
				StringBuffer value = prop->substr(colon+1);
				responseProperties.put(key.trim(),value.trim());
				LOG.debug("\t%s : %s", key.c_str(), value.c_str());
			}
			else {
				LOG.debug("\t%s", prop->c_str());
			}
		}
    }
    else {
        DWORD error = GetLastError();
    }
}

void WinTransportAgent::writeHttpHeaders(WString &headers)
{
    KeyValuePair p;
    headers = TEXT("");

    for (p=requestProperties.front(); !p.null(); p=requestProperties.next()) {
        StringBuffer prop;
        prop.sprintf("%s: %s\r\n", p.getKey().c_str(), p.getValue().c_str());

        WString wprop;
        wprop = prop; 
        headers += wprop;
    }
}

/**
 * Set the authentication object. The transport agent will only use authentication if this object is not null.
 *
 * @param auth The authentication object to use.
 */
void WinTransportAgent::setAuthentication(HttpAuthentication *auth) {
        this->auth = auth;
}
