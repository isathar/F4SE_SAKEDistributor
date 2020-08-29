#include "SAKELoader.h"
#include <chrono>


namespace SAKELoader
{
	// the current pass:  0=races, 1=everything else
	UInt8 iPassCount = 0;


	// reads the overrides from a json file at jsonPath
	void LoadOverride(const std::string & jsonPath)
	{
		_MESSAGE("\n  Loading Override file %s...", jsonPath.c_str());
		if (!IsPathValid(jsonPath)) {
			_MESSAGE("    WARNING: Invalid path (%s)!", jsonPath.c_str());
			return;
		}
		// get json data from file and check it
		json otObject;
		std::ifstream otFile(jsonPath.c_str());
		otFile >> otObject;
		otFile.close();
		if (otObject.is_null() || otObject.empty()) {
			_MESSAGE("    WARNING: Invalid json file!");
			return;
		}

		// pass 0 - GameSettings
		// - loaded first because some settings affect automatic calculation of form stats that may be edited by other overrides
		if (iPassCount == 0) {
			if (!otObject["gameSettings"].is_null()) {
				LoadGameSettings(otObject["gameSettings"]);
			}
			return;
		}

		// pass 3 - Name prefixes
		// - loaded last to respect any name changes in other form overrides
		if (iPassCount == 3) {
			if (otObject["namePrefixes"].is_null()) {
				_MESSAGE("\n\nFinished loading data.");
				return;
			}
			if (otObject["namePrefixes"].empty() || !otObject["namePrefixes"].is_array()) {
				_MESSAGE("    WARNING: Name Prefixes list exists but is empty.");
				_MESSAGE("\n\nFinished loading data.");
				return;
			}
			json curName;
			for (json::iterator itNames = otObject["namePrefixes"].begin(); itNames != otObject["namePrefixes"].end(); ++itNames) {
				curName = *itNames;
				if (!curName["prefix"].is_null() && !curName["forms"].is_null() && curName["forms"].is_array() && !curName["forms"].empty()) {
					std::string prefixStr = curName["prefix"];
					for (json::iterator itForms = curName["forms"].begin(); itForms != curName["forms"].end(); ++itForms) {
						std::string curFormID = *itForms;
						TESForm * curForm = GetFormFromIdentifier(curFormID);
						if (curForm) {
							LoadNamePrefix(curForm, prefixStr);
						}
					}
				}
			}
			return;
		}

		// passes 1 and 2 - forms
		json curOverride;
		if (otObject["overrides"].is_null()) {
			_MESSAGE("    WARNING: Overrides list is missing!");
			return;
		}
		if (otObject["overrides"].empty() || !otObject["overrides"].is_array()) {
			_MESSAGE("    WARNING: Overrides list is empty or not an array!");
			return;
		}

		// load the overrides list
		for (json::iterator itOverride = otObject["overrides"].begin(); itOverride != otObject["overrides"].end(); ++itOverride) {
			curOverride.clear();
			curOverride = *itOverride;
			std::string curFormID = curOverride["formID"].is_null() ? "none" : curOverride["formID"];
			TESForm * curForm = GetFormFromIdentifier(curFormID);
			if (!curForm) {
				_MESSAGE("\n      WARNING: Form not found!  (%s)", curFormID.c_str());
				continue;
			}

			if (iPassCount == 1) {
				// pass 1 - Race overrides
				// - needs to be done before actor overrides because races act as templates for actors
				if (curForm->formType == kFormType_RACE) {
					LoadOverrides_Race(reinterpret_cast<TESRace*>(curForm), curOverride);
				}
			}
			else if (iPassCount == 2) {
				// pass 2 - General form overrides
				switch (curForm->formType) {
					case kFormType_LVLI:
						LoadOverrides_LeveledItem(reinterpret_cast<TESLevItem*>(curForm), curOverride);
						break;
					case kFormType_LVLN:
						LoadOverrides_LeveledActor(reinterpret_cast<TESLevCharacter*>(curForm), curOverride);
						break;
					case kFormType_ARMO:
						LoadOverrides_Armor(reinterpret_cast<TESObjectARMO*>(curForm), curOverride);
						break;
					case kFormType_WEAP:
						LoadOverrides_Weapon(reinterpret_cast<TESObjectWEAP*>(curForm), curOverride);
						break;
					case kFormType_NPC_:
						LoadOverrides_Actor(reinterpret_cast<TESNPC*>(curForm), curOverride);
						break;
					case kFormType_AMMO:
						LoadOverrides_Ammo(reinterpret_cast<TempTESAmmo*>(curForm), curOverride);
						break;
					case kFormType_MISC:
						LoadOverrides_Misc(reinterpret_cast<TESObjectMISC*>(curForm), curOverride);
						break;
					case kFormType_KEYM:
						LoadOverrides_Key(reinterpret_cast<TempTESKey*>(curForm), curOverride);
						break;
					case kFormType_CMPO:
						LoadOverrides_Component(reinterpret_cast<BGSComponent*>(curForm), curOverride);
						break;
					case kFormType_ALCH:
						LoadOverrides_Ingestible(reinterpret_cast<AlchemyItem*>(curForm), curOverride);
						break;
					case kFormType_ECZN:
						LoadOverrides_EncounterZone(reinterpret_cast<BGSEncounterZone*>(curForm), curOverride);
						break;
					case kFormType_PROJ:
						LoadOverrides_Projectile(reinterpret_cast<TempBGSProjectile*>(curForm), curOverride);
						break;
					case kFormType_EXPL:
						LoadOverrides_Explosion(reinterpret_cast<TempBGSExplosion*>(curForm), curOverride);
						break;
					case kFormType_CONT:
						LoadOverrides_Container(reinterpret_cast<TESObjectCONT*>(curForm), curOverride);
						break;
						
					case kFormType_RACE:
						// races are done in pass 1
						break;

					default:
						_MESSAGE("    WARNING: Form 0x%08X has invalid formType %i", curForm->formID, curForm->formType);
				}
			}
		}
		return;
	}


