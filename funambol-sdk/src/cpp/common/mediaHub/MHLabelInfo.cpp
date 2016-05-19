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

#include "MediaHub/MHLabelInfo.h"
#include "base/Log.h"

BEGIN_FUNAMBOL_NAMESPACE

MHLabelInfo::MHLabelInfo(uint32_t guid, uint32_t luid, const char* name)
{
    this->guid = guid;
    this->luid = luid;
    
    if (name) {
        this->name = name;
    } else {
        this->name = "";
    }
    this->metaLabelType = META_LABEL_TYPE_NONE;
}

// default ctor
MHLabelInfo::MHLabelInfo() {
    luid = -1;
    guid = -1;
    this->name = "";
    this->metaLabelType = META_LABEL_TYPE_NONE;
}

MHLabelInfo::~MHLabelInfo() {
}

MHLabelInfo& MHLabelInfo::operator= (const MHLabelInfo &other) {
    if (this != &other) // protect against invalid self-assignment
    {
        luid = other.getLuid();
        guid = other.getGuid();
        setName(other.getName().c_str());
        this->metaLabelType = other.metaLabelType;
    }
    // by convention, always return *this
    return *this;
}

MHLabelInfo::MHLabelInfo(const MHLabelInfo& copy) {
    luid = copy.getLuid();
    guid = copy.getGuid();
    setName(copy.getName().c_str());
}

MHLabelInfo::MHLabelInfo(MHLabelInfo& copy) {
    luid = copy.getLuid();
    guid = copy.getGuid();
    setName(copy.getName().c_str());
}


void MHLabelInfo::setName(const char* name){
    if (name) {
        this->name = name;
    } else {
        name = "";
    }
}

const std::string& MHLabelInfo::getName() const {
    return name;
}

uint32_t MHLabelInfo::getLuid() const {
    return luid;
}

void MHLabelInfo::setLuid(uint32_t luid) {
    this->luid = luid;
}

uint32_t MHLabelInfo::getGuid() const {
    return guid;
}

void MHLabelInfo::setGuid(uint32_t guid) {
    this->guid = guid;
}

void MHLabelInfo::setMetaLabelType(uint32_t metaType) {
    metaLabelType = metaType;
}

int32_t MHLabelInfo::getMetaLabelType() {
    return metaLabelType;
}

END_FUNAMBOL_NAMESPACE

