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


#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <vector>
using std::vector;

#include "HubManager.h"
#include "ClientManager.h"
#include "CriticalSection.h"
#include "Client.h"
#include "SimpleXML.h" 
//#include "ScriptManager.h"
#include "PluginStructure.h"


class PreviewApplication {
public:
	typedef PreviewApplication* Ptr;
	typedef vector<Ptr> List;
	typedef List::iterator Iter;

	PreviewApplication() throw() {}
	PreviewApplication(string n, string a, string r, string e) : name(n), application(a), arguments(r), extension(e) {};
	~PreviewApplication() throw() { }	

	GETSET(string, name, Name);
	GETSET(string, application, Application);
	GETSET(string, arguments, Arguments);
	GETSET(string, extension, Extension);
};

class CPlugin{
public:
	CPlugin() : instance(NULL), filename(""), object(NULL) {}
	virtual ~CPlugin();

	HMODULE instance;
	string filename;
	CPluginStructure *object;
};

class PluginManager : public Singleton<PluginManager>, private SettingsManagerListener/*,  public Speaker<ScriptManagerListener>*/
{
public:
	PluginManager(){
		SettingsManager::getInstance()->addListener(this);
		m_hWnd = NULL;
	}

	~PluginManager(){
		SettingsManager::getInstance()->removeListener(this);
		unloadPlugins();
	}

	void unloadPlugins();
	void unloadPlugin(CPlugin *c);
	int reloadPlugins();
	void LoadPluginDirectory();
	void StartPlugins();
	vector<string> getPluginList();	

	CPlugin *getPlugin(int index){
		return plugins[index];
	}

	//Functions that call the dll
	string onIncomingChat(Client *hub, string message);
	string onOutgoingChat(Client *hub, string message);
	string onIncomingPM(Client *hub, string user, string message);
	string onOutgoingPM(Client *hub, string user, string message);
	bool onClientMessage(Client *hub, string message);

	//GUI
	void onCreateWindow(HWND hWnd);
	void onDestroyWindow(HWND hWnd);

	GETSET(HWND, m_hWnd, MainWindow)


		PreviewApplication::Ptr PluginManager::addPreviewApp(string name, string application, string arguments, string extension){
			PreviewApplication::Ptr pa = new PreviewApplication(name, application, arguments, extension);
			previewApps.push_back(pa);
			return pa;
		}

		void PluginManager::getPreviewApp(int index, PreviewApplication &pa){
			if(previewApps.size()>index)
				pa = *previewApps[index];	
		}

		void PluginManager::removePreviewApp(int index){
			if(previewApps.size()>index)
				previewApps.erase(previewApps.begin() + index);	
		}

		void PluginManager::updatePreviewApp(int index, PreviewApplication &pa){
			*previewApps[index] = pa;
		}

		PreviewApplication::List& getPreviewApps() { return previewApps; };

	/*	void debugMessage(string mess) {
			fire(ScriptManagerListener::DEBUG_MESSAGE, mess);
		}*/

private:
	// SettingsManagerListener
//	virtual void onAction(SettingsManagerListener::Types type, SimpleXML* xml) throw();
	virtual void on(SettingsManagerListener::Load, SimpleXML* xml) throw() { load(xml); }
	virtual void on(SettingsManagerListener::Save, SimpleXML* xml) throw() { save(xml); }
	void load(SimpleXML* aXml);
	void save(SimpleXML* aXml);

	RWLock rwcs;
	CriticalSection cs;
	PreviewApplication::List previewApps;
	CPlugin *LoadPlugin(string filename);
	vector <CPlugin*> plugins;
};

//Implementation of the Callback interface
//Plugins call functions in this
class CPluginCallBack : public CPluginCallBackInterface{
public:
	CPluginCallBack() : hub(NULL){}
	CPluginCallBack(Client *h) : hub(h){}

	virtual HWND getMainWin(){
		return PluginManager::getInstance()->getMainWindow();
	}

/*	virtual void sendChat(string message){
		if(hub!=NULL)
			hub->sendMessage(message);
	}

	virtual void sendPM(string user, string message){
		if(!ClientManager::getInstance()->getUser(user)->isOnline())return;
		if(hub!=NULL)
			hub->privateMessage(user,"<" + hub->getNick() + "> " + message);
	}*/

	virtual void addClientLine(string message){
		if(hub!=NULL)
			hub->fire(ClientListener::Message(), hub, message);
	}

	virtual void addFavUser(string user, string comment){
		User::Ptr u = ClientManager::getInstance()->getUser(user);
		u->setUserDescription(comment);
		HubManager::getInstance()->addFavoriteUser(u);	
	}

	virtual void remFavUser(string user){
		User::Ptr u = ClientManager::getInstance()->getUser(user);
		HubManager::getInstance()->removeFavoriteUser(u);
	}

	virtual bool isFavUser(string user){
		User::Ptr u = ClientManager::getInstance()->getUser(user);
		return u->isFavoriteUser();
	}	

/*	virtual void debugMessage(string mess) {
		PluginManager::getInstance()->debugMessage(mess);
	}*/

private:
	Client *hub;
};

#define CALL_PLUGIN(p, function, cb) \
	p->object->setCallBack(cb); \
	p->object->function; \
	p->object->setCallBack(NULL); 

#define CALL_PLUGIN_NO_CALLBACK(p, function) \
	CPluginCallBack *cb = new CPluginCallBack(); \
	CALL_PLUGIN(p, function, cb) \
	delete cb; cb = NULL;


#define FOREACH_CLIENT(function, client) \
	CPluginCallBack *cb = new CPluginCallBack(client); \
	FOREACH_CALLBACK(function,cb) \
	delete cb; cb = NULL;

#define FOREACH_CALLBACK(function, cb) \
	for(vector<CPlugin*>::iterator i = plugins.begin(); i != plugins.end(); ++i) { \
		CALL_PLUGIN((*i), function,cb) \
	} \

#define FOREACH(function) \
	CPluginCallBack *cb = new CPluginCallBack(); \
	FOREACH_CALLBACK(function,cb) \
	delete cb; cb = NULL;

#endif //PLUGIN_MANAGER_H