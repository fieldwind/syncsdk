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



#include "base/util/utils.h"
#include "mail/MailSyncSourceConfig.h"
#include "mail/MailAccount.h"
#include "base/globalsdef.h"
#include "client/MailSourceManagementNode.h"

USE_NAMESPACE


MailSyncSourceConfig::MailSyncSourceConfig() {
    name      = NULL;
    uri       = NULL;
    syncModes = NULL;
    type      = NULL;
    sync      = NULL;

    downloadAge = 0;
    bodySize    = 0;
    attachSize  = 0;

    schedule = 0;
}

MailSyncSourceConfig::~MailSyncSourceConfig() {
}

MailSyncSourceConfig::MailSyncSourceConfig(MailSyncSourceConfig& c) {
    assign(c);
}

void MailSyncSourceConfig::setDownloadAge(int age) {
    downloadAge = age;
}

int MailSyncSourceConfig::getDownloadAge() const {
    return downloadAge;
}

void MailSyncSourceConfig::setBodySize(int size) {
    bodySize = size;
}

int MailSyncSourceConfig::getBodySize() const {
    return bodySize;
}

void MailSyncSourceConfig::setAttachSize(int size) {
    attachSize = size;
}

int MailSyncSourceConfig::getAttachSize() const {
    return attachSize;
}

void MailSyncSourceConfig::setInbox(int v) {
    inbox = v;
}

int MailSyncSourceConfig::getInbox() const {
    return inbox;
}

void MailSyncSourceConfig::setOutbox(int v) {
    outbox = v;
}

int MailSyncSourceConfig::getOutbox() const {
    return outbox;
}

void MailSyncSourceConfig::setDraft(int v) {
    draft = v;
}

int MailSyncSourceConfig::getDraft() const {
    return draft;
}

void MailSyncSourceConfig::setTrash(int v) {
    trash = v;
}

int MailSyncSourceConfig::getTrash() const {
    return trash;
}

void MailSyncSourceConfig::setSent(int v) {
    sent = v;
}

int MailSyncSourceConfig::getSent() const {
    return sent;
}

void MailSyncSourceConfig::setSchedule(int v) {
    schedule = v;
}

int MailSyncSourceConfig::getSchedule() const {
    return schedule;
}

bool MailSyncSourceConfig::setToBeCleanedFlag(const char* accountName, bool tobecleaned){
    int size = mailAccounts.size();

    for (int i = 0; i < size ; i++){
        MailAccount* ma =((MailAccount*)mailAccounts[i]);
        StringBuffer val(ma->getName());
        if ( strcmp(accountName, val.c_str()) == 0 ){
            ma->setToBeCleaned(tobecleaned);
            return true;
        }
    }
    return false;
}

bool MailSyncSourceConfig::setDeletedMailAccount(const char* accountName){
    int size = mailAccounts.size();

    for (int i = 0; i < size ; i++){
        MailAccount* ma =((MailAccount*)mailAccounts[i]);
        StringBuffer val(ma->getName());
        if ( strcmp(accountName, val.c_str()) == 0 ){
            ma->setDeleted(true);
            return true;
        }
    }
    return false;
}

bool MailSyncSourceConfig::delMailAccount(const char* accountName){
    
    int i = 0;
    int size = mailAccounts.size();
    for (i = 0; i < size ; i++){
        MailAccount* ma =((MailAccount*)mailAccounts[i]);
        StringBuffer val(ma->getName());
        if ( strcmp(accountName, val.c_str()) == 0 ){
            mailAccounts.removeElementAt(i);
            return true;
        }
    }
    return false;
}

bool MailSyncSourceConfig::addMailAccount(const MailAccount& account) {
	const char* name = account.getName();
	int size = mailAccounts.size();
	
	if (name == NULL) { 
		LOG.error("can't add mail account: no account name found");
		return false;
	}

	for (int i = 0; i < size; i++ ) {
		MailAccount* storedAccount = static_cast<MailAccount *>(mailAccounts[i]);
        if (storedAccount) {
		    const char* storedName = storedAccount->getName();
		    if ((storedName != NULL) && (strcmp(name, storedName) == 0)) {
			    // prevent adding an already existing accout
			    LOG.error("can't add mail account: an account with such name already exist");
			    return false;
		    }
        }
	}

	LOG.debug("setting mail account \"%s\" from config", account.getName().c_str());
	
	mailAccounts.add((MailAccount&)account);
	
	return true;
}


bool MailSyncSourceConfig::modifyMailAccount(const MailAccount& account) {
	const char* name = account.getName();
	int size = mailAccounts.size();
	
	if (name == NULL) { 
		LOG.error("can't update mail account: no account name found");
		return false;
	}

	for (int i = 0; i < size; i++ ) {
		MailAccount* storedAccount = static_cast<MailAccount *>(mailAccounts[i]);
		const char* storedName = storedAccount->getName();

		if ((storedName != NULL) && (strcmp(name, storedName) == 0)) {
			// update existing account
			LOG.debug("updating mail account %s", account.getName().c_str());
			mailAccounts.removeElementAt(i);
			mailAccounts.add((MailAccount&)(account));
			
			return true;
		}
	}

	LOG.error("can't update mail account: an account with such name doesn't exist");
	return false;
}

// ------------------------------------------------------------- Private methods

void MailSyncSourceConfig::assign(const MailSyncSourceConfig& sc) {
    setName     (sc.getName     ());
    setURI      (sc.getURI      ());
    setSyncModes(sc.getSyncModes());
    setType     (sc.getType     ());
    setSync     (sc.getSync     ());
    setLast     (sc.getLast     ());

    setEncoding      (sc.getEncoding      ());
    setVersion       (sc.getVersion       ());
    setSupportedTypes(sc.getSupportedTypes());
    //setCtCap         (sc.getCtCap         ());
    setEncryption    (sc.getEncryption    ());

    setDownloadAge(sc.getDownloadAge());
    setBodySize(sc.getBodySize());
    setAttachSize(sc.getAttachSize());

    setInbox(sc.getInbox());
    setOutbox(sc.getOutbox());
    setSent(sc.getSent());
    setTrash(sc.getTrash());
    setDraft(sc.getDraft());
    setSchedule(sc.getSchedule());
    mailAccounts = sc.getMailAccounts();
}

StringBuffer MailSyncSourceConfig::print() {

    StringBuffer ret;
    ret = "** SOURCE: "; ret += getName(); ret += "**\r\n";
    
    ret += "URI:\t\t"; ret += getURI(); ret += "\r\n";
    ret += "SyncModes:\t"; ret += getSyncModes(); ret += "\r\n";
    ret += "Type:\t\t"; ret += getType(); ret += "\r\n";
    ret += "Sync:\t\t"; ret += getSync(); ret += "\r\n";
    ret += "Encoding:\t"; ret += getEncoding(); ret += "\r\n";
    ret += "Version:\t"; ret += getVersion(); ret += "\r\n";
    ret += "SupportedType:\t"; ret += getSupportedTypes(); ret += "\r\n";
    ret += "Last:\t\t"; ret.append(getLast()); ret += "\r\n";
    ret += "Encryption:\t"; ret += getEncryption(); ret += "\r\n";    
    ret += "Enabled:\t"; ret += (isEnabled() == true ? "1" : "0"); ret += "\r\n";    


    ret += "DownloadAge:\t"; ret.append(getDownloadAge()); ret += "\r\n";    
    ret += "BodySize:\t"; ret.append(getBodySize()); ret += "\r\n";
    ret += "AttachSize:\t"; ret.append(getAttachSize()); ret += "\r\n";

    return ret;
}
