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

#include <errno.h>
#include <string.h>
#include "Ws2tcpip.h"
#include <winsock.h>

#include "push/FSocket.h"
#include "base/globalsdef.h"
#include "base/Log.h"

USE_NAMESPACE

FSocket::FSocket() : winSocket ( INVALID_SOCKET ) {
    memset ( &rAddress, 0, sizeof ( rAddress ) );
}

FSocket::~FSocket() {
    close();
}

FSocket* FSocket::createSocket(const StringBuffer& peer, int32_t port) {

    if(customSocket) {
        return customSocket;
    }
    int ret = 0, errorCode = 0;
    //
    // Initialize Winsock
    //
    WORD versionRequested = MAKEWORD(1, 1);
	WSADATA wsaData;
    ret = WSAStartup(versionRequested, &wsaData);
    if (ret != NO_ERROR) {
        errorCode = WSAGetLastError();
        LOG.error("SOCKET WSAStartup() error %d", errorCode);
        //ctpState = CTP_STATE_DISCONNECTED;
        return NULL;
    }

    // Check if version is correct 
    if (wsaData.wVersion != versionRequested) {
        LOG.error("WinSock version not supported: %d (%d expected)", wsaData.wVersion, versionRequested);
        //ctpState = CTP_STATE_DISCONNECTED;
        return NULL;
	}

    struct addrinfo aiHints;
    struct addrinfo *aiList = NULL;
    
    // Setup the hints address info structure
    // which is passed to the getaddrinfo() function
    memset(&aiHints, 0, sizeof(aiHints));
    aiHints.ai_family   = AF_INET;				// Address family
    aiHints.ai_socktype = SOCK_STREAM;			// Socket type
    aiHints.ai_protocol = IPPROTO_TCP;		    // Protocol
    aiHints.ai_flags    = AI_CANONNAME;         // To get the canonical name

    const char* hostName = peer.c_str();
    char port1[10];
    sprintf(port1, "%d", port);
    LOG.info("HOSTNAME = '%s'  PORT = '%s'", hostName, port1);

    // Resolve the host name.
    ret = getaddrinfo(hostName, port1, &aiHints, &aiList);
    if (ret) {
		//lastErrorCode = ERR_HOST_NOT_FOUND;
        setErrorF(ERR_HOST_NOT_FOUND,"getaddrinfo() failed: %s", gai_strerror(ret));
        LOG.error("getaddrinfo() failed: %s", gai_strerror(ret));
        ret = -2;
        if (aiList) {
            freeaddrinfo(aiList);
        }
        return NULL;
    }

    struct addrinfo *addr = aiList;
    int err = WSAGetLastError();
    SOCKET sk = socket(AF_INET, SOCK_STREAM, 0);
    if (sk == INVALID_SOCKET) {
        int err = WSAGetLastError();
        return NULL;
    }
    err = WSAGetLastError();
/*
    // Loop on possible addresses
    while (addr != NULL) {
        //
        // Create a TCP/IP stream socket
        //
        LOG.debug("Create SOCKET connection...");
        sk = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sk == INVALID_SOCKET) {
            if (addr->ai_next != NULL) {
                addr=addr->ai_next;        // take next address
                continue;
            }
            else {
                errorCode = WSAGetLastError();
                LOG.error("SOCKET socket() error %d: %s", errorCode);
                ret = -3;
                return NULL;
            }
        }
    }
*/
/*
    
    // current implementation
    // Create a socket
    SOCKET sk = socket(AF_INET, SOCK_STREAM, 0);
    if (sk == INVALID_SOCKET) {
        int err = WSAGetLastError();
        return NULL;
    }
*/
    // Set socket options
    int on = 1;
    if ( setsockopt ( sk, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 ) {
        return NULL;
    }

    u_long remoteAddr = inet_addr(peer.c_str());
    if (remoteAddr == INADDR_NONE) {
        
        // peer isn't a dotted IP, so resolve it through DNS
        hostent* he = gethostbyname(peer.c_str());
        if (he == NULL) {
            // The peer address cannot be reached
            return NULL;
        }
        remoteAddr = *((u_long*)he->h_addr_list[0]);
    }

    sockaddr_in raddress;
    raddress.sin_family = AF_INET;
    raddress.sin_addr.s_addr = remoteAddr;
    raddress.sin_port = htons (port);
    
    // Connecting to the remote address
    int status = connect(sk, (sockaddr*)&raddress, sizeof(raddress));

    if ( status == SOCKET_ERROR ) {
        // Error while connecting to the remote host
        return NULL;
    } 
    FSocket* newSocket = new FSocket();
    newSocket->winSocket = sk;
    newSocket->rAddress = raddress;
    return newSocket;
}

int32_t FSocket::writeBuffer(const int8_t* buffer, int32_t len) {
    int32_t status = send (winSocket, buffer, len, 0 );
    return status;
}


int32_t FSocket::readBuffer(int8_t* buffer, int32_t maxLen) {
    memset ( buffer, 0, maxLen );
    int32_t readBytes = recv ( winSocket, buffer, maxLen, 0 );
    return readBytes;
}

const StringBuffer& FSocket::address() const {
    return pAddress;
}
const StringBuffer& FSocket::peerAddress() const {
    return pAddress;
}

void FSocket::close() {
    if ( isValid() )
        closesocket(winSocket);
}

bool FSocket::isValid() {
    return winSocket != INVALID_SOCKET;
}

FSocket* FSocket::customSocket;
