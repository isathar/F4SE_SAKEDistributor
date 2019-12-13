#include "SAKELoader.h"
#include "nlohmann/json.hpp"
#include <Windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

using json = nlohmann::json;


namespace SAKEUtilities
{
	// returns a form's plugin name or "Ref" if it's not a base form
	const char * GetPluginNameFromFormID(UInt32 formID)
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
	const char * GetIdentifierFromFormID(UInt32 formID)
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
	UInt32 GetFormIDFromIdentifier(const std::string & formIdentifier)
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
	TESForm * GetFormFromIdentifier(const std::string & formIdentifier)
	{
		UInt32 formId = GetFormIDFromIdentifier(formIdentifier);
		return (formId > 0x0) ? LookupFormByID(formId) : nullptr;
	}

	// returns true if the passed mod is loaded
	bool IsModLoaded(const std::string & modName)
	{
		return ((*g_dataHandler)->GetLoadedModIndex(modName.c_str()) != (UInt8)-1);
	}

	// returns a list of json file names at the passed path
	std::vector<std::string> GetFileNames(const std::string & folder)
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


	bool IsPathValid(const std::string & path)
	{
		std::string search_path = path + "*";
		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
		return (hFind != INVALID_HANDLE_VALUE);
	}

}



namespace SAKEData
{
	int iDebugLevel = 0;


	// ** Modifies Spell lists for Races and Actors
	bool LoadData_Spells(ATSpellListEntries * spellData, json & spellsList)
	{
		tArray<SpellItem*> tempSpells;
		bool bUpdateSpells = false;
		// copy any existing spells
		UInt32 iNumSpells = spellData->numSpells;
		if (iNumSpells != 0) {
			for (UInt32 i = 0; i < iNumSpells; i++) {
				tempSpells.Push(spellData->spells[i]);
			}
		}
		// add new spells
		json curSpellObj;
		for (json::iterator itSpells = spellsList.begin(); itSpells != spellsList.end(); ++itSpells) {
			curSpellObj.clear();
			curSpellObj = *itSpells;
			if (!curSpellObj["formID"].is_null()) {
				std::string curSpellID = curSpellObj["formID"];
				SpellItem * newSpell = (SpellItem*)SAKEUtilities::GetFormFromIdentifier(curSpellID);
				if (newSpell) {
					if (iDebugLevel == 2) {
						_MESSAGE("        Adding Spell: %s", curSpellID.c_str());
					}
					tempSpells.Push(newSpell);
					bUpdateSpells = true;
				}
			}
		}
		// rebuild the spell list
		iNumSpells = tempSpells.count;
		if (bUpdateSpells && (iNumSpells != 0)) {
			spellData->numSpells = iNumSpells;
			spellData->spells = new SpellItem*[iNumSpells];
			for (UInt32 j = 0; j < iNumSpells; j++) {
				spellData->spells[j] = tempSpells[j];
			}
		}
		tempSpells.Clear();

		return true;
	}


	// ** Modifies Keyword lists used by most Forms
	bool LoadData_KeywordForm(BGSKeywordForm * keywordForm, json & keywordData)
	{
		if (!keywordForm) {
			return false;
		}

		std::vector<UInt32> keywordsToAdd, keywordsToRemove, finalKeywordsList;
		// -- get keywords to remove
		if (keywordData["remove"].is_array() && !keywordData["remove"].empty()) {
			json kwRemoveList = keywordData["remove"];
			UInt32 remKWID = 0;
			for (json::iterator itRemove = kwRemoveList.begin(); itRemove != kwRemoveList.end(); ++itRemove) {
				std::string remKWStr = *itRemove;
				remKWID = SAKEUtilities::GetFormIDFromIdentifier(remKWStr.c_str());
				if (remKWID != 0) {
					keywordsToRemove.push_back(remKWID);
				}
				if (iDebugLevel == 2) {
					_MESSAGE("        Removing keyword %s", remKWStr.c_str());
				}
			}
		}

		// -- get keywords to add
		if (keywordData["add"].is_array() && !keywordData["add"].empty()) {
			json kwAddList = keywordData["add"];
			UInt32 addKWID = 0;
			for (json::iterator itAdd = kwAddList.begin(); itAdd != kwAddList.end(); ++itAdd) {
				std::string addKWStr = *itAdd;
				addKWID = SAKEUtilities::GetFormIDFromIdentifier(addKWStr.c_str());
				if (addKWID != 0) {
					keywordsToAdd.push_back(addKWID);
				}
				if (iDebugLevel == 2) {
					_MESSAGE("        Adding keyword %s", addKWStr.c_str());
				}
			}
		}

		// -- keywords list processing:
		if (!keywordsToAdd.empty() || !keywordsToRemove.empty()) {
			UInt32 baseKWCount = keywordForm->numKeywords;
			if (!keywordsToRemove.empty()) {
				// copy keywords, processing the remove list
				for (UInt32 j = 0; j < baseKWCount; j++) {
					BGSKeyword * tempKW = keywordForm->keywords[j];
					if (tempKW) {
						if (std::find(keywordsToRemove.begin(), keywordsToRemove.end(), tempKW->formID) == keywordsToRemove.end()) {
							finalKeywordsList.push_back(tempKW->formID);
						}
					}
				}
			}
			else {
				// just copy keywords
				for (UInt32 j = 0; j < baseKWCount; j++) {
					BGSKeyword * tempKW = keywordForm->keywords[j];
					if (tempKW) {
						finalKeywordsList.push_back(tempKW->formID);
					}
				}
			}
			// process add list
			if (!keywordsToAdd.empty()) {
				for (UInt32 j = 0; j < keywordsToAdd.size(); j++) {
					finalKeywordsList.push_back(keywordsToAdd[j]);
				}
			}
			// actually edit keywords list if needed
			if (!finalKeywordsList.empty()) {
				UInt32 kwCount = finalKeywordsList.size();
				keywordForm->numKeywords = kwCount;
				keywordForm->keywords = new BGSKeyword*[kwCount];
				for (UInt8 j = 0; j < kwCount; j++) {
					BGSKeyword * newKW = (BGSKeyword*)LookupFormByID(finalKeywordsList[j]);
					if (newKW) {
						keywordForm->keywords[j] = newKW;
					}
				}
				return true;
			}
		}
		return false;
	}


	// ** Modifies DamageTypes lists used by Weapons and Armors
	bool LoadData_DamageTypes(tArray<TBO_InstanceData::DamageTypes> * damageTypes, json & damageTypeData, bool clearExisting)
	{
		if (damageTypeData.empty()) {
			return false;
		}
		
		if (!damageTypes || clearExisting) {
			damageTypes = new tArray<TBO_InstanceData::DamageTypes>();
			if (clearExisting) {
				if (iDebugLevel == 2) {
					_MESSAGE("        Clearing damage types...");
				}
			}
		}

		UInt32 dtID = 0;
		json dtObj;
		for (json::iterator itDT = damageTypeData.begin(); itDT != damageTypeData.end(); ++itDT) {
			dtObj.clear();
			dtObj = *itDT;
			if (!dtObj["formID"].is_null()) {
				std::string dtIDStr = dtObj["formID"];
				dtID = SAKEUtilities::GetFormIDFromIdentifier(dtIDStr.c_str());
				int iDTSet = -1;
				if (!dtObj["set"].is_null()) {
					iDTSet = dtObj["set"];
				}
				int iDTAdd = 0;
				if (!dtObj["add"].is_null()) {
					iDTAdd = dtObj["add"];
				}

				// check if the damageType is already in the list, remove it
				if (damageTypes->count != 0) {
					for (UInt32 j = damageTypes->count; j >= 0; j--) {
						TBO_InstanceData::DamageTypes checkDT;
						if (damageTypes->GetNthItem(j, checkDT)) {
							if (checkDT.damageType->formID == dtID) {
								// save existing value if set isn't used
								if (iDTSet == -1) {
									iDTSet = checkDT.value;
								}
								if (iDebugLevel == 2) {
									_MESSAGE("        Found existing damage type: %s, value: %i", dtIDStr.c_str(), checkDT.value);
								}
								damageTypes->Remove(j);
								break;
							}
						}
					}
				}

				// add the damageType to the list
				int finalDTVal = (iDTSet > -1) ? (iDTSet + iDTAdd) : iDTAdd;
				if (finalDTVal > 0) {
					BGSDamageType * tempDT = (BGSDamageType*)LookupFormByID(dtID);
					if (tempDT) {
						TBO_InstanceData::DamageTypes newDT;
						newDT.damageType = tempDT;
						newDT.value = (UInt32)finalDTVal;
						damageTypes->Push(newDT);
						if (iDebugLevel == 2) {
							_MESSAGE("        Adding damage type: %s, set: %i, add: %i", dtIDStr.c_str(), iDTSet, iDTAdd);
						}
					}
				}
			}
		}

		return true;
	}


