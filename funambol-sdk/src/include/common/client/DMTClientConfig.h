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
#ifndef INCL_DM_CONFIG
#define INCL_DM_CONFIG
/** @cond API */
/** @addtogroup Client */
/** @{ */

#include <string>

#include "base/fscapi.h"
#include "http/constants.h"
#include "spdm/constants.h"
#include "spds/SyncManagerConfig.h"
#include "spds/AccessConfig.h"
#include "spds/DeviceConfig.h"
#include "spds/SyncSourceConfig.h"
#include "spdm/ConfigurationNode.h"
#include "spdm/ConfigurationTree.h"
#include "base/globalsdef.h"
#include "syncml/core/DataStore.h"

namespace Funambol {

class ConfigurationTree;

/**
 * This class is an extension of SyncManagerConfig that is DM tree aware; this
 * means that configuration properties are read/stored from/to the DM tree.
 */
class DMTClientConfig : public SyncManagerConfig {
    
    private:
    
        ConfigurationNode* syncMLNode;
        ConfigurationNode* sourcesNode;
        ConfigurationNode* serverNode;
    
        std::string sourceOrder; // client's sources display order as comma separated string
        std::string accessConfigUserToken; // client token for access config crypt/decrypt
        std::string credInfo; // client token for access config crypt/decrypt
        std::string subscriptionEnabled;

    protected:

        char*  rootContext;
        ConfigurationTree* dmt;
        
        void initialize();

        /* top level functions */
        virtual bool readRootConfig  ();
        virtual void saveRootConfig  ();
        virtual bool readAccessConfig(ConfigurationNode* n);
        virtual void saveAccessConfig(ConfigurationNode* n);
        virtual bool readDeviceConfig(ConfigurationNode* n, bool server = false);
        virtual void saveDeviceConfig(ConfigurationNode* n, bool server = false);
        virtual bool readMHConfig(ConfigurationNode* n);
        virtual void saveMHConfig(ConfigurationNode* n);
        virtual bool readSourceConfig(int i, ConfigurationNode* n);
        virtual void saveSourceConfig(int i, ConfigurationNode* n);

        /**
         * Called by readAccessConfig() to save authentication
         * settings.  The purpose of making this function virtual is
         * that a derived class can override it and then to read the
         * settings from a different than the default
         * "spds/syncml/auth" node by calling the base function with a
         * different \a authNode parameter or generate the settings in
         * some other way.
         *
         * @param syncMLNode     the "spds/syncml" node
         * @param authNode       the "spds/syncml/auth" node
         */
        virtual bool readAuthConfig(ConfigurationNode* syncMLNode,
                                    ConfigurationNode* authNode);
        /**
         * Same as readAccessConfig() for saving the settings.
         *
         * @param syncMLNode     the "spds/syncml" node
         * @param authNode       the "spds/syncml/auth" node
         */
        virtual void saveAuthConfig(ConfigurationNode* syncMLNode,
                                    ConfigurationNode* authNode);

        /**
         * Same as readAccessConfig() for reading connection
         * information.
         *
         * @param syncMLNode     the "spds/syncml" node
         * @param connNode       the "spds/syncml/conn" node
         */
        virtual bool readConnConfig(ConfigurationNode* syncMLNode,
                                    ConfigurationNode* connNode);
        /**
         * Same as readAccessConfig() for saving connection
         * information.
         *
         * @param syncMLNode     the "spds/syncml" node
         * @param connNode       the "spds/syncml/conn" node
         */
        virtual void saveConnConfig(ConfigurationNode* syncMLNode,
                                    ConfigurationNode* connNode);

        /**
         * Same as readAccessConfig() for reading additional access
         * information.
         *
         * @param syncMLNode     the "spds/syncml" node
         * @param extNode        the "spds/syncml/ext" node
         */
        virtual bool readExtAccessConfig(ConfigurationNode* syncMLNode,
                                         ConfigurationNode* extNode);
        /**
         * Same as readAccessConfig() for saving additional access
         * information.
         *
         * @param syncMLNode     the "spds/syncml" node
         * @param extNode        the "spds/syncml/ext" node
         */
        virtual void saveExtAccessConfig(ConfigurationNode* syncMLNode,
                                         ConfigurationNode* extNode);

