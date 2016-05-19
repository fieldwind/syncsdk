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

#ifndef INCL_EXTERNAL_SERVICE_ALBUM
#define INCL_EXTERNAL_SERVICE_ALBUM
/** @cond DEV */

#include "base/fscapi.h"
#include "base/constants.h"
#include "base/util/ArrayElement.h"
#include "base/util/StringMap.h"
#include "base/globalsdef.h"
#include "ioStream/FileInputStream.h"
#include "ioStream/FileOutputStream.h"
#include "MediaHub/MHStoreEntry.h"

BEGIN_FUNAMBOL_NAMESPACE


/**
 * This class rapresents a single album associated to the service the user's account can be linked to,
 * In the ExternalServiceAlbum store (SQL DB) it rapresents each entry.
 */
class ExternalServiceAlbum : public MHStoreEntry {

private:

    /// The service name: facebook, flickr, picasa...
    StringBuffer    serviceName;
    
    /// The album id 
    StringBuffer    albumId;
    
    /// The album name
    StringBuffer    albumName;
    
    /// The album privacy
    StringBuffer    privacy;
        

    
public:
    
    ExternalServiceAlbum();
    
    ExternalServiceAlbum(const char* service_name, const char* album_id);
    
    ~ExternalServiceAlbum();
    
    explicit ExternalServiceAlbum(ExternalServiceAlbum& copy);
    
    //
    // Getters
    //
    const char* getServiceName()        { return serviceName.c_str();           }
    const char* getAlbumId()            { return albumId.c_str();               }
    const char* getAlbumName()          { return albumName.c_str();             }
    const char* getPrivacy()            { return privacy.c_str();               }
        
    //
    // Setters
    //
    void setServiceName         (const char* serviceName)           { this->serviceName         = serviceName;          }
    void setAlbumId             (const char* albumId)               { this->albumId             = albumId;              }
    void setAlbumName           (const char* albumName)             { this->albumName           = albumName;            }
    void setPrivacy             (const char* privacy)               { this->privacy             = privacy;              }

};

END_FUNAMBOL_NAMESPACE

/** @endcond */
#endif
