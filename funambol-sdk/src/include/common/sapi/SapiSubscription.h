/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2009 Funambol, Inc.
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

#ifndef libfunambol_SapiSubscription_h
#define libfunambol_SapiSubscription_h

#include "base/util/StringBuffer.h"

BEGIN_FUNAMBOL_NAMESPACE

class SapiSubscription
{
private:
    StringBuffer name;
    StringBuffer price;
    StringBuffer currency;
    StringBuffer quota;
    StringBuffer period;
    StringBuffer displayname;
    StringBuffer description;
    StringBuffer message;
    StringBuffer status;
    StringBuffer nextRenewal;
    bool activateable;
    bool isDefault;
    
public:
    
    /// Constructs a new SapiSubscription object
    SapiSubscription();
    
    
    /// Destructor
    ~SapiSubscription();
    
    /// name
    const char* getName() const { return name.c_str(); }
    void setName(const char* name) { this->name = name; }
    
    /// price
    const char* getPrice() const  { return price.c_str(); }
    void setPrice(const char* price) { this->price = price; }
    
    /// currency
    StringBuffer getCurrency() const { return currency; }
    void setCurrency(const char* currency) { this->currency = currency; }
    
    /// quota
    const char* getQuota() const { return quota.c_str(); }
    void setQuota(const char* quota) { this->quota = quota; }    
    
    /// displayname
    const char* getDisplayname() const { return displayname.c_str(); }
    void setDisplayname(const char* displayname) { this->displayname = displayname; }
    
    /// description
    const char* getDescription() const { return description.c_str(); }
    void setDescription(const char* description) { this->description = description; }
    
    /// period
    const char* getPeriod() const { return period.c_str(); }
    void setPeriod(const char* period) { this->period = period; }
    
    /// activateable
    void setActivateable(bool val) { activateable = val; }
    bool getActivateable() { return this->activateable; }
    
    /// default
    void setIsDefault(bool val) { isDefault = val; }
    bool getIsDefault() { return this->isDefault; }
    
    /// message
    void setMessage(const char *val) { message = val; }
    const char* getMessage() { return this->message.c_str(); }
    
    //-- status
    const char* getStatus() const { return status.c_str(); }
    void setStatus(const char* status) { this->status = status; }
    
    //-- nextRenewal
    const char* getNextRenewal() const { return nextRenewal.c_str(); }
    void setNextRenewal(const char* nextRenewal) { this->nextRenewal = nextRenewal; }
};


END_FUNAMBOL_NAMESPACE

#endif
