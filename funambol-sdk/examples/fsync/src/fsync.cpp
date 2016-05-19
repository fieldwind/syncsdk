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

#include "base/fscapi.h"

#include "FSyncConfig.h"
#include "FSyncOpt.h"
#include "FSyncUpdater.h"

#include "base/adapter/PlatformAdapter.h"
#include "base/util/StringBuffer.h"
#include "base/util/utils.h"
#include "base/Log.h"
// Syncml sync classes
#include "client/SyncClient.h"
#include "client/FileSyncSource.h"
// Sapi sync classes
#include "sapi/SapiSyncManager.h"
#include "sapi/FileSapiSyncSource.h"

#ifndef EXIT_SUCCESS
#	define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
#	define EXIT_FAILURE 1
#endif

USE_NAMESPACE

static const char *progname = "fsync";

bool doSync(FSyncOpt& options, SyncReport& report) 
{
    SyncSourceConfig *srcconfig = FSyncConfig::getInstance()->getSyncSourceConfig(FSYNC_SOURCE_NAME);

    // Create the SyncSource passing its name, the SyncSourceConfig 
    FileSyncSource fSource(TEXT(FSYNC_SOURCE_NAME), srcconfig);    
    
    // Store the path to the folder to sync
    fSource.setDir(FSyncConfig::getInstance()->getSyncPath());

    // Initialize the SyncSource array to sync
    SyncSource* ssArray[] = { &fSource, NULL } ;

    // Create the SyncClient
    SyncClient fileClient;

    // SYNC!
    if (fileClient.sync(*FSyncConfig::getInstance(), ssArray)) {
        LOG.error("Error during sync.\n");
        return false;
    }

    // Save the anchors
    FSyncConfig::getInstance()->save();

    return true;
}

bool doMediaSync(FSyncOpt& options, SyncReport& report)
{
    int ret = 0;
    const char *sourcename = FSYNC_SOURCE_NAME;
    int syncMode = SYNC_TWO_WAY; // FIXME

    FSyncConfig *config = FSyncConfig::getInstance();
    SyncSourceConfig *ssconfig = config->getSyncSourceConfig(sourcename);
    if(!ssconfig){
        LOG.error("SyncSource %s not found", sourcename);
        return false;
    }
    
    SyncSourceReport ssReport(sourcename);
    FileSapiSyncSource source(*ssconfig, ssReport, 0, 0, config->getCacheDir());   
    
    // BeginSync ---------------------------------------------
    SapiSyncManager sapiManager(source, *config);
    ret = sapiManager.beginSync();
    config->saveSyncSourceConfig(sourcename);
    if (ret != 0) {
        goto finally;
    }
    
    // UPLOAD ------------------------
    if (syncMode == SYNC_TWO_WAY ||
        syncMode == SYNC_ONE_WAY_FROM_CLIENT) {
        if (report.getLastErrorCode() != ESSMServerQuotaExceeded) {
            ret = sapiManager.upload();
            config->saveSyncSourceConfig(sourcename);
        } else {
            source.getReport().setState(SOURCE_ERROR);
            source.getReport().setLastErrorCode(ESSMServerQuotaExceeded);
            source.getReport().setLastErrorType(ERROR_TYPE_SAPI_SYNC_MANAGER);
            source.getReport().setLastErrorMsg("Upload skipped: quota exceeded for a previous source");
            LOG.info("%s", source.getReport().getLastErrorMsg());
        }
    }
    if (ret == ESSMCanceled ||
        ret == ESSMNetworkError ||
        ret == ESSMAuthenticationError) {
        goto finally;
    }
    
    // DOWNLOAD
    if (syncMode == SYNC_TWO_WAY ||
        syncMode == SYNC_ONE_WAY_FROM_SERVER) {
        if (report.getLastErrorCode() != ESSMClientQuotaExceeded) {
            ret = sapiManager.download();
            config->saveSyncSourceConfig(sourcename);
        } else {
            source.getReport().setState(SOURCE_ERROR);
            source.getReport().setLastErrorType(ERROR_TYPE_SAPI_SYNC_MANAGER);
            source.getReport().setLastErrorCode(ESSMClientQuotaExceeded);
            source.getReport().setLastErrorMsg("Download skipped: local storage full for a previous source");
            LOG.info("%s", source.getReport().getLastErrorMsg());
        }
    }
    if (ret == ESSMCanceled ||
        ret == ESSMNetworkError ||
        ret == ESSMAuthenticationError) {
        goto finally;
    }
    
    ret = sapiManager.endSync();
    
finally:
    
    // set the last error for this source
    source.getConfig().setLastSourceError(ret);
    
    report.addSyncSourceReport(source.getReport());
    //delete source;
    
    return ret;
}

//------------------------------------------------------------------------ Main
int main(int argc, char** argv) 
{
    PlatformAdapter::init(FSYNC_APPLICATION_URI);

    // Get the config instance
    FSyncConfig *config = FSyncConfig::getInstance();
    // Initialize it (read from file or create the default one
    config->init();

    // Init LOG
    LOG.setLevel(config->getDeviceConfig().getLogLevel());
    LOG.reset(FSYNC_LOG_TITLE);

    // Initialize the command line options handler
    FSyncOpt opts(progname); 

    // Parse command line options
    if (opts.parseCmdline(argc, argv) == false) {
        fprintf(stderr, "error parsing options: %s\n", opts.getErr());
        exit(EXIT_FAILURE);
    }

    // On user help request exit smoothly without doing anything 
    if (opts.optionSet("help")) { 
        exit(EXIT_SUCCESS);
    }

    // Initialize the view class to handle the feedbacks to the user.
    FSyncUpdater updater;
    updater.setListeners(opts.getVerbosity());
    SyncReport report;

    // Check the presence of the sync folder
    if (createFolder(config->getSyncPath()) < 0) {
        LOG.error("error creating folder");
        exit(EXIT_FAILURE);
    }

    // Sync
    if (opts.optionSet("media")) {
	if (doMediaSync(opts, report) == false) {
	    // Sync failed
	    exit(EXIT_FAILURE);
	}
    } else {	
	if (doSync(opts, report) == false) {
	    // Sync failed
	    exit(EXIT_FAILURE);
	}
    }

    StringBuffer rep;
    report.toString(rep, true);
    LOG.info("\n%s", rep.c_str());
    
    exit(EXIT_SUCCESS);
}

