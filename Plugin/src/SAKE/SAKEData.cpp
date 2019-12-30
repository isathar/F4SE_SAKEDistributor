#include "SAKEData.h"


namespace SAKEData
{
	// adds a list of spells to Actor/Race spellLists
	inline void LoadData_Spells(std::vector<SpellItem*> & spellsIn, TESRace * targetRace, TESActorBase * targetActor, UInt8 iDebugLevel)
	{
		// redundant checks just to be safe
		if (spellsIn.empty()) {
			_MESSAGE("        WARNING: No spells to add or remove!");
			return;
		}
		if (!targetRace && !targetActor) {
			_MESSAGE("        WARNING: No target race or actor!");
			return;
		}

		ATSpellListEntries * spellData = nullptr;
		if (targetRace) {
			if (!targetRace->spellList.unk08) {
				targetRace->spellList.unk08 = new ATSpellListEntries();
			}
			spellData = (ATSpellListEntries*)targetRace->spellList.unk08;
		}
		else if (targetActor) {
			if (!targetActor->spellList.unk08) {
				targetActor->spellList.unk08 = new ATSpellListEntries();
			}
			spellData = (ATSpellListEntries*)targetActor->spellList.unk08;
		}
		if (!spellData) {
			_MESSAGE("        WARNING: Missing SpellData (report this error if you see it - something went wrong that shouldn't)!");
			return;
		}

		// add any current spells to the list
		if (spellData->numSpells != 0) {
			for (UInt32 i = 0; i < spellData->numSpells; i++) {
				if (std::find(spellsIn.begin(), spellsIn.end(), spellData->spells[i]) == spellsIn.end()) {
					spellsIn.push_back(spellData->spells[i]);
				}
				else {
					_MESSAGE("        WARNING: Spell 0x%08X is already in the list!", spellData->spells[i]->formID);
				}
			}
		}

		// rebuild the spells list
		spellData->spells = new SpellItem*[spellsIn.size()];
		UInt32 spellIdx = 0;

		if (iDebugLevel == 2) {
			_MESSAGE("        Edited Spells list:");
		}
		for (std::vector<SpellItem*>::iterator itAdd = spellsIn.begin(); itAdd != spellsIn.end(); ++itAdd) {
			spellData->spells[spellIdx] = *itAdd;
			if (iDebugLevel == 2) {
				_MESSAGE("          %i: 0x%08X", spellIdx, spellData->spells[spellIdx]->formID);
			}
			spellIdx += 1;
		}
	}

	// Modifies Keyword lists used by most Forms
	inline void LoadData_KeywordForm(BGSKeywordForm * keywordForm, json & keywordData, UInt8 iDebugLevel)
	{
		if (!keywordForm) {
			_MESSAGE("        WARNING: No KeywordForm!");
			return;
		}

		std::vector<UInt32> keywordsToAdd, keywordsToRemove, finalKeywordsList;
		// -- get keywords to remove
		if (keywordData["remove"].is_array() && !keywordData["remove"].empty()) {
			UInt32 remKWID = 0;
			for (json::iterator itRemove = keywordData["remove"].begin(); itRemove != keywordData["remove"].end(); ++itRemove) {
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
			UInt32 addKWID = 0;
			for (json::iterator itAdd = keywordData["add"].begin(); itAdd != keywordData["add"].end(); ++itAdd) {
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
			}
		}
	}

	// Modifies DamageTypes lists used by Weapons and Armors
	inline void LoadData_DamageTypes(tArray<TBO_InstanceData::DamageTypes> * damageTypes, json & damageTypeData, bool clearExisting, UInt8 iDebugLevel)
	{
		if (damageTypeData.empty()) {
			return;
		}

		if (!damageTypes) {
			damageTypes = new tArray<TBO_InstanceData::DamageTypes>();
		}

		if (clearExisting) {
			if (damageTypes->count != 0) {
				damageTypes->Clear();
				if (iDebugLevel == 2) {
					_MESSAGE("        Clearing damage types...");
				}
			}
		}

		UInt32 dtID = 0;
		json dtObj;
		std::vector<TBO_InstanceData::DamageTypes> finalDamageTypes;

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
					for (UInt32 j = 0; j < damageTypes->count; j++) {
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
						finalDamageTypes.push_back(newDT);
						if (iDebugLevel == 2) {
							_MESSAGE("        Adding damage type: %s, set: %i, add: %i", dtIDStr.c_str(), iDTSet, iDTAdd);
						}
					}
				}
			}
		}

		if (damageTypes->count != 0) {
			damageTypes->Clear();
		}
		if (!finalDamageTypes.empty()) {
			for (UInt32 j = 0; j < finalDamageTypes.size(); j++) {
				damageTypes->Push(finalDamageTypes[j]);
			}
		}
	}


	// ---- name editing funtions to avoid defining variables in switch

	inline void EditName_Armor(TESObjectARMO * armorForm, const std::string & newName)
	{
		if (armorForm) {
			armorForm->fullName.name = newName.c_str();
		}
	}
	inline void EditName_Weapon(TESObjectWEAP * weapForm, const std::string & newName)
	{
		if (weapForm) {
			weapForm->fullName.name = newName.c_str();
		}
	}
	inline void EditName_Ammo(TESAmmo * ammoForm, const std::string & newName)
	{
		if (ammoForm) {
			ammoForm->fullName.name = newName.c_str();
		}
	}
	inline void EditName_Misc(TESObjectMISC * miscForm, const std::string & newName)
	{
		if (miscForm) {
			miscForm->fullName.name = newName.c_str();
		}
	}
	inline void EditName_Compo(BGSComponent * compoForm, const std::string & newName)
	{
		if (compoForm) {
			compoForm->fullName.name = newName.c_str();
		}
	}
	inline void EditName_Ingestible(AlchemyItem * alchForm, const std::string & newName)
	{
		if (alchForm) {
			alchForm->name.name = newName.c_str();
		}
	}
}



