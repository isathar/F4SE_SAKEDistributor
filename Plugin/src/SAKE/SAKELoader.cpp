#include "SAKELoader.h"


namespace SAKEFileReader
{
	/** defines the amount of info to write to the log
			0: basic loading info, errors
			1: 0 + warnings
			2: 1 + extended loading info **/
	UInt8 iDebugLevel = 0;

	/** the current pass
			0: races
			1: other forms
			2: name prefixes **/
	UInt8 iPassCount = 0;


	/** the following loops need to be done quickly to minimize the initial loading delay, so
	suggest inline for a theoretical performance boost **/

	// reads the overrides from a json file at jsonPath, returns an error code if it fails
	inline int LoadOverride(const std::string & jsonPath)
	{
		if (iDebugLevel != 0) {
			_MESSAGE("\n  Loading Override file %s...", jsonPath.c_str());
		}
		if (!SAKEUtilities::IsPathValid(jsonPath)) {
			return 4;
		}
		json otObject;
		std::ifstream otFile(jsonPath.c_str());
		otFile >> otObject;
		if (otObject.is_null() || otObject.empty()) {
			otFile.close();
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
				otObject.clear();
				otFile.close();
				return 0;
			}
		}

		// ---- load overrides
		json curOverride;

		// check the pass number
		if (iPassCount == 0) {
			// pass 0 - Races
			if (otObject["raceOverrides"].is_null()) {
				otObject.clear();
				otFile.close();
				_MESSAGE("    INFO: Skipping file during first pass.");
				return 5;
			}
			else {
				if (!otObject["raceOverrides"].is_array() || otObject["raceOverrides"].empty()) {
					otObject.clear();
					otFile.close();
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
						SAKEData::LoadOverrides_Race((TESRace*)curForm, curOverride, iDebugLevel);
					}
				}
			}
		}
		else if (iPassCount == 1) {
			// pass 1 - forms besides Races
			if (otObject["overrides"].is_null()) {
				otObject.clear();
				otFile.close();
				return 2;
			}
			else {
				if (!otObject["overrides"].is_array() || otObject["overrides"].empty()) {
					otObject.clear();
					otFile.close();
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
							SAKEData::LoadOverrides_LeveledItem((TESLevItem*)curForm, curOverride, iDebugLevel);
							break;
						case kFormType_LVLN:
							SAKEData::LoadOverrides_LeveledActor((TESLevCharacter*)curForm, curOverride, iDebugLevel);
							break;
						case kFormType_ARMO:
							SAKEData::LoadOverrides_Armor((TESObjectARMO*)curForm, curOverride, iDebugLevel);
							break;
						case kFormType_WEAP:
							SAKEData::LoadOverrides_Weapon((TESObjectWEAP*)curForm, curOverride, iDebugLevel);
							break;
						case kFormType_NPC_:
							SAKEData::LoadOverrides_Actor((TESNPC*)curForm, curOverride, iDebugLevel);
							break;
						case kFormType_AMMO:
							SAKEData::LoadOverrides_Ammo((TESAmmo*)curForm, curOverride, iDebugLevel);
							break;
						case kFormType_MISC:
							SAKEData::LoadOverrides_Misc((TESObjectMISC*)curForm, curOverride, iDebugLevel);
							break;
						case kFormType_CMPO:
							SAKEData::LoadOverrides_Component((BGSComponent*)curForm, curOverride, iDebugLevel);
							break;
						case kFormType_ALCH:
							SAKEData::LoadOverrides_Ingestible((AlchemyItem*)curForm, curOverride, iDebugLevel);
							break;

						default:
							_MESSAGE("    WARNING: Form 0x%08X has invalid formType %i", curForm->formID, curForm->formType);
					}
				}
			}
		}
		else {
			// pass 2 - name prefixes
			for (json::iterator itOverride = otObject["namePrefixes"].begin(); itOverride != otObject["namePrefixes"].end(); ++itOverride) {
				curOverride.clear();
				curOverride = *itOverride;
				if (!curOverride["prefix"].is_null() && !curOverride["forms"].is_null() && curOverride["forms"].is_array() && !curOverride["forms"].empty()) {
					std::string prefixStr = curOverride["prefix"];
					for (json::iterator itForms = curOverride["forms"].begin(); itForms != curOverride["forms"].end(); ++itForms) {
						std::string curFormID = *itForms;
						TESForm * curForm = SAKEUtilities::GetFormFromIdentifier(curFormID);
						if (curForm) {
							SAKEData::LoadNamePrefix(curForm, prefixStr, iDebugLevel);
						}
					}
				}
			}
		}

		otObject.clear();
		otFile.close();
		return -1;
	}


	// reads the template at jsonPath, loading all found override files
	inline int LoadTemplate(const std::string & jsonPath)
	{
		if (iDebugLevel != 0) {
			_MESSAGE("\n\nLoading Template: %s", jsonPath.c_str());
		}
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


	// reads the profile at activeProfile from profilesPath, loading its templates and override files found at templatesPath
	inline int LoadProfile(const std::string & activeProfile, const std::string & profilesPath, const std::string & templatesPath)
	{
		std::string curProfile = profilesPath.c_str();
		//curProfile.append(profilesPath);
		curProfile.append(activeProfile);
		curProfile.append(".json");

		if (iDebugLevel != 0) {
			_MESSAGE("\n\nLoading Profile: %s", curProfile.c_str());
		}
		if (!SAKEUtilities::IsPathValid(curProfile)) {
			return 0;
		}
		json profileObject;
		std::ifstream profileFile(curProfile.c_str());
		profileFile >> profileObject;
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

		for (; iPassCount < 3; iPassCount++) {
			_MESSAGE("\n\nStarting pass %i", iPassCount);
			for (json::iterator itDir = overridesList.begin(); itDir != overridesList.end(); ++itDir) {
				includeObj.clear();
				excludeObj.clear();
				templateObj.clear();
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
				otPath.append(templatesPath.c_str());
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
					switch (LoadTemplate(otPath)) {
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
		profileObject.clear();
		profileFile.close();
		return -1;
	}

}


// temp form decoding + dev stuff
void TestingStuff()
{
	//	TESLevItem, TESLevCharacter decoding:
	//		.leveledList.unk08 = UseGlobal
	//			get global: (TESGlobal*)levlItem/Char->leveledList.unk08
	//			set global: (UInt64)globalVar
	//		.leveledList.unk2A = ChanceNone
	//		.leveledlist.entries[n].unk8 = ChanceNone

	// leveled item: Container_Loot_Raider_Safe [LVLI:001B8812]
	// test2: LL_Ammo_Cannonball [LVLI:0018ABE4]
	TESLevItem * testLL = (TESLevItem*)SAKEUtilities::GetFormFromIdentifier("Fallout4.esm|18ABE4");
	if (testLL) {
		TESGlobal * tempGlobal = (TESGlobal*)testLL->leveledList.unk08;
		_MESSAGE("\n\norig.global: 0x%016X, tempGlobal = 0x%016X", testLL->leveledList.unk08, tempGlobal ? (UInt64)tempGlobal : 0x0);
		_MESSAGE("\nLLTest: LL_Ammo_Cannonball:\n  length = %i, unk18 = 0x%04X, unk1B = 0x%02X, leveledlist.UseGlobal (unk08) = 0x%08X, leveledlist.ChanceNone (unk2A) = %i, leveledlist.unk2C = 0x%08X, pad1C = 0x%08X", testLL->leveledList.length, testLL->unk18, testLL->unk1B, (tempGlobal ? tempGlobal->formID : 0x0), testLL->leveledList.unk2A, testLL->leveledList.unk2C);
		for (UInt8 i = 0; i < testLL->leveledList.length; i++) {
			_MESSAGE("  %i: unk08 = 0x%08X, ChanceNone (unk8) = %i", i, (testLL->leveledList.entries[i].unk08 ? testLL->leveledList.entries[i].unk08 : 0x0), testLL->leveledList.entries[i].unk8);
		}
	}

	// leveled actor: LCharScavenger [LVLN:0002BEA0]
	TESLevCharacter * testLLC = (TESLevCharacter*)SAKEUtilities::GetFormFromIdentifier("Fallout4.esm|2BEA0");
	if (testLLC) {
		_MESSAGE("\nLLTest: LCharScavenger:\n  length = %i, unk18 = 0x%04X, unk1B = 0x%02X, leveledlist.unk08 = 0x%016X, leveledlist.unk2A = 0x%02X, leveledlist.unk2C = 0x%08X", testLLC->leveledList.length, testLLC->unk18, testLLC->unk1B, testLLC->leveledList.unk08, testLLC->leveledList.unk2A, testLLC->leveledList.unk2C);
		for (UInt8 i = 0; i < testLLC->leveledList.length; i++) {
			_MESSAGE("  %i: unk08 = 0x%08X, ChanceNone (unk8) = %i", i, (testLLC->leveledList.entries[i].unk08 ? testLLC->leveledList.entries[i].unk08 : 0x0), testLLC->leveledList.entries[i].unk8);
		}
	}

	// ammo: AmmoFusionCore "Fusion Core" [AMMO:00075FE4]
	//		TESAmmo.unk160[0] = projectile (BGSProjectile*)
	//		TESAmmo.unk160[1] = health (UInt)
	//		TESAmmo.unk160[2] = damage (UInt64)(float)
	TESAmmo * testAmmo = (TESAmmo*)SAKEUtilities::GetFormFromIdentifier("Fallout4.esm|75FE4");
	_MESSAGE("\nAmmoTest: AmmoFusionCore:\n  unk18 = 0x%04X, unk1B = 0x%02X, pad1C = 0x%08X", testAmmo->unk18, testAmmo->unk1B, testAmmo->pad1C);
	for (UInt8 i = 0; i < 10; i++) {
		_MESSAGE("  unk160[%i] = 0x%016X", i, testAmmo->unk160[i]);
	}

	// ammo: AmmoBloodBug "Blood Spray" [AMMO:00031FB7]
	TESAmmo * testAmmo2 = (TESAmmo*)SAKEUtilities::GetFormFromIdentifier("Fallout4.esm|31FB7");
	_MESSAGE("\nAmmoTest2: AmmoBloodBug:\n  unk18 = 0x%04X, unk1B = 0x%02X, pad1C = 0x%08X", testAmmo2->unk18, testAmmo2->unk1B, testAmmo2->pad1C);
	for (UInt8 i = 0; i < 10; i++) {
		_MESSAGE("  unk160[%i] = 0x%016X", i, testAmmo2->unk160[i]);
	}

	// .esl compatibility test
	TESObjectWEAP * testWeap = (TESObjectWEAP*)SAKEUtilities::GetFormFromIdentifier("ccfrsfo4002-antimaterielrifle.esl|F9C");
	if (testWeap) {
		_MESSAGE("\nFound weapon: name = %s", testWeap->fullName.name.c_str());
	}
	else {
		_MESSAGE("\nCan't find AMR");
	}

}



// starts the loading process
void SAKEFileReader::LoadGameData()
{
	json mainConfig;
	std::ifstream cf(".\\Data\\F4SE\\Plugins\\SAKE.json");
	cf >> mainConfig;

	if (mainConfig.is_null()) {
		_MESSAGE("\n\nERROR: Unable to load main config file!");
		cf.close();
		return;
	}

	std::string profilesPath = ".\\Data\\SAKE\\Profiles\\";
	std::string templatesPath = ".\\Data\\SAKE\\Templates\\";
	std::string activeProfile = "Default";

	if (!mainConfig["templatesPath"].is_null()) {
		std::string tempTemplatesStr = mainConfig["templatesPath"];
		templatesPath.clear();
		templatesPath.append(tempTemplatesStr);
	}
	if (!mainConfig["profilesPath"].is_null()) {
		std::string tempProfilesStr = mainConfig["profilesPath"];
		profilesPath.clear();
		profilesPath.append(tempProfilesStr);
	}
	if (!mainConfig["activeProfile"].is_null()) {
		std::string tempActiveProfileStr = mainConfig["activeProfile"];
		activeProfile.clear();
		activeProfile.append(tempActiveProfileStr);
	}
	if (!mainConfig["debugLogLevel"].is_null()) {
		iDebugLevel = mainConfig["debugLogLevel"];
	}

	_MESSAGE("Loaded main configuration file...\n  templatesPath: %s\n  profilesPath: %s\n  debugLogLevel: %i\n  activeProfile: %s", templatesPath.c_str(), profilesPath.c_str(), iDebugLevel, activeProfile.c_str());

	switch (SAKEFileReader::LoadProfile(activeProfile, profilesPath, templatesPath)) {
		case -1:
			_MESSAGE("\n\nFinished loading Profile!");
			break;
		case 0:
			_MESSAGE("  ERROR: Invalid Profile path...");
			break;
		case 1:
			_MESSAGE("  ERROR: Profile contains no active templates!");
			break;
		case 2:
			_MESSAGE("  ERROR: Invalid Profile json file!");
			break;
	}
	
	mainConfig.clear();
	cf.close();

	
	//TestingStuff();
}

