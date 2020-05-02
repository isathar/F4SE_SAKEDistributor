#include "SAKELoader.h"
#include "SimpleIni/SimpleIni.h"


namespace SAKEFileReader
{
	/** the current pass
			0: races
			1: other forms
			2: name prefixes **/
	UInt8 iPassCount = 0;


	// reads the overrides from a json file at jsonPath
	int LoadOverride(const std::string & jsonPath)
	{
		_MESSAGE("\n  Loading Override file %s...", jsonPath.c_str());
		if (!SAKEUtilities::IsPathValid(jsonPath)) {
			return 4;
		}

		json otObject;
		std::ifstream otFile(jsonPath.c_str());
		otFile >> otObject;
		otFile.close();

		if (otObject.is_null() || otObject.empty()) {
			return 3;
		}
		
		// ---- required plugins check
		if (!otObject["requirements"].is_null()) {
			bool bHasReqs = true;
			if (otObject["requirements"].is_array() && !otObject["requirements"].empty()) {
				for (json::iterator itReqs = otObject["requirements"].begin(); itReqs != otObject["requirements"].end(); ++itReqs) {
					std::string reqName = *itReqs;
					if (!SAKEUtilities::IsModLoaded(reqName)) {
						_MESSAGE("    WARNING: Missing required plugin: %s", reqName.c_str());
						bHasReqs = false;
					}
				}
			}
			if (!bHasReqs) {
				return 0;
			}
		}

		// ---- load overrides
		json curOverride;

		// check the pass number
		if (iPassCount == 0) {
			// pass 0 - Race Overrides
			if (otObject["raceOverrides"].is_null()) {
				_MESSAGE("    INFO: Skipping file during first pass.");
				return 5;
			}
			else {
				if (otObject["raceOverrides"].empty() || !otObject["raceOverrides"].is_array()) {
					return 1;
				}
				for (json::iterator itOverride = otObject["raceOverrides"].begin(); itOverride != otObject["raceOverrides"].end(); ++itOverride) {
					curOverride.clear();
					curOverride = *itOverride;
					std::string curFormID = curOverride["formID"].is_null() ? "none" : curOverride["formID"];
					TESForm * curForm = SAKEUtilities::GetFormFromIdentifier(curFormID);
					if (!curForm) {
						_MESSAGE("\n      ERROR: Form not found!  (%s)", curFormID.c_str());
						continue;
					}
					if (curForm->formType == kFormType_RACE) {
						SAKEData::LoadOverrides_Race(reinterpret_cast<TESRace*>(curForm), curOverride);
					}
				}
			}
		}
		else if (iPassCount == 1) {
			// pass 1 - GameSettings + Form Overrides
			if (!otObject["gameSettings"].is_null()) {
				SAKEData::LoadGameSettings(otObject["gameSettings"]);
			}

			if (otObject["overrides"].is_null()) {
				return 2;
			}
			else {
				if (otObject["overrides"].empty() || !otObject["overrides"].is_array()) {
					return 1;
				}
				for (json::iterator itOverride = otObject["overrides"].begin(); itOverride != otObject["overrides"].end(); ++itOverride) {
					curOverride.clear();
					curOverride = *itOverride;
					std::string curFormID = curOverride["formID"].is_null() ? "none" : curOverride["formID"];
					TESForm * curForm = SAKEUtilities::GetFormFromIdentifier(curFormID);
					if (!curForm) {
						_MESSAGE("\n      ERROR: Form not found!  (%s)", curFormID.c_str());
						continue;
					}

					switch (curForm->formType) {
						case kFormType_LVLI:
							SAKEData::LoadOverrides_LeveledItem(reinterpret_cast<TESLevItem*>(curForm), curOverride);
							break;
						case kFormType_LVLN:
							SAKEData::LoadOverrides_LeveledActor(reinterpret_cast<TESLevCharacter*>(curForm), curOverride);
							break;
						case kFormType_ARMO:
							SAKEData::LoadOverrides_Armor(reinterpret_cast<TESObjectARMO*>(curForm), curOverride);
							break;
						case kFormType_WEAP:
							SAKEData::LoadOverrides_Weapon(reinterpret_cast<TESObjectWEAP*>(curForm), curOverride);
							break;
						case kFormType_NPC_:
							SAKEData::LoadOverrides_Actor(reinterpret_cast<TESNPC*>(curForm), curOverride);
							break;
						case kFormType_AMMO:
							SAKEData::LoadOverrides_Ammo(reinterpret_cast<TempTESAmmo*>(curForm), curOverride);
							break;
						case kFormType_MISC:
							SAKEData::LoadOverrides_Misc(reinterpret_cast<TESObjectMISC*>(curForm), curOverride);
							break;
						case kFormType_KEYM:
							SAKEData::LoadOverrides_Key(reinterpret_cast<TempTESKey*>(curForm), curOverride);
							break;
						case kFormType_CMPO:
							SAKEData::LoadOverrides_Component(reinterpret_cast<BGSComponent*>(curForm), curOverride);
							break;
						case kFormType_ALCH:
							SAKEData::LoadOverrides_Ingestible(reinterpret_cast<AlchemyItem*>(curForm), curOverride);
							break;
						case kFormType_ECZN:
							SAKEData::LoadOverrides_EncounterZone(reinterpret_cast<BGSEncounterZone*>(curForm), curOverride);
							break;
						case kFormType_PROJ:
							SAKEData::LoadOverrides_Projectile(reinterpret_cast<TempBGSProjectile*>(curForm), curOverride);
							break;

						default:
							_MESSAGE("    WARNING: Form 0x%08X has invalid formType %i", curForm->formID, curForm->formType);
					}
				}
			}
		}
		else {
			// pass 2 - name prefixes
			if (otObject["namePrefixes"].is_null()) {
				_MESSAGE("    INFO: No Name Prefixes found.");
				return 5;
			}
			if (otObject["namePrefixes"].empty() || !otObject["namePrefixes"].is_array()) {
				_MESSAGE("    INFO: Name Prefixes found but empty.");
				return 5;
			}

			for (json::iterator itOverride = otObject["namePrefixes"].begin(); itOverride != otObject["namePrefixes"].end(); ++itOverride) {
				curOverride.clear();
				curOverride = *itOverride;
				if (!curOverride["prefix"].is_null() && !curOverride["forms"].is_null() && curOverride["forms"].is_array() && !curOverride["forms"].empty()) {
					std::string prefixStr = curOverride["prefix"];
					for (json::iterator itForms = curOverride["forms"].begin(); itForms != curOverride["forms"].end(); ++itForms) {
						std::string curFormID = *itForms;
						TESForm * curForm = SAKEUtilities::GetFormFromIdentifier(curFormID);
						if (curForm) {
							SAKEData::LoadNamePrefix(curForm, prefixStr);
						}
					}
				}
			}
			
		}

		return -1;
	}