// Weapon Form Overrides
void SAKEData::LoadOverrides_Weapon(TESObjectWEAP * weapForm, json & weaponOverride, UInt8 iDebugLevel)
{
	if (!weapForm) {
		_MESSAGE("        ERROR: No Weapon Form! dump: %s", weaponOverride.dump().c_str());
		return;
	}
	if (iDebugLevel != 0) {
		_MESSAGE("\n      Editing Weapon - 0x%08X (%s)", weapForm->formID, weapForm->GetFullName());
	}

	// -------- Base Form:

	// ---- Name
	if (!weaponOverride["name"].is_null()) {
		std::string weaponName = weaponOverride["name"];
		weapForm->fullName.name = BSFixedString(weaponName.c_str());
		if (iDebugLevel == 2) {
			_MESSAGE("        Edited base name: %s", weaponName.c_str());
		}
	}

	// ---- instance naming rules
	if (!weaponOverride["instanceNamingRules"].is_null()) {
		std::string inrID = weaponOverride["instanceNamingRules"];
		BGSInstanceNamingRules * newNamingRules = (BGSInstanceNamingRules*)SAKEUtilities::GetFormFromIdentifier(inrID.c_str());
		// null is acceptable here if the name should be static
		weapForm->namingRules.rules = newNamingRules;
		if (iDebugLevel == 2) {
			_MESSAGE("        Edited naming rules: %s", inrID.c_str());
		}
	}

	// ---- keywords
	if (!weaponOverride["keywords"].is_null()) {
		json keywordsObject = weaponOverride["keywords"];
		LoadData_KeywordForm(&weapForm->keyword, keywordsObject, iDebugLevel);
		if ((iDebugLevel == 2) && (weapForm->keyword.numKeywords != 0)) {
			_MESSAGE("        Final Keywords list:");
			for (UInt32 j = 0; j < weapForm->keyword.numKeywords; j++) {
				_MESSAGE("          %i: 0x%08X", j, weapForm->keyword.keywords[j]->formID);
			}
		}
	}

	// -------- InstanceData --------
	// anything past this point is only processed for weapons that can have ObjectMods
	TESObjectWEAP::InstanceData *instanceData = &weapForm->weapData;
	if (!instanceData) {
		_MESSAGE("\n      WARNING: Weapon has no instanceData!");
		return;
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
		LoadData_DamageTypes(instanceData->damageTypes, damageTypesObject, false, iDebugLevel);
		if ((iDebugLevel == 2) && (instanceData->damageTypes && (instanceData->damageTypes->count != 0))) {
			_MESSAGE("        Final Damage Types list:");
			for (UInt32 j = 0; j < instanceData->damageTypes->count; j++) {
				TBO_InstanceData::DamageTypes checkDT;
				if (instanceData->damageTypes->GetNthItem(j, checkDT)) {
					_MESSAGE("          %i:  0x%08X - %i", j, checkDT.damageType->formID, checkDT.value);
				}
			}
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
	if (weaponOverride["enchantments"].is_array() && !weaponOverride["enchantments"].empty()) {
		if (!instanceData->enchantments) {
			instanceData->enchantments = new tArray<EnchantmentItem*>();
		}
		for (json::iterator itEnch = weaponOverride["enchantments"].begin(); itEnch != weaponOverride["enchantments"].end(); ++itEnch) {
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
}


// Armor Form Overrides
void SAKEData::LoadOverrides_Armor(TESObjectARMO * armorForm, json & armorOverride, UInt8 iDebugLevel)
{
	if (!armorForm) {
		_MESSAGE("        ERROR: No Armor Form! dump: %s", armorOverride.dump().c_str());
		return;
	}
	if (iDebugLevel != 0) {
		_MESSAGE("\n      Editing Armor - 0x%08X (%s)", armorForm->formID, armorForm->GetFullName());
	}

	// ---- Base Form Edits:

	// ---- name
	if (!armorOverride["name"].is_null()) {
		std::string armorName = armorOverride["name"];
		armorForm->fullName.name = BSFixedString(armorName.c_str());
		if (iDebugLevel == 2) {
			_MESSAGE("        Edited base name: %s", armorName.c_str());
		}
	}

	// ---- instance naming rules
	if (!armorOverride["instanceNamingRules"].is_null()) {
		std::string inrID = armorOverride["instanceNamingRules"];
		BGSInstanceNamingRules * newNamingRules = (BGSInstanceNamingRules*)SAKEUtilities::GetFormFromIdentifier(inrID.c_str());
		armorForm->namingRules.rules = newNamingRules;
		if (iDebugLevel == 2) {
			_MESSAGE("        Edited naming rules: %s", inrID.c_str());
		}
	}

	// ---- keywords
	if (!armorOverride["keywords"].is_null()) {
		json keywordsObj = armorOverride["keywords"];
		LoadData_KeywordForm(&armorForm->keywordForm, keywordsObj, iDebugLevel);
		if ((iDebugLevel == 2) && (armorForm->keywordForm.numKeywords != 0)) {
			_MESSAGE("        Final Keywords list:");
			for (UInt32 j = 0; j < armorForm->keywordForm.numKeywords; j++) {
				_MESSAGE("          %i: 0x%08X", j, armorForm->keywordForm.keywords[j]->formID);
			}
		}
	}

	// ---- Armor.InstanceData - anything past here is only processed if the armor has default mods
	TESObjectARMO::InstanceData *instanceData = &armorForm->instanceData;
	if (!instanceData) {
		_MESSAGE("\n      WARNING: Armor has no instanceData!");
		return;
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
		LoadData_DamageTypes(instanceData->damageTypes, damageResistsObject, clearExisting, iDebugLevel);
		if ((iDebugLevel == 2) && instanceData->damageTypes && (instanceData->damageTypes->count != 0)) {
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


// Race Form Overrides
void SAKEData::LoadOverrides_Race(TESRace * raceForm, json & raceOverride, UInt8 iDebugLevel)
{
	if (!raceForm) {
		_MESSAGE("        ERROR: No Race Form! dump: %s", raceOverride.dump().c_str());
		return;
	}
	if (iDebugLevel != 0) {
		_MESSAGE("\n      Editing Race - 0x%08X (%s)", raceForm->formID, raceForm->GetFullName());
	}

	// ---- ActorValues
	if (!raceOverride["actorValues"].is_null()) {
		if (raceOverride["actorValues"].is_array() && !raceOverride["actorValues"].empty()) {
			if (!raceForm->propertySheet.sheet) {
				raceForm->propertySheet.sheet = new tArray<BGSPropertySheet::AVIFProperty>();
			}
			json curAV;
			for (json::iterator itAVs = raceOverride["actorValues"].begin(); itAVs != raceOverride["actorValues"].end(); ++itAVs) {
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
						if (raceForm->propertySheet.sheet->count != 0) {
							for (UInt32 i = 0; i < raceForm->propertySheet.sheet->count; i++) {
								BGSPropertySheet::AVIFProperty checkAVProp;
								if (raceForm->propertySheet.sheet->GetNthItem(i, checkAVProp)) {
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
							raceForm->propertySheet.sheet->Push(newAVProp);
						}
					}
				}
			}

			if (iDebugLevel == 2) {
				if (raceForm->propertySheet.sheet->count != 0) {
					_MESSAGE("        Final ActorValues list:");
					for (UInt32 i = 0; i < raceForm->propertySheet.sheet->count; i++) {
						BGSPropertySheet::AVIFProperty checkAVProp;
						if (raceForm->propertySheet.sheet->GetNthItem(i, checkAVProp)) {
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
			std::vector<SpellItem*> spellsList;
			for (json::iterator it = raceOverride["spells"].begin(); it != raceOverride["spells"].end(); ++it) {
				std::string spellIDStr = it.value()["formID"];
				if (!spellIDStr.empty()) {
					SpellItem * newSpell = (SpellItem*)SAKEUtilities::GetFormFromIdentifier(spellIDStr.c_str());
					if (newSpell && (std::find(spellsList.begin(), spellsList.end(), newSpell) == spellsList.end())) {
						spellsList.push_back(newSpell);
					}
				}
			}
			if (!spellsList.empty()) {
				LoadData_Spells(spellsList, raceForm, nullptr, iDebugLevel);
			}
			else {
				_MESSAGE("        WARNING: Failed to read spells from config!");
			}
			spellsList.clear();
		}
	}

	// ---- keywords
	if (!raceOverride["keywords"].is_null()) {
		LoadData_KeywordForm(&raceForm->keywordForm, raceOverride["keywords"], iDebugLevel);
		if ((iDebugLevel == 2) && (raceForm->keywordForm.numKeywords != 0)) {
			_MESSAGE("        Final Keywords list:");
			for (UInt32 j = 0; j < raceForm->keywordForm.numKeywords; j++) {
				_MESSAGE("          %i: 0x%08X", j, raceForm->keywordForm.keywords[j]->formID);
			}
		}
	}
}


// Actor Form Overrides
void SAKEData::LoadOverrides_Actor(TESNPC * actorForm, json & actorOverride, UInt8 iDebugLevel)
{
	if (!actorForm) {
		_MESSAGE("        ERROR: No Actor Form! dump: %s", actorOverride.dump().c_str());
		return;
	}
	if (iDebugLevel != 0) {
		_MESSAGE("\n      Editing Actor - 0x%08X (%s)", actorForm->formID, actorForm->GetFullName());
	}

	// ---- Name
	if (!actorOverride["name"].is_null()) {
		std::string actorName = actorOverride["name"];
		actorForm->fullName.name = BSFixedString(actorName.c_str());
		_MESSAGE("        Edited name:  %s", actorForm->GetFullName());
	}

	// ---- ActorValues
	if (!actorOverride["actorValues"].is_null()) {
		if (actorOverride["actorValues"].is_array() && !actorOverride["actorValues"].empty()) {
			json curAV;
			for (json::iterator itAVs = actorOverride["actorValues"].begin(); itAVs != actorOverride["actorValues"].end(); ++itAVs) {
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
							actorForm->actorValueOwner.SetBase(newAV, newAVValue);
						}

						if (iDebugLevel == 2) {
							if (actorForm->propertySheet.sheet->count != 0) {
								_MESSAGE("        Final ActorValues list:");
								for (UInt32 i = 0; i < actorForm->propertySheet.sheet->count; i++) {
									BGSPropertySheet::AVIFProperty checkAVProp;
									if (actorForm->propertySheet.sheet->GetNthItem(i, checkAVProp)) {
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
			std::vector<SpellItem*> spellsList;
			for (json::iterator it = actorOverride["spells"].begin(); it != actorOverride["spells"].end(); ++it) {
				std::string spellIDStr = it.value()["formID"];
				if (!spellIDStr.empty()) {
					SpellItem * newSpell = (SpellItem*)SAKEUtilities::GetFormFromIdentifier(spellIDStr.c_str());
					if (newSpell && (std::find(spellsList.begin(), spellsList.end(), newSpell) == spellsList.end())) {
						spellsList.push_back(newSpell);
					}
				}
			}
			if (!spellsList.empty()) {
				LoadData_Spells(spellsList, nullptr, actorForm, iDebugLevel);
			}
			else {
				_MESSAGE("        WARNING: Failed to read spells from config!");
			}
			spellsList.clear();
		}
	}

	// ---- keywords
	if (!actorOverride["keywords"].is_null()) {
		LoadData_KeywordForm(&actorForm->keywords, actorOverride["keywords"], iDebugLevel);
		if ((iDebugLevel == 2) && (actorForm->keywords.numKeywords != 0)) {
			_MESSAGE("        Final Keywords list:");
			for (UInt32 j = 0; j < actorForm->keywords.numKeywords; j++) {
				_MESSAGE("          %i: 0x%08X", j, actorForm->keywords.keywords[j]->formID);
			}
		}
	}

	// -- Class
	if (!actorOverride["npcClass"].is_null()) {
		std::string classFormID = actorOverride["npcClass"];
		TESClass * newClassForm = (TESClass*)SAKEUtilities::GetFormFromIdentifier(classFormID.c_str());
		if (newClassForm) {
			actorForm->npcClass = newClassForm;
			_MESSAGE("        Edited NPC Class : %s", classFormID.c_str());
		}
	}

	// -- CombatStyle
	if (!actorOverride["combatStyle"].is_null()) {
		std::string csFormID = actorOverride["combatStyle"];
		TESCombatStyle * newCSForm = (TESCombatStyle*)SAKEUtilities::GetFormFromIdentifier(csFormID.c_str());
		if (newCSForm) {
			actorForm->combatStyle = newCSForm;
			_MESSAGE("        Edited Combat Style : %s", csFormID.c_str());
		}
	}

	// -- Default Outfit
	if (!actorOverride["outfitDefault"].is_null()) {
		std::string outfit1FormID = actorOverride["outfitDefault"];
		BGSOutfit * newOutfitDef = (BGSOutfit*)SAKEUtilities::GetFormFromIdentifier(outfit1FormID.c_str());
		if (newOutfitDef) {
			actorForm->outfit[0] = newOutfitDef;
			_MESSAGE("        Edited Default Outfit : 0x%08X", newOutfitDef->formID);
		}
	}

	// -- Sleep Outfit
	if (!actorOverride["outfitSleep"].is_null()) {
		std::string outfit2FormID = actorOverride["outfitSleep"];
		BGSOutfit * newOutfitSlp = (BGSOutfit*)SAKEUtilities::GetFormFromIdentifier(outfit2FormID.c_str());
		if (newOutfitSlp) {
			actorForm->outfit[1] = newOutfitSlp;
			_MESSAGE("        Edited Sleep Outfit : 0x%08X", newOutfitSlp->formID);
		}
	}
}


// LeveledItem Form Overrides
void SAKEData::LoadOverrides_LeveledItem(TESLevItem * lliForm, json & llOverride, UInt8 iDebugLevel)
{
	//		LeveledItem.leveledList.unk08 = UseGlobal
	//			- get: (TESGlobal*)TESLevItem.leveledList.unk08
	//			- set: (UInt64)globalVar
	//		TESLevItem.leveledList.unk2A = ChanceNone
	//		TESLevItem.leveledlist.entries[n].unk8 = ChanceNone
	if (!lliForm) {
		_MESSAGE("        ERROR: No LeveledItem Form! dump: %s", llOverride.dump().c_str());
		return;
	}
	if (iDebugLevel != 0) {
		_MESSAGE("\n      Editing Leveled Item - 0x%08X", lliForm->formID);
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
	if (!llOverride["chanceNone"].is_null()) {
		int iTargetChanceNone = llOverride["chanceNone"];
		if (iTargetChanceNone > 0) {
			lliForm->leveledList.unk2A = (UInt8)iTargetChanceNone;
		}
	}
	if (!llOverride["useChanceGlobal"].is_null()) {
		std::string useglobalID = llOverride["useChanceGlobal"];
		TESGlobal * useglobal = (TESGlobal*)SAKEUtilities::GetFormFromIdentifier(useglobalID);
		if (useglobal) {
			lliForm->leveledList.unk08 = (UInt64)useglobal;
		}
	}

	if (bClearList) {
		// ---- clear the existing list, no need to check for entries to remove
		if (iDebugLevel == 2) {
			_MESSAGE("        Clearing original entries...");
		}
		bListModified = true;
	}
	else {
		// ---- copy the base list
		if (lliForm->leveledList.length != 0) {
			if (!llOverride["remove"].is_null() && !llOverride["remove"].empty()) {
				// -- check for and skip everything in the remove list
				json remEntry;
				UInt32 remID = 0;

				bool bRemove = false;
				for (UInt32 i = 0; i < lliForm->leveledList.length; i++) {
					bRemove = false;
					TESLeveledList::Entry curEntry = lliForm->leveledList.entries[i];
					// check the remove list
					for (json::iterator itRemove = llOverride["remove"].begin(); itRemove != llOverride["remove"].end(); ++itRemove) {
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
				if (lliForm->leveledList.length != 0) {
					for (UInt8 i = 0; i < lliForm->leveledList.length; i++) {
						TESLeveledList::Entry curEntry = lliForm->leveledList.entries[i];
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
	if (!llOverride["add"].is_null() && !llOverride["add"].empty()) {
		json addEntry;
		for (json::iterator itAdd = llOverride["add"].begin(); itAdd != llOverride["add"].end(); ++itAdd) {
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
					int curChanceNone = 0;
					if (!addEntry["chanceNone"].is_null()) {
						curChanceNone = addEntry["chanceNone"];
					}
					tempEntry.unk8 = (UInt32)curChanceNone;
					if (iDebugLevel == 2) {
						_MESSAGE("        Adding entry - ID: %s, level: %i, count: %i, chanceNone: %i", entryFormID.c_str(), tempEntry.level, tempEntry.count, tempEntry.unk8);
					}
					newEntries.push_back(tempEntry);
					bListModified = true;
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
			lliForm->leveledList.entries = new TESLeveledList::Entry[finalCount];
			lliForm->leveledList.length = finalCount;
			if (iDebugLevel == 2) {
				_MESSAGE("        Final Leveled List:  ChanceNone: %i, UseGlobal: 0x%08X", lliForm->leveledList.unk2A, (UInt32)lliForm->leveledList.unk08);
			}
			for (UInt8 i = 0; i < finalCount; i++) {
				lliForm->leveledList.entries[i] = newEntries[i];
				if (iDebugLevel == 2) {
					_MESSAGE("          %i: 0x%08X - level: %i, count: %i, chanceNone: %i", i, newEntries[i].form->formID, newEntries[i].level, newEntries[i].count, newEntries[i].unk8);
				}
			}
		}
	}
}

// LeveledActor Form Overrides
void SAKEData::LoadOverrides_LeveledActor(TESLevCharacter * llcForm, json & llOverride, UInt8 iDebugLevel)
{
	if (!llcForm) {
		_MESSAGE("        ERROR: No LeveledActor Form! dump: %s", llOverride.dump().c_str());
		return;
	}
	if (iDebugLevel != 0) {
		_MESSAGE("\n      Editing Leveled Actor - 0x%08X", llcForm->formID);
	}

	bool bClearList = false, bDelevel = false, bListModified = false;
	std::vector<TESLeveledList::Entry> newEntries;

	// -- check global modifiers
	if (!llOverride["clear"].is_null()) {
		bClearList = llOverride["clear"];
	}
	if (!llOverride["delevel"].is_null()) {
		bDelevel = llOverride["delevel"];
	}
	
	if (bClearList) {
		// ---- clear the existing list, no need to check for entries to remove
		if (iDebugLevel == 2) {
			_MESSAGE("        Clearing original entries...");
		}
		bListModified = true;
	}
	else {
		// ---- copy the base list
		if (llcForm->leveledList.length != 0) {
			if (!llOverride["remove"].is_null() && !llOverride["remove"].empty()) {
				// -- check for and skip everything in the remove list
				json remEntry;
				UInt32 remID = 0;

				bool bRemove = false;
				for (UInt32 i = 0; i < llcForm->leveledList.length; i++) {
					bRemove = false;
					TESLeveledList::Entry curEntry = llcForm->leveledList.entries[i];
					// check the remove list
					for (json::iterator itRemove = llOverride["remove"].begin(); itRemove != llOverride["remove"].end(); ++itRemove) {
						remEntry.clear();
						remEntry = *itRemove;
						if (!remEntry["formID"].is_null() && !remEntry["level"].is_null()) {
							std::string remFormID = remEntry["formID"];
							remID = SAKEUtilities::GetFormIDFromIdentifier(remFormID.c_str());
							// require an exact match for now, maybe add more options later
							if (curEntry.form->formID == remID) {
								UInt16 checkLevel = remEntry["level"];
								if (checkLevel == curEntry.level) {
									if (iDebugLevel == 2) {
										_MESSAGE("        Removing entry - ID: 0x%08X, level: %i", remFormID.c_str(), curEntry.level);
									}
									bRemove = true;
									break;
									
								}
							}
						}
					}
					if (bRemove) {
						// this entry is in the remove list, skip it
						continue;
					}
					if (bDelevel) {
						curEntry.level = 1;
					}
					newEntries.push_back(curEntry);
				}
			}
			else {
				// -- nothing to remove, just copy the original list
				if (llcForm->leveledList.length != 0) {
					for (UInt8 i = 0; i < llcForm->leveledList.length; i++) {
						TESLeveledList::Entry curEntry = llcForm->leveledList.entries[i];
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
	if (!llOverride["add"].is_null() && !llOverride["add"].empty()) {
		json addEntry;
		for (json::iterator itAdd = llOverride["add"].begin(); itAdd != llOverride["add"].end(); ++itAdd) {
			addEntry.clear();
			addEntry = *itAdd;
			// make sure all variables exist
			if (!addEntry["formID"].is_null() && !addEntry["level"].is_null()) {
				std::string entryFormID = addEntry["formID"];
				TESLeveledList::Entry tempEntry;
				tempEntry.form = SAKEUtilities::GetFormFromIdentifier(entryFormID.c_str());
				if (tempEntry.form) {
					if (!bDelevel) {
						tempEntry.level = addEntry["level"];
					}
					else {
						tempEntry.level = 1;
					}
					if (iDebugLevel == 2) {
						_MESSAGE("        Adding entry - ID: %s, level: %i", entryFormID.c_str(), tempEntry.level);
					}
					newEntries.push_back(tempEntry);
					bListModified = true;
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
			llcForm->leveledList.entries = new TESLeveledList::Entry[finalCount];
			llcForm->leveledList.length = finalCount;
			if (iDebugLevel == 2) {
				_MESSAGE("        Final Leveled List:");
			}
			for (UInt8 i = 0; i < finalCount; i++) {
				llcForm->leveledList.entries[i] = newEntries[i];
				if (iDebugLevel == 2) {
					_MESSAGE("          %i: 0x%08X - level: %i", i, newEntries[i].form->formID, newEntries[i].level);
				}
			}
		}
	}
}


// Ammo Form Overrides
void SAKEData::LoadOverrides_Ammo(TESAmmo * ammoForm, json & ammoOverride, UInt8 iDebugLevel)
{
	//	TESAmmo::unk160 -
	//		[0] = projectile (UInt64)(BGSProjectile*)
	//		[1] = health (UInt64)
	//		[2] = damage (UInt64)(float)
	//	the rest of these are guesses:
	//		[3] = short name? - no idea how it's stored, though
	//		[4,5] = padding or flags? - identical for all ammo forms I checked
	//		[6-9] = padding? - always 0

	if (!ammoForm) {
		_MESSAGE("        ERROR: No Ammo Form! dump: %s", ammoOverride.dump().c_str());
		return;
	}
	if (iDebugLevel != 0) {
		_MESSAGE("\n      Editing Ammo - 0x%08X (%s)", ammoForm->formID, ammoForm->GetFullName());
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
		LoadData_KeywordForm(&ammoForm->keywordForm, ammoOverride["keywords"], iDebugLevel);
		if ((iDebugLevel == 2) && (ammoForm->keywordForm.numKeywords != 0)) {
			_MESSAGE("        Final Keywords list:");
			for (UInt32 j = 0; j < ammoForm->keywordForm.numKeywords; j++) {
				_MESSAGE("          %i: 0x%08X", j, ammoForm->keywordForm.keywords[j]->formID);
			}
		}
	}

	// ---- projectile
	if (!ammoOverride["projectile"].is_null()) {
		std::string projID = ammoOverride["projectile"];
		BGSProjectile * ammoProj = (BGSProjectile*)SAKEUtilities::GetFormFromIdentifier(projID);
		if (ammoProj) {
			ammoForm->unk160[0] = (UInt64)ammoProj;
			_MESSAGE("        Edited Projectile: 0x%016X", ammoForm->unk160[0]);
		}
	}
	// ---- health/charge
	if (!ammoOverride["health"].is_null()) {
		int ammoHealth = ammoOverride["health"];
		if (ammoHealth > 0) {
			ammoForm->unk160[1] = (UInt64)ammoHealth;
			_MESSAGE("        Edited Health: %i", ammoForm->unk160[1]);
		}
	}
	// ---- damage
	if (!ammoOverride["damage"].is_null()) {
		float ammoDamage = ammoOverride["damage"];
		if (ammoDamage >= 0.0) {
			ammoForm->unk160[2] = (UInt64)ammoDamage;
			_MESSAGE("        Edited Damage: %.04f", (float)ammoForm->unk160[2]);
		}
	}
}


// MiscItem/Junk Form Overrides
void SAKEData::LoadOverrides_Misc(TESObjectMISC * miscForm, json & miscOverride, UInt8 iDebugLevel)
{
	if (!miscForm) {
		_MESSAGE("        ERROR: No MiscItem Form! dump: %s", miscOverride.dump().c_str());
		return;
	}
	if (iDebugLevel != 0) {
		_MESSAGE("\n      Editing MiscItem - 0x%08X (%s)", miscForm->formID, miscForm->GetFullName());
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
		LoadData_KeywordForm(&miscForm->keywordForm, miscOverride["keywords"], iDebugLevel);
		if ((iDebugLevel == 2) && (miscForm->keywordForm.numKeywords != 0)) {
			_MESSAGE("        Final Keywords list:");
			for (UInt32 j = 0; j < miscForm->keywordForm.numKeywords; j++) {
				_MESSAGE("          %i: 0x%08X", j, miscForm->keywordForm.keywords[j]->formID);
			}
		}
	}
	// -- components
	if (!miscOverride["components"].is_null()) {
		json composObj = miscOverride["components"];
		if (!composObj["clear"].is_null()) {
			bool clearCompos = composObj["clearList"];
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
				if (composObj["remove"].is_array() && !composObj["remove"].empty()) {
					json remEntry;
					for (json::iterator itRem = composObj["remove"].begin(); itRem != composObj["remove"].end(); ++itRem) {
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
			if (composObj["add"].is_array() && !composObj["add"].empty()) {
				json addEntry;
				for (json::iterator itAdd = composObj["add"].begin(); itAdd != composObj["add"].end(); ++itAdd) {
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
				}
				else {
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
}


// Crafting Component Form Overrides
void SAKEData::LoadOverrides_Component(BGSComponent * compoForm, json & compoOverride, UInt8 iDebugLevel)
{
	if (!compoForm) {
		_MESSAGE("        ERROR: No Component Form! dump: %s", compoOverride.dump().c_str());
		return;
	}
	if (iDebugLevel != 0) {
		_MESSAGE("\n      Editing Component - 0x%08X (%s)", compoForm->formID, compoForm->GetFullName());
	}

	// -- name
	if (!compoOverride["name"].is_null()) {
		std::string compoName = compoOverride["name"];
		compoForm->fullName.name = BSFixedString(compoName.c_str());
		if (iDebugLevel == 2) {
			_MESSAGE("        Edited Name: %s", compoForm->fullName.name.c_str());
		}
	}
	// -- value
	if (!compoOverride["value"].is_null()) {
		int compoValue = compoOverride["value"];
		if (compoValue > -1) {
			compoForm->value.value = (UInt32)compoValue;
			if (iDebugLevel == 2) {
				_MESSAGE("        Edited Value: %i", compoForm->value.value);
			}
		}
	}
	// -- scrap scalar global
	if (!compoOverride["scrapScalarGlobal"].is_null()) {
		std::string compoScrapGlobalID = compoOverride["scrapScalarGlobal"];
		TESGlobal * compoScrapGlobal = (TESGlobal*)SAKEUtilities::GetFormFromIdentifier(compoScrapGlobalID);
		if (compoScrapGlobal) {
			compoForm->scrapScalar = compoScrapGlobal;
			if (iDebugLevel == 2) {
				_MESSAGE("        Edited Scrap Scalar Global: 0x%08X", compoForm->scrapScalar->formID);
			}
		}
	}
	// -- scrap misc item
	if (!compoOverride["scrapMiscItem"].is_null()) {
		std::string compoScrapMiscItemID = compoOverride["scrapMiscItem"];
		TESObjectMISC * compoScrapMiscItem = (TESObjectMISC*)SAKEUtilities::GetFormFromIdentifier(compoScrapMiscItemID);
		if (compoScrapMiscItem) {
			compoForm->scrapItem = compoScrapMiscItem;
			if (iDebugLevel == 2) {
				_MESSAGE("        Edited Scrap MiscItem: 0x%08X", compoForm->scrapItem->formID);
			}
		}
	}
}


// Ingestible (Chem/Food/Drink) Form Overrides
void SAKEData::LoadOverrides_Ingestible(AlchemyItem * alchForm, json & alchOverride, UInt8 iDebugLevel)
{
	if (!alchForm) {
		_MESSAGE("        ERROR: No Ingestible Form! dump: %s", alchOverride.dump().c_str());
		return;
	}
	if (iDebugLevel != 0) {
		_MESSAGE("\n      Editing Ingestible - 0x%08X (%s)", alchForm->formID, alchForm->GetFullName());
	}

	// -- name
	if (!alchOverride["name"].is_null()) {
		std::string alchName = alchOverride["name"];
		alchForm->name.name = BSFixedString(alchName.c_str());
		if (iDebugLevel == 2) {
			_MESSAGE("        Edited Name: %s", alchForm->name.name.c_str());
		}
	}

	// -- weight
	if (!alchOverride["weight"].is_null()) {
		float alchWeight = alchOverride["weight"];
		if (alchWeight >= 0.0) {
			alchForm->weightForm.weight = alchWeight;
			if (iDebugLevel == 2) {
				_MESSAGE("        Edited Weight: %f", alchForm->weightForm.weight);
			}
		}
	}
	// ---- keywords
	if (!alchOverride["keywords"].is_null()) {
		LoadData_KeywordForm(&alchForm->keywordForm, alchOverride["keywords"], iDebugLevel);
		if ((iDebugLevel == 2) && (alchForm->keywordForm.numKeywords != 0)) {
			_MESSAGE("        Final Keywords list:");
			for (UInt32 j = 0; j < alchForm->keywordForm.numKeywords; j++) {
				_MESSAGE("          %i: 0x%08X", j, alchForm->keywordForm.keywords[j]->formID);
			}
		}
	}
}


// EncounterZone Form Overrides
void SAKEData::LoadOverrides_EncounterZone(BGSEncounterZone * enczForm, json & enczOverride, UInt8 iDebugLevel)
{
	if (!enczForm) {
		_MESSAGE("        ERROR: No EncounterZone Form! dump: %s", enczOverride.dump().c_str());
		return;
	}
	if (iDebugLevel != 0) {
		_MESSAGE("\n      Editing EncounterZone - 0x%08X (%s)", enczForm->formID, enczForm->GetFullName());
	}
	
	int levelMin = enczForm->minLevel, levelMax = enczForm->maxLevel;
	if (!enczOverride["levelMin"].is_null()) {
		levelMin = enczOverride["levelMin"];
	}
	if (!enczOverride["levelMax"].is_null()) {
		levelMax = enczOverride["levelMax"];
	}
	if (levelMax < levelMin) {
		levelMax = levelMin;
	}

	if ((levelMin != (int)enczForm->minLevel) || (levelMax != (int)enczForm->maxLevel)) {
		enczForm->minLevel = (UInt8)levelMin;
		enczForm->maxLevel = (UInt8)levelMax;
		if (iDebugLevel == 2) {
			_MESSAGE("        Edited Min-Max Levels: %i - %i", enczForm->minLevel, enczForm->maxLevel);
		}
	}
}


// Name Prefixes (any supported Forms)
void SAKEData::LoadNamePrefix(TESForm * targetForm, const std::string & prefixStr, UInt8 iDebugLevel)
{
	if (!targetForm || prefixStr.empty()) {
		return;
	}
	bool bEdited = true;
	std::string formName;
	formName.append(prefixStr);
	formName.append(targetForm->GetFullName());

	if (iDebugLevel == 2) {
		_MESSAGE("    Original Name: %s", targetForm->GetFullName());
	}
	
	switch (targetForm->formType) {
		case kFormType_ARMO:
			EditName_Armor((TESObjectARMO*)targetForm, formName);
			break;
		case kFormType_WEAP:
			EditName_Weapon((TESObjectWEAP*)targetForm, formName);
			break;
		case kFormType_AMMO:
			EditName_Ammo((TESAmmo*)targetForm, formName);
			break;
		case kFormType_MISC:
			EditName_Misc((TESObjectMISC*)targetForm, formName);
			break;
		case kFormType_CMPO:
			EditName_Compo((BGSComponent*)targetForm, formName);
			break;
		case kFormType_ALCH:
			EditName_Ingestible((AlchemyItem*)targetForm, formName);
			break;
		default:
			bEdited = false;
	}

	if ((iDebugLevel == 2) && bEdited) {
		_MESSAGE("    Edited Name: %s", targetForm->GetFullName());
	}
}


// Game Settings
void SAKEData::LoadGameSettings(json & settingOverrides, UInt8 iDebugLevel)
{
	if (settingOverrides.empty() || !settingOverrides.is_array()) {
		return;
	}
	
	_MESSAGE("\n      Editing GameSettings:");
	json curOverride;
	for (json::iterator itSettings = settingOverrides.begin(); itSettings != settingOverrides.end(); ++itSettings) {
		curOverride.clear();
		curOverride = *itSettings;
		if (!curOverride["name"].is_null()) {
			std::string settingName = curOverride["name"];
			Setting * curSetting = GetGameSetting(settingName.c_str());
			if (curSetting) {
				UInt32 settingType = curSetting->GetType();
				if (settingType == Setting::kType_Float) {
					if (!curOverride["valueFloat"].is_null()) {
						float newFloatVal = curOverride["valueFloat"];
						curSetting->data.f32 = newFloatVal;
						_MESSAGE("\n          %s: %.04f", settingName.c_str(), curSetting->data.f32);
					}
					continue;
				}
				if (settingType == Setting::kType_Integer) {
					if (!curOverride["valueInt"].is_null()) {
						int newIntVal = curOverride["valueInt"];
						curSetting->data.u32 = (UInt32)newIntVal;
						_MESSAGE("\n          %s: %i", settingName.c_str(), curSetting->data.u32);
					}
					continue;
				}
				if (settingType == Setting::kType_Bool) {
					if (!curOverride["valueBool"].is_null()) {
						bool newBoolVal = curOverride["valueBool"];
						curSetting->data.u8 = (UInt8)newBoolVal;
						_MESSAGE("\n          %s: %i", settingName.c_str(), curSetting->data.u8);
					}
					continue;
				}
				if (settingType == Setting::kType_String) {
					if (!curOverride["valueString"].is_null()) {
						std::string newStringVal = curOverride["valueString"];
						if (curSetting->SetString(newStringVal.c_str())) {
							_MESSAGE("\n          %s: %s", settingName.c_str(), curSetting->data.s);
						}
						else {
							_MESSAGE("\n          WARNING: Failed to set new string value for %s!", settingName.c_str());
						}
					}
					continue;
				}
				_MESSAGE("\n          WARNING: Invalid or missing setting type - %s!", settingName.c_str());
			}
		}
	}
	
}

