#include "SAKEShared.h"


// returns a form's plugin name or "Ref" if it's not a base form
const char * SAKELoader::GetPluginNameFromFormID(UInt32 formID)
{
	if (formID > 0x0) {
		char varVal[9] = "00000000";
		sprintf_s(varVal, 9, "%08X", formID);
		std::string formStr = varVal;
		std::string indexStr = formStr.substr(0, 2);
		unsigned int modIndexInt = 0;
		sscanf_s(indexStr.c_str(), "%02X", &modIndexInt);
		UInt8 modIndex = modIndexInt;
		ModInfo *tempMod = nullptr;
		if (modIndex != 0xFF) {
			// check if this is a light plugin
			if (modIndex == 0xFE) {
				std::string lightindexStr = formStr.substr(2, 2);
				unsigned int lightIndexInt = 0;
				sscanf_s(lightindexStr.c_str(), "%02X", &lightIndexInt);
				UInt8 lightIndex = lightIndexInt;

				if (lightIndex != 0xFF) {
					(*g_dataHandler)->modList.lightMods.GetNthItem(lightIndex, tempMod);
					return tempMod->name;
				}
			}
			else {
				(*g_dataHandler)->modList.loadedMods.GetNthItem(modIndex, tempMod);
				return tempMod->name;
			}
		}
		else {
			// editor-placed or in-game reference
			return "Ref";
		}
	}
	return "none";
}


// returns a formatted string containing (string=pluginName|UInt=formID without loadorder) 
const char * SAKELoader::GetIdentifierFromFormID(UInt32 formID)
{
	if (formID > 0x0) {
		std::string finalFormString = GetPluginNameFromFormID(formID);

		char varVal[9] = "00000000";
		sprintf_s(varVal, 9, "%08X", formID);
		std::string tempFormString = varVal;

		finalFormString.append("|");
		finalFormString.append(tempFormString.substr(2));

		return finalFormString.c_str();
	}
	else
		return "none";
}


// returns a formID from a formatted string
UInt32 SAKELoader::GetFormIDFromIdentifier(const std::string & formIdentifier)
{
	UInt32 formId = 0;
	if (formIdentifier.c_str() != "none") {
		std::size_t pos = formIdentifier.find_first_of("|");
		std::string modName = formIdentifier.substr(0, pos);
		std::string modForm = formIdentifier.substr(pos + 1);
		sscanf_s(modForm.c_str(), "%X", &formId);

		if (formId != 0x0) {
			UInt8 modIndex = (*g_dataHandler)->GetLoadedModIndex(modName.c_str());
			if (modIndex != 0xFF) {
				formId |= ((UInt32)modIndex) << 24;
			}
			else {
				UInt16 lightModIndex = (*g_dataHandler)->GetLoadedLightModIndex(modName.c_str());
				if (lightModIndex != 0xFFFF) {
					formId |= 0xFE000000 | (UInt32(lightModIndex) << 12);
				}
				else {
					_MESSAGE("FormID %s not found!", formIdentifier.c_str());
					formId = 0;
				}
			}
		}
	}
	return formId;
}


// returns a form from a formatted string
TESForm * SAKELoader::GetFormFromIdentifier(const std::string & formIdentifier)
{
	UInt32 formId = GetFormIDFromIdentifier(formIdentifier);
	return (formId != 0x0) ? LookupFormByID(formId) : nullptr;
}


// returns true if the passed mod is loaded
bool SAKELoader::IsModLoaded(const std::string & modName)
{
	if ((*g_dataHandler)->GetLoadedModIndex(modName.c_str()) != (UInt8)-1) {
		return true;
	}
	return ((*g_dataHandler)->GetLoadedLightModIndex(modName.c_str()) != (UInt16)-1);
}


// returns a list of json file names at the passed path
std::vector<std::string> SAKELoader::GetFileNames(const std::string & folder)
{
	std::vector<std::string> names;
	std::string search_path = folder + "/*.json";
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				names.push_back(fd.cFileName);
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
	return names;
}


// returns true if the passed path/filename exists
bool SAKELoader::IsPathValid(const std::string & path)
{
	std::string search_path = path + "*";
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	return (hFind != INVALID_HANDLE_VALUE);
}

