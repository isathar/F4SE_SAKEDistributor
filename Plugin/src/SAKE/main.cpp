#include <shlobj.h>
#include "f4se/PluginAPI.h"
#include "Config.h"
#include "SAKELoader.h"


IDebugLog				gLog;
PluginHandle			g_pluginHandle =	kPluginHandle_Invalid;
F4SEMessagingInterface	*g_messaging =		NULL;


void F4SEMessageHandler(F4SEMessagingInterface::Message* msg)
{
	if (msg->type == F4SEMessagingInterface::kMessage_GameDataReady) {
		if (msg->data == (void*)true) {
			// load extra data from config files
			SAKEData::LoadGameData();
		}
	}
}


extern "C"
{

bool F4SEPlugin_Query(const F4SEInterface * f4se, PluginInfo * info)
{
	gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\SAKE.log");

	info->infoVersion = PluginInfo::kInfoVersion;
	info->name =		PLUGIN_NAME_SHORT;
	info->version =		PLUGIN_VERSION;
	
	if (f4se->runtimeVersion != SUPPORTED_RUNTIME_VERSION) {
		_MESSAGE("Aborting - Game version mismatch");
		return false;
	}
	
	g_pluginHandle =	f4se->GetPluginHandle();
	
	g_messaging = (F4SEMessagingInterface *)f4se->QueryInterface(kInterface_Messaging);
	if (!g_messaging) {
		_MESSAGE("Aborting - Messaging query failed");
		return false;
	}

	return true;
}

bool F4SEPlugin_Load(const F4SEInterface *f4se)
{
	if (g_messaging) {
		g_messaging->RegisterListener(g_pluginHandle, "F4SE", F4SEMessageHandler);
	}

	_MESSAGE("%s v%s dll loaded...\n", PLUGIN_NAME_SHORT, PLUGIN_VERSION_STRING);
	
    return true;
}

}
