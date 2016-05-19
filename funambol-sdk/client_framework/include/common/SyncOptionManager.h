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

#ifndef __SYNC_OPTION_MANAGER_H__
#define __SYNC_OPTION_MANAGER_H__

#include "base/fscapi.h"
#include "base/util/StringMap.h"
#include "client/OptionParser.h"
#include "CustomizationParams.h"
#include <string>

namespace Funambol {

/**
 * This class allows to get options from the command line.
 */
class SyncOptionManager
{
    
public:
    
    /** Default constructor */
    SyncOptionManager(const char *progamName, CustomizationParams* customParams);
    
    /** parse the command line */
    bool parseCommandLine(int argc, char** argv);
    
    /** Get parser error */
    inline const char *getErrorMessage() const { return this->parser.getErrMsg(); }
    
    void printHelpMessage();
    
    /** Options */
    
    const bool hasSyncContactsOption() const { return this->syncContacts; }
    const bool hasSyncCalendarOption() const { return this->syncCalendar; }
    const bool hasC2SResetOption() const { return resetC2S; }
    const bool hasS2CResetOption() const { return resetS2C; }
    const bool hasSyncFilesOption() const { return this->syncFiles; }
    const bool hasSyncPicturesOption() const { return this->syncPictures; }
    const bool hasSyncVideosOption() const { return this->syncVideos; }
    const bool hasSyncMusicOption() const { return this->syncMusic; }
    const bool hasSyncConfigOption() const { return this->syncConfig; }
    const bool hasDoLoginOption() const { return this->doLogin; }
    const bool hasUseWaitQueueOption() const { return this->useWaitQueue; }
    const bool hasShowHelpOption() const { return this->showHelp; }
    const bool hasAnyNotTrivialSyncSourceOption() const {
		return this->syncContacts || this->syncCalendar || this->syncFiles || this->syncPictures || this->syncVideos || this->syncMusic;
    }

    const bool hasSyncSourceOption(const char *sourceName);
    const bool hasFastCheckOption() const { return fastCheck; }


private:
   
    std::string contactsSourceName;
    std::string calendarSourceName;
    std::string picturesSourceName;
    std::string videosSourceName;
    std::string filesSourceName;
    std::string musicSourceName;
    
     
    bool syncContacts;
    bool syncCalendar;
    bool resetS2C;
    bool resetC2S;
    
    bool syncFiles;
    bool syncPictures;
    bool syncVideos;
    bool syncMusic;
    
    bool syncConfig;
    
    bool doLogin;
    
    bool useWaitQueue;
    
    bool showHelp;

    bool fastCheck;
    
    OptionParser parser;
    StringMap options;
    ArrayList arguments;
    
    void resetOptions();
    bool validateOptions();

public:
    static const char* CLI_OPTION_SYNC_PIM_L;
    static const char CLI_OPTION_SYNC_PIM_S;
    static const char* CLI_OPTION_SYNC_MEDIA_L;
    static const char CLI_OPTION_SYNC_MEDIA_S;
    static const char* CLI_OPTION_FOLDER_L;
    static const char CLI_OPTION_FOLDER_S;
    static const char* CLI_OPTION_RESET_CLIENT_SERVER_PIM_L;
    static const char CLI_OPTION_RESET_CLIENT_SERVER_PIM_S;
    static const char* CLI_OPTION_RESET_SERVER_CLIENT_PIM_L;
    static const char CLI_OPTION_RESET_SERVER_CLIENT_PIM_S;
    static const char* CLI_OPTION_LOGIN_L;
    static const char CLI_OPTION_LOGIN_S;
    static const char* CLI_OPTION_COMPATIBILITY_MODE_L;
    static const char CLI_OPTION_COMPATIBILITY_MODE_S;
    static const char* CLI_OPTION_HELP_L;
    static const char CLI_OPTION_HELP_S;
    static const char* CLI_OPTION_WAIT_QUEUE_L;
    static const char CLI_OPTION_WAIT_QUEUE_S;
    static const char* CLI_OPTION_SYNC_ALL_L;
    static const char CLI_OPTION_SYNC_ALL_S;
    static const char* CLI_OPTION_FAST_CHECK_L;
    static const char CLI_OPTION_FAST_CHECK_S;
    
};

} // end namespace Funambol

#endif