	// ******** Weapon Form Overrides
	bool LoadOverrides_WeaponBase(json & weaponOverride)
	{
		if (weaponOverride["formID"].is_null()) {
			_MESSAGE("\n      ERROR: Weapon Override doesn't contain a formID!");
			return false;
		}
		std::string weaponID = weaponOverride["formID"];
		TESObjectWEAP *weaponBase = (TESObjectWEAP*)SAKEUtilities::GetFormFromIdentifier(weaponID);
		if (!weaponBase) {
			_MESSAGE("\n      ERROR: Weapon form not found!  (%s)", weaponID.c_str());
			return false;
		}
		if (iDebugLevel != 0) {
			_MESSAGE("\n      Weapon ID: %s", weaponID.c_str());
		}

		// -------- Base Form:

		// ---- name
		if (!weaponOverride["name"].is_null()) {
			std::string weaponName = weaponOverride["name"];
			weaponBase->fullName.name = BSFixedString(weaponName.c_str());
			if (iDebugLevel == 2) {
				_MESSAGE("        Edited base name: %s", weaponName.c_str());
			}
		}

		// ---- instance naming rules
		if (!weaponOverride["instanceNamingRules"].is_null()) {
			std::string inrID = weaponOverride["instanceNamingRules"];
			BGSInstanceNamingRules * newNamingRules = (BGSInstanceNamingRules*)SAKEUtilities::GetFormFromIdentifier(inrID.c_str());
			//if (newNamingRules) {
				weaponBase->namingRules.rules = newNamingRules;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited naming rules: %s", inrID.c_str());
				}
			//}
		}

		// ---- keywords
		if (!weaponOverride["keywords"].is_null()) {
			json keywordsObject = weaponOverride["keywords"];
			if (LoadData_KeywordForm(&weaponBase->keyword, keywordsObject)) {
				if (iDebugLevel == 2) {
					_MESSAGE("        Final Keywords list:");
					for (UInt32 j = 0; j < weaponBase->keyword.numKeywords; j++) {
						_MESSAGE("          %i: 0x%08X", j, weaponBase->keyword.keywords[j]->formID);
					}
				}
			}
		}


		// -------- InstanceData - anything past here is only processed for weapons with omod templates:
		TESObjectWEAP::InstanceData *instanceData = &weaponBase->weapData;
		if (!instanceData) {
			_MESSAGE("\n      WARNING: Weapon has no instanceData!");
			return true;
		}

