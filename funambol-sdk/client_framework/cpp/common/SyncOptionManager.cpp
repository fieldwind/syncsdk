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

#include "SyncOptionManager.h"
#include "base/fscapi.h"
#include "base/Log.h"


namespace Funambol {

const char* SyncOptionManager::CLI_OPTION_SYNC_PIM_L =  "sync-pim";
const char SyncOptionManager::CLI_OPTION_SYNC_PIM_S  =  'p';
const char* SyncOptionManager::CLI_OPTION_SYNC_MEDIA_L = "sync-media";
const char SyncOptionManager::CLI_OPTION_SYNC_MEDIA_S  = 'm';
const char* SyncOptionManager::CLI_OPTION_FOLDER_L     = "folder";
const char SyncOptionManager::CLI_OPTION_FOLDER_S      = 'f';
const char* SyncOptionManager::CLI_OPTION_RESET_CLIENT_SERVER_PIM_L = "reset-pim-from-client";
const char SyncOptionManager::CLI_OPTION_RESET_CLIENT_SERVER_PIM_S  = 'r';
const char* SyncOptionManager::CLI_OPTION_RESET_SERVER_CLIENT_PIM_L = "reset-pim-from-server";
const char SyncOptionManager::CLI_OPTION_RESET_SERVER_CLIENT_PIM_S  = 'R';
const char* SyncOptionManager::CLI_OPTION_LOGIN_L               = "login";
const char SyncOptionManager::CLI_OPTION_LOGIN_S                = 'l';
const char* SyncOptionManager::CLI_OPTION_COMPATIBILITY_MODE_L  = "comp";
const char SyncOptionManager::CLI_OPTION_COMPATIBILITY_MODE_S   = 'c';
const char* SyncOptionManager::CLI_OPTION_HELP_L                = "help";
const char SyncOptionManager::CLI_OPTION_HELP_S                 = 'h';
const char* SyncOptionManager::CLI_OPTION_WAIT_QUEUE_L          = "queue";
const char SyncOptionManager::CLI_OPTION_WAIT_QUEUE_S           = 'W';
const char* SyncOptionManager::CLI_OPTION_SYNC_ALL_L =  "sync";
const char SyncOptionManager::CLI_OPTION_SYNC_ALL_S  =  's';
const char* SyncOptionManager::CLI_OPTION_FAST_CHECK_L =  "fast-local-check";
const char SyncOptionManager::CLI_OPTION_FAST_CHECK_S  =  'a';


/** Default constructor */
SyncOptionManager::SyncOptionManager(const char *progamName, CustomizationParams* customParams) 
    : parser(progamName)
{
    char help[256];
    
    resetOptions();

    contactsSourceName = customParams->getContactsSourceName();
    calendarSourceName = customParams->getCalendarSourceName();
    picturesSourceName = customParams->getPicturesSourceName();
    videosSourceName   = customParams->getVideosSourceName();
    filesSourceName    = customParams->getFilesSourceName();
    musicSourceName    = customParams->getMusicSourceName();
  
    // ------------------------------------------------------------------------

    // "--sync-pim [-p] [addressbook,calendar] to sync pim"
    
    sprintf(help, "to sync pim [%s,%s]", contactsSourceName.c_str(), calendarSourceName.c_str());
    parser.addOption(CLI_OPTION_SYNC_PIM_S, CLI_OPTION_SYNC_PIM_L, help, false);

    // ------------------------------------------------------------------------

    // "--sync-media [-m] [picture,video,music,file] to sync media"
    
    sprintf(help, "to sync media [%s,%s,%s,%s]", picturesSourceName.c_str(), videosSourceName.c_str(), musicSourceName.c_str(), filesSourceName.c_str());
    parser.addOption(CLI_OPTION_SYNC_MEDIA_S, CLI_OPTION_SYNC_MEDIA_L, help, false);

    // ------------------------------------------------------------------------

    // "--folder [-f] [folder] to sync files in a folder. Default folder is $HOME/OneMediaHub"
    
    sprintf(help, "to sync files in a folder. Default folder is $HOME/OneMediaHub");
    parser.addOption(CLI_OPTION_FOLDER_S, CLI_OPTION_FOLDER_L, help, true);

    // ------------------------------------------------------------------------

    // "--reset-pim-from-client [-r] [addressbook,calendar] to reset your remote contacts or calendars"
    
    sprintf(help, "to reset your remote %s or %s with local data", contactsSourceName.c_str(), calendarSourceName.c_str());
    parser.addOption(CLI_OPTION_RESET_CLIENT_SERVER_PIM_S, CLI_OPTION_RESET_CLIENT_SERVER_PIM_L, help);

    // "--reset-pim-from-server [-R] [addressbook,calendar] to reset your local contacts"
    
    sprintf(help, "to reset your local %s or %s with server data", contactsSourceName.c_str(), calendarSourceName.c_str());
    parser.addOption(CLI_OPTION_RESET_SERVER_CLIENT_PIM_S, CLI_OPTION_RESET_SERVER_CLIENT_PIM_L, help);

    // ------------------------------------------------------------------------

    // "--login [-l] to authenticate"

    sprintf(help, "to authenticate");
    parser.addOption(CLI_OPTION_LOGIN_S, CLI_OPTION_LOGIN_L, help);
    
    // ------------------------------------------------------------------------
    
    // "--comp [-o] compatibility mode with v10 servers"
    
    sprintf(help, "compatibility mode with v10 servers");
    parser.addOption(CLI_OPTION_COMPATIBILITY_MODE_S, CLI_OPTION_COMPATIBILITY_MODE_L, help);
    
    // ------------------------------------------------------------------------
    
    // "--queue [-W] to queue the process while another sync is running (default behaviour is exiting with error)."

    sprintf(help, "to queue the process while another sync is running (default behaviour is exiting with error).");
    parser.addOption(CLI_OPTION_WAIT_QUEUE_S, CLI_OPTION_WAIT_QUEUE_L, help);
    
    // ------------------------------------------------------------------------
    
    // "--help [-h] to view this help message"
    sprintf(help, "to view this help message");
    parser.addOption(CLI_OPTION_HELP_S, CLI_OPTION_HELP_L, help);

    // ------------------------------------------------------------------------

    // "--sync [-s] [picture,video,music,file,contact,appointment] to sync pim"    
    sprintf(help, "to sync all sources [%s,%s,%s,%s,%s,%s]", 
                        picturesSourceName.c_str(), videosSourceName.c_str(), musicSourceName.c_str(), filesSourceName.c_str(),
                        contactsSourceName.c_str(), calendarSourceName.c_str());
    parser.addOption(CLI_OPTION_SYNC_ALL_S, CLI_OPTION_SYNC_ALL_L, help, false);

    // "--fast-local-check [-a] [contact,appointment] to check modification locally on pim"    
    sprintf(help, "to check sources [%s,%s]", contactsSourceName.c_str(), calendarSourceName.c_str());
    parser.addOption(CLI_OPTION_FAST_CHECK_S, CLI_OPTION_FAST_CHECK_L, help, false);

}


bool SyncOptionManager::parseCommandLine(int argc, char** argv)
{  
    this->resetOptions();
        
    if (this->parser.parse(argc, const_cast<const char **>(argv), this->options, this->arguments) == false) {
        return false;
    }
    
    StringBuffer sOption = NULL;
    
    // ------------------------------------------------------------------------
    // sync-pim
    sOption = this->options[CLI_OPTION_SYNC_PIM_L];
    
    if (!sOption.empty()) {
                
        // if no optional argument is specified, it synchronizes all pim sources by default
        if (sOption == "1") { // "1" == no option
            
            this->syncContacts = true;
            this->syncCalendar = true;
        
        // if any optional argument is specified, it checks what sync source is included
        }
        else {

            ArrayList list;
            sOption.split(list, ",");

            for (int i = 0; i < list.size(); i++) {
                StringBuffer* element = (StringBuffer*)list[i];
                if (element->icmp(contactsSourceName.c_str())) {
                    this->syncContacts = true;
                }
                else if (element->icmp(calendarSourceName.c_str())) {
                    this->syncCalendar = true;
                }
            }            
        }
    }

    // ------------------------------------------------------------------------
    // sync-media
    sOption = this->options[CLI_OPTION_SYNC_MEDIA_L];
    
    if (!sOption.empty()) {                       
        
        // if no optional argument is specified, it synchronizes all pim sources by default
        if (sOption == "1") { // "1" == no option
            
            this->syncFiles = true;
            this->syncPictures = true;
            this->syncVideos = true;
            this->syncMusic = true;

            // if any optional argument is specified, it checks what sync source is included
        } else {
            
            ArrayList list;
            sOption.split(list, ",");

            for (int i = 0; i < list.size(); i++) {
                StringBuffer* element = (StringBuffer*)list[i];
                if (element->icmp(filesSourceName.c_str())) {
                    this->syncFiles = true;
                }
                else if (element->icmp(picturesSourceName.c_str())) {
                    this->syncPictures = true;
                }
                else if (element->icmp(videosSourceName.c_str())) {
                    this->syncVideos = true;
                }
                else if (element->icmp(musicSourceName.c_str())) {
                    this->syncMusic = true;
                }
            }     

        }
    }
    

    // ------------------------------------------------------------------------
    // sync
    sOption = this->options[CLI_OPTION_SYNC_ALL_L];
    
    if (!sOption.empty()) {
                
        // if no optional argument is specified, it synchronizes all sources by default
        if (sOption == "1") { // "1" == no option
            
            this->syncFiles = true;
            this->syncPictures = true;
            this->syncVideos = true;
            this->syncMusic = true;
            this->syncContacts = true;
            this->syncCalendar = true;
        
        // if any optional argument is specified, it checks what sync source is included
        }
        else {

            ArrayList list;
            sOption.split(list, ",");

            for (int i = 0; i < list.size(); i++) {
                StringBuffer* element = (StringBuffer*)list[i];
                if (element->icmp(contactsSourceName.c_str())) {
                    this->syncContacts = true;
                }
                else if (element->icmp(calendarSourceName.c_str())) {
                    this->syncCalendar = true;
                }
                else if (element->icmp(filesSourceName.c_str())) {
                    this->syncFiles = true;
                }
                else if (element->icmp(picturesSourceName.c_str())) {
                    this->syncPictures = true;
                }
                else if (element->icmp(videosSourceName.c_str())) {
                    this->syncVideos = true;
                }
                else if (element->icmp(musicSourceName.c_str())) {
                    this->syncMusic = true;
                }
            }            
        }
    }
    // ------------------------------------------------------------------------
    
    // sync-config
    
    sOption = this->options[CLI_OPTION_COMPATIBILITY_MODE_L];
    
    if (!sOption.null()) {
        
        this->syncConfig = true;
    }
    
    // ------------------------------------------------------------------------
    
    // reset pim c2s
    sOption = this->options[CLI_OPTION_RESET_CLIENT_SERVER_PIM_L];
    
    if (!sOption.empty()) {
        resetC2S = true;
        // if no optional argument is specified, it synchronizes all pim sources by default
        if (sOption == "1") { // "1" == no option
            
            this->syncContacts = true;
            this->syncCalendar = true;
        
        // if any optional argument is specified, it checks what sync source is included
        }
        else {
            ArrayList list;
            sOption.split(list, ",");

            for (int i = 0; i < list.size(); i++) {
                StringBuffer* element = (StringBuffer*)list[i];
                if (element->icmp(contactsSourceName.c_str())) {
                    this->syncContacts = true;
                }
                else if (element->icmp(calendarSourceName.c_str())) {
                    this->syncCalendar = true;
                }
            }            
        }
    }
    
    // reset pim s2c
    sOption = this->options[CLI_OPTION_RESET_SERVER_CLIENT_PIM_L];
    
    if (!sOption.empty()) {
        resetS2C = true;
        // if no optional argument is specified, it synchronizes all pim sources by default
        if (sOption == "1") { // "1" == no option
            
            this->syncContacts = true;
            this->syncCalendar = true;
        
        // if any optional argument is specified, it checks what sync source is included
        }
        else {
            ArrayList list;
            sOption.split(list, ",");

            for (int i = 0; i < list.size(); i++) {
                StringBuffer* element = (StringBuffer*)list[i];
                if (element->icmp(contactsSourceName.c_str())) {
                    this->syncContacts = true;
                }
                else if (element->icmp(calendarSourceName.c_str())) {
                    this->syncCalendar = true;
                }
            }            
        }
    }
    
    // ------------------------------------------------------------------------
    
    // login
    
    sOption = this->options[CLI_OPTION_LOGIN_L];
    
    if (!sOption.null()) {
        
        this->doLogin = true;
    }
    
    // ------------------------------------------------------------------------
    
    // use-wait-queue
    
    sOption = this->options[CLI_OPTION_WAIT_QUEUE_L];
    
    if (!sOption.null()) {
        
        this->useWaitQueue = true;
    }
    
    // ------------------------------------------------------------------------
    
    // show-help
    
    sOption = this->options[CLI_OPTION_HELP_L];
    
    if (!sOption.null()) {
        
        this->showHelp = true;
    }
    
    // ------------------------------------------------------------------------
    // fast-local-check
    sOption = this->options[CLI_OPTION_FAST_CHECK_L];    
    if (!sOption.empty()) {  
        fastCheck = true;
        // if no optional argument is specified, it synchronizes all sources by default
        if (sOption == "1") { // "1" == no option. It is needed to specify at least one            
            this->syncContacts = false;
            this->syncCalendar = false;        
        }
        else {

            ArrayList list;
            sOption.split(list, ",");

            for (int i = 0; i < list.size(); i++) {
                StringBuffer* element = (StringBuffer*)list[i];
                if (element->icmp(contactsSourceName.c_str())) {
                    this->syncContacts = true;
                }
                else if (element->icmp(calendarSourceName.c_str())) {
                    this->syncCalendar = true;
                }                
            }            
        }
    }


    // ------------------------------------------------------------------------

    if (!this->validateOptions()) {
        
        this->resetOptions();
        return false;
    }
    else {
        
        return true;
    }
}

void SyncOptionManager::printHelpMessage()
{
    this->parser.usage();
}

const bool SyncOptionManager::hasSyncSourceOption(const char *sourceName)
{
    if (strcmp(sourceName, contactsSourceName.c_str()) == 0 && this->syncContacts) {
        return true;
    }
    if (strcmp(sourceName, calendarSourceName.c_str()) == 0 && this->syncCalendar) {
        return true;
    }
    if (strcmp(sourceName, filesSourceName.c_str()) == 0 && this->syncFiles) {
        return true;
    }
    if (strcmp(sourceName, picturesSourceName.c_str()) == 0 && this->syncPictures) {
        return true;
    }
    if (strcmp(sourceName, videosSourceName.c_str()) == 0 && this->syncVideos) {
        return true;
    }
    if (strcmp(sourceName, musicSourceName.c_str()) == 0 && this->syncMusic) {
        return true;
    }
    
    return false;
}

void SyncOptionManager::resetOptions()
{
    this->syncContacts = false;
    this->syncCalendar = false;
    resetS2C = false;
    resetC2S = false;
    this->syncFiles = false;
    this->syncPictures = false;
    this->syncVideos = false;
    this->syncMusic = false;
    
    this->syncConfig = false;

    this->doLogin = false;

    this->useWaitQueue = false;
    
    this->showHelp = false;
    this->fastCheck = false;
}


bool SyncOptionManager::validateOptions()
{
    bool valid = true;
    
    if (resetC2S && resetS2C) {
        valid = false;

        LOG.error("Mismatching options selected: %s %s)", CLI_OPTION_RESET_SERVER_CLIENT_PIM_L,
            CLI_OPTION_RESET_SERVER_CLIENT_PIM_L);
    } else if ((resetC2S || resetS2C) && (syncContacts == false && syncCalendar == false)) {
        valid = false;

        LOG.error("reset is available only for pim sources");
    }
    
    return valid;
}

} // end namespace Funambol
