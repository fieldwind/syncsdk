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

#ifndef __EXPORTED_ITEM_H__
#define __EXPORTED_ITEM_H__

#include "base/fscapi.h"
#include "base/constants.h"
#include "base/util/ArrayElement.h"
#include "base/util/StringMap.h"
#include "base/globalsdef.h"

BEGIN_FUNAMBOL_NAMESPACE

class ExportedItem : public ArrayElement
{
    public:
        ExportedItem();
        ExportedItem(const char* serviceName);

        void setServiceName    (const char* serviceName);
        void setItemGuid       (const char* itemId);
        void setItemPrivacy    (const char* itemPrivacy);
        void setAlbumId        (const char* albumId);
        void setItemName       (const char* itemName);
        void setItemDescription(const char* itemDescription);
        void setItemTitle      (const char* itemTitle);
        void setItemTags       (const ArrayList& itemTags);
        void setRecipients     (const ArrayList& recipients);
        void setItemId         (unsigned int);

        const char* getServiceName()     const { return serviceName_.c_str();     }
        const char* getItemGuid()        const { return itemGuid_.c_str();        }
        const char* getItemPrivacy()     const { return itemPrivacy_.c_str();     }
        const char* getAlbumId()         const { return albumId_.c_str();         }
        const char* getItemName()        const { return itemName_.c_str();        }
        const char* getItemDescription() const { return itemDescription_.c_str(); } 
        const char* getItemTitle()       const { return itemTitle_.c_str();       } 
        ArrayList   getItemTags()        const { return itemTags_;                }
        ArrayList   getRecipients()      const { return recipients_;              }
        unsigned int getItemId()         const { return itemId_;                  }

        // virtal ctor for ArrayElement
        ArrayElement* clone();

    private:
        StringBuffer serviceName_;
        StringBuffer itemGuid_;
        StringBuffer itemPrivacy_;
        StringBuffer albumId_;
        StringBuffer itemName_;
        StringBuffer itemDescription_;
        StringBuffer itemTitle_;
        ArrayList    itemTags_;
        ArrayList    recipients_;
        unsigned int itemId_;
};

END_FUNAMBOL_NAMESPACE

#endif