	// reads all json overrides found in the directory at jsonPath
	int LoadOverridesFolder(const std::string & jsonPath)
	{
		_MESSAGE("\n\nLoading Template: %s", jsonPath.c_str());
		if (!SAKEUtilities::IsPathValid(jsonPath)) {
			return 0;
		}

		std::vector<std::string> templateFiles = SAKEUtilities::GetFileNames(jsonPath.c_str());
		if (templateFiles.empty()) {
			return 1;
		}

		std::string otFilePath;
		for (std::vector<std::string>::iterator itFile = templateFiles.begin(); itFile != templateFiles.end(); ++itFile) {
			otFilePath.clear();
			otFilePath.append(jsonPath.c_str());
			otFilePath.append(itFile->c_str());
			switch (LoadOverride(otFilePath)) {
				case 0:
					_MESSAGE("    WARNING: Missing required mods!");
					break;
				case 1:
					_MESSAGE("    WARNING: Overrides list is empty!");
					break;
				case 2:
					_MESSAGE("    WARNING: File contains no overrides!");
					break;
				case 3:
					_MESSAGE("    WARNING: Invalid json file!");
					break;
				case 4:
					_MESSAGE("    WARNING: Invalid Override file path %s...", otFilePath.c_str());
					break;
			}
		}
		templateFiles.clear();
		return -1;
	}


