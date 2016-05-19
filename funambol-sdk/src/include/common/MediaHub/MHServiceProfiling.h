/*
 * Funambol is a mobile platform developed by Funambol, Inc.
 * Copyright (C) 2003 - 2011 Funambol, Inc.
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

#ifndef MH_SERVICE_PROFILING
#define MH_SERVICE_PROFILING

/** @cond API */
/** @addtogroup Client */
/** @{ */

#include "base/globalsdef.h"
#include "base/util/ArrayList.h"
#include "base/util/StringMap.h"
#include "spds/constants.h"
#include "spds/AbstractSyncConfig.h"
#include "MediaHub/MHMediaRequestManager.h"

BEGIN_FUNAMBOL_NAMESPACE

/**
 * Used to get profile informations for the current user account.
 * It executes a MH login, including the '&details' parameter to retrieve additional
 * informations such as:
 *  - dataplan expire time
 *  - auto-sync enabled/disabled
 *  - list of sources allowed
 *  - network warning enabled/disabled
 */
class MHServiceProfiling {

public:

    /**
     * Constructor
     * @param c  configuration object to read all required param for MH login
     */
    MHServiceProfiling(AbstractSyncConfig& c);

    /// Destructor
    virtual ~MHServiceProfiling();

    /**
     * Executes the MH login with "&details" parameter.
     * Stores the returned configuration parameters from the Server.
     * @return 0  if login was SUCCESSFUL
     *        -1  if there was some errors
     */
    int login();

    ESMRStatus    getError()            { return err;            }

private:

    /// The configuration object, containing all sync settings.
    AbstractSyncConfig& config;

	/// The error code returned by MHMediaRequestManager::login().
	ESMRStatus err;


};


END_FUNAMBOL_NAMESPACE

/** @} */
/** @endcond */
#endif

