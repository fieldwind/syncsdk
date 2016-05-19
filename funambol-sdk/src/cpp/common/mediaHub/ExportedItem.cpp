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

#include "MediaHub/ExportedItem.h"

BEGIN_FUNAMBOL_NAMESPACE

ExportedItem::ExportedItem() : 
    serviceName_(""), albumId_(""), itemName_(""), itemDescription_(""), itemTitle_("")
{
    itemTags_.clear();
    recipients_.clear();
}

ExportedItem::ExportedItem(const char* serviceName) : 
    serviceName_(serviceName), albumId_(""), itemName_(""), itemDescription_(""), itemTitle_("")
{
    itemTags_.clear();
    recipients_.clear();
}

ArrayElement* ExportedItem::clone()
{
    ExportedItem *expItem = new ExportedItem();
    
    expItem->setServiceName    (this->getServiceName());
    expItem->setAlbumId        (this->getAlbumId());
    expItem->setItemDescription(this->getItemDescription());
    expItem->setItemTitle      (this->getItemTitle());
    expItem->setItemId         (this->getItemId());
    expItem->setItemGuid       (this->getItemGuid());
    expItem->setItemName       (this->getItemName());
    expItem->setItemPrivacy    (this->getItemPrivacy());
    expItem->setItemTags       (this->getItemTags());
    expItem->setRecipients     (this->getRecipients());
    
    return expItem;
}

void ExportedItem::setServiceName(const char* serviceName)
{
    if (serviceName) {
        serviceName_ = serviceName;
    }
}

void ExportedItem::setItemGuid(const char* itemGuid)
{
    if (itemGuid) {
        itemGuid_ = itemGuid;
    }
}

void ExportedItem::setItemId(unsigned int itemId)
{
    itemId_ = itemId;
}

void ExportedItem::setItemPrivacy(const char* itemPrivacy)
{
    if (itemPrivacy) {
        itemPrivacy_ = itemPrivacy;
    }
}

void ExportedItem::setAlbumId(const char* albumId)
{
    if (albumId) {
        albumId_ = albumId;
    }
}

void ExportedItem::setItemName(const char* itemName)
{
    if (itemName) {
        itemName_ = itemName;
    }
}

void ExportedItem::setItemDescription(const char* itemDescription)
{
    if (itemDescription) {
        itemDescription_ = itemDescription;
    }
}

void ExportedItem::setItemTitle(const char* itemTitle)
{
    if (itemTitle) {
        itemTitle_ = itemTitle;
    }
}

void ExportedItem::setItemTags(const ArrayList& itemTags)
{
    itemTags_ = itemTags;
}

void ExportedItem::setRecipients(const ArrayList& recipients)
{
    recipients_ = recipients;
}

END_FUNAMBOL_NAMESPACE
