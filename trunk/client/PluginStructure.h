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

#ifndef PLUGIN_STRUCTURE_H
#define PLUGIN_STRUCTURE_H
#include <iostream>
#include <string>
#include <hash_map>
using namespace std;

//Callback interface used by Plugins to interact with PhantomDC
class CPluginCallBackInterface{
public:
		virtual void sendChat(string message){} //send chat to current hub
		virtual void sendPM(string user, string message){} //send private message to user
		virtual void addClientLine(string message){} //add status line to hub (not sent to hub)
		virtual void addFavUser(string user, string comment){} //add user to favs with comment
		virtual void remFavUser(string user){} //remove user from favs
		virtual bool isFavUser(string user){ return false;} //returns if the user is a fav
		virtual HWND getMainWin(){return NULL;} //gets the main PhantomDC window
		virtual void debugMessage(string mess){} //show a debug message
};

//Basic Structure for all plugins
class CPluginStructure{
public:
	CPluginStructure() : callBack(NULL) {}

	//functions that have to be implemented
	virtual string getName()=0;  //returns a unique name for the plugin
	virtual double getVersion()=0; //returns the plugin version
	virtual string getAuthor()=0; //return authors name
	virtual string getDescription()=0; //returns a short description for plugin

	//Overload these functions to use the feature
	virtual bool onLoad(){ return true; } //called when function loaded
	virtual void onUnload(){} //called when plugin is about to be unloaded
	virtual void setDefaults(){} //setup the default settings in this

	virtual void onEnter(string &text){}
	virtual void onHubEnter(string &text){} //Called with text about to be sent or a /command
	virtual void onPMEnter(string user, string &text){} //Called with text about to be sent or a /command
	virtual void onChat(string &text){}  //Incoming chat from hubs
	virtual void onPM(string user, string &text){} //Incoming private message from user	
	virtual void onClientMessage(string &message){} //Incoming Client Message

	virtual void onCreate(HWND hWnd){} //called when a new window is being created
	virtual void onDestroy(HWND hWnd){} //called before a window is destroyed

	//Don't touch these.
	void setCallBack(CPluginCallBackInterface *cb){
		callBack = cb;
	}

	//Set Setting 'name' to value
	void setSetting(string name, string value){
		settings[name] = value;
	}

	//return value of setting 'name'
	string getSetting(string name){
		return settings[name];
	}

	//returns all settings
	hash_map <string, string> getSettings(){
		return settings;
	}

	//remove setting 'name'
	void removeSetting(string name){
		for(hash_map <string, string>::iterator i = settings.begin(); i != settings.end(); i++) {	
			if(i->first == name) {
				settings.erase(i);
				return;
			}			
		}
	}

protected:
	CPluginCallBackInterface *callBack; //Pointer to a callback structure
	hash_map <string, string> settings; //hash_map for all settings
};

#endif //PLUGIN_STRUCTURE_H