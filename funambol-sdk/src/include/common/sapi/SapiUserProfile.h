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

#ifndef libfunambol_SapiUserProfile_h
#define libfunambol_SapiUserProfile_h

#include "base/util/StringBuffer.h"

BEGIN_FUNAMBOL_NAMESPACE
/*
 Example
 
 Request:
 GET /sapi/profile?action=get
 
 Response:
 {
    "data":{
        "user":{
            "generic":{
                 "firstname":"John",
                 "lastname":"Dawson",
                 "useremail":"john@dawson.com",
                 "timezone":"Europe/Rome",
                 "active":true,
                 "userid":"dawson",
                 "mailinglist":true,
                 "birthday":20020901,
                 "male":true,
                 "photo":true
            },
            "phones":[
                {
                    "deviceid":2,
                    "phonenumber":"+393333333333",
                    "modelid":1105,
                    "carrierid":9,
                    "countrya2":"IT",
                    "active":true,
                    "converttmz":1
                }
            ],
            "emails":[
 
            ]
        }
    }
 }
 */

/**
 */
class SapiUserProfile
{
    private:
        StringBuffer firstname;
        StringBuffer lastname;
        StringBuffer useremail;
        StringBuffer timezone;
        StringBuffer phonenumber;
        bool active;
        StringBuffer userid;
        bool mailinglist;
        int birthday;
        bool male;
        bool photo;
    
    public:
        
        /// Constructs a new SapiUserProfile object
        SapiUserProfile();
        SapiUserProfile(const char* email);
        SapiUserProfile(const char* name, const char* surname, const char* email);
        
    
        /// Destructor
        ~SapiUserProfile();
    
        
        /// firstname
        const char* getFirstname() const { return firstname.c_str(); }
        void setFirstname(const char* name) { this->firstname = name; }
        
        /// lastname
        const char* getLastname() const  { return lastname.c_str(); }
        void setLastname(const char* name) { this->lastname = name; }
    
        /// useremail
        StringBuffer getUseremail() const { return useremail; }
        void setUseremail(const char* email) { this->useremail = email; }
    
        /// timezone
        const char* getTimezone() const { return timezone.c_str(); }
        void setTimezone(const char* timezone) { this->timezone = timezone; }    
    
        /// userid
        const char* getUserid() const { return userid.c_str(); }
        void setUserid(const char* userid) { this->userid = userid; }
    
        /// active
        void setActive(bool val) { active = val; }
        bool getActive() { return this->active; }
    
        /// mailinglist
        void setMailinglist(bool val) { mailinglist = val; }
        bool getMailinglist() { return this->mailinglist; }
        
        /// male
        void setMale(bool val) { male = val; }
        bool getMale() { return this->male; }
    
        /// photo
        void setPhoto(bool val) { photo = val; }
        bool getPhoto() { return this->photo; }
        
        /// birthday
        void setBirthday(int val) { birthday = val; }
        bool getBirthday() { return this->birthday; }
        
        /// phonenumber
        void setPhoneNumber(const char* val) { phonenumber = val; }
        const char* getPhoneNumber() { return this->phonenumber.c_str(); }
};

END_FUNAMBOL_NAMESPACE

#endif