		// ---- base damage
		if (!weaponOverride["damage"].is_null()) {
			int iBaseDamage = weaponOverride["damage"];
			if (iBaseDamage > 0) {
				instanceData->baseDamage = (UInt16)iBaseDamage;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited base damage: %i", iBaseDamage);
				}
			}
		}

		// ---- damageTypes
		if (weaponOverride["damageTypes"].is_array() && !weaponOverride["damageTypes"].empty()) {
			json damageTypesObject = weaponOverride["damageTypes"];
			if (!instanceData->damageTypes) {
				instanceData->damageTypes = new tArray<TBO_InstanceData::DamageTypes>();
			}
			if (LoadData_DamageTypes(instanceData->damageTypes, damageTypesObject, false)) {
				if (iDebugLevel == 2) {
					if (instanceData->damageTypes && (int)instanceData->damageTypes->count > 0) {
						_MESSAGE("        Final Damage Types list:");
						for (UInt32 j = 0; j < instanceData->damageTypes->count; j++) {
							TBO_InstanceData::DamageTypes checkDT;
							if (instanceData->damageTypes->GetNthItem(j, checkDT)) {
								_MESSAGE("          %i:  0x%08X - %i", j, checkDT.damageType->formID, checkDT.value);
							}
						}
					}
					else {
						_MESSAGE("        Final Damage Types list: Empty");
					}
				}
			}
			else {
				_MESSAGE("        WARNING: LoadData_DamageTypes returned false");
			}
		}

		// ---- secondary/bash damage 
		if (!weaponOverride["secondaryDamage"].is_null()) {
			float fSecDmg = weaponOverride["secondaryDamage"];
			if (fSecDmg > 0.0) {
				instanceData->secondary = fSecDmg;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited secondary damage: %.2f", fSecDmg);
				}
			}
		}

		// ---- AP cost
		if (!weaponOverride["apCost"].is_null()) {
			float fAPCost = weaponOverride["apCost"];
			if (fAPCost > 0.0) {
				instanceData->actionCost = fAPCost;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited AP cost: %.2f", fAPCost);
				}
			}
		}

		// ---- crit charge multiplier
		if (!weaponOverride["critChargeMult"].is_null()) {
			float fCritChargeMult = weaponOverride["critChargeMult"];
			if (fCritChargeMult > 0.0) {
				instanceData->critChargeBonus = fCritChargeMult;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited crit charge multiplier: %.2f", fCritChargeMult);
				}
			}
		}

		// ---- crit damage multiplier
		if (!weaponOverride["critDamageMult"].is_null()) {
			float fCritDmgMult = weaponOverride["critDamageMult"];
			if (fCritDmgMult > 0.0) {
				instanceData->critDamageMult = fCritDmgMult;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited crit damage multiplier: %.2f", fCritDmgMult);
				}
			}
		}

		// ---- range
		// -- min
		if (!weaponOverride["rangeMin"].is_null()) {
			float fMinRange = weaponOverride["rangeMin"];
			if (fMinRange > 0.0) {
				instanceData->minRange = fMinRange;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited minimum range: %.2f", fMinRange);
				}
			}
		}
		// -- max
		if (!weaponOverride["rangeMax"].is_null()) {
			float fMaxRange = weaponOverride["rangeMax"];
			if (fMaxRange > 0.0) {
				instanceData->minRange = fMaxRange;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited maximum range: %.2f", fMaxRange);
				}
			}
		}

		// ---- out of range damage multiplier
		if (!weaponOverride["outOfRangeMult"].is_null()) {
			float fOoRMult = weaponOverride["outOfRangeMult"];
			if (fOoRMult > 0.0) {
				instanceData->outOfRangeMultiplier = fOoRMult;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited oor damage mult: %.2f", fOoRMult);
				}
			}
		}

		// ---- attack delay
		if (!weaponOverride["attackDelay"].is_null()) {
			float fAttackDelay = weaponOverride["attackDelay"];
			if (fAttackDelay > 0.0) {
				instanceData->attackDelay = fAttackDelay;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited attack delay: %.2f", fAttackDelay);
				}
			}
		}

		// ---- speed multiplier
		if (!weaponOverride["speedMult"].is_null()) {
			float fSpeedMult = weaponOverride["speedMult"];
			if (fSpeedMult > 0.0) {
				instanceData->speed = fSpeedMult;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited speed mult: %.2f", fSpeedMult);
				}
			}
		}

		// ---- reach
		if (!weaponOverride["reach"].is_null()) {
			float fReach = weaponOverride["reach"];
			if (fReach > 0.0) {
				instanceData->reach = fReach;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited reach: %.2f", fReach);
				}
			}
		}

		// ---- stagger
		if (!weaponOverride["stagger"].is_null()) {
			int iStagger = weaponOverride["stagger"];
			if (iStagger >= 0) {
				instanceData->stagger = (UInt32)iStagger;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited stagger: %i", iStagger);
				}
			}
		}

		// ---- value
		if (!weaponOverride["value"].is_null()) {
			int iBaseValue = weaponOverride["value"];
			if (iBaseValue > 0) {
				instanceData->value = (UInt32)iBaseValue;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited value: %i", iBaseValue);
				}
			}
		}

		// ---- weight
		if (!weaponOverride["weight"].is_null()) {
			float fWeight = weaponOverride["weight"];
			if (fWeight > 0.0) {
				instanceData->weight = fWeight;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited weight: %.2f", fWeight);
				}
			}
		}

		// ---- ammo
		if (!weaponOverride["ammo"].is_null()) {
			std::string ammoID = weaponOverride["ammo"];
			TESAmmo * newAmmo = (TESAmmo*)SAKEUtilities::GetFormFromIdentifier(ammoID.c_str());
			if (newAmmo) {
				instanceData->ammo = newAmmo;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited ammo form: %s", ammoID.c_str());
				}
			}
		}

		// ---- NPC ammo leveled list - can be none
		if (!weaponOverride["npcAmmoLeveledList"].is_null()) {
			std::string ammoListID = weaponOverride["npcAmmoLeveledList"];
			TESLevItem * newAmmoList = (TESLevItem*)SAKEUtilities::GetFormFromIdentifier(ammoListID.c_str());
			instanceData->addAmmoList = newAmmoList;
			if (iDebugLevel == 2) {
				_MESSAGE("        Edited NPC ammo list form: %s", ammoListID.c_str());
			}
		}

		// ---- impactDataSet
		if (!weaponOverride["impactDataSet"].is_null()) {
			std::string impactsID = weaponOverride["impactDataSet"];
			BGSImpactDataSet * newImpacts = (BGSImpactDataSet*)SAKEUtilities::GetFormFromIdentifier(impactsID.c_str());
			if (newImpacts) {
				instanceData->unk58 = newImpacts;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited impactDataSet form: %s", impactsID.c_str());
				}
			}
		}

		// ---- Enchantments
		if (weaponOverride["enchanments"].is_array() && !weaponOverride["enchanments"].empty()) {
			if (!instanceData->enchantments) {
				instanceData->enchantments = new tArray<EnchantmentItem*>();
			}
			for (json::iterator itEnch = weaponOverride["enchanments"].begin(); itEnch != weaponOverride["enchanments"].end(); ++itEnch) {
				json enchObj = *itEnch;
				if (!enchObj["formID"].is_null()) {
					std::string enchID = enchObj["formID"];
					EnchantmentItem * newEnchantment = (EnchantmentItem*)SAKEUtilities::GetFormFromIdentifier(enchID.c_str());
					if (newEnchantment) {
						instanceData->enchantments->Push(newEnchantment);
						if (iDebugLevel == 2) {
							_MESSAGE("        Adding Enchantment: %s", enchID.c_str());
						}
					}
				}
			}
			if (iDebugLevel == 2) {
				if (instanceData->enchantments->count != 0) {
					_MESSAGE("        Final Enchantments list:");
					for (UInt32 j = 0; j < instanceData->enchantments->count; j++) {
						EnchantmentItem * tempEnch = nullptr;
						if (instanceData->enchantments->GetNthItem(j, tempEnch)) {
							if (tempEnch) {
								_MESSAGE("          %i:  0x%08X", j, tempEnch->formID);
							}
						}
					}
				}
			}
		}

		// ---- ActorValue modifiers
		if (weaponOverride["actorValues"].is_array() && !weaponOverride["actorValues"].empty()) {
			if (!instanceData->modifiers) {
				instanceData->modifiers = new tArray<TBO_InstanceData::ValueModifier>();
			}
			json avObj;
			for (json::iterator itAV = weaponOverride["actorValues"].begin(); itAV != weaponOverride["actorValues"].end(); ++itAV) {
				avObj.clear();
				avObj = *itAV;
				if (!avObj["formID"].is_null()) {
					std::string avID = avObj["formID"];
					ActorValueInfo * newAV = (ActorValueInfo*)SAKEUtilities::GetFormFromIdentifier(avID.c_str());
					if (newAV) {
						int iAVSet = -1;
						if (!avObj["set"].is_null()) {
							iAVSet = avObj["set"];
						}
						int iAVAdd = 0;
						if (!avObj["add"].is_null()) {
							iAVAdd = avObj["add"];
						}

						for (UInt32 j = 0; j < instanceData->modifiers->count; j++) {
							TBO_InstanceData::ValueModifier checkAVMod;
							if (instanceData->modifiers->GetNthItem(j, checkAVMod)) {
								if (checkAVMod.avInfo->formID == newAV->formID) {
									if (iAVSet < 0) {
										iAVSet = checkAVMod.unk08;
									}
									instanceData->modifiers->Remove(j);
									break;
								}
							}
						}

						int newAVValue = (iAVSet < 0) ? iAVAdd : (iAVSet + iAVAdd);
						if (newAVValue > 0) {
							TBO_InstanceData::ValueModifier newAVMod;
							newAVMod.avInfo = newAV;
							newAVMod.unk08 = (UInt32)newAVValue;
							instanceData->modifiers->Push(newAVMod);
							if (iDebugLevel == 2) {
								_MESSAGE("        Adding actorValueMod: %s - set: %i, add: %i", avID.c_str(), iAVSet, iAVAdd);
							}
						}
					}
				}
			}
			if (iDebugLevel == 2) {
				_MESSAGE("        Final ActorValue Modifiers:");
				if ((int)instanceData->modifiers->count > 0) {
					for (UInt32 j = 0; j < instanceData->modifiers->count; j++) {
						TBO_InstanceData::ValueModifier checkAVMod;
						if (instanceData->modifiers->GetNthItem(j, checkAVMod)) {
							if (checkAVMod.avInfo) {
								_MESSAGE("          %i:  0x%08X - %i", j, checkAVMod.avInfo->formID, checkAVMod.unk08);
							}
						}
					}
				}
			}
		}

		// ---- Flags:
		if (instanceData->flags && !weaponOverride["flags"].is_null()) {
			json flagsObj = weaponOverride["flags"];
			// -- NPCs Use Ammo
			if (!flagsObj["npcsUseAmmo"].is_null()) {
				bool bUseAmmo = flagsObj["npcsUseAmmo"];
				if (bUseAmmo) {
					instanceData->flags |= wFlag_NPCsUseAmmo;
					if (iDebugLevel == 2) {
						_MESSAGE("        Enabled NPCsUseAmmo");
					}
				}
				else {
					instanceData->flags &= ~wFlag_NPCsUseAmmo;
					if (iDebugLevel == 2) {
						_MESSAGE("        Disabled NPCsUseAmmo");
					}
				}
			}
		}

		// -------- AimModel - guns only:
		if (instanceData->aimModel && !weaponOverride["aimModel"].is_null()) {
			json aimModelObject = weaponOverride["aimModel"];

			// ---- AimModel Form
			if (!aimModelObject["formID"].is_null()) {
				std::string aimmodelID = aimModelObject["formID"];
				BGSAimModel * newAimModel = (BGSAimModel*)SAKEUtilities::GetFormFromIdentifier(aimmodelID.c_str());
				if (newAimModel) {
					instanceData->aimModel = newAimModel;
					if (iDebugLevel == 2) {
						_MESSAGE("        Edited Aim Model Form: %s", aimmodelID.c_str());
					}
				}
			}

			ATAimModel *aimModel = (ATAimModel*)instanceData->aimModel;
			if (aimModel) {
				// ---- Cone of Fire:
				if (!aimModelObject["coneOfFire"].is_null()) {
					json cofObj = aimModelObject["coneOfFire"];
					// -- min angle
					float fCoFMin = 0.0;
					if (!cofObj["minAngle"].is_null()) {
						fCoFMin = cofObj["minAngle"];
						if (fCoFMin > 0.0) {
							aimModel->CoF_MinAngle = fCoFMin;
							if (iDebugLevel == 2) {
								_MESSAGE("        Edited AimModel: Cone of Fire - Min Angle: %.2f", fCoFMin);
							}
						}
					}
					else {
						// set to current for max angle value check
						fCoFMin = aimModel->CoF_MinAngle;
					}
					// -- max angle
					if (!cofObj["maxAngle"].is_null()) {
						float fCoFMax = cofObj["maxAngle"];
						if (fCoFMax > 0.0) {
							if (fCoFMax < fCoFMin) {
								fCoFMax = fCoFMin;
							}
							aimModel->CoF_MaxAngle = fCoFMax;
							if (iDebugLevel == 2) {
								_MESSAGE("        Edited AimModel: Cone of Fire - Max Angle: %.2f", fCoFMax);
							}
						}
					}
					// -- increase per shot
					if (!cofObj["increasePerShot"].is_null()) {
						float fCoFIncPerShot = cofObj["increasePerShot"];
						if (fCoFIncPerShot >= 0.0) {
							aimModel->CoF_IncrPerShot = fCoFIncPerShot;
							if (iDebugLevel == 2) {
								_MESSAGE("        Edited AimModel: Cone of Fire - Increase per Shot: %.2f", fCoFIncPerShot);
							}
						}
					}
					// -- decrease per second
					if (!cofObj["decreasePerSec"].is_null()) {
						float fCoFDecPerSec = cofObj["decreasePerSec"];
						if (fCoFDecPerSec >= 0.0) {
							aimModel->CoF_DecrPerSec = fCoFDecPerSec;
							if (iDebugLevel == 2) {
								_MESSAGE("        Edited AimModel: Cone of Fire - Decrease per Second: %.2f", fCoFDecPerSec);
							}
						}
					}
					// -- decrease delay ms
					if (!cofObj["decreaseDelayMS"].is_null()) {
						int iCoFDecDelayMS = cofObj["decreaseDelayMS"];
						if (iCoFDecDelayMS >= 0) {
							aimModel->CoF_DecrDelayMS = (UInt32)iCoFDecDelayMS;
							if (iDebugLevel == 2) {
								_MESSAGE("        Edited AimModel: Cone of Fire - Decrease Delay ms: %i", iCoFDecDelayMS);
							}
						}
					}
					// -- sneak multiplier
					if (!cofObj["sneakMult"].is_null()) {
						float fCoFSneakMult = cofObj["sneakMult"];
						if (fCoFSneakMult >= 0.0) {
							aimModel->CoF_SneakMult = fCoFSneakMult;
							if (iDebugLevel == 2) {
								_MESSAGE("        Edited AimModel: Cone of Fire - Sneak Multiplier: %.2f", fCoFSneakMult);
							}
						}
					}
					// -- ironsights multiplier
					if (!cofObj["ironSightsMult"].is_null()) {
						float fCoFIronsightsMult = cofObj["ironSightsMult"];
						if (fCoFIronsightsMult >= 0.0) {
							aimModel->CoF_IronSightsMult = fCoFIronsightsMult;
							if (iDebugLevel == 2) {
								_MESSAGE("        Edited AimModel: Cone of Fire - Ironsights Multiplier: %.2f", fCoFIronsightsMult);
							}
						}
					}
					cofObj.clear();
				}

				// -- Recoil:
				if (!aimModelObject["recoil"].is_null()) {
					json recObj = aimModelObject["recoil"];
					// ---- arc degrees
					if (!recObj["arcDegrees"].is_null()) {
						float fArcDegrees = recObj["arcDegrees"];
						if (fArcDegrees > 0.0) {
							aimModel->Rec_ArcMaxDegrees = fArcDegrees;
							if (iDebugLevel == 2) {
								_MESSAGE("        Edited AimModel: Recoil - Arc Degrees (max angle diff.): %.2f", fArcDegrees);
							}
						}
					}
					// ---- arc rotate
					if (!recObj["arcRotate"].is_null()) {
						float fArcRotate = recObj["arcRotate"];
						if (fArcRotate > 0.0) {
							aimModel->Rec_ArcRotate = fArcRotate;
							if (iDebugLevel == 2) {
								_MESSAGE("        Edited AimModel: Recoil - Arc Rotate (base angle): %.2f", fArcRotate);
							}
						}
					}
					// ---- diminish spring force
					if (!recObj["diminishSpringForce"].is_null()) {
						float fRecoilSpringForce = recObj["diminishSpringForce"];
						if (fRecoilSpringForce >= 0.0) {
							aimModel->Rec_DimSpringForce = fRecoilSpringForce;
							if (iDebugLevel == 2) {
								_MESSAGE("        Edited AimModel: Recoil - Diminish Spring Force: %.2f", fRecoilSpringForce);
							}
						}
					}
					// ---- diminish sights mult.
					if (!recObj["diminishSightsMult"].is_null()) {
						float fRecoilDimSightsMult = recObj["diminishSightsMult"];
						if (fRecoilDimSightsMult >= 0.0) {
							aimModel->Rec_DimSightsMult = fRecoilDimSightsMult;
							if (iDebugLevel == 2) {
								_MESSAGE("        Edited AimModel: Recoil - Diminish Sights Mult.: %.2f", fRecoilDimSightsMult);
							}
						}
					}
					// ---- min recoil
					float fRecoilMin = 0.0;
					if (!recObj["minPerShot"].is_null()) {
						fRecoilMin = recObj["minPerShot"];
						if (fRecoilMin >= 0.0) {
							aimModel->Rec_MinPerShot = fRecoilMin;
							if (iDebugLevel == 2) {
								_MESSAGE("        Edited AimModel: Recoil - Min. Per Shot: %.2f", fRecoilMin);
							}
						}
					}
					else {
						// set to current for min recoil per shot value check
						fRecoilMin = aimModel->Rec_MinPerShot;
					}
					// ---- max recoil
					if (!recObj["maxPerShot"].is_null()) {
						float fRecoilMax = recObj["maxPerShot"];
						if (fRecoilMax < fRecoilMin) {
							fRecoilMax = fRecoilMin;
						}
						if (fRecoilMax >= 0.0) {
							aimModel->Rec_MaxPerShot = fRecoilMax;
							if (iDebugLevel == 2) {
								_MESSAGE("        Edited AimModel: Recoil - Max. Per Shot: %.2f", fRecoilMax);
							}
						}
					}
					// ---- hip multiplier
					if (!recObj["hipMult"].is_null()) {
						float fHipMult = recObj["hipMult"];
						if (fHipMult > 0.0) {
							aimModel->Rec_HipMult = fHipMult;
							if (iDebugLevel == 2) {
								_MESSAGE("        Edited AimModel: Recoil - Hip Multiplier: %.2f", fHipMult);
							}
						}
					}
					recObj.clear();
				}
				
			}
			aimModelObject.clear();
		}

		// -------- InstanceData.FiringData - guns only:
		if (instanceData->firingData) {
			// ---- projectile override - can be none
			if (!weaponOverride["projectileOverride"].is_null()) {
				std::string projectileID = weaponOverride["projectileOverride"];
				BGSProjectile * newProjectile = (BGSProjectile*)SAKEUtilities::GetFormFromIdentifier(projectileID.c_str());
				instanceData->firingData->projectileOverride = newProjectile;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited projectile override form: %s", projectileID.c_str());
				}
			}
		}

		return true;
	}


	// **** Armor Form Overrides
	bool LoadOverrides_ArmorBase(json & armorOverride)
	{
		if (armorOverride["formID"].is_null()) {
			_MESSAGE("\n      WARNING: Armor Override doesn't contain a formID!");
			return false;
		}
		std::string armorID = armorOverride["formID"];
		TESObjectARMO *armorBase = (TESObjectARMO*)SAKEUtilities::GetFormFromIdentifier(armorID);
		if (!armorBase) {
			_MESSAGE("\n      WARNING: Armor form not found! (%s)", armorID.c_str());
			return false;
		}
		
		if (iDebugLevel != 0) {
			_MESSAGE("\n      Editing Armor  (%s)", armorID.c_str());
		}

		// ---- Base Form Edits:

		// ---- name
		if (!armorOverride["name"].is_null()) {
			std::string armorName = armorOverride["name"];
			armorBase->fullName.name = BSFixedString(armorName.c_str());
			if (iDebugLevel == 2) {
				_MESSAGE("        Edited base name: %s", armorName.c_str());
			}
		}

		// ---- instance naming rules
		if (!armorOverride["instanceNamingRules"].is_null()) {
			std::string inrID = armorOverride["instanceNamingRules"];
			BGSInstanceNamingRules * newNamingRules = (BGSInstanceNamingRules*)SAKEUtilities::GetFormFromIdentifier(inrID.c_str());
			armorBase->namingRules.rules = newNamingRules;
			if (iDebugLevel == 2) {
				_MESSAGE("        Edited naming rules: %s", inrID.c_str());
			}
		}

		// ---- keywords
		if (!armorOverride["keywords"].is_null()) {
			json keywordsObj = armorOverride["keywords"];
			if (LoadData_KeywordForm(&armorBase->keywordForm, keywordsObj)) {
				if (iDebugLevel == 2) {
					if (armorBase->keywordForm.numKeywords != 0) {
						_MESSAGE("        Final Keywords list:");
						for (UInt32 j = 0; j < armorBase->keywordForm.numKeywords; j++) {
							_MESSAGE("          %i: 0x%08X", j, armorBase->keywordForm.keywords[j]->formID);
						}
					}
				}
			}
		}


		// ---- Armor.InstanceData - anything past here is only processed if the armor has default mods
		TESObjectARMO::InstanceData *instanceData = &armorBase->instanceData;
		if (!instanceData) {
			_MESSAGE("\n      WARNING: Armor has no instanceData! (%s)", armorID.c_str());
			return true;
		}

		// ---- armor rating
		if (!armorOverride["armorRating"].is_null()) {
			int iArmorRating = armorOverride["armorRating"];
			if (iArmorRating > 0) {
				instanceData->armorRating = iArmorRating;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited armor rating: %i", iArmorRating);
				}
			}
		}

		// ---- value
		if (!armorOverride["value"].is_null()) {
			int iBaseValue = armorOverride["value"];
			if (iBaseValue > 0) {
				instanceData->value = iBaseValue;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited value: %i", iBaseValue);
				}
			}
		}

		// ---- weight
		if (!armorOverride["weight"].is_null()) {
			float fWeight = armorOverride["weight"];
			if (fWeight > 0.0) {
				instanceData->weight = fWeight;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited weight: %.2f", fWeight);
				}
			}
		}

		// ---- health
		if (!armorOverride["health"].is_null()) {
			int iBasehealth = armorOverride["health"];
			if (iBasehealth > 0) {
				instanceData->health = iBasehealth;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited health: %i", iBasehealth);
				}
			}
		}

		// ---- damageType resistances
		if (armorOverride["damageResistances"].is_array() && !armorOverride["damageResistances"].empty()) {
			json damageResistsObject = armorOverride["damageResistances"];
			bool clearExisting = false;
			if (!armorOverride["clearExistingDR"].is_null()) {
				clearExisting = armorOverride["clearExistingDR"];
			}
			if (!instanceData->damageTypes) {
				instanceData->damageTypes = new tArray<TBO_InstanceData::DamageTypes>();
			}
			if (LoadData_DamageTypes(instanceData->damageTypes, damageResistsObject, clearExisting)) {
				if (iDebugLevel == 2) {
					if (instanceData->damageTypes->count != 0) {
						_MESSAGE("        Final DR list:");
						for (UInt32 j = 0; j < instanceData->damageTypes->count; j++) {
							TBO_InstanceData::DamageTypes checkDT;
							if (instanceData->damageTypes->GetNthItem(j, checkDT)) {
								_MESSAGE("          %i:  0x%08X - %i", j, checkDT.damageType->formID, checkDT.value);
							}
						}
					}
				}
			}
		}

		return true;
	}


	// **** Race Form Overrides
	bool LoadOverrides_Race(json & raceOverride)
	{
		if (raceOverride["formID"].is_null()) {
			_MESSAGE("\n      ERROR: Race Override doesn't contain a formID!");
			return false;
		}
		std::string raceID = raceOverride["formID"];
		TESRace * targetRace = (TESRace*)SAKEUtilities::GetFormFromIdentifier(raceID);
		if (!targetRace) {
			_MESSAGE("\n      ERROR: Race form not found! (%s)", raceID.c_str());
			return false;
		}
		if (iDebugLevel != 0) {
			_MESSAGE("\n      Editing Race  (%s)", raceID.c_str());
		}

		// ---- ActorValues
		if (!raceOverride["actorValues"].is_null()) {
			json avsList = raceOverride["actorValues"];
			if (avsList.is_array() && !avsList.empty()) {
				if (!targetRace->propertySheet.sheet) {
					targetRace->propertySheet.sheet = new tArray<BGSPropertySheet::AVIFProperty>();
				}
				json curAV;
				for (json::iterator itAVs = avsList.begin(); itAVs != avsList.end(); ++itAVs) {
					curAV.clear();
					curAV = *itAVs;
					if (!curAV["formID"].is_null()) {
						std::string avIDStr = curAV["formID"];
						ActorValueInfo * newAV = (ActorValueInfo*)SAKEUtilities::GetFormFromIdentifier(avIDStr);
						if (newAV) {
							float fAVSet = -1.0;
							if (!curAV["set"].is_null()) {
								fAVSet = curAV["set"];
							}
							float fAVAdd = 0.0;
							if (!curAV["add"].is_null()) {
								fAVAdd = curAV["add"];
							}

							bool bFoundAV = false;
							if (targetRace->propertySheet.sheet->count != 0) {
								for (UInt32 i = 0; i < targetRace->propertySheet.sheet->count; i++) {
									BGSPropertySheet::AVIFProperty checkAVProp;
									if (targetRace->propertySheet.sheet->GetNthItem(i, checkAVProp)) {
										if (checkAVProp.actorValue->formID == newAV->formID) {
											checkAVProp.value = max(0.0, ((fAVSet < 0.0) ? (checkAVProp.value + fAVAdd) : (fAVSet + fAVAdd)));
											if (iDebugLevel == 2) {
												_MESSAGE("        Editing ActorValue %s - set: %.2f, add: %.2f", avIDStr.c_str(), fAVSet, fAVAdd);
											}
											bFoundAV = true;
											break;
										}
									}
								}
								if (bFoundAV) {
									continue;
								}
							}

							BGSPropertySheet::AVIFProperty newAVProp;
							newAVProp.actorValue = newAV;
							newAVProp.value = (fAVSet < 0.0) ? fAVAdd : (fAVSet + fAVAdd);
							if (newAVProp.value >= 0.0) {
								if (iDebugLevel == 2) {
									_MESSAGE("        Adding ActorValue %s - set: %.2f, add: %.2f", avIDStr.c_str(), fAVSet, fAVAdd);
								}
								targetRace->propertySheet.sheet->Push(newAVProp);
							}
						}
					}
				}

				if (iDebugLevel == 2) {
					if (targetRace->propertySheet.sheet->count != 0) {
						_MESSAGE("        Final ActorValues list:");
						for (UInt32 i = 0; i < targetRace->propertySheet.sheet->count; i++) {
							BGSPropertySheet::AVIFProperty checkAVProp;
							if (targetRace->propertySheet.sheet->GetNthItem(i, checkAVProp)) {
								_MESSAGE("          %i: 0x%08X - %.2f", i, checkAVProp.actorValue->formID, checkAVProp.value);
							}
						}
					}
				}
				
			}
		}

		// ---- Spells/Abilities
		if (!raceOverride["spells"].is_null()) {
			if (raceOverride["spells"].is_array() && !raceOverride["spells"].empty()) {
				json spellsList = raceOverride["spells"];
				if (!targetRace->spellList.unk08) {
					targetRace->spellList.unk08 = new ATSpellListEntries();
				}
				ATSpellListEntries * spellData = (ATSpellListEntries*)targetRace->spellList.unk08;
				if (spellData) {
					if (LoadData_Spells(spellData, spellsList)) {
						if (spellData->numSpells != 0) {
							if (iDebugLevel == 2) {
								_MESSAGE("        Final Spells list:");
								for (UInt32 j = 0; j < spellData->numSpells; j++) {
									_MESSAGE("          %i: 0x%08X", j, spellData->spells[j]->formID);
								}
							}
						}
					}
				}
			}
		}

		// ---- keywords
		if (!raceOverride["keywords"].is_null()) {
			json keywordsObj = raceOverride["keywords"];
			if (LoadData_KeywordForm(&targetRace->keywordForm, keywordsObj)) {
				if (iDebugLevel == 2) {
					if (targetRace->keywordForm.numKeywords != 0) {
						_MESSAGE("        Final Keywords list:");
						for (UInt32 j = 0; j < targetRace->keywordForm.numKeywords; j++) {
							_MESSAGE("          %i: 0x%08X", j, targetRace->keywordForm.keywords[j]->formID);
						}
					}
				}
			}
		}

		return true;
	}


	// **** Actor Form Overrides
	bool LoadOverrides_ActorBase(json & actorOverride)
	{
		if (actorOverride["formID"].is_null()) {
			_MESSAGE("\n      ERROR: Actor Override doesn't contain a formID!");
			return false;
		}
		std::string actorID = actorOverride["formID"];
		TESActorBase * targetActor = (TESActorBase*)SAKEUtilities::GetFormFromIdentifier(actorID);
		if (!targetActor) {
			_MESSAGE("\n      ERROR: Actor form not found!  (%s)", actorID.c_str());
			return false;
		}
		if (iDebugLevel != 0) {
			_MESSAGE("\n      Editing Actor  (%s)", actorID.c_str());
		}

		// ---- ActorValues
		if (!actorOverride["actorValues"].is_null()) {
			json avsList = actorOverride["actorValues"];
			if (avsList.is_array() && !avsList.empty()) {
				
				json curAV;
				for (json::iterator itAVs = avsList.begin(); itAVs != avsList.end(); ++itAVs) {
					curAV.clear();
					curAV = *itAVs;
					if (!curAV["formID"].is_null()) {
						std::string avIDStr = curAV["formID"];
						ActorValueInfo * newAV = (ActorValueInfo*)SAKEUtilities::GetFormFromIdentifier(avIDStr);
						if (newAV) {
							float fAVSet = -1.0;
							if (!curAV["set"].is_null()) {
								fAVSet = curAV["set"];
							}
							float fAVAdd = 0.0;
							if (!curAV["add"].is_null()) {
								fAVAdd = curAV["add"];
							}

							float newAVValue = (fAVSet < 0.0) ? fAVAdd : (fAVSet + fAVAdd);
							if (newAVValue >= 0.0) {
								_MESSAGE("        Adding ActorValue %s - set: %.2f, add: %.2f", avIDStr.c_str(), fAVSet, fAVAdd);
								targetActor->actorValueOwner.SetBase(newAV, newAVValue);
							}

							if (iDebugLevel == 2) {
								if (targetActor->propertySheet.sheet->count != 0) {
									_MESSAGE("        Final ActorValues list:");
									for (UInt32 i = 0; i < targetActor->propertySheet.sheet->count; i++) {
										BGSPropertySheet::AVIFProperty checkAVProp;
										if (targetActor->propertySheet.sheet->GetNthItem(i, checkAVProp)) {
											_MESSAGE("          %i: 0x%08X - %.2f", i, checkAVProp.actorValue->formID, checkAVProp.value);
										}
									}
								}
							}
						}
					}
				}
			}
		}

		// ---- Spells/Abilities
		if (!actorOverride["spells"].is_null()) {
			if (actorOverride["spells"].is_array() && !actorOverride["spells"].empty()) {
				json spellsList = actorOverride["spells"];
				if (!spellsList.empty()) {
					if (!targetActor->spellList.unk08) {
						targetActor->spellList.unk08 = new ATSpellListEntries();
					}
					ATSpellListEntries * spellData = (ATSpellListEntries*)targetActor->spellList.unk08;
					if (spellData) {
						if (LoadData_Spells(spellData, spellsList)) {
							if (iDebugLevel == 2) {
								if (spellData->numSpells != 0) {
									_MESSAGE("        Final Spells list:");
									for (UInt32 j = 0; j < spellData->numSpells; j++) {
										_MESSAGE("          %i: 0x%08X", j, spellData->spells[j]->formID);
									}
								}
							}
						}
					}
				}
			}
		}

		// ---- keywords
		if (!actorOverride["keywords"].is_null()) {
			json keywordsObj = actorOverride["keywords"];
			if (LoadData_KeywordForm(&targetActor->keywords, keywordsObj)) {
				if (iDebugLevel == 2) {
					if (targetActor->keywords.numKeywords != 0) {
						_MESSAGE("        Final Keywords list:");
						for (UInt32 j = 0; j < targetActor->keywords.numKeywords; j++) {
							_MESSAGE("          %i: 0x%08X", j, targetActor->keywords.keywords[j]->formID);
						}
					}
				}
			}
		}

		return true;
	}


	// **** LeveledItem/LeveledActor/(LeveledSpell?) Form Overrides
	bool LoadOverrides_LeveledList(json & llOverride)
	{
		if (llOverride["formID"].is_null()) {
			_MESSAGE("\n      ERROR: Leveled List Override doesn't contain a formID!");
			return false;
		}
		std::string targetListID = llOverride["formID"];
		TESLevItem * targetLList = (TESLevItem*)SAKEUtilities::GetFormFromIdentifier(targetListID.c_str());
		if (!targetLList) {
			_MESSAGE("\n      ERROR: Leveled List form not found!  (%s)", targetListID.c_str());
			return false;
		}

		if (iDebugLevel != 0) {
			_MESSAGE("\n      Editing Leveled List  (%s)", targetListID.c_str());
		}
		bool bClearList = false, bDelevel = false, bDoCountMult = false, bListModified = false;
		float fCountMult = 0.0;
		std::vector<TESLeveledList::Entry> newEntries;

		// -- check global modifiers
		if (!llOverride["clear"].is_null()) {
			bClearList = llOverride["clear"];
		}
		if (!llOverride["delevel"].is_null()) {
			bDelevel = llOverride["delevel"];
		}
		if (!llOverride["countMult"].is_null()) {
			fCountMult = llOverride["countMult"];
			if (fCountMult > 0.0) {
				bDoCountMult = true;
			}
		}

		if (bClearList) {
			// ---- clear the existing list, no need to check for entries to remove
			if (iDebugLevel == 2) {
				_MESSAGE("        Clearing original entries...");
			}
			bListModified = true;
		} else {
			// ---- copy the base list
			if (targetLList->leveledList.length != 0) {
				json listRemove = llOverride["remove"];
				if (!listRemove.is_null() && !listRemove.empty()) {
					// -- check for and skip everything in the remove list
					json remEntry;
					UInt32 remID = 0;

					bool bRemove = false;
					for (UInt32 i = 0; i < targetLList->leveledList.length; i++) {
						bRemove = false;
						TESLeveledList::Entry curEntry = targetLList->leveledList.entries[i];
						// check the remove list
						for (json::iterator itRemove = listRemove.begin(); itRemove != listRemove.end(); ++itRemove) {
							remEntry.clear();
							remEntry = *itRemove;
							if (!remEntry["formID"].is_null() && !remEntry["level"].is_null() && !remEntry["count"].is_null()) {
								std::string remFormID = remEntry["formID"];
								remID = SAKEUtilities::GetFormIDFromIdentifier(remFormID.c_str());
								// require an exact match for now, maybe add more options later
								if (curEntry.form->formID == remID) {
									UInt16 checkLevel = remEntry["level"];
									UInt16 checkCount = remEntry["count"];
									if (checkLevel == curEntry.level) {
										if (checkCount == curEntry.count) {
											if (iDebugLevel == 2) {
												_MESSAGE("        Removing entry - ID: 0x%08X, level: %i, count: %i", remFormID.c_str(), curEntry.level, curEntry.count);
											}
											bRemove = true;
											break;
										}
									}
								}
							}
						}
						if (bRemove) {
							// this entry is in the remove list, skip it
							continue;
						}
						// apply count multiplier + deleveling
						if (bDoCountMult) {
							curEntry.count = max(1, (int)((float)(int)curEntry.count * fCountMult));
						}
						if (bDelevel) {
							curEntry.level = 1;
						}
						newEntries.push_back(curEntry);
					}
				}
				else {
					// -- nothing to remove, just copy the original list
					if (targetLList->leveledList.length != 0) {
						for (UInt8 i = 0; i < targetLList->leveledList.length; i++) {
							TESLeveledList::Entry curEntry = targetLList->leveledList.entries[i];
							// apply count multiplier + deleveling
							if (bDoCountMult) {
								curEntry.count = max(1, (int)((float)(int)curEntry.count * fCountMult));
							}
							if (bDelevel) {
								curEntry.level = 1;
							}
							newEntries.push_back(curEntry);
							bListModified = true;
						}
					}
				}
			}
		}

		// ---- add new entries
		if (!llOverride["add"].is_null()) {
			json listAdd = llOverride["add"];
			if (!listAdd.empty()) {
				json addEntry;
				for (json::iterator itAdd = listAdd.begin(); itAdd != listAdd.end(); ++itAdd) {
					addEntry.clear();
					addEntry = *itAdd;
					// make sure all variables exist
					if (!addEntry["formID"].is_null() && !addEntry["level"].is_null() && !addEntry["count"].is_null()) {
						std::string entryFormID = addEntry["formID"];
						TESLeveledList::Entry tempEntry;
						tempEntry.form = SAKEUtilities::GetFormFromIdentifier(entryFormID.c_str());
						if (tempEntry.form) {
							tempEntry.count = addEntry["count"];
							if (bDoCountMult) {
								tempEntry.count = max(1, (int)((float)(int)tempEntry.count * fCountMult));
							}
							if (!bDelevel) {
								tempEntry.level = addEntry["level"];
							}
							else {
								tempEntry.level = 1;
							}
							if (iDebugLevel == 2) {
								_MESSAGE("        Adding entry - ID: %s, level: %i, count: %i", entryFormID.c_str(), tempEntry.level, tempEntry.count);
							}
							newEntries.push_back(tempEntry);
							bListModified = true;
						}
					}
				}
			}
		}

		// -- actual leveled list edits:
		if (bListModified) {
			//if (finalCount > 0) {
			if (!newEntries.empty()) {
				// 255 max entries since leveledList.length is an UInt8
				UInt8 finalCount = newEntries.size();
				targetLList->leveledList.entries = new TESLeveledList::Entry[finalCount];
				targetLList->leveledList.length = finalCount;
				if (iDebugLevel == 2) {
					_MESSAGE("        Final Leveled List:");
				}
				for (UInt8 i = 0; i < finalCount; i++) {
					targetLList->leveledList.entries[i] = newEntries[i];
					if (iDebugLevel == 2) {
						_MESSAGE("          %i: 0x%08X - level: %i, count: %i", i, newEntries[i].form->formID, newEntries[i].level, newEntries[i].count);
					}
				}
			}
		}
		return true;
	}


	// **** Ammo Form Overrides
	bool LoadOverrides_Ammo(json & ammoOverride)
	{
		if (ammoOverride["formID"].is_null()) {
			_MESSAGE("\n      ERROR: Ammo Override doesn't contain a formID!");
			return false;
		}
		std::string ammoID = ammoOverride["formID"];
		TESAmmo * ammoForm = (TESAmmo*)SAKEUtilities::GetFormFromIdentifier(ammoID.c_str());
		if (!ammoForm) {
			_MESSAGE("\n      ERROR: Ammo form not found!  (%s)", ammoID.c_str());
			return false;
		}

		if (iDebugLevel != 0) {
			_MESSAGE("\n      Editing Ammo  (%s)", ammoID.c_str());
		}

		if (!ammoOverride["name"].is_null()) {
			std::string ammoName = ammoOverride["name"];
			ammoForm->fullName.name = BSFixedString(ammoName.c_str());
			if (iDebugLevel == 2) {
				_MESSAGE("        Edited Name: %s", ammoForm->fullName.name.c_str());
			}
		}
		if (!ammoOverride["value"].is_null()) {
			int ammoValue = ammoOverride["value"];
			if (ammoValue > -1) {
				ammoForm->value.value = (UInt32)ammoValue;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited Value: %i", ammoForm->value.value);
				}
			}
		}
		if (!ammoOverride["weight"].is_null()) {
			float ammoWeight = ammoOverride["weight"];
			if (ammoWeight >= 0.0) {
				ammoForm->weight.weight = ammoWeight;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited Weight: %f", ammoForm->weight.weight);
				}
			}
		}
		// ---- keywords
		if (!ammoOverride["keywords"].is_null()) {
			json keywordsObj = ammoOverride["keywords"];
			if (LoadData_KeywordForm(&ammoForm->keywordForm, keywordsObj)) {
				if (iDebugLevel == 2) {
					if (ammoForm->keywordForm.numKeywords != 0) {
						_MESSAGE("        Final Keywords list:");
						for (UInt32 j = 0; j < ammoForm->keywordForm.numKeywords; j++) {
							_MESSAGE("          %i: 0x%08X", j, ammoForm->keywordForm.keywords[j]->formID);
						}
					}
				}
			}
		}
		return true;
	}


	// **** Misc/Junk item form overrides
	bool LoadOverrides_Misc(json & miscOverride)
	{
		if (miscOverride["formID"].is_null()) {
			_MESSAGE("\n      ERROR: MiscItem Override doesn't contain a formID!");
			return false;
		}
		std::string miscID = miscOverride["formID"];
		TESObjectMISC * miscForm = (TESObjectMISC*)SAKEUtilities::GetFormFromIdentifier(miscID.c_str());
		if (!miscForm) {
			_MESSAGE("\n      ERROR: MiscItem form not found!  (%s)", miscID.c_str());
			return false;
		}
		if (iDebugLevel != 0) {
			_MESSAGE("\n      Editing MiscItem  (%s)", miscID.c_str());
		}

		// -- name
		if (!miscOverride["name"].is_null()) {
			std::string miscName = miscOverride["name"];
			miscForm->fullName.name = BSFixedString(miscName.c_str());
			if (iDebugLevel == 2) {
				_MESSAGE("        Edited Name: %s", miscForm->fullName.name.c_str());
			}
		}
		// -- value
		if (!miscOverride["value"].is_null()) {
			int miscValue = miscOverride["value"];
			if (miscValue > -1) {
				miscForm->value.value = (UInt32)miscValue;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited Value: %i", miscForm->value.value);
				}
			}
		}
		// -- weight
		if (!miscOverride["weight"].is_null()) {
			float miscWeight = miscOverride["weight"];
			if (miscWeight >= 0.0) {
				miscForm->weight.weight = miscWeight;
				if (iDebugLevel == 2) {
					_MESSAGE("        Edited Weight: %f", miscForm->weight.weight);
				}
			}
		}
		// ---- keywords
		if (!miscOverride["keywords"].is_null()) {
			json keywordsObj = miscOverride["keywords"];
			if (LoadData_KeywordForm(&miscForm->keywordForm, keywordsObj)) {
				if (iDebugLevel == 2) {
					if (miscForm->keywordForm.numKeywords != 0) {
						_MESSAGE("        Final Keywords list:");
						for (UInt32 j = 0; j < miscForm->keywordForm.numKeywords; j++) {
							_MESSAGE("          %i: 0x%08X", j, miscForm->keywordForm.keywords[j]->formID);
						}
					}
				}
			}
		}
		// -- components
		if (!miscOverride["components"].is_null()) {
			json composObj = miscOverride["components"];
			bool clearCompos = false;
			if (!composObj["clearList"].is_null()) {
				clearCompos = composObj["clearList"];
				if (clearCompos && miscForm->components) {
					if (iDebugLevel == 2) {
						_MESSAGE("        Clearing Components list...");
					}
					miscForm->components->Clear();
				}
			}
			
			std::vector<TESObjectMISC::Component> composRemList, composAddList, finalCompos;

			if (miscForm->components && miscForm->components->count != 0) {
				if (!composObj["remove"].is_null()) {
					json composRem = composObj["remove"];
					if (composRem.is_array() && !composRem.empty()) {
						json remEntry;
						for (json::iterator itRem = composRem.begin(); itRem != composRem.end(); ++itRem) {
							remEntry = *itRem;
							if (!remEntry["formID"].is_null() && !remEntry["count"].is_null()) {
								std::string compoID = remEntry["formID"];
								BGSComponent * tempCompo = (BGSComponent*)SAKEUtilities::GetFormFromIdentifier(compoID);
								if (tempCompo) {
									int tempCount = remEntry["count"];
									TESObjectMISC::Component curRem;
									curRem.component = tempCompo;
									curRem.count = (UInt64)tempCount;
									if (iDebugLevel == 2) {
										_MESSAGE("        Removing compo - ID: %s, count: %i", compoID.c_str(), tempCount);
									}
									composRemList.push_back(curRem);
								}
							}
						}
					}
				}
			}

			if (!composObj["add"].is_null()) {
				json composAddObj = composObj["add"];
				if (composAddObj.is_array() && !composAddObj.empty()) {
					json addEntry;
					for (json::iterator itAdd = composAddObj.begin(); itAdd != composAddObj.end(); ++itAdd) {
						addEntry = *itAdd;
						if (!addEntry["formID"].is_null() && !addEntry["count"].is_null()) {
							std::string compoID = addEntry["formID"];
							BGSComponent * tempCompo = (BGSComponent*)SAKEUtilities::GetFormFromIdentifier(compoID);
							if (tempCompo) {
								int tempCount = addEntry["count"];
								TESObjectMISC::Component curAdd;
								curAdd.component = tempCompo;
								curAdd.count = (UInt64)tempCount;
								if (iDebugLevel == 2) {
									_MESSAGE("        Adding compo - ID: %s, count: %i", compoID.c_str(), tempCount);
								}
								composAddList.push_back(curAdd);
							}
						}
					}
				}
			}

			if (!composAddList.empty() || !composRemList.empty()) {
				if (!miscForm->components) {
					miscForm->components = new tArray<TESObjectMISC::Component>;
				}

				if (miscForm->components->count != 0) {
					if (!composRemList.empty()) {
						for (UInt32 i = 0; i < miscForm->components->count; i++) {
							TESObjectMISC::Component tempCompo;
							if (miscForm->components->GetNthItem(i, tempCompo)) {
								bool bFound = false;
								int remCount = 0;
								for (UInt32 j = 0; j < composRemList.size(); j++) {
									if (tempCompo.component->formID == composRemList[j].component->formID) {
										bFound = true;
										remCount = composRemList[j].count;
										break;
									}
								}
								if (bFound) {
									if (remCount < (int)tempCompo.count) {
										tempCompo.count -= remCount;
										finalCompos.push_back(tempCompo);
									}
								}
								else {
									finalCompos.push_back(tempCompo);
								}
							}
						}
					} else {
						for (UInt32 i = 0; i < miscForm->components->count; i++) {
							TESObjectMISC::Component tempCompo;
							if (miscForm->components->GetNthItem(i, tempCompo)) {
								finalCompos.push_back(tempCompo);
							}
						}
					}
				}

				if (!composAddList.empty()) {
					if (finalCompos.size() != 0) {
						for (UInt32 i = 0; i < composAddList.size(); i++) {
							TESObjectMISC::Component tempCompo = composAddList[i];
							bool bFound = false;
							for (UInt32 j = 0; j < finalCompos.size(); j++) {
								if (finalCompos[j].component->formID == tempCompo.component->formID) {
									finalCompos[j].count += tempCompo.count;
									bFound = true;
									break;
								}
							}
							if (!bFound) {
								finalCompos.push_back(tempCompo);
							}
						}
					}
					else {
						for (UInt32 i = 0; i < composAddList.size(); i++) {
							TESObjectMISC::Component tempCompo = composAddList[i];
							finalCompos.push_back(tempCompo);
						}
					}
				}

				miscForm->components->Clear();
				if (finalCompos.size() != 0) {
					for (UInt32 i = 0; i < finalCompos.size(); i++) {
						TESObjectMISC::Component tempCompo = finalCompos[i];
						miscForm->components->Push(tempCompo);
					}
					if (iDebugLevel == 2 && miscForm->components->count != 0) {
						_MESSAGE("        Final Components list:");
						for (UInt32 i = 0; i < miscForm->components->count; i++) {
							TESObjectMISC::Component tempCompo;
							if (miscForm->components->GetNthItem(i, tempCompo)) {
								_MESSAGE("          %i: 0x%08X x%i", i, tempCompo.component->formID, tempCompo.count);
							}
						}
					}
				}
			}
		}
		return true;
	}

}