        /**
         * Same as readAccessConfig() for reading device information.
         *
         * @param syncMLNode     the "spds/syncml" node
         * @param devInfoNode    the "spds/syncml/devinfo" node
         */
        virtual bool readDevInfoConfig(ConfigurationNode* syncMLNode,
                                       ConfigurationNode* devInfoNode, bool server = false);
        /**
         * Same as readAccessConfig() for saving device information.
         *
         * @param syncMLNode     the "spds/syncml" node
         * @param devInfoNode    the "spds/syncml/devinfo" node
         */
        virtual void saveDevInfoConfig(ConfigurationNode* syncMLNode,
                                       ConfigurationNode* devInfoNode, bool server = false);

        /**
         * Same as readAccessConfig() for reading additional device
         * information.
         *
         * @param syncMLNode     the "spds/syncml" node
         * @param devDetailNode  the "spds/syncml/devdetail" node
         */
        virtual bool readDevDetailConfig(ConfigurationNode* syncMLNode,
                                         ConfigurationNode* devDetailNode, bool server = false);
        /**
         * Same as readAccessConfig() for saving additional device
         * information.
         *
         * @param syncMLNode     the "spds/syncml" node
         * @param devDetailNode  the "spds/syncml/devdetail" node
         */
        virtual void saveDevDetailConfig(ConfigurationNode* syncMLNode,
                                         ConfigurationNode* devDetailNode, bool server = false);

        /**
         * Same as readAccessConfig() for reading some more additional
         * device information.
         *
         * @param syncMLNode     the "spds/syncml" node
         * @param extNode        the "spds/syncml/ext" node
         */
        virtual bool readExtDevConfig(ConfigurationNode* syncMLNode,
                                      ConfigurationNode* extNode, bool server = false);
        /**
         * Same as readAccessConfig() for saving some more additional
         * device information.
         *
         * @param syncMLNode     the "spds/syncml" node
         * @param extNode        the "spds/syncml/ext" node
         */
        virtual void saveExtDevConfig(ConfigurationNode* syncMLNode,
                                      ConfigurationNode* extNode, bool server = false);

        /**
         * Called by readMHConfig() to save MH media sync
         * settings.  The purpose of making this function virtual is
         * that a derived class can override it and then to read the
         * settings from a different than the default
         * "spds/syncml/MH" node by calling the base function with a
         * different \a MHNode parameter or generate the settings in
         * some other way.
         *
         * @param syncMLNode     the "spds/syncml" node
         * @param MHNode       the "spds/syncml/MH" node
         */
        virtual bool readMHConfig(ConfigurationNode* syncMLNode,
                                  ConfigurationNode* MHNode);


        /**
         * Same as readMHConfig() for saving the settings.
         *
         * @param syncMLNode     the "spds/syncml" node
         * @param MHNode       the "spds/syncml/MH" node
         */
        virtual void saveMHConfig(ConfigurationNode* syncMLNode,
                                  ConfigurationNode* MHNode);

        /**
         * Same as readAccessConfig() for reading variables that the
         * library uses internally, like anchors.
         *
         * @param i              index of the source
         * @param sourcesNode    the "spds/sources" node
         * @param sourceNode     the "spds/sources/<source name>" node
         */
        virtual bool readSourceVars(int i,
                                    ConfigurationNode* sourcesNode,
                                    ConfigurationNode* sourceNode);

        /**
         * Same as readAccessConfig() for saveing variables that the
         * library uses internally, like anchors.
         *
         * @param i              index of the source
         * @param sourcesNode    the "spds/sources" node
         * @param sourceNode     the "spds/sources/<source name>" node
         */
        virtual void saveSourceVars(int i,
                                    ConfigurationNode* sourcesNode,
                                    ConfigurationNode* sourceNode);

        /**
         * Same as readAccessConfig() for reading the normal
         * properties of a sync source, i.e. excluding variables like
         * anchors.
         *
         * @param i              index of the source
         * @param sourcesNode    the "spds/sources" node
         * @param sourceNode     the "spds/sources/<source name>" node
         */
        virtual bool readSourceConfig(int i,
                                      ConfigurationNode* sourcesNode,
                                      ConfigurationNode* sourceNode);

