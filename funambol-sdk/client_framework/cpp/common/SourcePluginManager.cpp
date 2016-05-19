/*
 * Funambol is a mobile platform developed by Funambol, Inc.
 * Copyright (C) 2012 Funambol, Inc.
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

#include "SourcePluginManager.h"
#include "SourcePlugin.h"
#include "base/adapter/PlatformAdapter.h"
#include "base/util/ArrayList.h"
#include "client/DMTClientConfig.h"

namespace Funambol {

class PtrElement : public ArrayElement {
    
private:
    SourcePlugin* plugin;
    
public:
    PtrElement(SourcePlugin* plugin) {
        this->plugin = plugin;
    }
    
    SourcePlugin* get() {
        return plugin;
    }
    
    ArrayElement* clone() {
        PtrElement *res = new PtrElement(plugin);
        return res;
    }
};


SourcePluginManager* SourcePluginManager::instance = NULL;

SourcePluginManager::SourcePluginManager(DMTClientConfig* clientConfig_) : nextId(0), clientConfig(clientConfig_) {
}

SourcePluginManager::~SourcePluginManager()
{
}

SourcePluginManager* SourcePluginManager::getInstance(DMTClientConfig *clientConfig) {
    if (instance == NULL) {
        instance = new SourcePluginManager(clientConfig);
    }
    return instance;
}

void SourcePluginManager::dispose()
{
    delete instance; 
    instance = NULL;
}

void SourcePluginManager::registerSource(SourcePlugin* source) {
    registeredSources.push_back(source);
    source->setId(nextId++);
}

int SourcePluginManager::getNumRegisteredSources() const {
    return registeredSources.size();
}

int SourcePluginManager::getNumActiveSources() {
    if (orderedSources.size() == 0) {
        getOrderedSources();
    }
    return orderedSources.size();
}

SourcePlugin* SourcePluginManager::getSourceByName(const char* name) {
    for(int i=0;i<getNumRegisteredSources();++i) {
        SourcePlugin* source = getSourcePlugin(i);
        if (strcmp(source->getName(), name) == 0) {
            return source;
        }
    }
    return NULL;
}

SourcePlugin* SourcePluginManager::getSourceBySapiUri(const char* sapiUri) {
    for(int i=0;i<getNumRegisteredSources();++i) {
        SourcePlugin* source = getSourcePlugin(i);
        if (strcmp(source->getSapiUri(), sapiUri) == 0) {
            return source;
        }
    }
    return NULL;    
}

std::vector<SourcePlugin*>& SourcePluginManager::getOrderedSources() {
    
    if (orderedSources.size() == 0) {
        StringBuffer ordervalue(clientConfig->getSourceOrder());

        ArrayList order;
        ordervalue.split(order, ",");
        
        for (int i = 0; i < order.size(); i++){
            const char* sourceName = ((StringBuffer*)order.get(i))->c_str();
            // Search in the registered sources for this one
            for(int j=0;j<registeredSources.size();++j) {
                SourcePlugin* plugin = getSourcePlugin(j);
                if (plugin->getActive() && strcmp(sourceName, plugin->getName()) == 0) {
                    orderedSources.push_back(plugin);
                    break;
                }
            }
        }
    }
    return orderedSources;
}

SourcePlugin* SourcePluginManager::getSourcePlugin(int idx) const {
    return registeredSources[idx];
}

} // end Funambol namespace