	// reads the profile at activeProfile from profilesPath, loading its override files and folders found at templatesPath
	int LoadProfile(const std::string & presetName, const std::string & basePath)
	{
		std::string curProfile = basePath.c_str();
		curProfile.append("Config\\");
		curProfile.append(presetName.c_str());
		curProfile.append(".json");

		_MESSAGE("\n\nLoading Profile: %s", curProfile.c_str());
		if (!SAKEUtilities::IsPathValid(curProfile)) {
			return 0;
		}
		json profileObject;
		std::ifstream profileFile(curProfile.c_str());
		profileFile >> profileObject;
		profileFile.close();

		if (profileObject.is_null() || profileObject["active"].is_null()) {
			return 2;
		}
		json overridesList = profileObject["active"];
		if (overridesList.empty()) {
			return 1;
		}

		std::vector<std::string> templateFiles;
		std::string otPath;
		json templateObj, includeObj, excludeObj;
		bool reqCheckPassed = true;

		std::string overridesPath = basePath.c_str();
		overridesPath.append("Overrides\\");

		for (; iPassCount < 3; iPassCount++) {
			_MESSAGE("\n\nStarting pass %i", iPassCount);
			for (json::iterator itDir = overridesList.begin(); itDir != overridesList.end(); ++itDir) {
				templateObj = *itDir;

				if (templateObj["name"].is_null()) {
					_MESSAGE("\n\nWARNING: Template name is empty!");
					continue;
				}

				std::string otName = templateObj["name"];

				reqCheckPassed = true;
				// check for required plugins
				if (!templateObj["includeIf"].is_null()) {
					includeObj = templateObj["includeIf"];
					for (json::iterator itIncl = includeObj.begin(); itIncl != includeObj.end(); ++itIncl) {
						std::string curInclude = *itIncl;
						if (!SAKEUtilities::IsModLoaded(curInclude)) {
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
					excludeObj = templateObj["excludeIf"];
					for (json::iterator itExcl = excludeObj.begin(); itExcl != excludeObj.end(); ++itExcl) {
						std::string curExclude = *itExcl;
						if (SAKEUtilities::IsModLoaded(curExclude)) {
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
					_MESSAGE("\n\nLoading loose Override file:");
					switch (LoadOverride(otPath)) {
					case 0:
						_MESSAGE("    WARNING: Missing required mods!");
						break;
					case 1:
						_MESSAGE("    WARNING: Overrides list is empty!");
						break;
					case 2:
						_MESSAGE("    WARNING: File contains no overrides!");
						break;
					case 3:
						_MESSAGE("    WARNING: Invalid json file!");
						break;
					case 4:
						_MESSAGE("    WARNING: Invalid Override file path %s...", otPath.c_str());
						break;
					}
				}
				else {
					// -- Load as Template
					otPath.append("\\");
					switch (LoadOverridesFolder(otPath)) {
					case 0:
						_MESSAGE("  WARNING: Invalid Template path!");
						break;
					case 1:
						_MESSAGE("  WARNING: Template is empty!");
						break;
					}
				}
			}
		}
		return -1;
	}

}



// temp form decoding + dev stuff
void TestingStuff()
{
	
}


// starts the loading process
void SAKEFileReader::LoadGameData()
{
	std::string sDataPath = ".\\Data\\F4SE\\Config\\SAKE\\";
	std::string sConfigPreset = "Default";
	CSimpleIniA iniBase;

	iniBase.SetUnicode();
	if (iniBase.LoadFile(".\\Data\\F4SE\\Plugins\\SAKE.ini") > -1) {
		sDataPath = iniBase.GetValue("Settings", "sDataPath", ".\\Data\\F4SE\\Config\\SAKE\\");
		sConfigPreset = iniBase.GetValue("Settings", "sConfigPreset", "Default");
		_MESSAGE("\nLoaded SAKE.ini...\n  dataPath: %s\n  configPreset: %s", sDataPath.c_str(), sConfigPreset.c_str());
	}
	else {
		_MESSAGE("\nUnable to load SAKE.ini... Using defaults.\n  dataPath: %s\n  configPreset: %s", sDataPath.c_str(), sConfigPreset.c_str());
	}

	switch (LoadProfile(sConfigPreset, sDataPath)) {
		case -1:
			_MESSAGE("\n\nFinished loading data.");
			break;
		case 0:
			_MESSAGE("  ERROR: Invalid Config path...");
			break;
		case 1:
			_MESSAGE("  ERROR: Config contains no active templates!");
			break;
		case 2:
			_MESSAGE("  ERROR: Invalid/Corrupted Config file!");
			break;
	}
	
	//TestingStuff();
}