        /**
         * Same as readAccessConfig() for reading the normal
         * properties of a sync source, i.e. excluding variables like
         * anchors.
         *
         * @param i              index of the source
         * @param sourcesNode    the "spds/sources" node
         * @param sourceNode     the "spds/sources/<source name>" node
         */
        virtual void saveSourceConfig(int i,
                                      ConfigurationNode* sourcesNode,
                                      ConfigurationNode* sourceNode);

        /**
         * Reads the dataStores tree and sets values in serverConfig::dataStores
         * arrayList of DataStores. Only for Server config.
         *
         * @param dataStoresNode  the "server/DataStores" node to read from
         * @return                true if operation succeded
         */
        bool readDataStoresConfig(ConfigurationNode* dataStoresNode);

        /**
         * Reads a single dataStore and returns a DataStore object populated.
         * Only for Server config.
         *
         * @param dataStoreNode  the "server/DataStores/<datastore-name>" node to read from
         * @return               a new allocated DataStore
         */
        DataStore* readDataStoreConfig(ConfigurationNode* dataStoreNode);

        /**
         * Saves the dataStores tree from serverConfig::dataStores arrayList 
         * into the passed config node. Only for Server config.
         * Cleans up the existing nodes, then calls saveDataStoreConfig() for
         * each datastore in the array.
         *
         * @param dataStoresNode  the "server/DataStores" node to write into
         */
        void saveDataStoresConfig(ConfigurationNode* dataStoresNode);

        /**
         * Saves a single dataStore into the passed config node. 
         * Only for Server config.
         *
         * @param dataStoreNode   the "server/DataStores/<datastore-name>" node to write into
         * @param dataStore       the DataStore object with params to read
         */
        void saveDataStoreConfig(ConfigurationNode* dataStoreNode, DataStore* dataStore);
    
        /**
         * Encode a string before it is stored. Credentials are encoded.
         */
        virtual std::string encodeForSaving(const char*);
    
        /**
         * Decode a string after is has been read from the store.
         */
        virtual std::string decodeAfterReading(const char*);
    
    public:
    
        DMTClientConfig(ConfigurationTree* configTree);
        DMTClientConfig(const char* root, ConfigurationTree* configTree);
    
        /**
         * Initialize a DMTClientConfig with the given application context.
         * It's deprecated, use PlatformAdapter::init(appContext) and 
         * DMTClientConfig() instead.
         *
         * @param root the application context
         */
        DMTClientConfig(const char* root);
        
        /**
         * Initialize a DMTClientConfig. 
         * You must call PlatformAdapter::init(appContext) before.
         */
        DMTClientConfig();
    
        virtual ~DMTClientConfig();

        SyncSourceConfig* getSyncSourceConfig(const char* name, bool refresh = false);
        SyncSourceConfig* getSyncSourceConfig(unsigned int i,   bool refresh = false);

        virtual bool read();
        virtual bool save();

        /**
         * Gets number of sync source configurations, -1 if not open.
         */
        virtual int getNumSources();


        /**
         * Set client's sources display order
         */
         
        virtual void setSourceOrder(const char* sourceOrder);
        /**
         * Gets client's sources display order 
         */
        virtual const char* getSourceOrder() const;
       
        virtual void setAccessConfigUserToken(const char* accessConfigUserToken);
        virtual void setSubscriptionsEnabled(const char* subscriptionEnabled);
        virtual const char* getSubscriptionsEnabled() const;
        
         
        /**
         * Get the specified sync source configuration.
         *
         * @param index    number of the requested sync source configuration
         * @return node pointer owned by config and valid while the config is open
         */
        virtual ConfigurationNode* getSyncSourceNode(int index);

        /**
         * Get the specified sync source configuration by name.
         */
        virtual ConfigurationNode* getSyncSourceNode(const char* name);


        virtual bool saveSyncSourceConfig(const char* name);
        virtual const char* getCredInfo() const;
        virtual void setCredInfo(const char* credInfo_);
};


} // end namespace Funambol

/** @} */
/** @endcond */
#endif