namespace SAKEFileReader
{
	// first pass = races, then everything else because race AV edits can reset Actor AV edits
	bool bIsFirstPass = true;

	// these loops need to be done quickly to minimize the initial loading delay, so
	// suggest inline for a theoretical performance boost

	// reads the overrides from a json file at jsonPath, returns an error code if it fails
	inline int LoadOverride(const std::string & jsonPath)
	{
		if (SAKEData::iDebugLevel != 0) {
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
		if (otObject["overrides"].is_null()) {
			otObject.clear();
			otFile.close();
			return 2;
		}
		if (!otObject["overrides"].is_array() || otObject["overrides"].empty()) {
			otObject.clear();
			otFile.close();
			return 1;
		}
		if (bIsFirstPass && !otObject["hasRaceEdits"].is_null()) {
			bool bHasRaceEdits = otObject["hasRaceEdits"];
			if (!bHasRaceEdits) {
				_MESSAGE("    INFO: Skipping %s during first pass.", jsonPath.c_str());
				return 5;
			}
		}

		// required plugins check
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

		// load overrides
		json curOverride;
		for (json::iterator itOverride = otObject["overrides"].begin(); itOverride != otObject["overrides"].end(); ++itOverride) {
			curOverride.clear();
			curOverride = *itOverride;
			if (!curOverride["formType"].is_null()) {
				int iFormType = curOverride["formType"];
				if (bIsFirstPass) {
					if (iFormType == 2) {
						SAKEData::LoadOverrides_Race(curOverride);
					}
				}
				else {
					switch (iFormType) {
						case 1:
							SAKEData::LoadOverrides_LeveledList(curOverride);
							break;
						//case 2:
						//	SAKEData::LoadOverrides_Race(curOverride);
						//	break;
						case 3:
							SAKEData::LoadOverrides_ActorBase(curOverride);
							break;
						case 4:
							SAKEData::LoadOverrides_ArmorBase(curOverride);
							break;
						case 5:
							SAKEData::LoadOverrides_WeaponBase(curOverride);
							break;
						case 6:
							SAKEData::LoadOverrides_Ammo(curOverride);
							break;
						case 7:
							SAKEData::LoadOverrides_Misc(curOverride);
							break;
						default:
							_MESSAGE("    WARNING: Invalid formType index: %i", iFormType);
					}
				}
			}
			else {
				_MESSAGE("    WARNING: Missing formType index.\n      Data: %s", curOverride.dump().c_str());
			}
		}
		otObject.clear();
		otFile.close();
		return -1;
	}


	// reads the template at jsonPath, loading all found override files
	inline int LoadTemplate(const std::string & jsonPath)
	{
		if (SAKEData::iDebugLevel != 0) {
			_MESSAGE("\n\nLoading Template: %s", jsonPath.c_str());
		}
		if (!SAKEUtilities::IsPathValid(jsonPath)) {
			return 0;
		}

		std::vector<std::string> templateFiles = SAKEUtilities::GetFileNames(jsonPath.c_str());
		if (templateFiles.empty()) {
			return 1;
		}

		bool bHasReqs = true;
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
		std::string curProfile;
		curProfile.append(profilesPath);
		curProfile.append(activeProfile);
		curProfile.append(".json");

		if (SAKEData::iDebugLevel != 0) {
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

		for (UInt8 i = 1; i < 3; i++) {
			_MESSAGE("\n\nINFO: Starting pass %i", i);
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
			bIsFirstPass = false;
		}
		profileObject.clear();
		profileFile.close();
		return -1;
	}

}


// start loading config files
void SAKEData::LoadGameData()
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
}