	// reads all json overrides found at jsonPath
	void LoadOverridesFolder(const std::string & jsonPath)
	{
		_MESSAGE("\n\nLoading Template: %s", jsonPath.c_str());
		if (!IsPathValid(jsonPath)) {
			_MESSAGE("  WARNING: Invalid Template path!");
			return;
		}
		// get the file list
		std::vector<std::string> templateFiles = GetFileNames(jsonPath.c_str());
		if (templateFiles.empty()) {
			_MESSAGE("  WARNING: Template is empty!");
			return;
		}
		// load each file
		std::string otFilePath;
		for (std::vector<std::string>::iterator itFile = templateFiles.begin(); itFile != templateFiles.end(); ++itFile) {
			otFilePath.clear();
			otFilePath.append(jsonPath.c_str());
			otFilePath.append(itFile->c_str());
			LoadOverride(otFilePath);
		}
		templateFiles.clear();
	}


}


void TestReadForm(UInt32 formID)
{
	
}

// form decoding + dev stuff
void TestingStuff()
{
	
}


// starts the loading process
void SAKELoader::LoadGameData()
{
	// load timer
	int endTime = 0;
	int startTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();

	std::string curProfile = ".\\Data\\Config\\SAKELoader\\Config.json";
	if (!IsPathValid(curProfile)) {
		_MESSAGE("  ERROR: Config path doesn't exist!");
		return;
	}
	json profileObject;
	std::ifstream profileFile(curProfile.c_str());
	profileFile >> profileObject;
	profileFile.close();

	if (profileObject.is_null()) {
		_MESSAGE("  ERROR: Invalid/Corrupted Config.json!");
		return;
	}

	if (profileObject["activeOverrides"].is_null()) {
		_MESSAGE("  ERROR: Preset contains no active overrides list!");
		return;
	}
	json overridesList = profileObject["activeOverrides"];
	if (overridesList.empty()) {
		_MESSAGE("  ERROR: Preset active overrides list is empty!");
		return;
	}

	std::vector<std::string> templateFiles;
	std::string otPath;
	json templateObj;
	bool reqCheckPassed = true;
	std::string overridesPath = ".\\Data\\Config\\SAKELoader\\Overrides\\";

	for (; iPassCount < 4; iPassCount++) {
		_MESSAGE("\n\nStarting pass %i", iPassCount);
		for (json::iterator itDir = overridesList.begin(); itDir != overridesList.end(); ++itDir) {
			templateObj = *itDir;

			if (templateObj["path"].is_null()) {
				_MESSAGE("\n\nWARNING: Override path is empty!");
				continue;
			}

			std::string otName = templateObj["path"];

			reqCheckPassed = true;
			// check for required plugins
			if (!templateObj["includeIf"].is_null()) {
				for (json::iterator itIncl = templateObj["includeIf"].begin(); itIncl != templateObj["includeIf"].end(); ++itIncl) {
					std::string curInclude = *itIncl;
					if (!IsModLoaded(curInclude)) {
						reqCheckPassed = false;
						_MESSAGE("\n\nWARNING: %s requires %s - skipping", otName.c_str(), curInclude.c_str());
						break;
					}
				}
				if (!reqCheckPassed) {
					continue;
				}
			}

			// check for incompatible plugins
			if (!templateObj["excludeIf"].is_null()) {
				for (json::iterator itExcl = templateObj["excludeIf"].begin(); itExcl != templateObj["excludeIf"].end(); ++itExcl) {
					std::string curExclude = *itExcl;
					if (IsModLoaded(curExclude)) {
						reqCheckPassed = false;
						_MESSAGE("\n\nWARNING: %s is incompatible with %s - skipping", otName.c_str(), curExclude.c_str());
						break;
					}
				}
				if (!reqCheckPassed) {
					continue;
				}
			}

			otPath.clear();
			otPath.append(overridesPath.c_str());
			otPath.append(otName.c_str());

			if (otName.find(".json") != std::string::npos) {
				// -- Load as Override JSON file
				_MESSAGE("\n\nLoading Override file:");
				LoadOverride(otPath);
			}
			else {
				// -- Load as a directory
				otPath.append("\\");
				LoadOverridesFolder(otPath);
			}
		}
	}

	//TestingStuff();

	endTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	_MESSAGE("\nFinished. Time elapsed: %.04f s\n\n", (float)(endTime - startTime) * 0.000000001);
}

