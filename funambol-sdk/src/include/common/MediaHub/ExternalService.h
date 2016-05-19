/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2012 Funambol, Inc.
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

#ifndef INCL_EXTERNAL_SERVICE
#define INCL_EXTERNAL_SERVICE
/** @cond DEV */

#include "base/fscapi.h"
#include "base/constants.h"
#include "base/util/StringMap.h"
#include "base/globalsdef.h"
#include "ioStream/FileInputStream.h"
#include "ioStream/FileOutputStream.h"
#include "MediaHub/MHStoreEntry.h"

BEGIN_FUNAMBOL_NAMESPACE


/**
 * This class rapresents a single external service the user's account can be linked to,
 * like Facebook, Picasa, Twitter...
 * In the ExternalService store (SQL DB) it rapresents each entry.
 */
class ExternalService : public MHStoreEntry {

private:

    /// The service name: this is the KEY.
    StringBuffer    serviceName;
    
    /// The display name
    StringBuffer    displayName;
    
    /// If false, the client should first authenticate via 'authUrl' to use this service
    bool            authorized;
    
    StringBuffer    authUrl;
    
    /// The url to retrieve the updated icon to show for this service
    StringBuffer    iconUrl;

    /// The local path to downloaded service icon
    StringBuffer    iconPath;
    
    /// User account name
    StringBuffer    accountName;
    
    /// comma separated names of sources supported by this service
    StringBuffer    sources;
    
    StringBuffer    apiKey;
    
    /// comma separated attributes for items
    StringBuffer    itemAttributes;
    
    /// comma separated privacy levels for items
    StringBuffer    itemPrivacy;
    
    StringBuffer    lastUsedItemPrivacy;
    
    bool            exportMultiple;
    
    //
    // Albums related:
    //
    bool            hasAlbums;
    
    StringBuffer    lastUsedAlbum;
    
    /// comma separated privacy levels for albums
    StringBuffer    albumPrivacy;
    
    /// comma separated attributes for albums
    StringBuffer    albumAttributes;
    

    
public:
    
    ExternalService();
    
    ExternalService(const char* serviceName);
    
    ~ExternalService();
    
    //
    // Getters
    //
    const char* getServiceName()        { return serviceName.c_str();           }
    const char* getDisplayName()        { return displayName.c_str();           }
    const char* getAuthUrl()            { return authUrl.c_str();               }
    const char* getIconUrl()            { return iconUrl.c_str();               }
    const char* getIconPath()           { return iconPath.c_str();              }
    const char* getAccountName()        { return accountName.c_str();           }
    const char* getSources()            { return sources.c_str();               }
    const char* getApiKey()             { return apiKey.c_str();                }
    const char* getItemAttributes()     { return itemAttributes.c_str();        }
    const char* getItemPrivacy()        { return itemPrivacy.c_str();           }
    const char* getLastUsedItemPrivacy(){ return lastUsedItemPrivacy.c_str();   }
    const char* getLastUsedAlbum()      { return lastUsedAlbum.c_str();         }
    const char* getAlbumPrivacy()       { return albumPrivacy.c_str();          }
    const char* getAlbumAttributes()    { return albumAttributes.c_str();       }
    
    bool getAuthorized()                { return authorized;                    }
    bool getExportMultiple()            { return exportMultiple;                }
    bool getHasAlbums()                 { return hasAlbums;                     }
    bool getHasAlbumsForSource(const char* sourceName); //-- there are no albums for facebook videos
    
    //
    // Setters
    //
    void setServiceName         (const char* serviceName)           { this->serviceName         = serviceName;          }
    void setDisplayName         (const char* displayName)           { this->displayName         = displayName;          }
    void setAuthUrl             (const char* authUrl)               { this->authUrl             = authUrl;              }
    void setIconUrl             (const char* iconUrl)               { this->iconUrl             = iconUrl;              }
    void setIconPath            (const char* iconPath)              { this->iconPath            = iconPath;             }
    void setAccountName         (const char* accountName)           { this->accountName         = accountName;          }
    void setSources             (const char* sources)               { this->sources             = sources;              }
    void setApiKey              (const char* apiKey)                { this->apiKey              = apiKey;               }
    void setItemAttributes      (const char* itemAttributes)        { this->itemAttributes      = itemAttributes;       }
    void setItemPrivacy         (const char* itemPrivacy)           { this->itemPrivacy         = itemPrivacy;          }
    void setLastUsedItemPrivacy (const char* lastUsedItemPrivacy)   { this->lastUsedItemPrivacy = lastUsedItemPrivacy;  }
    void setLastUsedAlbum       (const char* lastUsedAlbum)         { this->lastUsedAlbum       = lastUsedAlbum;        }
    void setAlbumPrivacy        (const char* albumPrivacy)          { this->albumPrivacy        = albumPrivacy;         }
    void setAlbumAttributes     (const char* albumAttributes)       { this->albumAttributes     = albumAttributes;      }
    
    void setAuthorized(bool authorized)         { this->authorized      = authorized;       }
    void setExportMultiple(bool exportMultiple) { this->exportMultiple  = exportMultiple;   }    
    void setHasAlbums(bool hasAlbums)           { this->hasAlbums       = hasAlbums;        }
    
    
    /// Returns true if the passed source is found within the sources array.
    bool isSourceSupported(const char* sourceName);
};

END_FUNAMBOL_NAMESPACE

/** @endcond */
#endif
