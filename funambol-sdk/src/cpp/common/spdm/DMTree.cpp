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
#include "base/util/utils.h"
#include "base/Log.h"

#include "spdm/ManagementNode.h"
#include "spdm/ConfigurationNode.h"
#include "spdm/DeviceManagementNode.h"
#include "spdm/DMTree.h"
#include "base/globalsdef.h"

USE_NAMESPACE


static void addNodeChildren(ConfigurationTree* tree, ConfigurationNode* n, const char* name) {

    if (!n) {
        return;
    }
    int childrenCount = n->getChildrenMaxCount();

    if (childrenCount) {
        char** childrenNames = n->getChildrenNames();

        if (!childrenNames){
            LOG.error("%s: error in getChildrenNames: node name =  %s", __FUNCTION__, name? name:"(null)");
            return;
        }
        int i = 0;
        for (i = 0; i < childrenCount; i++) {
            if (childrenNames[i] == NULL) {
                LOG.error("%s: null child name #%d", __FUNCTION__, i);
                continue;
            }
            // Create the new node
            DeviceManagementNode newNode(name, childrenNames[i]);
            n->addChild(&newNode);
        }
        for (i = 0; i < childrenCount; i++) {
            if (childrenNames[i] != NULL) {
                delete [] childrenNames[i]; childrenNames[i] = NULL;
            }
        }
        delete [] childrenNames;
    }
}




/*
 * Basic implementation of DMTree, can be re-defined if platform specific
 * variant is needed.
 */

/*
 * Constructor
 */
DMTree::DMTree(const char *root) : ConfigurationTree(root) {
}

/*
 * Destructor
 */
DMTree::~DMTree() {
}

bool DMTree::isLeaf(const char *node) {
    DeviceManagementNode dmn(node);

    return (dmn.getChildrenMaxCount() == 0);
}

ConfigurationNode* DMTree::getNode(const char* node) {

    StringBuffer context(root);
    if (root.endsWith('/')) {
        context = root.substr(0, root.length()-1);
    }
    StringBuffer name(node);
    if (node[0] == '/') {
        name = &node[1];
    }

    StringBuffer completeNodeName;
    completeNodeName.sprintf("%s/%s", context.c_str(), name.c_str());
    ManagementNode *n = new DeviceManagementNode(completeNodeName.c_str());
    return n;
}


ConfigurationNode* DMTree::readManagementNode(const char* node) {

    if (!node) {
        LOG.error("DMTree::readManagementNode - NULL node name");
        return NULL;
    }
    
    // Safe check: the ManagementNode MUST contain a "/" char, to correctly
    // split context and name values (see ManagementNode::setFullName)
    StringBuffer nodeName(node);
    if (nodeName.find("/") == StringBuffer::npos) {
        // It's the root context
        nodeName.append("/");
    }
    
    ManagementNode *n = new DeviceManagementNode(nodeName.c_str());
    if (!n) {
        LOG.error("DMTree::readManagementNode - NULL node");
        return NULL;
    }

    addNodeChildren(this, n, node);

    return n;
}

void DMTree::refreshNodeChildren(ConfigurationNode *n)
{    
    if (!n) {
        LOG.error("DMTree::refreshNodeChildren - NULL node");
        return;
    }
    
    n->removeAllChildren();
    const char* fn = n->createFullName();
    addNodeChildren(this, n, fn);
    delete [] fn;
}


ConfigurationNode* DMTree::readManagementNode(const char* context, const char* name) {

    if (!context || !name) {
        LOG.error("DMTree::readManagementNode - NULL node name");
        return NULL;
    }

    ManagementNode *n = new DeviceManagementNode(context, name);
    if (!n) {
        LOG.error("DMTree::readManagementNode - NULL node");
        return NULL;
    }

    char* fullName = n->createFullName();
    addNodeChildren(this, n, fullName);
    delete [] fullName;

    return n;
}

ConfigurationNode* DMTree::createConfigurationNode(const char* parent, const char* name) {
    return new DeviceManagementNode(parent, name);
}

bool DMTree::checkNodeExistence(const char* fullName) {
    return true;
}