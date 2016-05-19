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
#include "base/debug.h"
#include "base/errors.h"
#include "base/Log.h"
#include "base/globalsdef.h"
#include "base/util/utils.h"
#include "base/adapter/PlatformAdapter.h"

#include "client/DMTClientConfig.h"
#include "spdm/constants.h"
#include "spdm/DMTreeFactory.h"
#include "spdm/DMTree.h"
#include "spdm/ManagementNode.h"
#include "spds/DefaultConfigFactory.h"
#include "spds/spdsutils.h"
#include "client/MailSourceManagementNode.h"

namespace Funambol {

void DMTClientConfig::initialize() {

    
    LOG.debug("%s: init dmt nodes", __FUNCTION__);
    StringBuffer nodeName;
    
    if (dmt == NULL) {
        rootContext = new char[PlatformAdapter::getAppContext().length()+1];
        strcpy(rootContext, PlatformAdapter::getAppContext().c_str());
        dmt = DMTreeFactory::getDMTree(rootContext);
    } else {
        rootContext = stringdup(dmt->getRootContext());
    }
    
    nodeName.sprintf("%s%s", rootContext, CONTEXT_SPDS_SYNCML);
    syncMLNode = dmt->readManagementNode(nodeName.c_str());
    if (!syncMLNode) {
        LOG.error("%s: error reading syncML node: %s", __FUNCTION__, nodeName.c_str());
        return;
    }
    
    nodeName.sprintf("%s%s", rootContext, CONTEXT_SERVER);
    serverNode = dmt->readManagementNode(nodeName.c_str());
    if (!serverNode) {
        LOG.error("%s: error reading server node: %s", __FUNCTION__, nodeName.c_str());
        return;
    }
    
    nodeName.sprintf("%s%s", rootContext, CONTEXT_SPDS_SOURCES);
    sourcesNode = dmt->readManagementNode(nodeName.c_str());
    if (!sourcesNode) {
        LOG.error("%s: error reading sources node: %s", __FUNCTION__, nodeName.c_str());
        return;
    }
}
    
    
DMTClientConfig::DMTClientConfig(const char* root, ConfigurationTree* configTree) :
    SyncManagerConfig(), sourceOrder(""), accessConfigUserToken(""), subscriptionEnabled(""), dmt(configTree)
{
    PlatformAdapter::init(root);
    initialize();
}

DMTClientConfig::DMTClientConfig() :
    SyncManagerConfig(), sourceOrder(""), accessConfigUserToken(""), subscriptionEnabled(""), dmt(NULL)
{
    initialize();
}
    
    
DMTClientConfig::DMTClientConfig(ConfigurationTree* configTree) :
    SyncManagerConfig(), sourceOrder(""), accessConfigUserToken(""), subscriptionEnabled(""), dmt(configTree)
{
    initialize();
}


DMTClientConfig::DMTClientConfig(const char* root): 
    SyncManagerConfig(), sourceOrder(""), accessConfigUserToken(""), subscriptionEnabled(""), dmt(NULL)
{
    LOG.info("DMTClientConfig(root): deprecated method, use PlatformAdapter::init(root) instead.");
    PlatformAdapter::init(root);
    initialize();
}


DMTClientConfig::~DMTClientConfig() 
{
    LOG.debug("%s: releasing dmt nodes", __FUNCTION__);
    delete [] rootContext;
    delete syncMLNode;
    delete sourcesNode;
    delete serverNode;
    delete dmt;
}

SyncSourceConfig* DMTClientConfig::getSyncSourceConfig(const char* name, bool refresh) 
{
    if ((name == NULL) || (strlen(name) == 0)) {
        return NULL;
    }

    //
    // If refresh is true, we need to re-read the syncsource settings
    // from the DM tree.
    //
    // PS: for now we use a brute force approach so that we refresh the
    // entire configuration. A better implementation would be to just
    // refresh the single source.
    //
    if (refresh) {
        read();
    }

    for (unsigned int i=0; i<sourceConfigsCount; ++i) {
        if (strcmp(sourceConfigs[i].getName(), name) == 0) {
            return &sourceConfigs[i];
        }
    }

    return NULL;
}


SyncSourceConfig* DMTClientConfig::getSyncSourceConfig(unsigned int i, bool refresh) 
{
    if (i >= sourceConfigsCount) {
        return NULL;
    }

    //
    // If refresh is true, we need to re-read the syncsource settings
    // from the DM tree.
    //
    // PS: for now we use a brute force approach so that we refresh the
    // entire configuration. A better implementation would be to just
    // refresh the single source.
    //
    if (refresh) {
        read();
    }

    return &sourceConfigs[i];
}


bool DMTClientConfig::read() 
{
    int n = 0, i = 0; // number of sync sources
    bool ret = false;

    LOG.debug("%s", DBG_READING_CONFIG_FROM_DM);
    if (!dmt || !syncMLNode || !serverNode || !sourcesNode) {
        LOG.error("%s: dmt nodes not initialized", __FUNCTION__);
        return false;
    }
    
    bool exist = dmt->checkNodeExistence(rootContext);
    
    // Need to refresh nodes, in case children have been added (i.e. createConfig on first run, or upgrade)
    dmt->refreshNodeChildren(syncMLNode);
    dmt->refreshNodeChildren(serverNode);
    dmt->refreshNodeChildren(sourcesNode);


    //
    // Check if the server device config exists by analyzing the last error code
    // and create it otherwise.
    //
    resetError();
    // reading Server Device Config (the true is to switch to the serverConfig object)
    readDeviceConfig(serverNode, true);
    if(getLastErrorCode() != 0) {
        LOG.debug("Server DeviceConfig not found, create a default one.");
        
        DeviceConfig* sdc = DefaultConfigFactory::getServerDeviceConfig();
        setServerConfig(*sdc);
        delete sdc; sdc = NULL;

        saveDeviceConfig(serverNode, true);
        resetError();
        readDeviceConfig(serverNode, true);
    }

    // Reading AccessConfig
    readAccessConfig(syncMLNode);

    // Reading DeviceConfig
    readDeviceConfig(syncMLNode);

    n = sourcesNode->getChildrenMaxCount();
    
    //
    // Let's remove previously created config objects and reinitialize
    // sourceConfigs
    //
    if (sourceConfigs) {
        delete [] sourceConfigs;
    }
    if (n>0) {
        sourceConfigs = new SyncSourceConfig[n];
    }
    sourceConfigsCount = n;

    for (i=0; i<n; ++i) {
        // node owns children, we must not delete them
        readSourceConfig(i, sourcesNode);
    }

    //
    // Read root params
    //
    readRootConfig();
    
    // Reading MHConfig
    readMHConfig(syncMLNode);

    // TODO: As soon as we can get rid of the DMTree we will no longer rely on the global variable
    // and we will remove this code
    ret = (getLastErrorCode() == 0);
    /////////
    
    ret = ret && exist;

    return ret;
}


bool DMTClientConfig::save() 
{
    bool ret = false;

    LOG.debug("%s", DBG_WRITING_CONFIG_TO_DM);
    if (!dmt || !syncMLNode || !serverNode || !sourcesNode) {
        LOG.error("%s: dmt nodes not initialized", __FUNCTION__);
        return false;
    }
     
    if (accessConfig.getDirty()) {
        resetError();

        //
        // SyncML management node
        //
        saveAccessConfig(syncMLNode);
    }

    saveDeviceConfig(syncMLNode);
    saveDeviceConfig(serverNode, true);

    // Save MHConfig node
    saveMHConfig(syncMLNode);

    //
    // Sources management node
    //
    resetError();
    for(int i=0; i<sourceConfigsCount; i++) {
        saveSourceConfig(i, sourcesNode);
    }


    //
    // Save root params
    //
    saveRootConfig();

    
    resetError(); // FIXME 

    ret = (getLastErrorCode() == 0);

    return ret;
}

int DMTClientConfig::getNumSources() {
    return sourcesNode ?
        sourcesNode->getChildrenMaxCount() :
        -1;
}

ConfigurationNode* DMTClientConfig::getSyncSourceNode(int index) {
    return sourcesNode ?
        sourcesNode->getChild(index) :
        NULL;
}

ConfigurationNode* DMTClientConfig::getSyncSourceNode(const char* name) {
    return sourcesNode ?
        sourcesNode->getChild(name) :
        NULL;
}


bool DMTClientConfig::readRootConfig() {

    if (!dmt) {
        LOG.error("%s: dmt nodes not initialized", __FUNCTION__);
        return false;
    }

    ConfigurationNode* rootNode = dmt->readManagementNode(rootContext);
    if (!rootNode) {
        return false;
    }

    char* tmp = rootNode->readPropertyValue(PROPERTY_LAST_GLOBAL_ERROR);
    int lastError = 0;
    if (tmp && strlen(tmp)>0) {
        lastError = atoi(tmp);
    }
    setLastGlobalError(lastError);

    tmp = rootNode->readPropertyValue(PROPERTY_INSTALLATION_TIMESTAMP);
    unsigned long timeStamp = 0;
    if (tmp && strlen(tmp) > 0) {        
        timeStamp = strtoul(tmp, NULL, 10);        
    }    
    if (timeStamp == 0) {
        timeStamp = (unsigned long)time(NULL);
    }
    
    setInstallationTimestamp(timeStamp);
    delete [] tmp;

    tmp = rootNode->readPropertyValue(PROPERTY_SOURCE_ORDER);
    if (tmp) {
        sourceOrder = tmp;
    } 
    delete [] tmp;

    
    tmp = rootNode->readPropertyValue(PROPERTY_CREDINFO_KEY);
    if (tmp) {
        credInfo = tmp;
    } 
    delete [] tmp;
    
    tmp = rootNode->readPropertyValue(PROPERTY_SESSION_ID);
    if (tmp) {
        setSessionId(tmp);
    } 
    delete [] tmp;
    
    time_t sessionIdTS = 0;
    tmp = rootNode->readPropertyValue(PROPERTY_SESSION_ID_SET_TIME);
    if (tmp && strlen(tmp) > 0) {        
        sessionIdTS = strtod(tmp, NULL);        
    }    
    
    setSessionIdSetTime(sessionIdTS);
    delete [] tmp;
    
    tmp = rootNode->readPropertyValue(PROPERTY_SESSION_ID_EXP_TIME);
    if (tmp && strlen(tmp) > 0) {        
        sessionIdTS = strtod(tmp, NULL);        
    }    
    
    setSessionIdExpirationTime(sessionIdTS);
    delete [] tmp;

    // validationKey is always stored encrypted
    tmp = rootNode->readPropertyValue(PROPERTY_VALIDATION_KEY);
    if (tmp) {
        std::string clearValidationKey = decodeAfterReading(tmp);
        setValidationKey(clearValidationKey.c_str());
    }
    delete [] tmp;
    
    tmp = rootNode->readPropertyValue(PROPERTY_SUBSCRIPTION_ENABLED);
    if (tmp && strlen(tmp) > 0) {   
        setSubscriptionsEnabled(tmp);
    } 
    delete [] tmp;
    
    delete rootNode;
    return true;
}

void DMTClientConfig::saveRootConfig() {

    if (!dmt) {
        LOG.error("%s: dmt nodes not initialized", __FUNCTION__);
        return;
    }

    ConfigurationNode* rootNode = dmt->readManagementNode(rootContext);
    if (!rootNode) {
        return;
    }

    StringBuffer buf;
    buf.sprintf("%i", getLastGlobalError());
    rootNode->setPropertyValue(PROPERTY_LAST_GLOBAL_ERROR, buf.c_str());
    
    buf = "0";
    buf.sprintf("%lu", getInstallationTimestamp());
    rootNode->setPropertyValue(PROPERTY_INSTALLATION_TIMESTAMP, buf.c_str());
  
    // save sources order
    rootNode->setPropertyValue(PROPERTY_SOURCE_ORDER, sourceOrder.c_str());

    LOG.debug("%s: saving cred info property with value: %s", __FUNCTION__,
        credInfo.c_str());
    rootNode->setPropertyValue(PROPERTY_CREDINFO_KEY, credInfo.c_str());

    // save session id related props
    rootNode->setPropertyValue(PROPERTY_SESSION_ID, getSessionId());
    
    buf.reset();
    buf.sprintf("%ld", getSessionIdSetTime()); 
    rootNode->setPropertyValue(PROPERTY_SESSION_ID_SET_TIME, buf.c_str());
    
    buf.reset();
    buf.sprintf("%ld", getSessionIdExpirationTime()); 
    rootNode->setPropertyValue(PROPERTY_SESSION_ID_EXP_TIME, buf.c_str());

    // validationKey is always stored encrypted
    std::string encodedValidationKey = encodeForSaving(getValidationKey());
    rootNode->setPropertyValue(PROPERTY_VALIDATION_KEY, encodedValidationKey.c_str());

    buf.reset();
    buf.sprintf("%s", getSubscriptionsEnabled());
    rootNode->setPropertyValue(PROPERTY_SUBSCRIPTION_ENABLED, buf.c_str());
   
    
    delete rootNode;
}


/*
 * Read Access Config properties stored in DMTree.
 * Access properties are placed in 3 nodes under syncML node
 * (Auth - Conn - Ext)
 *
 * @param n: the 'syncml' node (parent node)
 * @return : true if config is correctly read
 */
bool DMTClientConfig::readAccessConfig(ConfigurationNode* n) {

    if (!dmt) {
        LOG.error("%s: dmt nodes not initialized", __FUNCTION__);
        return false;
    }

    bool ret = true;
    StringBuffer nodeName;
    ConfigurationNode* node;
    char* fn = n->createFullName();


    //
    // Auth properties
    //
    nodeName.sprintf("%s%s", fn, CONTEXT_AUTH);
    node = dmt->readManagementNode(nodeName.c_str());
    if (node) {
        if (!readAuthConfig(n, node)) {
            ret = false;
        }
        delete node;
        node = NULL;
    }
    else {
        ret = false;
    }

    //
    // Conn properties
    //
    nodeName.sprintf("%s%s", fn, CONTEXT_CONN);
    node = dmt->readManagementNode(nodeName.c_str());
    if (node) {
        if (!readConnConfig(n, node)) {
            ret = false;
        }
        delete node;
        node = NULL;
    }
    else {
        ret = false;
    }

    //
    // Ext properties (other misc props)
    //
    nodeName.sprintf("%s%s", fn, CONTEXT_EXT);
    node = dmt->readManagementNode(nodeName.c_str());
    if (node) {
        if (!readExtAccessConfig(n, node)) {
            ret = false;
        }
        delete node;
        node = NULL;
    }
    else {
        ret = false;
    }

    delete [] fn;
    return true;
}

/*
 * Save Access Config properties in DMTree.
 * Access properties are placed in 3 nodes under syncML node
 * (Auth - Conn - Ext)
 *
 * @param n: the 'syncml' node (parent node)
 */
void DMTClientConfig::saveAccessConfig(ConfigurationNode* n) {
    
    if (!dmt) {
        LOG.error("%s: dmt nodes not initialized", __FUNCTION__);
        return;
    }

    ConfigurationNode* node;
    StringBuffer nodeName;
    char* fn = n->createFullName();

    //
    // Auth properties
    //
    nodeName.sprintf("%s%s", fn, CONTEXT_AUTH);
    node = dmt->readManagementNode(nodeName.c_str());
    if (node) {
        saveAuthConfig(n, node);
        delete node;
        node = NULL;
    }

    //
    // Conn properties
    //
    nodeName.sprintf("%s%s", fn, CONTEXT_CONN);
    node = dmt->readManagementNode(nodeName.c_str());
    if (node) {
        saveConnConfig(n, node);
        delete node;
        node = NULL;
    }

    //
    // Ext properties (other misc props)
    //
    nodeName.sprintf("%s%s", fn, CONTEXT_EXT);
    node = dmt->readManagementNode(nodeName.c_str());
    if (node) {
        saveExtAccessConfig(n, node);
        delete node;
        node = NULL;
    }
    
    delete [] fn;
}


/*
 * Read Device Config properties stored in DMTree.
 * Device properties are placed in 3 nodes under syncML node
 * (DevInfo - DevDetail - Ext)
 *
 * @param n: the 'syncml' node (parent node)
 * @return : true if config is correctly read
 */
bool DMTClientConfig::readDeviceConfig(ConfigurationNode* n, bool server) {
    
    if (!dmt) {
        LOG.error("%s: dmt nodes not initialized", __FUNCTION__);
        return false;
    }

    bool ret = true;
    ConfigurationNode* node;
    StringBuffer nodeName;
    char* fn = n->createFullName();
    
    //
    // DevInfo properties
    //
    nodeName.sprintf("%s%s", fn, CONTEXT_DEV_INFO);
    node = dmt->readManagementNode(nodeName.c_str());
    if (node) {
        if (!readDevInfoConfig(n, node, server)) {
            ret = false;
        }
        delete node;
        node = NULL;
    }
    else {
        ret = false;
    }

    //
    // DevDetail properties
    //
    nodeName.sprintf("%s%s", fn, CONTEXT_DEV_DETAIL);
    node = dmt->readManagementNode(nodeName.c_str());
    if (node) {
        if (!readDevDetailConfig(n, node, server)) {
            ret = false;
        }
        delete node;
        node = NULL;
    }
    else {
        ret = false;
    }

    //
    // Ext properties (other misc props)
    //
    nodeName.sprintf("%s%s", fn, CONTEXT_EXT);
    node = dmt->readManagementNode(nodeName.c_str());
    if (node) {
        if (!readExtDevConfig(n, node, server)) {
            ret = false;
        }
        delete node;
        node = NULL;
    }
    else {
        ret = false;
    }

    //
    // DataStores (Server config only)
    //
    if (server) {
        nodeName.sprintf("%s%s", fn, CONTEXT_DATASTORES);
        node = dmt->readManagementNode(nodeName.c_str());
        if (node) {
            if (!readDataStoresConfig(node)) {
                ret = false;
            }
            delete node;
            node = NULL;
        }
        else {
            // may not be found, if upgrading from old client: no error returned.
            // (Server datastores were ignored before v.8.2)
        }
    }

    delete [] fn;
    return ret;
}


/*
 * Save Device Config properties in DMTree.
 * Device properties are placed in 3 nodes under syncML node
 * (DevInfo - DevDetail - Ext)
 *
 * @param n: the 'syncml' node (parent node)
 */
void DMTClientConfig::saveDeviceConfig(ConfigurationNode* n, bool server) {

    if (!dmt) {
        LOG.error("%s: dmt nodes not initialized", __FUNCTION__);
        return;
    }

    ConfigurationNode* node;
    StringBuffer nodeName;
    char* fn = n->createFullName();

    //
    // DevInfo properties
    //
    nodeName.sprintf("%s%s", fn, CONTEXT_DEV_INFO);
    node = dmt->readManagementNode(nodeName.c_str());
    if (node) {
        saveDevInfoConfig(n, node, server);
        delete node;
        node = NULL;
    }

    //
    // DevDetail properties
    //
    nodeName.sprintf("%s%s", fn, CONTEXT_DEV_DETAIL);
    node = dmt->readManagementNode(nodeName.c_str());
    if (node) {
        saveDevDetailConfig(n, node, server);
        delete node;
        node = NULL;
    }

    //
    // DataStores properties (Server config only)
    // Rewrite the entire dataStore tree ONLY IF it changed (check the dirty flag).
    //
    if (server && serverConfig.isDirty(DIRTY_DATASTORES)) {
        // Cleanup ALL existing dataStore nodes.
        // The Server is expected to always send ALL dataStores supported.
        n->deletePropertyNode(PROPERTY_DATASTORES);

        // Save the new dataStores
        nodeName.sprintf("%s%s", fn, CONTEXT_DATASTORES);
        node = dmt->readManagementNode(nodeName.c_str());
        if (node) {
            LOG.debug("saving dataStores...");
            saveDataStoresConfig(node);
            LOG.debug("dataStores saved");
            delete node;
            node = NULL;
        }
        // reset the direty flag
        serverConfig.setDirty(0);
    }

    //
    // Ext properties (other misc props)
    // We set the 'Ext/lastSyncURL' property as the last one, so it will not be
    // stored if something went wrong during the save.
    //
    nodeName.sprintf("%s%s", fn, CONTEXT_EXT);
    node = dmt->readManagementNode(nodeName.c_str());
    if (node) {
        saveExtDevConfig(n, node, server);
        delete node;
        node = NULL;
    }
    
    delete [] fn;
}



/*
 * Read Source Config properties in DMTree for the desired Source.
 * Source properties are placed in specific node under sources node.
 *
 * @param i   : the index of SyncSource
 * @param n   : the sourceNode (parent node)
 */
bool DMTClientConfig::readSourceConfig(int i, ConfigurationNode* n) {

    ConfigurationNode* node = n->getChild(i);

    if (node) {
        if (!readSourceConfig(i, n, node) ||
            !readSourceVars(i, n, node)) {
            return false;
        }
        // *** TBD ***
        // CTCap c = getCtCap that is stored somewhere...
        //sourceConfigs[i].setCtCap(c);
    }
    return true;
}


/*
 * Save Source Config properties in DMTree for the desired Source.
 * Source properties are placed in specific node under sources node.
 * Note:
 * if the node for the current source is not found, it is created.
 *
 * @param i   : the index of SyncSource
 * @param n   : the sourceNode (parent node)
 */
void DMTClientConfig::saveSourceConfig(int i, ConfigurationNode* n) {
    
    if (!dmt) {
        LOG.error("%s: dmt nodes not initialized", __FUNCTION__);
        return;
    }

    if( strcmp( sourceConfigs[i].getName(), "mail" ) == 0 ){

        char* fn = n->createFullName();
        MailSourceManagementNode* msmn = new MailSourceManagementNode(fn, sourceConfigs[i].getName());
        MailSyncSourceConfig& mssc = ((MailSyncSourceConfig&)((sourceConfigs[i])));
        msmn->setMailSourceConfig(mssc);
        delete msmn;
        delete [] fn;

    }else{
        ConfigurationNode* node;
        StringBuffer nodeName;

        if (n->getChild(i) == NULL) {
            // Create node from Source name.
            char* fn = n->createFullName();
            nodeName.sprintf("%s/%s", fn, sourceConfigs[i].getName());
            delete [] fn;
            node = dmt->readManagementNode(nodeName.c_str());
        }
        else {
            node = (ConfigurationNode*)n->getChild(i)->clone();
        }

        if (node) {
            saveSourceConfig(i, n, node);
            saveSourceVars(i, n, node);

            // *** TBD ***
            // CTCap c = sourceConfigs[i].getCtCap();
            // saveCtCap() somewhere...

            delete node;
        }
    }
}

bool DMTClientConfig::readAuthConfig(ConfigurationNode* n,
                                     ConfigurationNode* authNode) {
    const char *tmp;
    
    tmp = authNode->readPropertyValue(PROPERTY_ACCESS_CONFIG_ENCRYPTION_MODE);
    if (tmp == NULL || strlen(tmp) == 0) {
        accessConfig.setEncryptionMode(AccessConfig::NOT_ENCRYPTED);
    } else {
        accessConfig.setEncryptionMode((AccessConfig::EncryptionMode)atoi(tmp));
    }
    delete [] tmp;
    
    tmp = authNode->readPropertyValue(PROPERTY_AUTH_TYPE);
    setAuthenticationType(tmp);
    delete [] tmp;

    tmp = authNode->readPropertyValue(PROPERTY_USERNAME);
    if (accessConfig.getEncryptionMode() == AccessConfig::ENCRYPTED) {
        std::string clearUsername = decodeAfterReading(tmp);
        accessConfig.setUsername(clearUsername.c_str());
    } else {
        accessConfig.setUsername(tmp);
    }
    delete [] tmp;

    tmp = authNode->readPropertyValue(PROPERTY_PASSWORD);
    if (accessConfig.getEncryptionMode() == AccessConfig::ENCRYPTED) {
        std::string clearPwd = decodeAfterReading(tmp);
        accessConfig.setPassword(clearPwd.c_str());
    } else {
        accessConfig.setPassword(tmp);
    }
    delete [] tmp;

    tmp = authNode->readPropertyValue(PROPERTY_SERVER_ID);
    accessConfig.setServerID(tmp);
    delete [] tmp;

    tmp = authNode->readPropertyValue(PROPERTY_SERVER_PWD);
    accessConfig.setServerPWD(tmp);
    delete [] tmp;

    tmp = authNode->readPropertyValue(PROPERTY_SERVER_NONCE);
    accessConfig.setServerNonce(tmp);
    delete [] tmp;

    tmp = authNode->readPropertyValue(PROPERTY_CLIENT_NONCE);
    accessConfig.setClientNonce(tmp);
    delete [] tmp;

    tmp = authNode->readPropertyValue(PROPERTY_CLIENT_AUTH_TYPE);
    accessConfig.setClientAuthType(tmp);
    delete [] tmp;

    tmp = authNode->readPropertyValue(PROPERTY_SERVER_AUTH_TYPE);
    accessConfig.setServerAuthType(tmp);
    delete [] tmp;

    tmp = authNode->readPropertyValue(PROPERTY_IS_SERVER_REQUIRED);
    accessConfig.setServerAuthRequired((*tmp == '1') ? true : false);
    delete [] tmp;
    
    std::string authenticationType = getAuthenticationType();
    
    if (authenticationType == AUTH_TYPE_OAUTH) {
        // OAuth2 nodes
        tmp = authNode->readPropertyValue(PROPERTY_OAUTH2_ACCESS_TOKEN);
        if (tmp && strlen(tmp) > 0) {   
            setOAuth2AccessToken(tmp);
        } 
        delete [] tmp;
        
        tmp = authNode->readPropertyValue(PROPERTY_OAUTH2_REFRESH_TOKEN);
        if (tmp && strlen(tmp) > 0) {   
            setOAuth2RefreshToken(tmp);
        } 
        delete [] tmp;

        tmp = authNode->readPropertyValue(PROPERTY_OAUTH2_EXPIRES_IN);
        if (tmp && strlen(tmp) > 0) {   
            setOAuth2ExpiresIn(tmp);
        } 
        delete [] tmp;
        
        tmp = authNode->readPropertyValue(PROPERTY_OAUTH2_CLIENT_TYPE);
        if (tmp && strlen(tmp) > 0) {   
            setOAuth2ClientType(tmp);
        }
        
        delete [] tmp;
        tmp = authNode->readPropertyValue(PROPERTY_OAUTH2_SCOPE);
        if (tmp && strlen(tmp) > 0) {   
            setOAuth2Scope(tmp);
        }
        delete [] tmp;
        
        tmp = authNode->readPropertyValue(PROPERTY_OAUTH2_ACCESS_TOKEN_SET_TIME);
        if (tmp && strlen(tmp) > 0) {   
            time_t setTime = strtod(tmp, NULL);
            setOAuth2AccessTokenSetTime(setTime);
        }
        delete [] tmp;
    }
    
    if (authenticationType == AUTH_TYPE_WASSUP) {
        tmp = authNode->readPropertyValue(PROPERTY_WASSUP_AUTH_TOKEN);
        if (accessConfig.getEncryptionMode() == AccessConfig::ENCRYPTED) {
            std::string authTokenDecoded = decodeAfterReading(tmp);
            accessConfig.setWassupAuthToken(authTokenDecoded.c_str());
        } else {
            accessConfig.setWassupAuthToken(tmp);
        }
        
        delete [] tmp;
    }
    
    // If credentials are save without encryption, then we force the node to be rewritten
    if (accessConfig.getEncryptionMode() == AccessConfig::NOT_ENCRYPTED) {
        saveAuthConfig(n, authNode);
    }

    return true;
}

void DMTClientConfig::saveAuthConfig(ConfigurationNode* /* syncMLNode */,
                                     ConfigurationNode* authNode) {
    
    // We always save the username encrypted
    std::string encodedUsername = encodeForSaving(accessConfig.getUsername());
    authNode->setPropertyValue(PROPERTY_USERNAME, encodedUsername.c_str());
    // We always save the password encrypted
    std::string encodedPwd = encodeForSaving(accessConfig.getPassword());
    authNode->setPropertyValue(PROPERTY_PASSWORD, encodedPwd.c_str());
    
    accessConfig.setEncryptionMode(AccessConfig::ENCRYPTED);
    authNode->setPropertyValue(PROPERTY_ACCESS_CONFIG_ENCRYPTION_MODE, "1");
    
    std::string authenticationType = getAuthenticationType();
    authNode->setPropertyValue(PROPERTY_AUTH_TYPE, authenticationType.c_str());
    authNode->setPropertyValue(PROPERTY_SERVER_ID, accessConfig.getServerID());
    authNode->setPropertyValue(PROPERTY_SERVER_PWD, accessConfig.getServerPWD());
    authNode->setPropertyValue(PROPERTY_SERVER_NONCE, accessConfig.getServerNonce());
    authNode->setPropertyValue(PROPERTY_CLIENT_NONCE, accessConfig.getClientNonce());
    authNode->setPropertyValue(PROPERTY_CLIENT_AUTH_TYPE, accessConfig.getClientAuthType());
    authNode->setPropertyValue(PROPERTY_SERVER_AUTH_TYPE, accessConfig.getServerAuthType());
    authNode->setPropertyValue(PROPERTY_IS_SERVER_REQUIRED,
                              (accessConfig.getServerAuthRequired() ? "1" : "0" ) );
    
    // OAuth2
    if (authenticationType == AUTH_TYPE_OAUTH) {
        authNode->setPropertyValue(PROPERTY_OAUTH2_ACCESS_TOKEN, getOAuth2AccessToken());
        authNode->setPropertyValue(PROPERTY_OAUTH2_REFRESH_TOKEN, getOAuth2RefreshToken());
        authNode->setPropertyValue(PROPERTY_OAUTH2_EXPIRES_IN, getOAuth2ExpiresIn());
        authNode->setPropertyValue(PROPERTY_OAUTH2_CLIENT_TYPE, getOAuth2ClientType());
        authNode->setPropertyValue(PROPERTY_OAUTH2_SCOPE, getOAuth2Scope());
     
        StringBuffer buf;
        buf.sprintf("%lu", getOAuth2AccessTokenSetTime());
        authNode->setPropertyValue(PROPERTY_OAUTH2_ACCESS_TOKEN_SET_TIME, buf.c_str());
    }
    
    if (authenticationType == AUTH_TYPE_WASSUP) {
        std::string encodedWassupAuthToken = encodeForSaving(accessConfig.getWassupAuthToken());
        authNode->setPropertyValue(PROPERTY_WASSUP_AUTH_TOKEN, encodedWassupAuthToken.c_str());
    }
}

std::string DMTClientConfig::encodeForSaving(const char* userData) {
    return Funambol::encodeForSaving(userData, accessConfigUserToken.c_str());
}

std::string DMTClientConfig::decodeAfterReading(const char* userData) {
    return Funambol::decodeAfterReading(userData, accessConfigUserToken.c_str());
}

bool DMTClientConfig::readConnConfig(ConfigurationNode* /* syncMLNode */,
                                     ConfigurationNode* connNode) {
    char* tmp;

    tmp = connNode->readPropertyValue(PROPERTY_SYNC_URL);
    accessConfig.setSyncURL(tmp);
    delete [] tmp;

    tmp = connNode->readPropertyValue(PROPERTY_USE_PROXY);
    if (tmp) {
        if ((*tmp == '1') || (strcmp(tmp, "true") == 0)) {
            accessConfig.setUseProxy(true);
        } else {
            accessConfig.setUseProxy(false);
        }
    } else {
        accessConfig.setUseProxy(false);
    }
    // accessConfig.setUseProxy(((*tmp == '1') || (tmp == '1')) ? true : false);
    delete [] tmp;

    tmp = connNode->readPropertyValue(PROPERTY_PROXY_HOST);
    accessConfig.setProxyHost(tmp);
    delete [] tmp;

    tmp = connNode->readPropertyValue(PROPERTY_PROXY_PORT);
    accessConfig.setProxyPort(strtol(tmp, NULL, 10));
    delete [] tmp;

    tmp = connNode->readPropertyValue(PROPERTY_PROXY_USERNAME);
    if (accessConfig.getEncryptionMode() == AccessConfig::ENCRYPTED) {
        std::string clearUsername = decodeAfterReading(tmp);
        accessConfig.setProxyUsername(clearUsername.c_str());
    } else {
        accessConfig.setProxyUsername(tmp);
    }
    delete [] tmp;

    tmp = connNode->readPropertyValue(PROPERTY_PROXY_PASSWORD);
    if (accessConfig.getEncryptionMode() == AccessConfig::ENCRYPTED) {
        std::string clearPwd = decodeAfterReading(tmp);
        accessConfig.setProxyPassword(clearPwd.c_str());
    } else {
        accessConfig.setProxyPassword(tmp);
    }
    delete [] tmp;


    /*tmp = connNode.readPropertyValue(PROPERTY_PROXY_USERNAME);
    accessConfig.setProxyUsername(tmp);
    delete [] tmp;

    tmp = connNode.readPropertyValue(PROPERTY_PROXY_PASSWORD);
    accessConfig.setProxyPassword(tmp);
    delete [] tmp;*/

    tmp = connNode->readPropertyValue(PROPERTY_CHECK_CONN);
    accessConfig.setCheckConn((*tmp == '1') ? true : false);
    delete [] tmp;

    tmp = connNode->readPropertyValue(PROPERTY_RESPONSE_TIMEOUT);
    accessConfig.setResponseTimeout(strtol(tmp, NULL, 10));
    delete [] tmp;

    tmp = connNode->readPropertyValue(PROPERTY_READ_BUFFER_SIZE);
    accessConfig.setReadBufferSize(strtol(tmp, NULL, 10));
    delete [] tmp;

    tmp = connNode->readPropertyValue(PROPERTY_USER_AGENT);
    accessConfig.setUserAgent(tmp);
    delete [] tmp;

    tmp = connNode->readPropertyValue(PROPERTY_ENABLE_COMPRESSION);
    accessConfig.setCompression((strcmp(tmp,  "1")==0) ? true : false);
    delete [] tmp;

    return true;
}

void DMTClientConfig::saveConnConfig(ConfigurationNode* /* syncMLNode */,
                                     ConfigurationNode* connNode) {
    char buf[512];

    connNode->setPropertyValue(PROPERTY_SYNC_URL, accessConfig.getSyncURL());
    connNode->setPropertyValue(PROPERTY_USE_PROXY,
                              (accessConfig.getUseProxy() ? "1": "0") );
    connNode->setPropertyValue(PROPERTY_PROXY_HOST, accessConfig.getProxyHost());
    sprintf(buf, "%d", accessConfig.getProxyPort());
    connNode->setPropertyValue(PROPERTY_PROXY_PORT, buf);

    // We always save the username encrypted
    std::string encodedUsername = encodeForSaving(accessConfig.getProxyUsername());
    connNode->setPropertyValue(PROPERTY_PROXY_USERNAME, encodedUsername.c_str());
    // We always save the password encrypted
    std::string encodedPwd = encodeForSaving(accessConfig.getProxyPassword());
    connNode->setPropertyValue(PROPERTY_PROXY_PASSWORD, encodedPwd.c_str());
    
    //accessConfig.setEncryptionMode(AccessConfig::ENCRYPTED);

    //connNode.setPropertyValue(PROPERTY_PROXY_USERNAME, accessConfig.getProxyUsername());
    //connNode.setPropertyValue(PROPERTY_PROXY_PASSWORD, accessConfig.getProxyPassword());
    connNode->setPropertyValue(PROPERTY_CHECK_CONN,
                              (accessConfig.getCheckConn() ? "1": "0") );
    sprintf(buf, "%u", accessConfig.getResponseTimeout());
    connNode->setPropertyValue(PROPERTY_RESPONSE_TIMEOUT, buf);
    sprintf(buf, "%lu", accessConfig.getReadBufferSize());
    connNode->setPropertyValue(PROPERTY_READ_BUFFER_SIZE, buf);
    connNode->setPropertyValue(PROPERTY_USER_AGENT, accessConfig.getUserAgent());
    connNode->setPropertyValue(PROPERTY_ENABLE_COMPRESSION, accessConfig.getCompression() ? "1": "0");
}

bool DMTClientConfig::readExtAccessConfig(ConfigurationNode* /* syncMLNode */,
                                          ConfigurationNode* extNode) {
    char* tmp;

    tmp = extNode->readPropertyValue(PROPERTY_FIRST_TIME_SYNC_MODE);
    SyncMode i = (SyncMode)(*tmp ? strtol(tmp, NULL, 10) : 0);
    accessConfig.setFirstTimeSyncMode(i);
    delete [] tmp;

    tmp = extNode->readPropertyValue(PROPERTY_MAX_MSG_SIZE);
    accessConfig.setMaxMsgSize(strtol(tmp, NULL, 10));
    delete [] tmp;

    tmp = extNode->readPropertyValue(PROPERTY_SYNC_BEGIN);
    accessConfig.setBeginSync(strtol(tmp, NULL, 10));
    delete [] tmp;

    tmp = extNode->readPropertyValue(PROPERTY_SYNC_END);
    accessConfig.setEndSync(strtol(tmp, NULL, 10));
    delete [] tmp;

    return true;
}

void DMTClientConfig::saveExtAccessConfig(ConfigurationNode* /* syncMLNode */,
                                          ConfigurationNode* extNode) {
    char buf[512];

    sprintf(buf, "%u", (unsigned)accessConfig.getFirstTimeSyncMode());
    extNode->setPropertyValue(PROPERTY_FIRST_TIME_SYNC_MODE, buf);

    sprintf(buf, "%lu", accessConfig.getMaxMsgSize());
    extNode->setPropertyValue(PROPERTY_MAX_MSG_SIZE, buf);

    timestampToAnchor(accessConfig.getBeginSync(), buf);
    extNode->setPropertyValue(PROPERTY_SYNC_BEGIN, buf);

    timestampToAnchor(accessConfig.getEndSync(), buf);
    extNode->setPropertyValue(PROPERTY_SYNC_END, buf);

}

bool DMTClientConfig::readDevInfoConfig(ConfigurationNode* /* syncMLNode */,
                                        ConfigurationNode* devInfoNode, bool server) {
    char* tmp;

    tmp = devInfoNode->readPropertyValue(PROPERTY_DEVICE_ID);
    if(server){
        serverConfig.setDevID(tmp);
    }else{
        clientConfig.setDevID(tmp);
    }
    delete [] tmp;

    tmp = devInfoNode->readPropertyValue(PROPERTY_MANUFACTURER);
    if(server){
        serverConfig.setMan(tmp);
    }else{
        clientConfig.setMan(tmp);
    }
    delete [] tmp;

    tmp = devInfoNode->readPropertyValue(PROPERTY_MODEL);
    if(server){
        serverConfig.setMod(tmp);
    }else{
        clientConfig.setMod(tmp);
    }
    delete [] tmp;

    tmp = devInfoNode->readPropertyValue(PROPERTY_DS_VERSION);
    clientConfig.setDsV(tmp);
    delete [] tmp;

    return true;
}

void DMTClientConfig::saveDevInfoConfig(ConfigurationNode* /* syncMLNode */,
                                        ConfigurationNode* devInfoNode, bool server) {
    if (!server){
        devInfoNode->setPropertyValue(PROPERTY_DEVICE_ID, clientConfig.getDevID());
        devInfoNode->setPropertyValue(PROPERTY_MANUFACTURER, clientConfig.getMan());
        devInfoNode->setPropertyValue(PROPERTY_MODEL, clientConfig.getMod());
        devInfoNode->setPropertyValue(PROPERTY_DS_VERSION, clientConfig.getDsV());
    } else{
        devInfoNode->setPropertyValue(PROPERTY_DEVICE_ID, serverConfig.getDevID());
        devInfoNode->setPropertyValue(PROPERTY_MANUFACTURER, serverConfig.getMan());
        devInfoNode->setPropertyValue(PROPERTY_MODEL, serverConfig.getMod());
        devInfoNode->setPropertyValue(PROPERTY_DS_VERSION, serverConfig.getDsV());
    }
}

bool DMTClientConfig::readDevDetailConfig(ConfigurationNode* /* syncMLNode */,
                                          ConfigurationNode* devDetailNode, bool server) {
    char* tmp;

    tmp = devDetailNode->readPropertyValue(PROPERTY_DEVICE_TYPE);
	if(server){
	    serverConfig.setDevType(tmp);
	}else{
		clientConfig.setDevType(tmp);
	}
    delete [] tmp;

    tmp = devDetailNode->readPropertyValue(PROPERTY_OEM);
	if(server){
	    serverConfig.setOem(tmp);
	}else{
		clientConfig.setOem(tmp);
	}
    delete [] tmp;

    tmp = devDetailNode->readPropertyValue(PROPERTY_FIRMWARE_VERSION);
    if(server){
	    serverConfig.setFwv(tmp);
	}else{
		clientConfig.setFwv(tmp);
	}
    delete [] tmp;

    tmp = devDetailNode->readPropertyValue(PROPERTY_SOFTWARE_VERSION);
    if(server){
	    serverConfig.setSwv(tmp);
	}else{
		clientConfig.setSwv(tmp);
	}
	delete [] tmp;

    tmp = devDetailNode->readPropertyValue(PROPERTY_HARDWARE_VERSION);
    if(server){
	    serverConfig.setHwv(tmp);
	}else{
		clientConfig.setHwv(tmp);
	}    
    delete [] tmp;

    tmp = devDetailNode->readPropertyValue(PROPERTY_LARGE_OBJECT_SUPPORT);
	if(server){
		serverConfig.setLoSupport((*tmp == '1') ? true : false);
	}else{
		clientConfig.setLoSupport((*tmp == '1') ? true : false);
	}
    delete [] tmp;

    return true;
}

void DMTClientConfig::saveDevDetailConfig(ConfigurationNode* /* syncMLNode */,
                                          ConfigurationNode* devDetailNode, bool server) {
                                              
    if(server){
        devDetailNode->setPropertyValue(PROPERTY_DEVICE_TYPE, serverConfig.getDevType());
        devDetailNode->setPropertyValue(PROPERTY_OEM, serverConfig.getOem());
        devDetailNode->setPropertyValue(PROPERTY_FIRMWARE_VERSION, serverConfig.getFwv());
        devDetailNode->setPropertyValue(PROPERTY_SOFTWARE_VERSION, serverConfig.getSwv());
        devDetailNode->setPropertyValue(PROPERTY_HARDWARE_VERSION, serverConfig.getHwv());
        devDetailNode->setPropertyValue(PROPERTY_LARGE_OBJECT_SUPPORT,
                                   (serverConfig.getLoSupport() ? "1": "0") );

    }else{
        devDetailNode->setPropertyValue(PROPERTY_DEVICE_TYPE, clientConfig.getDevType());
        devDetailNode->setPropertyValue(PROPERTY_OEM, clientConfig.getOem());
        devDetailNode->setPropertyValue(PROPERTY_FIRMWARE_VERSION, clientConfig.getFwv());
        devDetailNode->setPropertyValue(PROPERTY_SOFTWARE_VERSION, clientConfig.getSwv());
        devDetailNode->setPropertyValue(PROPERTY_HARDWARE_VERSION, clientConfig.getHwv());
        devDetailNode->setPropertyValue(PROPERTY_LARGE_OBJECT_SUPPORT,
                                   (clientConfig.getLoSupport() ? "1": "0") );
    }
}

bool DMTClientConfig::readExtDevConfig(ConfigurationNode* /* syncMLNode */,
                                       ConfigurationNode* extNode, bool server) {
    char* tmp;

    if(server){

        tmp = extNode->readPropertyValue(PROPERTY_SMART_SLOW_SYNC);
		if(strcmp(tmp,"")==0){
			serverConfig.setSmartSlowSync(2);
		}else if(strcmp(tmp,"0")==0){
			serverConfig.setSmartSlowSync(0);
		}else if(strcmp(tmp,"1")==0){
			serverConfig.setSmartSlowSync(1);
		}else if(strcmp(tmp,"2")==0){
			serverConfig.setSmartSlowSync(2);
		}
        delete [] tmp;

        tmp = extNode->readPropertyValue(PROPERTY_MULTIPLE_EMAIL_ACCOUNT);
		if(strcmp(tmp,"")==0){
			serverConfig.setMultipleEmailAccount(2);
		}else if(strcmp(tmp,"0")==0){
			serverConfig.setMultipleEmailAccount(0);
		}else if(strcmp(tmp,"1")==0){
			serverConfig.setMultipleEmailAccount(1);
		}else if(strcmp(tmp,"2")==0){
			serverConfig.setMultipleEmailAccount(2);
		}
        delete [] tmp;

		tmp = extNode->readPropertyValue(PROPERTY_MEDIA_HTTP_UPLOAD);
        serverConfig.setMediaHttpUpload((*tmp == '1') ? true : false);
        delete [] tmp;
        
        tmp = extNode->readPropertyValue(PROPERTY_NO_FIELD_LEVEL_REPLACE);
        serverConfig.setNoFieldLevelReplace(tmp);
        delete [] tmp;

        tmp = extNode->readPropertyValue(PROPERTY_UTC);
        serverConfig.setUtc((*tmp == '1') ? true : false);
        delete [] tmp;
	    tmp = extNode->readPropertyValue(PROPERTY_NUMBER_OF_CHANGES_SUPPORT);
        serverConfig.setNocSupport((*tmp == '1') ? true : false);
        delete [] tmp;
		tmp = extNode->readPropertyValue(PROPERTY_VER_DTD);
        serverConfig.setVerDTD(tmp);
        delete [] tmp;
        tmp = extNode->readPropertyValue(PROPERTY_SERVER_LAST_SYNC_URL);
        serverConfig.setServerLastSyncURL(tmp);
        delete [] tmp;
     
    }else{
        tmp = extNode->readPropertyValue(PROPERTY_UTC);
        clientConfig.setUtc((*tmp == '1') ? true : false);
        delete [] tmp;

        tmp = extNode->readPropertyValue(PROPERTY_NUMBER_OF_CHANGES_SUPPORT);
        clientConfig.setNocSupport((*tmp == '1') ? true : false);
        delete [] tmp;

        tmp = extNode->readPropertyValue(PROPERTY_LOG_LEVEL);
        LogLevel l = (LogLevel)strtol(tmp, NULL, 10);
        clientConfig.setLogLevel(l);
        delete [] tmp;

        tmp = extNode->readPropertyValue(PROPERTY_MAX_OBJ_SIZE);
        clientConfig.setMaxObjSize(strtol(tmp, NULL, 10));
        delete [] tmp;

        tmp = extNode->readPropertyValue(PROPERTY_DEVINF_HASH);
        clientConfig.setDevInfHash(tmp);
        delete [] tmp;

        tmp = extNode->readPropertyValue(PROPERTY_SEND_CLIENT_DEVINF);
        clientConfig.setSendDevInfo((*tmp == '0') ? false : true);          // So if different the default is true (send client devinf)
        delete [] tmp;

		// managing auto-sync param (reading the value from config) (V.10+)
		tmp = extNode->readPropertyValue(PROPERTY_AUTO_SYNC);
        clientConfig.setAutoSync((*tmp == '1') ? true : false);
        delete [] tmp;

		// expiration date param (V.10+)
		tmp = extNode->readPropertyValue(PROPERTY_DATAPLAN_EXPIRATION_DATE);
		unsigned long lex = (unsigned long) atol(tmp);
		clientConfig.setDataplanExpirationDate(lex);
        delete [] tmp;

        // push type (V.11)
        lex = 0;
        tmp = extNode->readPropertyValue(PROPERTY_PUSH_TYPE);
		lex = (unsigned long) atol(tmp);
		clientConfig.setPushType(lex);
        delete [] tmp;


		// managing networkWarning param (reading the value from config) (V.10+)
		tmp = extNode->readPropertyValue(PROPERTY_NETWORK_WARNING);
        clientConfig.setNetworkWarning((*tmp == '1') ? true : false);
        delete [] tmp;


        // forceServerDevInfo is not stored: default is false and it's intended to
        // be set to true by the Client everytime we need to force it.
    }

    return true;
}

void DMTClientConfig::saveExtDevConfig(ConfigurationNode* /* syncMLNode */,
                                       ConfigurationNode* extNode, bool server) {
    char buf[512];
    if(server){
		switch (serverConfig.getSmartSlowSync()){
			case 0:
			extNode->setPropertyValue(PROPERTY_SMART_SLOW_SYNC, "0");
			break;
			case 1:
			extNode->setPropertyValue(PROPERTY_SMART_SLOW_SYNC, "1");
			break;
			case 2:
			extNode->setPropertyValue(PROPERTY_SMART_SLOW_SYNC, "2");
			break;
		}

        switch (serverConfig.getMultipleEmailAccount()){
			case 0:
			extNode->setPropertyValue(PROPERTY_MULTIPLE_EMAIL_ACCOUNT, "0");
			break;
			case 1:
			extNode->setPropertyValue(PROPERTY_MULTIPLE_EMAIL_ACCOUNT, "1");
			break;
			case 2:
			extNode->setPropertyValue(PROPERTY_MULTIPLE_EMAIL_ACCOUNT, "2");
			break;
		}

        extNode->setPropertyValue(PROPERTY_MEDIA_HTTP_UPLOAD, serverConfig.getMediaHttpUpload() ? "1": "0" );
        extNode->setPropertyValue(PROPERTY_NO_FIELD_LEVEL_REPLACE, serverConfig.getNoFieldLevelReplace());
        

		//extNode.setPropertyValue(PROPERTY_SMART_SLOW_SYNC, itow(  ));
		extNode->setPropertyValue(PROPERTY_UTC,
                             (serverConfig.getUtc() ? "1": "0") );
        extNode->setPropertyValue(PROPERTY_NUMBER_OF_CHANGES_SUPPORT,
                             (serverConfig.getNocSupport() ? "1": "0") );
		extNode->setPropertyValue(PROPERTY_VER_DTD, serverConfig.getVerDTD());
        extNode->setPropertyValue(PROPERTY_SERVER_LAST_SYNC_URL,
                              serverConfig.getServerLastSyncURL());
		
    }else{
    
        extNode->setPropertyValue(PROPERTY_DEVINF_HASH, clientConfig.getDevInfHash());
        extNode->setPropertyValue(PROPERTY_UTC,
                             (clientConfig.getUtc() ? "1": "0") );
        extNode->setPropertyValue(PROPERTY_NUMBER_OF_CHANGES_SUPPORT,
                             (clientConfig.getNocSupport() ? "1": "0") );

        sprintf(buf, "%d", clientConfig.getLogLevel());
        extNode->setPropertyValue(PROPERTY_LOG_LEVEL, buf);

        sprintf(buf, "%u", clientConfig.getMaxObjSize());
        extNode->setPropertyValue(PROPERTY_MAX_OBJ_SIZE, buf);

        extNode->setPropertyValue(PROPERTY_SEND_CLIENT_DEVINF,      (clientConfig.getSendDevInfo() ? "1": "0") );



		// saving auto-sync  parameters (V.10+)
		extNode->setPropertyValue(PROPERTY_AUTO_SYNC,
			(clientConfig.getAutoSync() ? "1": "0") );

		// saving dataplan expiration date timestamp
		sprintf(buf, "%lu", clientConfig.getDataplanExpirationDate());
        extNode->setPropertyValue(PROPERTY_DATAPLAN_EXPIRATION_DATE, buf);

        // saving pushType mode
		sprintf(buf, "%lu", clientConfig.getPushType());
        extNode->setPropertyValue(PROPERTY_PUSH_TYPE, buf);


		// saving networkWarning param (V.10+)
		extNode->setPropertyValue(PROPERTY_NETWORK_WARNING,
			(clientConfig.getNetworkWarning() ? "1": "0") );

        // forceServerDevInfo is not stored: default is false and it's intended to
        // be set to true by the Client everytime we need to force it.
    }
}

bool DMTClientConfig::readSourceVars(int i,
                                     ConfigurationNode* /* sourcesNode */,
                                     ConfigurationNode* sourceNode) {
    char* tmp;

    tmp = sourceNode->readPropertyValue(PROPERTY_SOURCE_LAST_SYNC);
    sourceConfigs[i].setLast( ((*tmp) ? strtoul(tmp, NULL, 10) : 0) );
    delete [] tmp;

    return true;
}

void DMTClientConfig::saveSourceVars(int i,
                                     ConfigurationNode* /* sourcesNode */,
                                     ConfigurationNode* sourceNode) {
    char buf[512];

    timestampToAnchor(sourceConfigs[i].getLast(), buf);
    sourceNode->setPropertyValue(PROPERTY_SOURCE_LAST_SYNC, buf);
}

bool DMTClientConfig::readSourceConfig(int i,
                                       ConfigurationNode* /* sourcesNode */,
                                       ConfigurationNode* sourceNode) {
    char* tmp;

    tmp = sourceNode->readPropertyValue(PROPERTY_SOURCE_NAME);
    sourceConfigs[i].setName(tmp);
    delete [] tmp;

    tmp = sourceNode->readPropertyValue(PROPERTY_SOURCE_URI);
    sourceConfigs[i].setURI(tmp);
    delete [] tmp;

    tmp = sourceNode->readPropertyValue(PROPERTY_SOURCE_SYNC_MODES);
    sourceConfigs[i].setSyncModes(tmp);
    delete [] tmp;

    tmp = sourceNode->readPropertyValue(PROPERTY_SOURCE_SYNC);
    sourceConfigs[i].setSync(tmp);
    delete [] tmp;

    tmp = sourceNode->readPropertyValue(PROPERTY_SOURCE_TYPE);
    sourceConfigs[i].setType(tmp);
    delete [] tmp;

    tmp = sourceNode->readPropertyValue(PROPERTY_SOURCE_VERSION);
    sourceConfigs[i].setVersion(tmp);
    delete [] tmp;

    tmp = sourceNode->readPropertyValue(PROPERTY_SOURCE_ENCODING);
    sourceConfigs[i].setEncoding(tmp);
    delete [] tmp;

    tmp = sourceNode->readPropertyValue(PROPERTY_SOURCE_ENCRYPTION);
    sourceConfigs[i].setEncryption(tmp);
    delete [] tmp;

    tmp = sourceNode->readPropertyValue(PROPERTY_SOURCE_SUPP_TYPES);
    sourceConfigs[i].setSupportedTypes(tmp);
    delete [] tmp;
    
    tmp = sourceNode->readPropertyValue(PROPERTY_SOURCE_SYNC_MODE);
    if (strcmp(tmp, PROPERTY_SYNC_MODE_DISABLED) == 0) {
        sourceConfigs[i].setSyncMode(SYNC_MODE_DISABLED);
    } else if (strcmp(tmp, PROPERTY_SYNC_MODE_MANUAL) == 0) {
        sourceConfigs[i].setSyncMode(SYNC_MODE_MANUAL);
    } else {
        sourceConfigs[i].setSyncMode(SYNC_MODE_AUTO);
    }
    delete [] tmp;

	tmp = sourceNode->readPropertyValue(PROPERTY_SOURCE_ALLOWED);
    sourceConfigs[i].setIsAllowed(strcmp(tmp, "0")? true:false);    // Set true if any value different from "0" (also if empty)
    delete [] tmp;
	
    tmp = sourceNode->readPropertyValue(PROPERTY_SOURCE_LAST_ERROR);
    int lastError = 0;
    if (tmp && strlen(tmp)>0) {
        lastError = atoi(tmp);
    }
    sourceConfigs[i].setLastSourceError(lastError);
    delete [] tmp;
    
    tmp = sourceNode->readPropertyValue(PROPERTY_LAST_SYNC_SERVER_TIME);
    if (tmp) {
        sourceConfigs[i].setLastSyncServerTime(atol(tmp));
    } else {
        sourceConfigs[i].setLastSyncServerTime(0);
    }
    delete [] tmp;
    
    StringMap* stringMap = sourceNode->readAllProperties();
    KeyValuePair kvp = stringMap->front(); 
    while (kvp.null() == false) {
        
        if (kvp.getKey() != PROPERTY_SOURCE_NAME && kvp.getKey() != PROPERTY_SOURCE_URI && 
            kvp.getKey() != PROPERTY_SOURCE_TYPE && kvp.getKey() != PROPERTY_SOURCE_VERSION &&
            kvp.getKey() != PROPERTY_SOURCE_SYNC_MODES && kvp.getKey() != PROPERTY_SOURCE_SYNC &&
            kvp.getKey() != PROPERTY_SOURCE_ENCODING && kvp.getKey() != PROPERTY_SOURCE_SUPP_TYPES &&
            kvp.getKey() != PROPERTY_SOURCE_SYNC_MODE && kvp.getKey() != PROPERTY_SOURCE_ENCRYPTION &&
			kvp.getKey() != PROPERTY_SOURCE_ALLOWED &&
            kvp.getKey() != PROPERTY_SOURCE_LAST_ERROR && kvp.getKey() != PROPERTY_SOURCE_LAST_SYNC &&
            kvp.getKey() != PROPERTY_DUMMY_KEY) {
            
            sourceConfigs[i].setProperty(kvp.getKey().c_str(), kvp.getValue().c_str());
            
        }
        kvp = stringMap->next();
    }
    delete stringMap;
    
    return true;
}

void DMTClientConfig::saveSourceConfig(int i,
                                       ConfigurationNode* /* sourcesNode */,
                                       ConfigurationNode* sourceNode) {
    sourceNode->setPropertyValue(PROPERTY_SOURCE_NAME,       sourceConfigs[i].getName());
    sourceNode->setPropertyValue(PROPERTY_SOURCE_URI,        sourceConfigs[i].getURI());
    sourceNode->setPropertyValue(PROPERTY_SOURCE_TYPE,       sourceConfigs[i].getType());
    sourceNode->setPropertyValue(PROPERTY_SOURCE_VERSION,    sourceConfigs[i].getVersion());
    sourceNode->setPropertyValue(PROPERTY_SOURCE_SYNC_MODES, sourceConfigs[i].getSyncModes());
    sourceNode->setPropertyValue(PROPERTY_SOURCE_SYNC,       sourceConfigs[i].getSync());
    sourceNode->setPropertyValue(PROPERTY_SOURCE_ENCODING,   sourceConfigs[i].getEncoding());
    sourceNode->setPropertyValue(PROPERTY_SOURCE_SUPP_TYPES, sourceConfigs[i].getSupportedTypes());
    if (sourceConfigs[i].getSyncMode() == SYNC_MODE_DISABLED) {
        sourceNode->setPropertyValue(PROPERTY_SOURCE_SYNC_MODE, PROPERTY_SYNC_MODE_DISABLED);
    } else if (sourceConfigs[i].getSyncMode() == SYNC_MODE_MANUAL) {
        sourceNode->setPropertyValue(PROPERTY_SOURCE_SYNC_MODE, PROPERTY_SYNC_MODE_MANUAL);
    } else {
        sourceNode->setPropertyValue(PROPERTY_SOURCE_SYNC_MODE, PROPERTY_SYNC_MODE_AUTO);
    }
	sourceNode->setPropertyValue(PROPERTY_SOURCE_ALLOWED,    sourceConfigs[i].isAllowed()? "1":"0");
    sourceNode->setPropertyValue(PROPERTY_SOURCE_ENCRYPTION, sourceConfigs[i].getEncryption());

    StringBuffer buf;
    buf.sprintf("%i", sourceConfigs[i].getLastSourceError());
    sourceNode->setPropertyValue(PROPERTY_SOURCE_LAST_ERROR, buf.c_str());
    
    buf.reset();
    buf.sprintf("%lu", sourceConfigs[i].getLastSyncServerTime());
    sourceNode->setPropertyValue(PROPERTY_LAST_SYNC_SERVER_TIME, buf.c_str());
    
    StringMap& stringMap = const_cast<StringMap &>(sourceConfigs[i].getExtraProps());
    KeyValuePair kvp = stringMap.front(); 
    
    while (kvp.null() == false) {
        
        if (kvp.getKey() != PROPERTY_SOURCE_NAME && kvp.getKey() != PROPERTY_SOURCE_URI && 
            kvp.getKey() != PROPERTY_SOURCE_TYPE && kvp.getKey() != PROPERTY_SOURCE_VERSION &&
            kvp.getKey() != PROPERTY_SOURCE_SYNC_MODES && kvp.getKey() != PROPERTY_SOURCE_SYNC &&
            kvp.getKey() != PROPERTY_SOURCE_ENCODING && kvp.getKey() != PROPERTY_SOURCE_SUPP_TYPES &&
            kvp.getKey() != PROPERTY_SOURCE_SYNC_MODE && kvp.getKey() != PROPERTY_SOURCE_ENCRYPTION &&
			kvp.getKey() != PROPERTY_SOURCE_ALLOWED &&
            kvp.getKey() != PROPERTY_SOURCE_LAST_ERROR && kvp.getKey() != PROPERTY_SOURCE_LAST_SYNC &&
            kvp.getKey() != PROPERTY_DUMMY_KEY &&
            kvp.getKey() != PROPERTY_LAST_SYNC_SERVER_TIME) {
            
                sourceNode->setPropertyValue(kvp.getKey().c_str(), kvp.getValue().c_str());
            
        }
        kvp = stringMap.next();
    }

}


// SERVER CONFIG ONLY!
bool DMTClientConfig::readDataStoresConfig(ConfigurationNode* dataStoresNode) {

    // Just reset current dataStores array (doesn't set the dirty flag).
    serverConfig.resetDataStores();
    
    if (!dmt) {
        LOG.error("%s: dmt nodes not initialized", __FUNCTION__);
        return false;
    }

    int numDataStores = dataStoresNode->getChildrenCount();
    if (numDataStores == 0) {
        return true;
    }

    char* fn = dataStoresNode->createFullName();
    StringBuffer fullName(fn);
    delete [] fn;

    //
    // Read all datastores from 'DataStores' node
    //
    char** dataStoreNames = dataStoresNode->getChildrenNames();
    for (int i=0; i<numDataStores; i++) {

        ConfigurationNode* node = dmt->readManagementNode(fullName.c_str(), dataStoreNames[i]);
        if (node) {
            DataStore* dataStore = readDataStoreConfig(node);
            if (dataStore) {
                serverConfig.addDataStore(dataStore);
                delete dataStore;
            }
            delete node;
        }
    }

    for (int i=0; i<numDataStores; i++) {
        delete [] dataStoreNames[i];
    }
    delete [] dataStoreNames;
    return true;
}


// SERVER CONFIG ONLY!
DataStore* DMTClientConfig::readDataStoreConfig(ConfigurationNode* dataStoreNode) {

    char* tmp = NULL;
    DataStore* dataStore = new DataStore();

    tmp = dataStoreNode->readPropertyValue(PROPERTY_SOURCE_REF);
    SourceRef sourceRef(tmp);
    dataStore->setSourceRef(&sourceRef);
    delete [] tmp;

    tmp = dataStoreNode->readPropertyValue(PROPERTY_DISPLAY_NAME);
    dataStore->setDisplayName(tmp);
    delete [] tmp;

    tmp = dataStoreNode->readPropertyValue(PROPERTY_MAX_GUID_SIZE);
    dataStore->setMaxGUIDSize(strtol(tmp, NULL, 10));
    delete [] tmp;

    // rx-Pref is set only if type not empty
    tmp = dataStoreNode->readPropertyValue(PROPERTY_RX_PREF_TYPE);
    if (tmp && strlen(tmp)>0) {
        char* version = dataStoreNode->readPropertyValue(PROPERTY_RX_PREF_VERSION);
        ContentTypeInfo rxPref(tmp, version);
        dataStore->setRxPref(&rxPref);
        delete [] version;
        delete [] tmp;
    }

    // tx-Pref is set only if type not empty
    tmp = dataStoreNode->readPropertyValue(PROPERTY_TX_PREF_TYPE);
    if (tmp && strlen(tmp)>0) {
        char* version = dataStoreNode->readPropertyValue(PROPERTY_TX_PREF_VERSION);
        ContentTypeInfo txPref(tmp, version);
        dataStore->setTxPref(&txPref);
        delete [] version;
        delete [] tmp;
    }

    ArrayList* syncModeList = NULL;
    tmp = dataStoreNode->readPropertyValue(PROPERTY_SOURCE_SYNC_MODES);
    if (tmp && strlen(tmp)>0) {
        syncModeList = syncModesStringToList(tmp);
        SyncCap syncCap(syncModeList);
        dataStore->setSyncCap(&syncCap);
        delete syncModeList;
        delete [] tmp;
    }
    
    return dataStore;
}


// SERVER CONFIG ONLY!
void DMTClientConfig::saveDataStoresConfig(ConfigurationNode* dataStoresNode) {
    
    if (!dmt) {
        LOG.error("%s: dmt nodes not initialized", __FUNCTION__);
        return;
    }

    const ArrayList* dataStores = serverConfig.getDataStores();

    if (!dataStores || dataStores->size() == 0) {
        return;
    }

    char* fn = dataStoresNode->createFullName();
    StringBuffer fullName(fn);
    delete [] fn;
    
    //
    // Save all datastores
    //
    for (int i=0; i < dataStores->size(); i++) {
        DataStore* dataStore = (DataStore*)dataStores->get(i);
        if (dataStore) {
            // the 'sourceRef' param is mandatory
            StringBuffer sourceRef(dataStore->getSourceRef()->getValue());
            if (sourceRef.empty()) {
                continue;
            }

            ConfigurationNode* node = dmt->readManagementNode(fullName.c_str(), sourceRef.c_str());
            if (node) {
                saveDataStoreConfig(node, dataStore);
                delete node;
            }
        }
    }
}


// SERVER CONFIG ONLY!
void DMTClientConfig::saveDataStoreConfig(ConfigurationNode* dataStoreNode, DataStore* dataStore) {

    dataStoreNode->setPropertyValue(PROPERTY_SOURCE_REF,   dataStore->getSourceRef()->getValue());
    dataStoreNode->setPropertyValue(PROPERTY_DISPLAY_NAME, dataStore->getDisplayName());

    if (dataStore->getMaxGUIDSize()) {
        StringBuffer buf;
        buf.sprintf("%li", dataStore->getMaxGUIDSize());
        dataStoreNode->setPropertyValue(PROPERTY_MAX_GUID_SIZE, buf);
    }
    if (dataStore->getRxPref()) {
        dataStoreNode->setPropertyValue(PROPERTY_RX_PREF_TYPE,    dataStore->getRxPref()->getCTType());
        dataStoreNode->setPropertyValue(PROPERTY_RX_PREF_VERSION, dataStore->getRxPref()->getVerCT());
    }
    if (dataStore->getTxPref()) {
        dataStoreNode->setPropertyValue(PROPERTY_TX_PREF_TYPE,    dataStore->getTxPref()->getCTType());
        dataStoreNode->setPropertyValue(PROPERTY_TX_PREF_VERSION, dataStore->getTxPref()->getVerCT());
    }

    //
    // sync capabilities: 
    // from a list of numbers, extract a string like "two-way,one-way-server,..."
    //
    if (dataStore->getSyncCap()) {
        ArrayList* syncTypes = dataStore->getSyncCap()->getSyncType();
        StringBuffer syncModes = syncModesListToString(*syncTypes);
        dataStoreNode->setPropertyValue(PROPERTY_SOURCE_SYNC_MODES, syncModes.c_str());
    }
}

bool DMTClientConfig::saveSyncSourceConfig(const char* name) {
    
    LOG.debug("%s", __FUNCTION__);
    
    bool ret = false;
    unsigned int i = 0;

    if (!dmt || !sourcesNode) {
        LOG.error("%s: dmt nodes not initialized", __FUNCTION__);
        return false;
    }

    //
    // Sources management node
    //
    resetError();
    for(i = 0; i < sourceConfigsCount; ++i) {
        SyncSourceConfig* ss = getSyncSourceConfig(i);
        if (ss && strcmp(ss->getName(), name) == 0) {
            saveSourceConfig(i, sourcesNode);
            break;
        }
    }
    
    resetError(); // FIXME 

    ret = (getLastErrorCode() == 0);

    return ret;
}


bool DMTClientConfig::readMHConfig(ConfigurationNode* n) {
    
    if (!dmt) {
        LOG.error("%s: dmt nodes not initialized", __FUNCTION__);
        return false;
    }

    bool ret = true;
    ConfigurationNode* node;
    StringBuffer nodeName;
    
    char* fn = n->createFullName();
    nodeName.sprintf("%s%s", fn, CONTEXT_MEDIA_HUB);
    delete [] fn;
    
    // read all properties
    node = dmt->readManagementNode(nodeName.c_str());
    if (node) {
        if (!readMHConfig(n, node)) {
            ret = false;
        }
        delete node;
        node = NULL;
    }
    else {
        ret = false;
    }

    return true;
}


bool DMTClientConfig::readMHConfig(ConfigurationNode* /* syncMLNode */, ConfigurationNode* mhNode) {

    // All properties are in the extra-map
    StringMap* stringMap = mhNode->readAllProperties();
    KeyValuePair kvp = stringMap->front(); 
    
    while (kvp.null() == false) {
        if (kvp.getKey() != PROPERTY_DUMMY_KEY) {
            mhConfig.setProperty(kvp.getKey().c_str(), kvp.getValue().c_str());
        }
        kvp = stringMap->next();
    }
    delete stringMap;
    
    return true;
}

void DMTClientConfig::saveMHConfig(ConfigurationNode* n) {
    
    if (!dmt) {
        LOG.error("%s: dmt nodes not initialized", __FUNCTION__);
        return;
    }

    ConfigurationNode* node;
    StringBuffer nodeName;
    char* fn = n->createFullName();

    // Save all properties
    nodeName.sprintf("%s%s", fn, CONTEXT_MEDIA_HUB);
    node = dmt->readManagementNode(nodeName.c_str());
    if (node) {
        saveMHConfig(n, node);
        delete node;
        node = NULL;
    }
    
    delete [] fn;
}

void DMTClientConfig::saveMHConfig(ConfigurationNode* /* syncMLNode */, ConfigurationNode* mhNode) {
    
    //LOG.debug("entering %s", __FUNCTION__);

    // All properties are in the extra-map
    StringMap& stringMap = const_cast<StringMap &>(mhConfig.getExtraProps());
    KeyValuePair kvp = stringMap.front(); 

    while (kvp.null() == false) {
        if (kvp.getKey() != PROPERTY_DUMMY_KEY) {
            mhNode->setPropertyValue(kvp.getKey().c_str(), kvp.getValue().c_str());
        }
        kvp = stringMap.next();
    }
}

void DMTClientConfig::setSourceOrder(const char* sourceOrder_)
{
    sourceOrder = sourceOrder_;
}

const char* DMTClientConfig::getSourceOrder() const
{
    return sourceOrder.c_str();
}
    
void DMTClientConfig::setCredInfo(const char* credInfo_)
{
    credInfo = credInfo_;
}
    
const char* DMTClientConfig::getCredInfo() const
{
    return credInfo.c_str();
}

void DMTClientConfig::setAccessConfigUserToken(const char* accessConfigUserToken)
{
    this->accessConfigUserToken = accessConfigUserToken;
}

const char* DMTClientConfig::getSubscriptionsEnabled() const
{
    return subscriptionEnabled.c_str();
}

void DMTClientConfig::setSubscriptionsEnabled(const char* subscriptionEnabled)
{
    this->subscriptionEnabled = subscriptionEnabled;
}

}
