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

#ifndef INCL_COMMAND_REPORT
#define INCL_COMMAND_REPORT
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


class CommandReport : public MHStoreEntry {

private:

    /// The command id 
    StringBuffer    commandId;
    
    /// The command name  
    StringBuffer    commandName;
    
    /// The final status of the command
    int             commandStatus;
    
    /// The timestamp the command has been completed
    StringBuffer    commandTimestamp;
    
    /// The message to be shown in the summary list
    StringBuffer    shortMessage;
    
    /// The message to be shown in the detailed list    
    StringBuffer    longMessage;
        
    unsigned        itemsAdded;
    unsigned        itemsReceived;
    unsigned        itemsDeleted;
    
    unsigned        itemsAddedSuccessfully;
    unsigned        itemsReceivedSuccessfully;
    unsigned        itemsDeletedSuccessfully;

    
    
    unsigned        itemsUpdatedByClient;
    unsigned        itemsUpdatedByServer;
    unsigned        itemsDeletedByClient;
    unsigned        itemsDeletedByServer;
    unsigned        itemsAddedByClient;
    unsigned        itemsAddedByServer;
    
public:
    
    CommandReport();
    ~CommandReport();
    explicit CommandReport(CommandReport& copy);
    
    //
    // Getters
    //
    const char* getCommandId()          const    { return commandId.c_str();         }
    const char* getCommandName()        const    { return commandName.c_str();       }
    int         getCommandStatus()      const    { return commandStatus;             }
    const char* getCommandTimestamp()   const    { return commandTimestamp.c_str();  }
    const char* getShortMessage()       const    { return shortMessage.c_str();      }
    const char* getLongMessage()        const    { return longMessage.c_str();       }
   
    unsigned    getItemsAdded()         const    { return itemsAdded;                }
    unsigned    getItemsReceived()      const    { return itemsReceived;             }
    unsigned    getItemsDeleted()       const    { return itemsDeleted;              }

    unsigned    getItemsUpdatedByClient()       const    { return itemsUpdatedByClient;              }
    unsigned    getItemsUpdatedByServer()       const    { return itemsUpdatedByServer;              }
    unsigned    getItemsDeletedByClient()       const    { return itemsDeletedByClient;              }
    unsigned    getItemsDeletedByServer()       const    { return itemsDeletedByServer;              }
    unsigned    getItemsAddedByClient()         const    { return itemsAddedByClient;              }
    unsigned    getItemsAddedByServer()         const    { return itemsAddedByServer;              }
    
    unsigned    getItemsAddedSuccessfully()         const    { return itemsAddedSuccessfully;                }
    unsigned    getItemsReceivedSuccessfully()      const    { return itemsReceivedSuccessfully;             }
    unsigned    getItemsDeletedSuccessfully()       const    { return itemsDeletedSuccessfully;              }
    
    //
    // Setters
    //
    void setCommandId           (unsigned int commandId);
    void setCommandId           (const char* commandId)         { this->commandId           = commandId;            }
    void setCommandName         (const char* commandName)       { this->commandName         = commandName;          }
    void setCommandStatus       (int  _commandStatus)           { this->commandStatus       = _commandStatus;       }
    void setCommandTimestamp    (const char* commandTimestamp)  { this->commandTimestamp    = commandTimestamp;     }
    void setShortMessage        (const char* shortMessage)      { this->shortMessage        = shortMessage;         }
    void setLongMessage         (const char* longMessage)       { this->longMessage         = longMessage;          }
    
    void setItemsAdded(unsigned count)    { itemsAdded    = count; }
    void setItemsReceived(unsigned count) { itemsReceived = count; }
    void setItemsDeleted(unsigned count)  { itemsDeleted  = count; }

    void setItemsUpdatedByClient(unsigned count)  { itemsUpdatedByClient  = count; }
    void setItemsUpdatedByServer(unsigned count)  { itemsUpdatedByServer  = count; }
    void setItemsDeletedByClient(unsigned count)  { itemsDeletedByClient  = count; }
    void setItemsDeletedByServer(unsigned count)  { itemsDeletedByServer  = count; }
    void setItemsAddedByClient(unsigned count)  { itemsAddedByClient  = count; }
    void setItemsAddedByServer(unsigned count)  { itemsAddedByServer  = count; }
    
    void setItemsAddedSuccessfully(unsigned count) { itemsAddedSuccessfully = count; }
    void setItemsReceivedSuccessfully(unsigned count) { itemsReceivedSuccessfully = count; }
    void setItemsDeletedSuccessfully(unsigned count)  { itemsDeletedSuccessfully  = count; }
    
};

END_FUNAMBOL_NAMESPACE

/** @endcond */
#endif
