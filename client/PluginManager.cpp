/* 
* Copyright (C) 2003 Twink, spm7@waikato.ac.nz
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "stdinc.h"
#include "DCPlusPlus.h"
#include "PluginManager.h"
#include "Util.h"

CPlugin::~CPlugin(){
	if(object){		
		CALL_PLUGIN_NO_CALLBACK(this, onUnload())	
		object = NULL;
	}
	if(instance != NULL)
		FreeLibrary ( instance );
}


PluginManager* Singleton<PluginManager>::instance = NULL;

int PluginManager::reloadPlugins(){
	unloadPlugins();
	LoadPluginDirectory();	
	return plugins.size();
}

void PluginManager::unloadPlugins(){
	Lock l(cs);
	for(vector<CPlugin*>::iterator i = plugins.begin(); i != plugins.end(); i++){
		if((*i)!=NULL)delete *i;
		*i=NULL; 
	}
	plugins.clear();
}

void PluginManager::unloadPlugin(CPlugin* p){
	Lock l(cs);
	for(vector<CPlugin*>::iterator i = plugins.begin(); i != plugins.end();){
		if(p == *i) {
			if((*i)!=NULL)delete *i;
			*i=NULL; 
			i = plugins.erase(i);
		} else i++;
	}
	p = NULL;
}

void PluginManager::LoadPluginDirectory(){
	Lock l(cs);
	WIN32_FIND_DATA data;
	HANDLE hFind;

	hFind = FindFirstFile( (Util::getAppPath() + "\\plugins\\*.dll").c_str(), &data);

	if( hFind != INVALID_HANDLE_VALUE ){
		do{

			CPlugin *plugin = LoadPlugin(Util::getAppPath() + "\\plugins\\" + data.cFileName);
			if(plugin!=NULL)plugins.push_back(plugin);
		}while(FindNextFile(hFind,&data));
	}

}

CPlugin *PluginManager::LoadPlugin(string filename){
	HMODULE hr = LoadLibrary(filename.c_str());
	if(!hr) return NULL; 

	typedef CPluginStructure* (CALLBACK* LPFNDLLFUNC)();
	LPFNDLLFUNC pfn = (LPFNDLLFUNC)GetProcAddress(hr,(LPCSTR)"getObject");
	if(pfn == NULL) return NULL;

	CPlugin *plugin = new CPlugin();		

	if((plugin->object = pfn()) == NULL){
		delete plugin;
		return NULL;
	}
	plugin->instance = hr;
	plugin->filename = filename;

	return plugin;
}

void PluginManager::StartPlugins(){
	Lock l(cs);
	FOREACH(onLoad());
}

vector<string> PluginManager::getPluginList(){
	Lock l(cs);
	vector<string> pluginnames;
	
	for(vector<CPlugin*>::iterator i = plugins.begin(); i != plugins.end(); ++i){
		pluginnames.push_back((*i)->object->getName() + " " + Util::toString((*i)->object->getVersion()));
	}
	return pluginnames;
}

string PluginManager::onIncomingChat(Client *client, string text){
	Lock l(cs);
	FOREACH_CLIENT(onChat(text), client);
	return text;
}

string PluginManager::onIncomingPM(Client *client, string user, string text){
	Lock l(cs);
	FOREACH_CLIENT(onPM(user, text), client);
	return text;
}


string PluginManager::onOutgoingChat(Client *client, string text){
	Lock l(cs);
	{FOREACH_CLIENT(onEnter(text), client);}
	{FOREACH_CLIENT(onHubEnter(text), client);}
	return text;
}

string PluginManager::onOutgoingPM(Client *client, string user,  string text){
	Lock l(cs);
	{FOREACH_CLIENT(onEnter(text), client);}
	{FOREACH_CLIENT(onPMEnter(user, text), client);}
	return text;
}

bool PluginManager::onClientMessage(Client *client, string message) {	
	Lock l(cs);
	FOREACH_CLIENT(onClientMessage(message), client);
	return (message == "")? FALSE:TRUE;
}

void PluginManager::onCreateWindow(HWND hWnd){
	FOREACH(onCreate(hWnd));
}

void PluginManager::onDestroyWindow(HWND hWnd){
	FOREACH(onDestroy(hWnd));
}

void PluginManager::load(SimpleXML* aXml){
	WLock l(rwcs);

	aXml->resetCurrentChild();
	if(aXml->findChild("PreviewApps")) {
		aXml->stepIn();
		while(aXml->findChild("Application")) {					
			addPreviewApp(	aXml->getChildAttrib("Name"), aXml->getChildAttrib("Application"), aXml->getChildAttrib("Arguments"), aXml->getChildAttrib("Extension"));			
		}
		aXml->stepOut();
	}	



	aXml->resetCurrentChild();
	if(aXml->findChild("Plugins")) {
		aXml->stepIn();
		for(vector<CPlugin*>::iterator i = plugins.begin(); i != plugins.end(); ++i){
			(*i)->object->setDefaults();
			if(aXml->findChild((*i)->object->getName())) {
				StringList settings = aXml->getChildAttribs();
				for(StringList::iterator j = settings.begin(); j != settings.end(); ++j){
					(*i)->object->setSetting((*j), aXml->getChildAttrib((*j), (*i)->object->getSetting(*j)));					
				}
			}
		}
		aXml->stepOut();
	}	

 }

void PluginManager::save(SimpleXML* aXml){
	RLock l(rwcs);
	aXml->addTag("PreviewApps");
	aXml->stepIn();
	for(PreviewApplication::Iter i = previewApps.begin(); i != previewApps.end(); ++i) {
		aXml->addTag("Application");
		aXml->addChildAttrib("Name", (*i)->getName());
		aXml->addChildAttrib("Application", (*i)->getApplication());
		aXml->addChildAttrib("Arguments", (*i)->getArguments());
		aXml->addChildAttrib("Extension", (*i)->getExtension());
	}
	aXml->stepOut();

	aXml->addTag("Plugins");
	aXml->stepIn();
	for(vector<CPlugin*>::iterator i = plugins.begin(); i != plugins.end(); ++i){
		aXml->addTag((*i)->object->getName());
		hash_map <string, string> settings = (*i)->object->getSettings();
		for(hash_map <string, string>::iterator j = settings.begin(); j != settings.end(); ++j){
			aXml->addChildAttrib(j->first, 	j->second);			
		}
	}
	aXml->stepOut();
}