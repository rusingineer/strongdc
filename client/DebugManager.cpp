#include "stdinc.h"
#include "DebugManager.h"


DebugManager::DebugManager() {}
void DebugManager::SendDebugMessage(const string &mess) {
	fire(DebugManagerListener::DEBUG_MESSAGE, mess);
}
DebugManager::~DebugManager() {}