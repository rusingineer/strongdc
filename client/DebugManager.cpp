#include "stdinc.h"
#include "DebugManager.h"


DebugManager::DebugManager() {}
void DebugManager::SendDebugMessage(const string &mess) {
	fire(DebugManagerListener::DebugMessage(), mess);
}
DebugManager::~DebugManager() {}