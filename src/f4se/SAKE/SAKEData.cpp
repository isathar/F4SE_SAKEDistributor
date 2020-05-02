#include "SAKEData.h"


namespace SAKEData
{
	// adds a list of spells to Actor/Race spellLists
	void LoadData_Spells(std::vector<SpellItem*> & spellsIn, TESRace * targetRace, TESActorBase * targetActor)
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

		TempSpellListEntries * spellData = nullptr;
		if (targetRace) {
			if (!targetRace->spellList.unk08) {
				targetRace->spellList.unk08 = new TempSpellListEntries();
			}
			spellData = reinterpret_cast<TempSpellListEntries*>(targetRace->spellList.unk08);
		}
		else if (targetActor) {
			if (!targetActor->spellList.unk08) {
				targetActor->spellList.unk08 = new TempSpellListEntries();
			}
			spellData = reinterpret_cast<TempSpellListEntries*>(targetActor->spellList.unk08);
		}
		if (!spellData) {
			_MESSAGE("        WARNING: Missing SpellData!");
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
		spellData->numSpells = spellsIn.size();
		UInt32 spellIdx = 0;

		_MESSAGE("        Edited Spells list:");
		for (std::vector<SpellItem*>::iterator itAdd = spellsIn.begin(); itAdd != spellsIn.end(); ++itAdd) {
			spellData->spells[spellIdx] = *itAdd;
			_MESSAGE("          %i: 0x%08X", spellIdx, spellData->spells[spellIdx]->formID);
			spellIdx += 1;
		}
	}

	// Modifies Keyword lists used by most Forms
	void LoadData_KeywordForm(BGSKeywordForm * keywordForm, json & keywordData)
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
				_MESSAGE("        Removing keyword %s", remKWStr.c_str());
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
				_MESSAGE("        Adding keyword %s", addKWStr.c_str());
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
					BGSKeyword * newKW = reinterpret_cast<BGSKeyword*>(LookupFormByID(finalKeywordsList[j]));
					if (newKW) {
						keywordForm->keywords[j] = newKW;
					}
				}
			}
		}
	}

	// Modifies DamageTypes lists used by Weapons and Armors
	void LoadData_DamageTypes(tArray<TBO_InstanceData::DamageTypes> * damageTypes, json & damageTypeData, bool clearExisting)
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
				_MESSAGE("        Clearing damage types...");
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
								_MESSAGE("        Found existing damage type: %s, value: %i", dtIDStr.c_str(), checkDT.value);
								break;
							}
						}
					}
				}

				// add the damageType to the list
				int finalDTVal = (iDTSet > -1) ? (iDTSet + iDTAdd) : iDTAdd;
				if (finalDTVal > 0) {
					BGSDamageType * tempDT = reinterpret_cast<BGSDamageType*>(LookupFormByID(dtID));
					if (tempDT) {
						TBO_InstanceData::DamageTypes newDT;
						newDT.damageType = tempDT;
						newDT.value = (UInt32)finalDTVal;
						finalDamageTypes.push_back(newDT);
						_MESSAGE("        Adding damage type: %s, set: %i, add: %i", dtIDStr.c_str(), iDTSet, iDTAdd);
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


	// name editing functions to get around not being able to define variables in switches:

	void EditName_Armor(TESObjectARMO * armorForm, const std::string & newName)
	{
		if (armorForm) {
			armorForm->fullName.name = newName.c_str();
		}
	}
	void EditName_Weapon(TESObjectWEAP * weapForm, const std::string & newName)
	{
		if (weapForm) {
			weapForm->fullName.name = newName.c_str();
		}
	}
	void EditName_Ammo(TESAmmo * ammoForm, const std::string & newName)
	{
		if (ammoForm) {
			ammoForm->fullName.name = newName.c_str();
		}
	}
	void EditName_Misc(TESObjectMISC * miscForm, const std::string & newName)
	{
		if (miscForm) {
			miscForm->fullName.name = newName.c_str();
		}
	}
	void EditName_Compo(BGSComponent * compoForm, const std::string & newName)
	{
		if (compoForm) {
			compoForm->fullName.name = newName.c_str();
		}
	}
	void EditName_Ingestible(AlchemyItem * alchForm, const std::string & newName)
	{
		if (alchForm) {
			alchForm->name.name = newName.c_str();
		}
	}
	void EditName_Key(TempTESKey * keyForm, const std::string & newName)
	{
		if (keyForm) {
			keyForm->fullName.name = newName.c_str();
		}
	}
}


// ---- Weapon - WEAP
void SAKEData::LoadOverrides_Weapon(TESObjectWEAP * weapForm, json & weaponOverride)
{
	if (!weapForm) {
		_MESSAGE("        ERROR: No Weapon Form! dump: %s", weaponOverride.dump().c_str());
		return;
	}
	_MESSAGE("\n      Editing Weapon - 0x%08X (%s)", weapForm->formID, weapForm->GetFullName());

	// ---- Base Form:
	// -- Name
	if (!weaponOverride["name"].is_null()) {
		std::string weaponName = weaponOverride["name"];
		weapForm->fullName.name = BSFixedString(weaponName.c_str());
		_MESSAGE("        Base Name: %s", weapForm->fullName.name.c_str());
	}
	// -- instance naming rules
	if (!weaponOverride["instanceNamingRules"].is_null()) {
		std::string inrID = weaponOverride["instanceNamingRules"];
		BGSInstanceNamingRules * newNamingRules = reinterpret_cast<BGSInstanceNamingRules*>(SAKEUtilities::GetFormFromIdentifier(inrID.c_str()));
		// null is acceptable here if the name should be static
		weapForm->namingRules.rules = newNamingRules;
		if (weapForm->namingRules.rules) {
			_MESSAGE("        Instance Naming Rules: 0x%08X", weapForm->namingRules.rules->formID);
		}
		else {
			_MESSAGE("        Instance Naming Rules: none");
		}
	}
	// -- keywords
	if (!weaponOverride["keywords"].is_null()) {
		json keywordsObject = weaponOverride["keywords"];
		LoadData_KeywordForm(&weapForm->keyword, keywordsObject);
		if (weapForm->keyword.numKeywords != 0) {
			_MESSAGE("        Final Keywords list:");
			for (UInt32 j = 0; j < weapForm->keyword.numKeywords; j++) {
				_MESSAGE("          %i: 0x%08X (%s)", j, weapForm->keyword.keywords[j]->formID, weapForm->keyword.keywords[j]->keyword.c_str());
			}
		}
	}

	// ---- InstanceData
	// anything past this point is only processed for weapons that can have ObjectMods
	TESObjectWEAP::InstanceData *instanceData = &weapForm->weapData;
	if (!instanceData) {
		_MESSAGE("\n      WARNING: Weapon has no instanceData!");
		return;
	}

	// -- base damage
	if (!weaponOverride["damage"].is_null()) {
		int iBaseDamage = weaponOverride["damage"];
		if (iBaseDamage > 0) {
			instanceData->baseDamage = (UInt16)iBaseDamage;
			_MESSAGE("        Base Damage: %i", instanceData->baseDamage);
		}
	}
	// -- damageTypes
	if (weaponOverride["damageTypes"].is_array() && !weaponOverride["damageTypes"].empty()) {
		json damageTypesObject = weaponOverride["damageTypes"];
		if (!instanceData->damageTypes) {
			instanceData->damageTypes = new tArray<TBO_InstanceData::DamageTypes>();
		}
		LoadData_DamageTypes(instanceData->damageTypes, damageTypesObject, false);
		if (instanceData->damageTypes && (instanceData->damageTypes->count != 0)) {
			_MESSAGE("        Final Damage Types list:");
			for (UInt32 j = 0; j < instanceData->damageTypes->count; j++) {
				TBO_InstanceData::DamageTypes checkDT;
				if (instanceData->damageTypes->GetNthItem(j, checkDT)) {
					_MESSAGE("          %i:  0x%08X - %i", j, checkDT.damageType->formID, checkDT.value);
				}
			}
		}
	}
	// -- secondary/bash damage 
	if (!weaponOverride["secondaryDamage"].is_null()) {
		float fSecDmg = weaponOverride["secondaryDamage"];
		if (fSecDmg > 0.0) {
			instanceData->secondary = fSecDmg;
			_MESSAGE("        Base Secondary Damage: %.2f", instanceData->secondary);
		}
	}
	// -- AP cost
	if (!weaponOverride["apCost"].is_null()) {
		float fAPCost = weaponOverride["apCost"];
		if (fAPCost > 0.0) {
			instanceData->actionCost = fAPCost;
			_MESSAGE("        Base AP Cost: %.2f", instanceData->actionCost);
		}
	}
	// -- crit charge multiplier
	if (!weaponOverride["critChargeMult"].is_null()) {
		float fCritChargeMult = weaponOverride["critChargeMult"];
		if (fCritChargeMult > 0.0) {
			instanceData->critChargeBonus = fCritChargeMult;
			_MESSAGE("        Base Crit Charge Multiplier: %.2f", instanceData->critChargeBonus);
		}
	}
	// -- crit damage multiplier
	if (!weaponOverride["critDamageMult"].is_null()) {
		float fCritDmgMult = weaponOverride["critDamageMult"];
		if (fCritDmgMult > 0.0) {
			instanceData->critDamageMult = fCritDmgMult;
			_MESSAGE("        Base Crit Damage Multiplier: %.2f", fCritDmgMult);
		}
	}
	// ---- range:
	// -- min
	if (!weaponOverride["rangeMin"].is_null()) {
		float fMinRange = weaponOverride["rangeMin"];
		if (fMinRange > 0.0) {
			instanceData->minRange = fMinRange;
			_MESSAGE("        Base Minimum Range: %.2f", instanceData->minRange);
		}
	}
	// -- max
	if (!weaponOverride["rangeMax"].is_null()) {
		float fMaxRange = weaponOverride["rangeMax"];
		if (fMaxRange > 0.0) {
			instanceData->minRange = fMaxRange;
			_MESSAGE("        Base Maximum Range: %.2f", instanceData->minRange);
		}
	}
	// -- out of range damage multiplier
	if (!weaponOverride["outOfRangeMult"].is_null()) {
		float fOoRMult = weaponOverride["outOfRangeMult"];
		if (fOoRMult > 0.0) {
			instanceData->outOfRangeMultiplier = fOoRMult;
			_MESSAGE("        Base OOR Damage Mult: %.2f", instanceData->outOfRangeMultiplier);
		}
	}
	// -- attack delay
	if (!weaponOverride["attackDelay"].is_null()) {
		float fAttackDelay = weaponOverride["attackDelay"];
		if (fAttackDelay > 0.0) {
			instanceData->attackDelay = fAttackDelay;
			_MESSAGE("        Base Attack Delay: %.2f", instanceData->attackDelay);
		}
	}
	// -- speed multiplier
	if (!weaponOverride["speedMult"].is_null()) {
		float fSpeedMult = weaponOverride["speedMult"];
		if (fSpeedMult > 0.0) {
			instanceData->speed = fSpeedMult;
			_MESSAGE("        Base Speed Mult: %.2f", instanceData->speed);
		}
	}
	// -- reach
	if (!weaponOverride["reach"].is_null()) {
		float fReach = weaponOverride["reach"];
		if (fReach > 0.0) {
			instanceData->reach = fReach;
			_MESSAGE("        Base Reach: %.2f", instanceData->reach);
		}
	}
	// -- stagger
	if (!weaponOverride["stagger"].is_null()) {
		int iStagger = weaponOverride["stagger"];
		if (iStagger >= 0) {
			instanceData->stagger = (UInt32)iStagger;
			_MESSAGE("        Base Stagger: %i", instanceData->stagger);
		}
	}
	// -- value
	if (!weaponOverride["value"].is_null()) {
		int iBaseValue = weaponOverride["value"];
		if (iBaseValue > 0) {
			instanceData->value = (UInt32)iBaseValue;
			_MESSAGE("        Base Value: %i", instanceData->value);
		}
	}
	// -- weight
	if (!weaponOverride["weight"].is_null()) {
		float fWeight = weaponOverride["weight"];
		if (fWeight > 0.0) {
			instanceData->weight = fWeight;
			_MESSAGE("        Base Weight: %.2f", instanceData->weight);
		}
	}
	// -- ammo
	if (!weaponOverride["ammo"].is_null()) {
		std::string ammoID = weaponOverride["ammo"];
		TESAmmo * newAmmo = reinterpret_cast<TESAmmo*>(SAKEUtilities::GetFormFromIdentifier(ammoID.c_str()));
		if (newAmmo) {
			instanceData->ammo = newAmmo;
			_MESSAGE("        Ammo: 0x%08X", instanceData->ammo->formID);
		}
	}
	// -- NPC ammo leveled list - can be none
	if (!weaponOverride["npcAmmoLeveledList"].is_null()) {
		std::string ammoListID = weaponOverride["npcAmmoLeveledList"];
		TESLevItem * newAmmoList = reinterpret_cast<TESLevItem*>(SAKEUtilities::GetFormFromIdentifier(ammoListID.c_str()));
		instanceData->addAmmoList = newAmmoList;
		if (instanceData->addAmmoList) {
			_MESSAGE("        NPC Ammo List: 0x%08X", instanceData->addAmmoList->formID);
		}
		else {
			_MESSAGE("        NPC Ammo List: none");
		}
	}
	// -- impactDataSet
	if (!weaponOverride["impactDataSet"].is_null()) {
		std::string impactsID = weaponOverride["impactDataSet"];
		BGSImpactDataSet * newImpacts = reinterpret_cast<BGSImpactDataSet*>(SAKEUtilities::GetFormFromIdentifier(impactsID.c_str()));
		if (newImpacts) {
			instanceData->unk58 = newImpacts;
			_MESSAGE("        ImpactDataSet: %s", impactsID.c_str());
		}
	}
	// -- Enchantments
	if (weaponOverride["enchantments"].is_array() && !weaponOverride["enchantments"].empty()) {
		if (!instanceData->enchantments) {
			instanceData->enchantments = new tArray<EnchantmentItem*>();
		}
		for (json::iterator itEnch = weaponOverride["enchantments"].begin(); itEnch != weaponOverride["enchantments"].end(); ++itEnch) {
			json enchObj = *itEnch;
			if (!enchObj["formID"].is_null()) {
				std::string enchID = enchObj["formID"];
				EnchantmentItem * newEnchantment = reinterpret_cast<EnchantmentItem*>(SAKEUtilities::GetFormFromIdentifier(enchID.c_str()));
				if (newEnchantment) {
					instanceData->enchantments->Push(newEnchantment);
					_MESSAGE("        Adding Enchantment: %s", enchID.c_str());
				}
			}
		}
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
	// -- ActorValue modifiers
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
				ActorValueInfo * newAV = reinterpret_cast<ActorValueInfo*>(SAKEUtilities::GetFormFromIdentifier(avID.c_str()));
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
						_MESSAGE("        Adding actorValueMod: %s - set: %i, add: %i", avID.c_str(), iAVSet, iAVAdd);
					}
				}
			}
		}
		_MESSAGE("        Final ActorValue Modifiers:");
		if (instanceData->modifiers->count != 0) {
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
	// -- Flags:
	if (instanceData->flags && !weaponOverride["flags"].is_null()) {
		json flagsObj = weaponOverride["flags"];
		bool bFlagCheck = false;
		// -- PlayerOnly
		if (!flagsObj["PlayerOnly"].is_null()) {
			bFlagCheck = flagsObj["PlayerOnly"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_PlayerOnly;
			}
			else {
				instanceData->flags &= ~wFlag_PlayerOnly;
			}
		}
		// -- NPCsUseAmmo
		if (!flagsObj["NPCsUseAmmo"].is_null()) {
			bFlagCheck = flagsObj["NPCsUseAmmo"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_NPCsUseAmmo;
			}
			else {
				instanceData->flags &= ~wFlag_NPCsUseAmmo;
			}
		}
		// -- NoJamAfterReload
		if (!flagsObj["NoJamAfterReload"].is_null()) {
			bFlagCheck = flagsObj["NoJamAfterReload"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_NoJamAfterReload;
			}
			else {
				instanceData->flags &= ~wFlag_NoJamAfterReload;
			}
		}
		// -- ChargingReload
		if (!flagsObj["ChargingReload"].is_null()) {
			bFlagCheck = flagsObj["ChargingReload"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_ChargingReload;
			}
			else {
				instanceData->flags &= ~wFlag_ChargingReload;
			}
		}
		// -- MinorCrime
		if (!flagsObj["MinorCrime"].is_null()) {
			bFlagCheck = flagsObj["MinorCrime"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_MinorCrime;
			}
			else {
				instanceData->flags &= ~wFlag_MinorCrime;
			}
		}
		// -- FixedRange
		if (!flagsObj["FixedRange"].is_null()) {
			bFlagCheck = flagsObj["FixedRange"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_FixedRange;
			}
			else {
				instanceData->flags &= ~wFlag_FixedRange;
			}
		}
		// -- NotUsedInNormalCombat
		if (!flagsObj["NotUsedInNormalCombat"].is_null()) {
			bFlagCheck = flagsObj["NotUsedInNormalCombat"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_NotUsedInNormalCombat;
			}
			else {
				instanceData->flags &= ~wFlag_NotUsedInNormalCombat;
			}
		}
		// -- CritEffectonDeath
		if (!flagsObj["CritEffectonDeath"].is_null()) {
			bFlagCheck = flagsObj["CritEffectonDeath"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_CritEffectonDeath;
			}
			else {
				instanceData->flags &= ~wFlag_CritEffectonDeath;
			}
		}
		// -- ChargingAttack
		if (!flagsObj["ChargingAttack"].is_null()) {
			bFlagCheck = flagsObj["ChargingAttack"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_ChargingAttack;
			}
			else {
				instanceData->flags &= ~wFlag_ChargingAttack;
			}
		}
		// -- HoldInputToPower
		if (!flagsObj["HoldInputToPower"].is_null()) {
			bFlagCheck = flagsObj["HoldInputToPower"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_HoldInputToPower;
			}
			else {
				instanceData->flags &= ~wFlag_HoldInputToPower;
			}
		}
		// -- NonHostile
		if (!flagsObj["NonHostile"].is_null()) {
			bFlagCheck = flagsObj["NonHostile"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_NonHostile;
			}
			else {
				instanceData->flags &= ~wFlag_NonHostile;
			}
		}
		// -- BoundWeapon
		if (!flagsObj["BoundWeapon"].is_null()) {
			bFlagCheck = flagsObj["BoundWeapon"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_BoundWeapon;
			}
			else {
				instanceData->flags &= ~wFlag_BoundWeapon;
			}
		}
		// -- IgnoresNormalResist
		if (!flagsObj["IgnoresNormalResist"].is_null()) {
			bFlagCheck = flagsObj["IgnoresNormalResist"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_IgnoresNormalResist;
			}
			else {
				instanceData->flags &= ~wFlag_IgnoresNormalResist;
			}
		}
		// -- Automatic
		if (!flagsObj["Automatic"].is_null()) {
			bFlagCheck = flagsObj["Automatic"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_Automatic;
			}
			else {
				instanceData->flags &= ~wFlag_Automatic;
			}
		}
		// -- RepeatableSingleFire
		if (!flagsObj["RepeatableSingleFire"].is_null()) {
			bFlagCheck = flagsObj["RepeatableSingleFire"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_RepeatableSingleFire;
			}
			else {
				instanceData->flags &= ~wFlag_RepeatableSingleFire;
			}
		}
		// -- CantDrop
		if (!flagsObj["CantDrop"].is_null()) {
			bFlagCheck = flagsObj["CantDrop"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_CantDrop;
			}
			else {
				instanceData->flags &= ~wFlag_CantDrop;
			}
		}
		// -- HideBackpack
		if (!flagsObj["HideBackpack"].is_null()) {
			bFlagCheck = flagsObj["HideBackpack"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_HideBackpack;
			}
			else {
				instanceData->flags &= ~wFlag_HideBackpack;
			}
		}
		// -- EmbeddedWeapon
		if (!flagsObj["EmbeddedWeapon"].is_null()) {
			bFlagCheck = flagsObj["EmbeddedWeapon"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_EmbeddedWeapon;
			}
			else {
				instanceData->flags &= ~wFlag_EmbeddedWeapon;
			}
		}
		// -- NotPlayable
		if (!flagsObj["NotPlayable"].is_null()) {
			bFlagCheck = flagsObj["NotPlayable"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_NotPlayable;
			}
			else {
				instanceData->flags &= ~wFlag_NotPlayable;
			}
		}
		// -- HasScope
		if (!flagsObj["HasScope"].is_null()) {
			bFlagCheck = flagsObj["HasScope"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_HasScope;
			}
			else {
				instanceData->flags &= ~wFlag_HasScope;
			}
		}
		// -- BoltAction
		if (!flagsObj["BoltAction"].is_null()) {
			bFlagCheck = flagsObj["BoltAction"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_BoltAction;
			}
			else {
				instanceData->flags &= ~wFlag_BoltAction;
			}
		}
		// -- SecondaryWeapon
		if (!flagsObj["SecondaryWeapon"].is_null()) {
			bFlagCheck = flagsObj["SecondaryWeapon"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_SecondaryWeapon;
			}
			else {
				instanceData->flags &= ~wFlag_SecondaryWeapon;
			}
		}
		// -- DisableShells
		if (!flagsObj["DisableShells"].is_null()) {
			bFlagCheck = flagsObj["DisableShells"];
			if (bFlagCheck) {
				instanceData->flags |= wFlag_DisableShells;
			}
			else {
				instanceData->flags &= ~wFlag_DisableShells;
			}
		}
	}
	// ---- AimModel - guns only:
	if (instanceData->aimModel && !weaponOverride["aimModel"].is_null()) {
		json aimModelObject = weaponOverride["aimModel"];

		// -- AimModel Form
		if (!aimModelObject["formID"].is_null()) {
			std::string aimmodelID = aimModelObject["formID"];
			BGSAimModel * newAimModel = reinterpret_cast<BGSAimModel*>(SAKEUtilities::GetFormFromIdentifier(aimmodelID.c_str()));
			if (newAimModel) {
				instanceData->aimModel = newAimModel;
				_MESSAGE("        Aim Model: %s", aimmodelID.c_str());
			}
		}

		TempBGSAimModel *aimModel = reinterpret_cast<TempBGSAimModel*>(instanceData->aimModel);
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
						_MESSAGE("        AimModel: Cone of Fire - Min Angle: %.2f", aimModel->CoF_MinAngle);
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
						_MESSAGE("        AimModel: Cone of Fire - Max Angle: %.2f", aimModel->CoF_MaxAngle);
					}
				}
				// -- increase per shot
				if (!cofObj["increasePerShot"].is_null()) {
					float fCoFIncPerShot = cofObj["increasePerShot"];
					if (fCoFIncPerShot >= 0.0) {
						aimModel->CoF_IncrPerShot = fCoFIncPerShot;
						_MESSAGE("        AimModel: Cone of Fire - Increase per Shot: %.2f", aimModel->CoF_IncrPerShot);
					}
				}
				// -- decrease per second
				if (!cofObj["decreasePerSec"].is_null()) {
					float fCoFDecPerSec = cofObj["decreasePerSec"];
					if (fCoFDecPerSec >= 0.0) {
						aimModel->CoF_DecrPerSec = fCoFDecPerSec;
						_MESSAGE("        AimModel: Cone of Fire - Decrease per Second: %.2f", aimModel->CoF_DecrPerSec);
					}
				}
				// -- decrease delay ms
				if (!cofObj["decreaseDelayMS"].is_null()) {
					int iCoFDecDelayMS = cofObj["decreaseDelayMS"];
					if (iCoFDecDelayMS >= 0) {
						aimModel->CoF_DecrDelayMS = (UInt32)iCoFDecDelayMS;
						_MESSAGE("        AimModel: Cone of Fire - Decrease Delay ms: %i", aimModel->CoF_DecrDelayMS);
					}
				}
				// -- sneak multiplier
				if (!cofObj["sneakMult"].is_null()) {
					float fCoFSneakMult = cofObj["sneakMult"];
					if (fCoFSneakMult >= 0.0) {
						aimModel->CoF_SneakMult = fCoFSneakMult;
						_MESSAGE("        AimModel: Cone of Fire - Sneak Multiplier: %.2f", aimModel->CoF_SneakMult);
					}
				}
				// -- ironsights multiplier
				if (!cofObj["ironSightsMult"].is_null()) {
					float fCoFIronsightsMult = cofObj["ironSightsMult"];
					if (fCoFIronsightsMult >= 0.0) {
						aimModel->CoF_IronSightsMult = fCoFIronsightsMult;
						_MESSAGE("        AimModel: Cone of Fire - Ironsights Multiplier: %.2f", aimModel->CoF_IronSightsMult);
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
						_MESSAGE("        AimModel: Recoil - Arc Degrees (max angle diff.): %.2f", aimModel->Rec_ArcMaxDegrees);
					}
				}
				// ---- arc rotate
				if (!recObj["arcRotate"].is_null()) {
					float fArcRotate = recObj["arcRotate"];
					if (fArcRotate > 0.0) {
						aimModel->Rec_ArcRotate = fArcRotate;
						_MESSAGE("        AimModel: Recoil - Arc Rotate (base angle): %.2f", aimModel->Rec_ArcRotate);
					}
				}
				// ---- diminish spring force
				if (!recObj["diminishSpringForce"].is_null()) {
					float fRecoilSpringForce = recObj["diminishSpringForce"];
					if (fRecoilSpringForce >= 0.0) {
						aimModel->Rec_DimSpringForce = fRecoilSpringForce;
						_MESSAGE("        AimModel: Recoil - Diminish Spring Force: %.2f", aimModel->Rec_DimSpringForce);
					}
				}
				// ---- diminish sights mult.
				if (!recObj["diminishSightsMult"].is_null()) {
					float fRecoilDimSightsMult = recObj["diminishSightsMult"];
					if (fRecoilDimSightsMult >= 0.0) {
						aimModel->Rec_DimSightsMult = fRecoilDimSightsMult;
						_MESSAGE("        AimModel: Recoil - Diminish Sights Mult.: %.2f", aimModel->Rec_DimSightsMult);
					}
				}
				// ---- min recoil
				float fRecoilMin = 0.0;
				if (!recObj["minPerShot"].is_null()) {
					fRecoilMin = recObj["minPerShot"];
					if (fRecoilMin >= 0.0) {
						aimModel->Rec_MinPerShot = fRecoilMin;
						_MESSAGE("        AimModel: Recoil - Min. Per Shot: %.2f", aimModel->Rec_MinPerShot);
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
						_MESSAGE("        AimModel: Recoil - Max. Per Shot: %.2f", aimModel->Rec_MaxPerShot);
					}
				}
				// ---- hip multiplier
				if (!recObj["hipMult"].is_null()) {
					float fHipMult = recObj["hipMult"];
					if (fHipMult > 0.0) {
						aimModel->Rec_HipMult = fHipMult;
						_MESSAGE("        AimModel: Recoil - Hip Multiplier: %.2f", aimModel->Rec_HipMult);
					}
				}
				recObj.clear();
			}
		}
		aimModelObject.clear();
	}
	// ---- InstanceData.FiringData - guns only:
	if (instanceData->firingData) {
		// -- projectile override - can be none
		if (!weaponOverride["projectileOverride"].is_null()) {
			std::string projectileID = weaponOverride["projectileOverride"];
			BGSProjectile * newProjectile = reinterpret_cast<BGSProjectile*>(SAKEUtilities::GetFormFromIdentifier(projectileID.c_str()));
			instanceData->firingData->projectileOverride = newProjectile;
			if (instanceData->firingData->projectileOverride) {
				_MESSAGE("        Projectile Override: %s", instanceData->firingData->projectileOverride->formID);
			}
			else {
				_MESSAGE("        Projectile Override: none");
			}
		}
	}
}


// ---- Armor - ARMO
void SAKEData::LoadOverrides_Armor(TESObjectARMO * armorForm, json & armorOverride)
{
	if (!armorForm) {
		_MESSAGE("        ERROR: No Armor Form! dump: %s", armorOverride.dump().c_str());
		return;
	}
	_MESSAGE("\n      Editing Armor - 0x%08X (%s)", armorForm->formID, armorForm->GetFullName());
	
	// ---- Base Form Edits:
	// -- name
	if (!armorOverride["name"].is_null()) {
		std::string armorName = armorOverride["name"];
		armorForm->fullName.name = BSFixedString(armorName.c_str());
		_MESSAGE("        Base Name: %s", armorForm->fullName.name.c_str());
	}
	// -- instance naming rules
	if (!armorOverride["instanceNamingRules"].is_null()) {
		std::string inrID = armorOverride["instanceNamingRules"];
		BGSInstanceNamingRules * newNamingRules = reinterpret_cast<BGSInstanceNamingRules*>(SAKEUtilities::GetFormFromIdentifier(inrID.c_str()));
		armorForm->namingRules.rules = newNamingRules;
		if (armorForm->namingRules.rules) {
			_MESSAGE("        Instance Naming Rules: 0x%08X", armorForm->namingRules.rules->formID);
		}
		else {
			_MESSAGE("        Instance Naming Rules: none");
		}
	}
	// -- keywords
	if (!armorOverride["keywords"].is_null()) {
		json keywordsObj = armorOverride["keywords"];
		LoadData_KeywordForm(&armorForm->keywordForm, keywordsObj);
		if (armorForm->keywordForm.numKeywords != 0) {
			_MESSAGE("        Final Keywords list:");
			for (UInt32 j = 0; j < armorForm->keywordForm.numKeywords; j++) {
				_MESSAGE("          %i: 0x%08X (%s)", j, armorForm->keywordForm.keywords[j]->formID, armorForm->keywordForm.keywords[j]->keyword.c_str());
			}
		}
	}

	// ---- Armor.InstanceData - anything past here is only processed if the armor has default mods
	// unk10 = enchantments
	// unk18 = materialswaps
	// unk38 = actorvaluemods
	TESObjectARMO::InstanceData *instanceData = &armorForm->instanceData;
	if (!instanceData) {
		_MESSAGE("\n      WARNING: Armor has no instanceData!");
		return;
	}

	// -- armor rating
	if (!armorOverride["armorRating"].is_null()) {
		int iArmorRating = armorOverride["armorRating"];
		if (iArmorRating > 0) {
			instanceData->armorRating = iArmorRating;
			_MESSAGE("        Base Armor Rating: %i", instanceData->armorRating);
		}
	}
	// -- value
	if (!armorOverride["value"].is_null()) {
		int iBaseValue = armorOverride["value"];
		if (iBaseValue > 0) {
			instanceData->value = iBaseValue;
			_MESSAGE("        Base Value: %i", instanceData->value);
		}
	}
	// -- weight
	if (!armorOverride["weight"].is_null()) {
		float fWeight = armorOverride["weight"];
		if (fWeight > 0.0) {
			instanceData->weight = fWeight;
			_MESSAGE("        Base Weight: %.2f", instanceData->weight);
		}
	}
	// -- health
	if (!armorOverride["health"].is_null()) {
		int iBasehealth = armorOverride["health"];
		if (iBasehealth > 0) {
			instanceData->health = iBasehealth;
			_MESSAGE("        Base Health: %i", instanceData->health);
		}
	}
	// -- damageType resistances
	if (armorOverride["damageTypes"].is_array() && !armorOverride["damageTypes"].empty()) {
		json damageResistsObject = armorOverride["damageTypes"];
		bool clearExisting = false;
		if (!armorOverride["clearExistingDR"].is_null()) {
			clearExisting = armorOverride["clearExistingDR"];
		}
		if (!instanceData->damageTypes) {
			instanceData->damageTypes = new tArray<TBO_InstanceData::DamageTypes>();
		}
		LoadData_DamageTypes(instanceData->damageTypes, damageResistsObject, clearExisting);

		if (instanceData->damageTypes && (instanceData->damageTypes->count != 0)) {
			_MESSAGE("        Final DR list:");
			for (UInt32 j = 0; j < instanceData->damageTypes->count; j++) {
				TBO_InstanceData::DamageTypes checkDT;
				if (instanceData->damageTypes->GetNthItem(j, checkDT)) {
					_MESSAGE("          %i:  0x%08X - %i", j, checkDT.damageType->formID, checkDT.value);
				}
			}
		}
	}
	// -- Enchantments (unk10)
	if (armorOverride["enchantments"].is_array() && !armorOverride["enchantments"].empty()) {
		tArray<EnchantmentItem*> * tempEnchantments = nullptr;
		if (instanceData->unk10 == 0) {
			tempEnchantments = new tArray<EnchantmentItem*>();
		}
		else {
			tempEnchantments = reinterpret_cast<tArray<EnchantmentItem*>*>(instanceData->unk10);
		}
		if (tempEnchantments) {
			for (json::iterator itEnch = armorOverride["enchantments"].begin(); itEnch != armorOverride["enchantments"].end(); ++itEnch) {
				json enchObj = *itEnch;
				if (!enchObj["formID"].is_null()) {
					std::string enchID = enchObj["formID"];
					EnchantmentItem * newEnchantment = reinterpret_cast<EnchantmentItem*>(SAKEUtilities::GetFormFromIdentifier(enchID.c_str()));
					if (newEnchantment) {
						tempEnchantments->Push(newEnchantment);
						_MESSAGE("        Adding Enchantment: %s", enchID.c_str());
					}
				}
			}
			if (tempEnchantments->count != 0) {
				instanceData->unk10 = (UInt64)tempEnchantments;
				tArray<EnchantmentItem*> * testEnchantments = reinterpret_cast<tArray<EnchantmentItem*>*>(instanceData->unk10);
				if (testEnchantments && testEnchantments->count != 0) {
					_MESSAGE("        Final Enchantments list (0x%016X):", instanceData->unk10);
					for (UInt32 j = 0; j < testEnchantments->count; j++) {
						EnchantmentItem * tempEnch = nullptr;
						if (testEnchantments->GetNthItem(j, tempEnch)) {
							if (tempEnch) {
								_MESSAGE("          %i:  0x%08X", j, tempEnch->formID);
							}
						}
					}
				}
				
			}
			else {
				instanceData->unk10 = 0;
			}
		}
	}
	// -- ActorValue modifiers (unk38)
	if (armorOverride["actorValues"].is_array() && !armorOverride["actorValues"].empty()) {
		tArray<TBO_InstanceData::ValueModifier> * tempModifiers = nullptr;
		if (instanceData->unk38 == 0) {
			tempModifiers = new tArray<TBO_InstanceData::ValueModifier>();
		}
		else {
			tempModifiers = reinterpret_cast<tArray<TBO_InstanceData::ValueModifier>*>(instanceData->unk38);
		}
		if (tempModifiers) {
			json avObj;
			for (json::iterator itAV = armorOverride["actorValues"].begin(); itAV != armorOverride["actorValues"].end(); ++itAV) {
				avObj.clear();
				avObj = *itAV;
				if (!avObj["formID"].is_null()) {
					std::string avID = avObj["formID"];
					ActorValueInfo * newAV = reinterpret_cast<ActorValueInfo*>(SAKEUtilities::GetFormFromIdentifier(avID.c_str()));
					if (newAV) {
						int iAVSet = -1;
						if (!avObj["set"].is_null()) {
							iAVSet = avObj["set"];
						}
						int iAVAdd = 0;
						if (!avObj["add"].is_null()) {
							iAVAdd = avObj["add"];
						}
						for (UInt32 j = 0; j < tempModifiers->count; j++) {
							TBO_InstanceData::ValueModifier checkAVMod;
							if (tempModifiers->GetNthItem(j, checkAVMod)) {
								if (checkAVMod.avInfo->formID == newAV->formID) {
									if (iAVSet < 0) {
										iAVSet = checkAVMod.unk08;
									}
									tempModifiers->Remove(j);
									break;
								}
							}
						}

						int newAVValue = (iAVSet < 0) ? iAVAdd : (iAVSet + iAVAdd);
						if (newAVValue > 0) {
							TBO_InstanceData::ValueModifier newAVMod;
							newAVMod.avInfo = newAV;
							newAVMod.unk08 = (UInt32)newAVValue;
							tempModifiers->Push(newAVMod);
							_MESSAGE("        Adding actorValueMod: %s - set: %i, add: %i", avID.c_str(), iAVSet, iAVAdd);
						}
					}
				}
			}

			if (tempModifiers->count != 0) {
				instanceData->unk38 = (UInt64)tempModifiers;
				tArray<TBO_InstanceData::ValueModifier> * testModifiers = reinterpret_cast<tArray<TBO_InstanceData::ValueModifier>*>(instanceData->unk38);
				_MESSAGE("        Final ActorValue Modifiers (0x%016X):", instanceData->unk38);
				if (testModifiers->count != 0) {
					for (UInt32 j = 0; j < testModifiers->count; j++) {
						TBO_InstanceData::ValueModifier checkAVMod;
						if (testModifiers->GetNthItem(j, checkAVMod)) {
							if (checkAVMod.avInfo) {
								_MESSAGE("          %i:  0x%08X - %i", j, checkAVMod.avInfo->formID, checkAVMod.unk08);
							}
						}
					}
				}
			}
			else {
				instanceData->unk38 = 0;
			}
		}
	}
}


// ---- Race
void SAKEData::LoadOverrides_Race(TESRace * raceForm, json & raceOverride)
{
	if (!raceForm) {
		_MESSAGE("        ERROR: No Race Form! dump: %s", raceOverride.dump().c_str());
		return;
	}
	_MESSAGE("\n      Editing Race - 0x%08X (%s)", raceForm->formID, raceForm->GetFullName());

	// -- ActorValues
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
					ActorValueInfo * newAV = reinterpret_cast<ActorValueInfo*>(SAKEUtilities::GetFormFromIdentifier(avIDStr));
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
										_MESSAGE("        Editing ActorValue %s - set: %.2f, add: %.2f", avIDStr.c_str(), fAVSet, fAVAdd);
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
							_MESSAGE("        Adding ActorValue %s - set: %.2f, add: %.2f", avIDStr.c_str(), fAVSet, fAVAdd);
							raceForm->propertySheet.sheet->Push(newAVProp);
						}
					}
				}
			}

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
	// -- Spells/Abilities
	if (!raceOverride["spells"].is_null()) {
		if (raceOverride["spells"].is_array() && !raceOverride["spells"].empty()) {
			std::vector<SpellItem*> spellsList;
			for (json::iterator it = raceOverride["spells"].begin(); it != raceOverride["spells"].end(); ++it) {
				std::string spellIDStr = it.value()["formID"];
				if (!spellIDStr.empty()) {
					SpellItem * newSpell = reinterpret_cast<SpellItem*>(SAKEUtilities::GetFormFromIdentifier(spellIDStr.c_str()));
					if (newSpell && (std::find(spellsList.begin(), spellsList.end(), newSpell) == spellsList.end())) {
						spellsList.push_back(newSpell);
					}
				}
			}
			if (!spellsList.empty()) {
				LoadData_Spells(spellsList, raceForm, nullptr);
			}
			else {
				_MESSAGE("        WARNING: Failed to read spells from config!");
			}
			spellsList.clear();
		}
	}
	// -- keywords
	if (!raceOverride["keywords"].is_null()) {
		LoadData_KeywordForm(&raceForm->keywordForm, raceOverride["keywords"]);
		if (raceForm->keywordForm.numKeywords != 0) {
			_MESSAGE("        Final Keywords list:");
			for (UInt32 j = 0; j < raceForm->keywordForm.numKeywords; j++) {
				_MESSAGE("          %i: 0x%08X (%s)", j, raceForm->keywordForm.keywords[j]->formID, raceForm->keywordForm.keywords[j]->keyword.c_str());
			}
		}
	}
}


// ---- Actor - NPC_
void SAKEData::LoadOverrides_Actor(TESNPC * actorForm, json & actorOverride)
{
	if (!actorForm) {
		_MESSAGE("        ERROR: No Actor Form! dump: %s", actorOverride.dump().c_str());
		return;
	}
	_MESSAGE("\n      Editing Actor - 0x%08X (%s)", actorForm->formID, actorForm->GetFullName());
	
	// -- Name
	if (!actorOverride["name"].is_null()) {
		std::string actorName = actorOverride["name"];
		actorForm->fullName.name = BSFixedString(actorName.c_str());
		_MESSAGE("        Name:  %s", actorForm->GetFullName());
	}
	// -- ActorValues
	if (!actorOverride["actorValues"].is_null()) {
		if (actorOverride["actorValues"].is_array() && !actorOverride["actorValues"].empty()) {
			json curAV;
			for (json::iterator itAVs = actorOverride["actorValues"].begin(); itAVs != actorOverride["actorValues"].end(); ++itAVs) {
				curAV.clear();
				curAV = *itAVs;
				if (!curAV["formID"].is_null()) {
					std::string avIDStr = curAV["formID"];
					ActorValueInfo * newAV = reinterpret_cast<ActorValueInfo*>(SAKEUtilities::GetFormFromIdentifier(avIDStr));
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
	// -- Spells/Abilities
	if (!actorOverride["spells"].is_null()) {
		if (actorOverride["spells"].is_array() && !actorOverride["spells"].empty()) {
			std::vector<SpellItem*> spellsList;
			for (json::iterator it = actorOverride["spells"].begin(); it != actorOverride["spells"].end(); ++it) {
				std::string spellIDStr = it.value()["formID"];
				if (!spellIDStr.empty()) {
					SpellItem * newSpell = reinterpret_cast<SpellItem*>(SAKEUtilities::GetFormFromIdentifier(spellIDStr.c_str()));
					if (newSpell && (std::find(spellsList.begin(), spellsList.end(), newSpell) == spellsList.end())) {
						spellsList.push_back(newSpell);
					}
				}
			}
			if (!spellsList.empty()) {
				LoadData_Spells(spellsList, nullptr, actorForm);
			}
			else {
				_MESSAGE("        WARNING: Failed to read spells from config!");
			}
			spellsList.clear();
		}
	}
	// -- keywords
	if (!actorOverride["keywords"].is_null()) {
		LoadData_KeywordForm(&actorForm->keywords, actorOverride["keywords"]);
		if (actorForm->keywords.numKeywords != 0) {
			_MESSAGE("        Final Keywords list:");
			for (UInt32 j = 0; j < actorForm->keywords.numKeywords; j++) {
				_MESSAGE("          %i: 0x%08X (%s)", j, actorForm->keywords.keywords[j]->formID, actorForm->keywords.keywords[j]->keyword.c_str());
			}
		}
	}
	// -- Class
	if (!actorOverride["npcClass"].is_null()) {
		std::string classFormID = actorOverride["npcClass"];
		TESClass * newClassForm = reinterpret_cast<TESClass*>(SAKEUtilities::GetFormFromIdentifier(classFormID.c_str()));
		if (newClassForm) {
			actorForm->npcClass = newClassForm;
			_MESSAGE("        Edited NPC Class : %s", classFormID.c_str());
		}
	}
	// -- CombatStyle
	if (!actorOverride["combatStyle"].is_null()) {
		std::string csFormID = actorOverride["combatStyle"];
		TESCombatStyle * newCSForm = reinterpret_cast<TESCombatStyle*>(SAKEUtilities::GetFormFromIdentifier(csFormID.c_str()));
		if (newCSForm) {
			actorForm->combatStyle = newCSForm;
			_MESSAGE("        Combat Style : %s", csFormID.c_str());
		}
	}
	// -- Default Outfit
	if (!actorOverride["outfitDefault"].is_null()) {
		std::string outfit1FormID = actorOverride["outfitDefault"];
		BGSOutfit * newOutfitDef = reinterpret_cast<BGSOutfit*>(SAKEUtilities::GetFormFromIdentifier(outfit1FormID.c_str()));
		if (newOutfitDef) {
			actorForm->outfit[0] = newOutfitDef;
			_MESSAGE("        Default Outfit : 0x%08X", newOutfitDef->formID);
		}
	}
	// -- Sleep Outfit
	if (!actorOverride["outfitSleep"].is_null()) {
		std::string outfit2FormID = actorOverride["outfitSleep"];
		BGSOutfit * newOutfitSlp = reinterpret_cast<BGSOutfit*>(SAKEUtilities::GetFormFromIdentifier(outfit2FormID.c_str()));
		if (newOutfitSlp) {
			actorForm->outfit[1] = newOutfitSlp;
			_MESSAGE("        Sleep Outfit : 0x%08X", newOutfitSlp->formID);
		}
	}
}


// ---- Leveled Item - LVLI
void SAKEData::LoadOverrides_LeveledItem(TESLevItem * lliForm, json & llOverride)
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
	_MESSAGE("\n      Editing Leveled Item - 0x%08X", lliForm->formID);

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
		TESGlobal * useglobal = reinterpret_cast<TESGlobal*>(SAKEUtilities::GetFormFromIdentifier(useglobalID));
		if (useglobal) {
			lliForm->leveledList.unk08 = (UInt64)useglobal;
		}
	}

	if (bClearList) {
		// ---- clear the existing list, no need to check for entries to remove
		_MESSAGE("        Clearing original entries...");
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
										_MESSAGE("        Removing entry - ID: 0x%08X, level: %i, count: %i", remFormID.c_str(), curEntry.level, curEntry.count);
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
					_MESSAGE("        Adding entry - ID: %s, level: %i, count: %i, chanceNone: %i", entryFormID.c_str(), tempEntry.level, tempEntry.count, tempEntry.unk8);
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
			
			_MESSAGE("        Final Leveled List:  ChanceNone: %i, UseGlobal: 0x%08X", lliForm->leveledList.unk2A, (UInt32)lliForm->leveledList.unk08);
			for (UInt8 i = 0; i < finalCount; i++) {
				lliForm->leveledList.entries[i] = newEntries[i];
				_MESSAGE("          %i: 0x%08X - level: %i, count: %i, chanceNone: %i", i, newEntries[i].form->formID, newEntries[i].level, newEntries[i].count, newEntries[i].unk8);
			}
		}
	}
}

// ---- Leveled NPC - LVLN
void SAKEData::LoadOverrides_LeveledActor(TESLevCharacter * llcForm, json & llOverride)
{
	if (!llcForm) {
		_MESSAGE("        ERROR: No LeveledActor Form! dump: %s", llOverride.dump().c_str());
		return;
	}
	_MESSAGE("\n      Editing Leveled Actor - 0x%08X", llcForm->formID);

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
		_MESSAGE("        Clearing original entries...");
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
									_MESSAGE("        Removing entry - ID: 0x%08X, level: %i", remFormID.c_str(), curEntry.level);
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
					_MESSAGE("        Adding entry - ID: %s, level: %i", entryFormID.c_str(), tempEntry.level);
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

			_MESSAGE("        Final Leveled List:");
			for (UInt8 i = 0; i < finalCount; i++) {
				llcForm->leveledList.entries[i] = newEntries[i];
				_MESSAGE("          %i: 0x%08X - level: %i", i, newEntries[i].form->formID, newEntries[i].level);
			}
		}
	}
}


// ---- Ammo
void SAKEData::LoadOverrides_Ammo(TempTESAmmo * ammoForm, json & ammoOverride)
{
	if (!ammoForm) {
		_MESSAGE("        ERROR: No Ammo Form! dump: %s", ammoOverride.dump().c_str());
		return;
	}
	_MESSAGE("\n      Editing Ammo - 0x%08X (%s)", ammoForm->formID, ammoForm->GetFullName());
	
	// ---- name
	if (!ammoOverride["name"].is_null()) {
		std::string ammoName = ammoOverride["name"];
		ammoForm->fullName.name = BSFixedString(ammoName.c_str());
		_MESSAGE("        Name: %s", ammoForm->fullName.name.c_str());
	}
	// ---- shortName
	if (!ammoOverride["shortName"].is_null()) {
		std::string shortName = ammoOverride["shortName"];
		ammoForm->shortName = BSFixedString(shortName.c_str());
		_MESSAGE("        Short Name: %s", ammoForm->shortName.c_str());
	}
	// ---- model
	if (!ammoOverride["model"].is_null()) {
		std::string ammoModel = ammoOverride["model"];
		ammoForm->materialSwap.SetModelName(ammoModel.c_str());
		_MESSAGE("        Model: %s", ammoForm->materialSwap.GetModelName());
	}
	// ---- casingModel
	if (!ammoOverride["casingModel"].is_null()) {
		std::string casingModel = ammoOverride["casingModel"];
		ammoForm->casingModel.SetModelName(casingModel.c_str());
		_MESSAGE("        Casing Model: %s", ammoForm->casingModel.GetModelName());
	}
	// ---- value
	if (!ammoOverride["value"].is_null()) {
		int ammoValue = ammoOverride["value"];
		if (ammoValue > -1) {
			ammoForm->value.value = (UInt32)ammoValue;
			_MESSAGE("        Value: %i", ammoForm->value.value);
		}
	}
	// ---- weight
	if (!ammoOverride["weight"].is_null()) {
		float ammoWeight = ammoOverride["weight"];
		if (ammoWeight >= 0.0) {
			ammoForm->weight.weight = ammoWeight;
			_MESSAGE("        Weight: %f", ammoForm->weight.weight);
		}
	}
	// ---- keywords
	if (!ammoOverride["keywords"].is_null()) {
		LoadData_KeywordForm(&ammoForm->keywordForm, ammoOverride["keywords"]);
		if (ammoForm->keywordForm.numKeywords != 0) {
			_MESSAGE("        Keywords:");
			for (UInt32 j = 0; j < ammoForm->keywordForm.numKeywords; j++) {
				_MESSAGE("          %i: 0x%08X (%s)", j, ammoForm->keywordForm.keywords[j]->formID, ammoForm->keywordForm.keywords[j]->keyword.c_str());
			}
		}
	}
	// ---- projectile
	if (!ammoOverride["projectile"].is_null()) {
		std::string projID = ammoOverride["projectile"];
		ammoForm->projectile = reinterpret_cast<BGSProjectile*>(SAKEUtilities::GetFormFromIdentifier(projID));
		if (ammoForm->projectile) {
			_MESSAGE("        Projectile: 0x%08X", ammoForm->projectile->formID);
		}
		else {
			_MESSAGE("        Projectile: none");
		}
	}
	// ---- health/charge
	if (!ammoOverride["health"].is_null()) {
		int ammoHealth = ammoOverride["health"];
		if (ammoHealth >= 0) {
			ammoForm->health = (UInt32)ammoHealth;
			_MESSAGE("        Health: %i", ammoForm->health);
		}
	}
	// ---- damage
	if (!ammoOverride["damage"].is_null()) {
		float ammoDamage = ammoOverride["damage"];
		if (ammoDamage >= 0.0) {
			ammoForm->damage = ammoDamage;
			_MESSAGE("        Damage: %.04f", ammoForm->damage);
		}
	}
	// ---- bounds
	if (!ammoOverride["bounds"].is_null() && !ammoOverride["bounds"].empty()) {
		int boundVal = 0;
		if (!ammoOverride["bounds"]["x1"].is_null()) {
			boundVal = ammoOverride["bounds"]["x1"];
			ammoForm->bounds1.x = (UInt16)boundVal;
		}
		if (!ammoOverride["bounds"]["y1"].is_null()) {
			boundVal = ammoOverride["bounds"]["y1"];
			ammoForm->bounds1.y = (UInt16)boundVal;
		}
		if (!ammoOverride["bounds"]["z1"].is_null()) {
			boundVal = ammoOverride["bounds"]["z1"];
			ammoForm->bounds1.z = (UInt16)boundVal;
		}
		if (!ammoOverride["bounds"]["x2"].is_null()) {
			boundVal = ammoOverride["bounds"]["x2"];
			ammoForm->bounds2.x = (UInt16)boundVal;
		}
		if (!ammoOverride["bounds"]["y2"].is_null()) {
			boundVal = ammoOverride["bounds"]["y2"];
			ammoForm->bounds2.y = (UInt16)boundVal;
		}
		if (!ammoOverride["bounds"]["z2"].is_null()) {
			boundVal = ammoOverride["bounds"]["z2"];
			ammoForm->bounds2.z = (UInt16)boundVal;
		}
		_MESSAGE("        Bounds:\n          x1 - %i, y1 - %i, z1 - %i\n          x2 - %i, y2 - %i, z2 - %i",
			(int)ammoForm->bounds1.x, (int)ammoForm->bounds1.y, (int)ammoForm->bounds1.z, (int)ammoForm->bounds2.x, (int)ammoForm->bounds2.y, (int)ammoForm->bounds2.z
		);
	}
	// ---- Flags:
	if (!ammoOverride["flags"].is_null()) {
		json flagsObj = ammoOverride["flags"];
		bool bFlagCheck = false;
		_MESSAGE("        Flags:");
		// -- IgnoreNormalResistance
		if (!flagsObj["IgnoreNormalResistance"].is_null()) {
			bFlagCheck = flagsObj["IgnoreNormalResistance"];
			if (bFlagCheck) {
				ammoForm->flags |= TempTESAmmo::aFlag_IgnoreNormalResistance;
				_MESSAGE("          IgnoreNormalResistance: true");
			}
			else {
				ammoForm->flags &= ~TempTESAmmo::aFlag_IgnoreNormalResistance;
				_MESSAGE("          IgnoreNormalResistance: false");
			}
		}
		// -- NonPlayable
		if (!flagsObj["NonPlayable"].is_null()) {
			bFlagCheck = flagsObj["NonPlayable"];
			if (bFlagCheck) {
				ammoForm->flags |= TempTESAmmo::aFlag_NonPlayable;
				_MESSAGE("          NonPlayable: true");
			}
			else {
				ammoForm->flags &= ~TempTESAmmo::aFlag_NonPlayable;
				_MESSAGE("          NonPlayable: false");
			}
		}
		// -- CountBased3D
		if (!flagsObj["CountBased3D"].is_null()) {
			bFlagCheck = flagsObj["CountBased3D"];
			if (bFlagCheck) {
				ammoForm->flags |= TempTESAmmo::aFlag_CountBased3D;
				_MESSAGE("          CountBased3D: true");
			}
			else {
				ammoForm->flags &= ~TempTESAmmo::aFlag_CountBased3D;
				_MESSAGE("          CountBased3D: false");
			}
		}
	}
	// ---- destructibleSource
	if (!ammoOverride["destructibleSource"].is_null()) {
		std::string destrID = ammoOverride["destructibleSource"];
		TempTESAmmo * explSource = reinterpret_cast<TempTESAmmo*>(SAKEUtilities::GetFormFromIdentifier(destrID));
		if (explSource) {
			ammoForm->destructible = explSource->destructible;
			_MESSAGE("        Destructible Source: 0x%08X", explSource->formID);
		}
	}
}


// ---- MiscItem/Junk - MISC
void SAKEData::LoadOverrides_Misc(TESObjectMISC * miscForm, json & miscOverride)
{
	if (!miscForm) {
		_MESSAGE("        ERROR: No MiscItem Form! dump: %s", miscOverride.dump().c_str());
		return;
	}
	_MESSAGE("\n      Editing MiscItem - 0x%08X (%s)", miscForm->formID, miscForm->GetFullName());
	
	// -- name
	if (!miscOverride["name"].is_null()) {
		std::string miscName = miscOverride["name"];
		miscForm->fullName.name = BSFixedString(miscName.c_str());
		_MESSAGE("        Name: %s", miscForm->fullName.name.c_str());
	}
	// -- model
	if (!miscOverride["model"].is_null()) {
		std::string miscModel = miscOverride["model"];
		miscForm->materialSwap.SetModelName(miscModel.c_str());
		_MESSAGE("        Model: %s", miscForm->materialSwap.GetModelName());
	}
	// ---- bounds
	if (!miscOverride["bounds"].is_null() && !miscOverride["bounds"].empty()) {
		int boundVal = 0;
		if (!miscOverride["bounds"]["x1"].is_null()) {
			boundVal = miscOverride["bounds"]["x1"];
			miscForm->bounds1.x = (UInt16)boundVal;
		}
		if (!miscOverride["bounds"]["y1"].is_null()) {
			boundVal = miscOverride["bounds"]["y1"];
			miscForm->bounds1.y = (UInt16)boundVal;
		}
		if (!miscOverride["bounds"]["z1"].is_null()) {
			boundVal = miscOverride["bounds"]["z1"];
			miscForm->bounds1.z = (UInt16)boundVal;
		}
		if (!miscOverride["bounds"]["x2"].is_null()) {
			boundVal = miscOverride["bounds"]["x2"];
			miscForm->bounds2.x = (UInt16)boundVal;
		}
		if (!miscOverride["bounds"]["y2"].is_null()) {
			boundVal = miscOverride["bounds"]["y2"];
			miscForm->bounds2.y = (UInt16)boundVal;
		}
		if (!miscOverride["bounds"]["z2"].is_null()) {
			boundVal = miscOverride["bounds"]["z2"];
			miscForm->bounds2.z = (UInt16)boundVal;
		}
		_MESSAGE("        Bounds:\n          x1 - %i, y1 - %i, z1 - %i\n          x2 - %i, y2 - %i, z2 - %i",
			(int)miscForm->bounds1.x, (int)miscForm->bounds1.y, (int)miscForm->bounds1.z, (int)miscForm->bounds2.x, (int)miscForm->bounds2.y, (int)miscForm->bounds2.z
		);
	}
	// -- value
	if (!miscOverride["value"].is_null()) {
		int miscValue = miscOverride["value"];
		if (miscValue > -1) {
			miscForm->value.value = (UInt32)miscValue;
			_MESSAGE("        Value: %i", miscForm->value.value);
		}
	}
	// -- weight
	if (!miscOverride["weight"].is_null()) {
		float miscWeight = miscOverride["weight"];
		if (miscWeight >= 0.0) {
			miscForm->weight.weight = miscWeight;
			_MESSAGE("        Weight: %f", miscForm->weight.weight);
		}
	}
	// ---- keywords
	if (!miscOverride["keywords"].is_null()) {
		LoadData_KeywordForm(&miscForm->keywordForm, miscOverride["keywords"]);
		if (miscForm->keywordForm.numKeywords != 0) {
			_MESSAGE("        Keywords:");
			for (UInt32 j = 0; j < miscForm->keywordForm.numKeywords; j++) {
				_MESSAGE("          %i: 0x%08X (%s)", j, miscForm->keywordForm.keywords[j]->formID, miscForm->keywordForm.keywords[j]->keyword.c_str());
			}
		}
	}
	// -- components
	if (!miscOverride["components"].is_null()) {
		json composObj = miscOverride["components"];
		if (!composObj["clear"].is_null()) {
			bool clearCompos = composObj["clearList"];
			if (clearCompos && miscForm->components) {
				_MESSAGE("        Clearing Components list...");
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
							BGSComponent * tempCompo = reinterpret_cast<BGSComponent*>(SAKEUtilities::GetFormFromIdentifier(compoID));
							if (tempCompo) {
								int tempCount = remEntry["count"];
								TESObjectMISC::Component curRem;
								curRem.component = tempCompo;
								curRem.count = (UInt64)tempCount;
								_MESSAGE("        Removing compo - ID: %s, count: %i", compoID.c_str(), tempCount);
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
						BGSComponent * tempCompo = reinterpret_cast<BGSComponent*>(SAKEUtilities::GetFormFromIdentifier(compoID));
						if (tempCompo) {
							int tempCount = addEntry["count"];
							TESObjectMISC::Component curAdd;
							curAdd.component = tempCompo;
							curAdd.count = (UInt64)tempCount;
							_MESSAGE("        Adding compo - ID: %s, count: %i", compoID.c_str(), tempCount);
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
			}
		}

		if (miscForm->components->count != 0) {
			_MESSAGE("        Components:");
			for (UInt32 i = 0; i < miscForm->components->count; i++) {
				TESObjectMISC::Component tempCompo;
				if (miscForm->components->GetNthItem(i, tempCompo)) {
					_MESSAGE("          %i: 0x%08X x%i", i, tempCompo.component->formID, tempCompo.count);
				}
			}
		}
	}
	// ---- destructibleSource
	if (!miscOverride["destructibleSource"].is_null()) {
		std::string destrID = miscOverride["destructibleSource"];
		TESObjectMISC * explSource = reinterpret_cast<TESObjectMISC*>(SAKEUtilities::GetFormFromIdentifier(destrID));
		if (explSource) {
			miscForm->destructible = explSource->destructible;
			_MESSAGE("        Destructible Source: 0x%08X", explSource->formID);
		}
	}
}


// ---- Key - KEYM
void SAKEData::LoadOverrides_Key(TempTESKey * keyForm, json & miscOverride)
{
	if (!keyForm) {
		_MESSAGE("        ERROR: No Key Form! dump: %s", miscOverride.dump().c_str());
		return;
	}
	_MESSAGE("\n      Editing Key - 0x%08X (%s)", keyForm->formID, keyForm->GetFullName());
	
	// -- name
	if (!miscOverride["name"].is_null()) {
		std::string miscName = miscOverride["name"];
		keyForm->fullName.name = BSFixedString(miscName.c_str());
		_MESSAGE("        Name: %s", keyForm->fullName.name.c_str());
	}
	// -- model
	if (!miscOverride["model"].is_null()) {
		std::string miscModel = miscOverride["model"];
		keyForm->materialSwap.SetModelName(miscModel.c_str());
		_MESSAGE("        Model: %s", keyForm->materialSwap.GetModelName());
	}
	// ---- bounds
	if (!miscOverride["bounds"].is_null() && !miscOverride["bounds"].empty()) {
		int boundVal = 0;
		if (!miscOverride["bounds"]["x1"].is_null()) {
			boundVal = miscOverride["bounds"]["x1"];
			keyForm->bounds1.x = (UInt16)boundVal;
		}
		if (!miscOverride["bounds"]["y1"].is_null()) {
			boundVal = miscOverride["bounds"]["y1"];
			keyForm->bounds1.y = (UInt16)boundVal;
		}
		if (!miscOverride["bounds"]["z1"].is_null()) {
			boundVal = miscOverride["bounds"]["z1"];
			keyForm->bounds1.z = (UInt16)boundVal;
		}
		if (!miscOverride["bounds"]["x2"].is_null()) {
			boundVal = miscOverride["bounds"]["x2"];
			keyForm->bounds2.x = (UInt16)boundVal;
		}
		if (!miscOverride["bounds"]["y2"].is_null()) {
			boundVal = miscOverride["bounds"]["y2"];
			keyForm->bounds2.y = (UInt16)boundVal;
		}
		if (!miscOverride["bounds"]["z2"].is_null()) {
			boundVal = miscOverride["bounds"]["z2"];
			keyForm->bounds2.z = (UInt16)boundVal;
		}
		_MESSAGE("        Bounds:\n          x1 - %i, y1 - %i, z1 - %i\n          x2 - %i, y2 - %i, z2 - %i",
			(int)keyForm->bounds1.x, (int)keyForm->bounds1.y, (int)keyForm->bounds1.z, (int)keyForm->bounds2.x, (int)keyForm->bounds2.y, (int)keyForm->bounds2.z
		);
	}
	// -- value
	if (!miscOverride["value"].is_null()) {
		int miscValue = miscOverride["value"];
		if (miscValue > -1) {
			keyForm->value.value = (UInt32)miscValue;
			_MESSAGE("        Value: %i", keyForm->value.value);
		}
	}
	// -- weight
	if (!miscOverride["weight"].is_null()) {
		float miscWeight = miscOverride["weight"];
		if (miscWeight >= 0.0) {
			keyForm->weight.weight = miscWeight;
			_MESSAGE("        Weight: %f", keyForm->weight.weight);
		}
	}
	// -- keywords
	if (!miscOverride["keywords"].is_null()) {
		LoadData_KeywordForm(&keyForm->keywordForm, miscOverride["keywords"]);
		if (keyForm->keywordForm.numKeywords != 0) {
			_MESSAGE("        Keywords:");
			for (UInt32 j = 0; j < keyForm->keywordForm.numKeywords; j++) {
				_MESSAGE("          %i: 0x%08X (%s)", j, keyForm->keywordForm.keywords[j]->formID, keyForm->keywordForm.keywords[j]->keyword.c_str());
			}
		}
	}
}


// ---- Crafting Component - CMPO
void SAKEData::LoadOverrides_Component(BGSComponent * compoForm, json & compoOverride)
{
	if (!compoForm) {
		_MESSAGE("        ERROR: No Component Form! dump: %s", compoOverride.dump().c_str());
		return;
	}
	_MESSAGE("\n      Editing Component - 0x%08X (%s)", compoForm->formID, compoForm->GetFullName());

	// -- name
	if (!compoOverride["name"].is_null()) {
		std::string compoName = compoOverride["name"];
		compoForm->fullName.name = BSFixedString(compoName.c_str());
		_MESSAGE("        Name: %s", compoForm->fullName.name.c_str());
	}
	// -- value
	if (!compoOverride["value"].is_null()) {
		int compoValue = compoOverride["value"];
		if (compoValue > -1) {
			compoForm->value.value = (UInt32)compoValue;
			_MESSAGE("        Value: %i", compoForm->value.value);
		}
	}
	// -- scrap scalar global
	if (!compoOverride["scrapScalarGlobal"].is_null()) {
		std::string compoScrapGlobalID = compoOverride["scrapScalarGlobal"];
		TESGlobal * compoScrapGlobal = reinterpret_cast<TESGlobal*>(SAKEUtilities::GetFormFromIdentifier(compoScrapGlobalID));
		if (compoScrapGlobal) {
			compoForm->scrapScalar = compoScrapGlobal;
			_MESSAGE("        Scrap Scalar Global: 0x%08X", compoForm->scrapScalar->formID);
		}
	}
	// -- scrap misc item
	if (!compoOverride["scrapMiscItem"].is_null()) {
		std::string compoScrapMiscItemID = compoOverride["scrapMiscItem"];
		TESObjectMISC * compoScrapMiscItem = reinterpret_cast<TESObjectMISC*>(SAKEUtilities::GetFormFromIdentifier(compoScrapMiscItemID));
		if (compoScrapMiscItem) {
			compoForm->scrapItem = compoScrapMiscItem;
			_MESSAGE("        Scrap MiscItem: 0x%08X", compoForm->scrapItem->formID);
		}
	}
}


// ---- Ingestible - ALCH
void SAKEData::LoadOverrides_Ingestible(AlchemyItem * alchForm, json & alchOverride)
{
	if (!alchForm) {
		_MESSAGE("        ERROR: No Ingestible Form! dump: %s", alchOverride.dump().c_str());
		return;
	}
	_MESSAGE("\n      Editing Ingestible - 0x%08X (%s)", alchForm->formID, alchForm->GetFullName());
	
	// -- name
	if (!alchOverride["name"].is_null()) {
		std::string alchName = alchOverride["name"];
		alchForm->name.name = BSFixedString(alchName.c_str());
		_MESSAGE("        Name: %s", alchForm->name.name.c_str());
	}
	// -- model
	if (!alchOverride["model"].is_null()) {
		std::string miscModel = alchOverride["model"];
		alchForm->materialSwap.SetModelName(miscModel.c_str());
		_MESSAGE("        Model: %s", alchForm->materialSwap.GetModelName());
	}
	// -- bounds
	if (!alchOverride["bounds"].is_null() && !alchOverride["bounds"].empty()) {
		int boundVal = 0;
		if (!alchOverride["bounds"]["x1"].is_null()) {
			boundVal = alchOverride["bounds"]["x1"];
			alchForm->bounds1.x = (UInt16)boundVal;
		}
		if (!alchOverride["bounds"]["y1"].is_null()) {
			boundVal = alchOverride["bounds"]["y1"];
			alchForm->bounds1.y = (UInt16)boundVal;
		}
		if (!alchOverride["bounds"]["z1"].is_null()) {
			boundVal = alchOverride["bounds"]["z1"];
			alchForm->bounds1.z = (UInt16)boundVal;
		}
		if (!alchOverride["bounds"]["x2"].is_null()) {
			boundVal = alchOverride["bounds"]["x2"];
			alchForm->bounds2.x = (UInt16)boundVal;
		}
		if (!alchOverride["bounds"]["y2"].is_null()) {
			boundVal = alchOverride["bounds"]["y2"];
			alchForm->bounds2.y = (UInt16)boundVal;
		}
		if (!alchOverride["bounds"]["z2"].is_null()) {
			boundVal = alchOverride["bounds"]["z2"];
			alchForm->bounds2.z = (UInt16)boundVal;
		}
		_MESSAGE("        Bounds:\n          x1 - %i, y1 - %i, z1 - %i\n          x2 - %i, y2 - %i, z2 - %i",
			(int)alchForm->bounds1.x, (int)alchForm->bounds1.y, (int)alchForm->bounds1.z, (int)alchForm->bounds2.x, (int)alchForm->bounds2.y, (int)alchForm->bounds2.z
		);
	}
	// -- weight
	if (!alchOverride["weight"].is_null()) {
		float alchWeight = alchOverride["weight"];
		if (alchWeight >= 0.0) {
			alchForm->weightForm.weight = alchWeight;
			_MESSAGE("        Weight: %f", alchForm->weightForm.weight);
		}
	}
	// -- value
	if (!alchOverride["value"].is_null()) {
		int alchVal = alchOverride["value"];
		if (alchVal >= 0) {
			alchForm->unk1A8 = (UInt32)alchVal;
			_MESSAGE("        Value: %i", alchForm->unk1A8);
		}
	}
	// -- keywords
	if (!alchOverride["keywords"].is_null()) {
		LoadData_KeywordForm(&alchForm->keywordForm, alchOverride["keywords"]);
		if (alchForm->keywordForm.numKeywords != 0) {
			_MESSAGE("        Keywords:");
			for (UInt32 j = 0; j < alchForm->keywordForm.numKeywords; j++) {
				_MESSAGE("          %i: 0x%08X (%s)", j, alchForm->keywordForm.keywords[j]->formID, alchForm->keywordForm.keywords[j]->keyword.c_str());
			}
		}
	}
	// -- destructibleSource
	if (!alchOverride["destructibleSource"].is_null()) {
		std::string destrID = alchOverride["destructibleSource"];
		AlchemyItem * explSource = reinterpret_cast<AlchemyItem*>(SAKEUtilities::GetFormFromIdentifier(destrID));
		if (explSource) {
			alchForm->destructible = explSource->destructible;
			_MESSAGE("        Destructible Source: 0x%08X", explSource->formID);
		}
	}
}


// ---- Projectile - PROJ
void SAKEData::LoadOverrides_Projectile(TempBGSProjectile * projForm, json & projOverride)
{
	if (!projForm) {
		_MESSAGE("        ERROR: No Projectile Form! dump: %s", projOverride.dump().c_str());
		return;
	}
	_MESSAGE("\n      Editing Projectile - 0x%08X", projForm->formID);
	
	// -- type
	if (!projOverride["type"].is_null()) {
		int projType = projOverride["type"];
		if (projType >= 0) {
			projForm->data.type = projType;
			_MESSAGE("        Type: %i", projForm->data.type);
		}
	}
	// -- name
	if (!projOverride["name"].is_null()) {
		std::string projName = projOverride["name"];
		projForm->fullName.name = BSFixedString(projName.c_str());
		_MESSAGE("        Name: %s", projForm->fullName.name.c_str());
	}
	// -- model
	if (!projOverride["model"].is_null()) {
		std::string projModel = projOverride["model"];
		projForm->model.SetModelName(projModel.c_str());
		_MESSAGE("        Model: %s", projForm->model.GetModelName());
	}
	// -- bounds
	if (!projOverride["bounds"].is_null() && !projOverride["bounds"].empty()) {
		int boundVal = 0;
		if (!projOverride["bounds"]["x1"].is_null()) {
			boundVal = projOverride["bounds"]["x1"];
			projForm->bounds1.x = (UInt16)boundVal;
		}
		if (!projOverride["bounds"]["y1"].is_null()) {
			boundVal = projOverride["bounds"]["y1"];
			projForm->bounds1.y = (UInt16)boundVal;
		}
		if (!projOverride["bounds"]["z1"].is_null()) {
			boundVal = projOverride["bounds"]["z1"];
			projForm->bounds1.z = (UInt16)boundVal;
		}
		if (!projOverride["bounds"]["x2"].is_null()) {
			boundVal = projOverride["bounds"]["x2"];
			projForm->bounds2.x = (UInt16)boundVal;
		}
		if (!projOverride["bounds"]["y2"].is_null()) {
			boundVal = projOverride["bounds"]["y2"];
			projForm->bounds2.y = (UInt16)boundVal;
		}
		if (!projOverride["bounds"]["z2"].is_null()) {
			boundVal = projOverride["bounds"]["z2"];
			projForm->bounds2.z = (UInt16)boundVal;
		}
		_MESSAGE("        Bounds:\n          x1 - %i, y1 - %i, z1 - %i\n          x2 - %i, y2 - %i, z2 - %i",
			(int)projForm->bounds1.x, (int)projForm->bounds1.y, (int)projForm->bounds1.z, (int)projForm->bounds2.x, (int)projForm->bounds2.y, (int)projForm->bounds2.z
		);
	}
	// -- light
	if (!projOverride["light"].is_null()) {
		std::string lightID = projOverride["light"];
		projForm->data.light = SAKEUtilities::GetFormFromIdentifier(lightID);
		if (projForm->data.light) {
			_MESSAGE("        Light: 0x%08X", projForm->data.light->formID);
		}
		else {
			_MESSAGE("        Light: none");
		}
	}
	// -- sound
	if (!projOverride["sound"].is_null()) {
		std::string soundID = projOverride["sound"];
		projForm->data.sound = reinterpret_cast<BGSSoundDescriptorForm*>(SAKEUtilities::GetFormFromIdentifier(soundID));
		if (projForm->data.sound) {
			_MESSAGE("        Sound: 0x%08X", projForm->data.sound->formID);
		}
		else {
			_MESSAGE("        Sound: none");
		}
	}
	// -- soundLevel
	if (!projOverride["soundLevel"].is_null()) {
		int soundLevel = projOverride["soundLevel"];
		if (soundLevel >= 0) {
			projForm->soundLevel = soundLevel;
			_MESSAGE("        Sound Level: %i", projForm->soundLevel);
		}
	}
	// -- muzzle flash model
	if (!projOverride["muzFlashModel"].is_null()) {
		std::string muzFlashModel = projOverride["muzFlashModel"];
		projForm->muzFlashModel.SetModelName(muzFlashModel.c_str());
		_MESSAGE("        Muzzle Flash Model: %s", projForm->muzFlashModel.GetModelName());
	}
	// -- muzzle flash light
	if (!projOverride["muzFlashLight"].is_null()) {
		std::string muzFlashLightID = projOverride["muzFlashLight"];
		projForm->data.muzFlashLight = SAKEUtilities::GetFormFromIdentifier(muzFlashLightID);
		if (projForm->data.muzFlashLight) {
			_MESSAGE("        Muzzle Flash Light: 0x%08X", projForm->data.muzFlashLight->formID);
		}
		else {
			_MESSAGE("        Muzzle Flash Light: none");
		}
	}
	// -- muzzle flash duration
	if (!projOverride["muzFlashDuration"].is_null()) {
		float muzFlashDur = projOverride["muzFlashDuration"];
		if (muzFlashDur >= 0.0) {
			projForm->data.muzflashDuration = muzFlashDur;
			_MESSAGE("        Muzzle Flash Duration: %f", projForm->data.muzflashDuration);
		}
	}
	// -- fadeDuration
	if (!projOverride["fadeDuration"].is_null()) {
		float fadeDuration = projOverride["fadeDuration"];
		if (fadeDuration >= 0.0) {
			projForm->data.fadeDuration = fadeDuration;
			_MESSAGE("        Fade Duration: %f", projForm->data.fadeDuration);
		}
	}
	// -- lifeTime
	if (!projOverride["lifeTime"].is_null()) {
		float lifeTime = projOverride["lifeTime"];
		if (lifeTime >= 0.0) {
			projForm->data.lifeTime = lifeTime;
			_MESSAGE("        Lifetime: %f", projForm->data.lifeTime);
		}
	}
	// -- relaunchInterval
	if (!projOverride["relaunchInterval"].is_null()) {
		float relaunchInterval = projOverride["relaunchInterval"];
		if (relaunchInterval >= 0.0) {
			projForm->data.relaunchInterval = relaunchInterval;
			_MESSAGE("        Relaunch Interval: %f", projForm->data.relaunchInterval);
		}
	}
	// -- speed
	if (!projOverride["speed"].is_null()) {
		float speed = projOverride["speed"];
		if (speed >= 0.0) {
			projForm->data.speed = speed;
			_MESSAGE("        Speed: %f", projForm->data.speed);
		}
	}
	// -- range
	if (!projOverride["range"].is_null()) {
		float range = projOverride["range"];
		if (range >= 0.0) {
			projForm->data.range = range;
			_MESSAGE("        Range: %f", projForm->data.range);
		}
	}
	// -- gravity
	if (!projOverride["gravity"].is_null()) {
		float gravity = projOverride["gravity"];
		if (gravity >= 0.0) {
			projForm->data.gravity = gravity;
			_MESSAGE("        Gravity: %f", projForm->data.gravity);
		}
	}
	// -- impactForce
	if (!projOverride["impactForce"].is_null()) {
		float impactForce = projOverride["impactForce"];
		if (impactForce >= 0.0) {
			projForm->data.impactForce = impactForce;
			_MESSAGE("        Impact Force: %f", projForm->data.impactForce);
		}
	}
	// -- collisionRadius
	if (!projOverride["collisionRadius"].is_null()) {
		float collisionRadius = projOverride["collisionRadius"];
		if (collisionRadius >= 0.0) {
			projForm->data.collisionRadius = collisionRadius;
			_MESSAGE("        Collision Radius: %f", projForm->data.impactForce);
		}
	}
	// -- explosion
	if (!projOverride["explosion"].is_null()) {
		std::string explosionID = projOverride["explosion"];
		projForm->data.explosion = SAKEUtilities::GetFormFromIdentifier(explosionID);
		if (projForm->data.explosion) {
			_MESSAGE("        Explosion: 0x%08X", projForm->data.explosion->formID);
		}
		else {
			_MESSAGE("        Explosion: none");
		}
	}
	// -- explosionProximity
	if (!projOverride["explosionProximity"].is_null()) {
		float explosionProximity = projOverride["explosionProximity"];
		if (explosionProximity >= 0.0) {
			projForm->data.explosionProximity = explosionProximity;
			_MESSAGE("        Explosion Proximity: %f", projForm->data.explosionProximity);
		}
	}
	// -- explosionTimer
	if (!projOverride["explosionTimer"].is_null()) {
		float explosionTimer = projOverride["explosionTimer"];
		if (explosionTimer >= 0.0) {
			projForm->data.explosionTimer = explosionTimer;
			_MESSAGE("        Explosion Timer: %f", projForm->data.explosionTimer);
		}
	}
	// -- countdownSound
	if (!projOverride["countdownSound"].is_null()) {
		std::string countdownSoundID = projOverride["countdownSound"];
		projForm->data.countdownSound = reinterpret_cast<BGSSoundDescriptorForm*>(SAKEUtilities::GetFormFromIdentifier(countdownSoundID));
		if (projForm->data.countdownSound) {
			_MESSAGE("        Countdown Sound: 0x%08X", projForm->data.countdownSound->formID);
		}
		else {
			_MESSAGE("        Countdown Sound: none");
		}
	}
	// -- disableSound
	if (!projOverride["disableSound"].is_null()) {
		std::string disableSoundID = projOverride["disableSound"];
		projForm->data.disableSound = reinterpret_cast<BGSSoundDescriptorForm*>(SAKEUtilities::GetFormFromIdentifier(disableSoundID));
		if (projForm->data.disableSound) {
			_MESSAGE("        Disable Sound: 0x%08X", projForm->data.disableSound->formID);
		}
		else {
			_MESSAGE("        Disable Sound: none");
		}
	}
	// -- weaponSource
	if (!projOverride["weaponSource"].is_null()) {
		std::string weaponSourceID = projOverride["weaponSource"];
		projForm->data.weaponSource = reinterpret_cast<TESObjectWEAP*>(SAKEUtilities::GetFormFromIdentifier(weaponSourceID));
		if (projForm->data.weaponSource) {
			_MESSAGE("        Weapon Source: 0x%08X", projForm->data.weaponSource->formID);
		}
		else {
			_MESSAGE("        Weapon Source: none");
		}
	}
	// -- vatsProjectile
	if (!projOverride["vatsProjectile"].is_null()) {
		std::string vatsProjectileID = projOverride["vatsProjectile"];
		projForm->data.vatsProjectile = reinterpret_cast<BGSProjectile*>(SAKEUtilities::GetFormFromIdentifier(vatsProjectileID));
		if (projForm->data.vatsProjectile) {
			_MESSAGE("        VATS Projectile: 0x%08X", projForm->data.vatsProjectile->formID);
		}
		else {
			_MESSAGE("        VATS Projectile: none");
		}
	}
	// -- coneSpread
	if (!projOverride["coneSpread"].is_null()) {
		float coneSpread = projOverride["coneSpread"];
		if (coneSpread >= 0.0) {
			projForm->data.coneSpread = coneSpread;
			_MESSAGE("        Cone Spread: %f", projForm->data.coneSpread);
		}
	}
	// -- tracerFrequency
	if (!projOverride["tracerFrequency"].is_null()) {
		int tracerFrequency = projOverride["tracerFrequency"];
		if (tracerFrequency >= 0) {
			projForm->data.tracerFrequency = tracerFrequency;
			_MESSAGE("        Tracer Frequency: %f", projForm->data.tracerFrequency);
		}
	}
	// -- collisionLayer
	if (!projOverride["collisionLayer"].is_null()) {
		std::string collisionLayerID = projOverride["collisionLayer"];
		projForm->data.collisionLayer = SAKEUtilities::GetFormFromIdentifier(collisionLayerID);
		if (projForm->data.collisionLayer) {
			_MESSAGE("        CollisionLayer: 0x%08X", projForm->data.collisionLayer->formID);
		}
		else {
			_MESSAGE("        CollisionLayer: none");
		}
	}
	// -- decalData
	if (!projOverride["decalData"].is_null()) {
		std::string decalDataID = projOverride["decalData"];
		projForm->data.decalData = SAKEUtilities::GetFormFromIdentifier(decalDataID);
		if (projForm->data.decalData) {
			_MESSAGE("        DecalData: 0x%08X", projForm->data.decalData->formID);
		}
		else {
			_MESSAGE("        DecalData: none");
		}
	}
	// ---- Flags:
	if (!projOverride["flags"].is_null()) {
		json flagsObj = projOverride["flags"];
		bool bFlagCheck = false;
		_MESSAGE("        Flags:");
		// -- HitScan
		if (!flagsObj["HitScan"].is_null()) {
			bFlagCheck = flagsObj["HitScan"];
			if (bFlagCheck) {
				projForm->data.flags |= TempBGSProjectile::pFlag_Hitscan;
				_MESSAGE("          Hitscan: true");
			}
			else {
				projForm->data.flags &= ~TempBGSProjectile::pFlag_Hitscan;
				_MESSAGE("          Hitscan: false");
			}
		}
		// -- Explosion
		if (!flagsObj["Explosion"].is_null()) {
			bFlagCheck = flagsObj["Explosion"];
			if (bFlagCheck) {
				projForm->data.flags |= TempBGSProjectile::pFlag_Explosion;
				_MESSAGE("          Explosion: true");
			}
			else {
				projForm->data.flags &= ~TempBGSProjectile::pFlag_Explosion;
				_MESSAGE("          Explosion: false");
			}
		}
		// -- AltTrigger
		if (!flagsObj["AltTrigger"].is_null()) {
			bFlagCheck = flagsObj["AltTrigger"];
			if (bFlagCheck) {
				projForm->data.flags |= TempBGSProjectile::pFlag_AltTrigger;
				_MESSAGE("          AltTrigger: true");
			}
			else {
				projForm->data.flags &= ~TempBGSProjectile::pFlag_AltTrigger;
				_MESSAGE("          AltTrigger: false");
			}
		}
		// -- MuzzleFlash
		if (!flagsObj["MuzzleFlash"].is_null()) {
			bFlagCheck = flagsObj["MuzzleFlash"];
			if (bFlagCheck) {
				projForm->data.flags |= TempBGSProjectile::pFlag_MuzzleFlash;
				_MESSAGE("          MuzzleFlash: true");
			}
			else {
				projForm->data.flags &= ~TempBGSProjectile::pFlag_MuzzleFlash;
				_MESSAGE("          MuzzleFlash: false");
			}
		}
		// -- Unknown4
		if (!flagsObj["Unknown4"].is_null()) {
			bFlagCheck = flagsObj["Unknown4"];
			if (bFlagCheck) {
				projForm->data.flags |= TempBGSProjectile::pFlag_Unknown4;
				_MESSAGE("          Unknown4: true");
			}
			else {
				projForm->data.flags &= ~TempBGSProjectile::pFlag_Unknown4;
				_MESSAGE("          Unknown4: false");
			}
		}
		// -- CanBeDisabled
		if (!flagsObj["CanBeDisabled"].is_null()) {
			bFlagCheck = flagsObj["CanBeDisabled"];
			if (bFlagCheck) {
				projForm->data.flags |= TempBGSProjectile::pFlag_CanBeDisabled;
				_MESSAGE("          CanBeDisabled: true");
			}
			else {
				projForm->data.flags &= ~TempBGSProjectile::pFlag_CanBeDisabled;
				_MESSAGE("          CanBeDisabled: false");
			}
		}
		// -- CanBePickedUp
		if (!flagsObj["CanBePickedUp"].is_null()) {
			bFlagCheck = flagsObj["CanBePickedUp"];
			if (bFlagCheck) {
				projForm->data.flags |= TempBGSProjectile::pFlag_CanBePickedUp;
				_MESSAGE("          CanBePickedUp: true");
			}
			else {
				projForm->data.flags &= ~TempBGSProjectile::pFlag_CanBePickedUp;
				_MESSAGE("          CanBePickedUp: false");
			}
		}
		// -- Supersonic
		if (!flagsObj["Supersonic"].is_null()) {
			bFlagCheck = flagsObj["Supersonic"];
			if (bFlagCheck) {
				projForm->data.flags |= TempBGSProjectile::pFlag_Supersonic;
				_MESSAGE("          Supersonic: true");
			}
			else {
				projForm->data.flags &= ~TempBGSProjectile::pFlag_Supersonic;
				_MESSAGE("          Supersonic: false");
			}
		}
		// -- PinsLimbs
		if (!flagsObj["PinsLimbs"].is_null()) {
			bFlagCheck = flagsObj["PinsLimbs"];
			if (bFlagCheck) {
				projForm->data.flags |= TempBGSProjectile::pFlag_PinsLimbs;
				_MESSAGE("          PinsLimbs: true");
			}
			else {
				projForm->data.flags &= ~TempBGSProjectile::pFlag_PinsLimbs;
				_MESSAGE("          PinsLimbs: false");
			}
		}
		// -- PassThroughSmallTransparent
		if (!flagsObj["PassThroughSmallTransparent"].is_null()) {
			bFlagCheck = flagsObj["PassThroughSmallTransparent"];
			if (bFlagCheck) {
				projForm->data.flags |= TempBGSProjectile::pFlag_PassThroughSmallTransparent;
				_MESSAGE("          PassThroughSmallTransparent: true");
			}
			else {
				projForm->data.flags &= ~TempBGSProjectile::pFlag_PassThroughSmallTransparent;
				_MESSAGE("          PassThroughSmallTransparent: false");
			}
		}
		// -- DisableCombatAimCorrection
		if (!flagsObj["DisableCombatAimCorrection"].is_null()) {
			bFlagCheck = flagsObj["DisableCombatAimCorrection"];
			if (bFlagCheck) {
				projForm->data.flags |= TempBGSProjectile::pFlag_DisableCombatAimCorrection;
				_MESSAGE("          DisableCombatAimCorrection: true");
			}
			else {
				projForm->data.flags &= ~TempBGSProjectile::pFlag_DisableCombatAimCorrection;
				_MESSAGE("          DisableCombatAimCorrection: false");
			}
		}
		// -- PenetratesGeometry
		if (!flagsObj["PenetratesGeometry"].is_null()) {
			bFlagCheck = flagsObj["PenetratesGeometry"];
			if (bFlagCheck) {
				projForm->data.flags |= TempBGSProjectile::pFlag_PenetratesGeometry;
				_MESSAGE("          PenetratesGeometry: true");
			}
			else {
				projForm->data.flags &= ~TempBGSProjectile::pFlag_PenetratesGeometry;
				_MESSAGE("          PenetratesGeometry: false");
			}
		}
		// -- ContinuousUpdate
		if (!flagsObj["ContinuousUpdate"].is_null()) {
			bFlagCheck = flagsObj["ContinuousUpdate"];
			if (bFlagCheck) {
				projForm->data.flags |= TempBGSProjectile::pFlag_ContinuousUpdate;
				_MESSAGE("          ContinuousUpdate: true");
			}
			else {
				projForm->data.flags &= ~TempBGSProjectile::pFlag_ContinuousUpdate;
				_MESSAGE("          ContinuousUpdate: false");
			}
		}
		// -- SeeksTarget
		if (!flagsObj["SeeksTarget"].is_null()) {
			bFlagCheck = flagsObj["SeeksTarget"];
			if (bFlagCheck) {
				projForm->data.flags |= TempBGSProjectile::pFlag_SeeksTarget;
				_MESSAGE("          SeeksTarget: true");
			}
			else {
				projForm->data.flags &= ~TempBGSProjectile::pFlag_SeeksTarget;
				_MESSAGE("          SeeksTarget: false");
			}
		}
	}
	// -- destructibleSource
	if (!projOverride["destructibleSource"].is_null()) {
		std::string destrID = projOverride["destructibleSource"];
		TempBGSProjectile * explSource = reinterpret_cast<TempBGSProjectile*>(SAKEUtilities::GetFormFromIdentifier(destrID));
		if (explSource) {
			projForm->destructible = explSource->destructible;
			_MESSAGE("        Destructible Source: 0x%08X", explSource->formID);
		}
	}
}


// ---- EncounterZone - ECZN
void SAKEData::LoadOverrides_EncounterZone(BGSEncounterZone * enczForm, json & enczOverride)
{
	if (!enczForm) {
		_MESSAGE("        ERROR: No EncounterZone Form! dump: %s", enczOverride.dump().c_str());
		return;
	}
	_MESSAGE("\n      Editing EncounterZone - 0x%08X (%s)", enczForm->formID, enczForm->GetFullName());
	
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
		_MESSAGE("        Edited Levels - Min: %i, Max: %i", enczForm->minLevel, enczForm->maxLevel);
	}
}


// ---- Name Prefixes - any item Forms
void SAKEData::LoadNamePrefix(TESForm * targetForm, const std::string & prefixStr)
{
	if (!targetForm || prefixStr.empty()) {
		return;
	}
	bool bEdited = true;
	std::string formName;
	formName.append(prefixStr);
	formName.append(" ");
	formName.append(targetForm->GetFullName());

	_MESSAGE("    Original Name: %s", targetForm->GetFullName());
	
	switch (targetForm->formType) {
		case kFormType_ARMO:
			EditName_Armor(reinterpret_cast<TESObjectARMO*>(targetForm), formName);
			break;
		case kFormType_WEAP:
			EditName_Weapon(reinterpret_cast<TESObjectWEAP*>(targetForm), formName);
			break;
		case kFormType_AMMO:
			EditName_Ammo(reinterpret_cast<TESAmmo*>(targetForm), formName);
			break;
		case kFormType_MISC:
			EditName_Misc(reinterpret_cast<TESObjectMISC*>(targetForm), formName);
			break;
		case kFormType_CMPO:
			EditName_Compo(reinterpret_cast<BGSComponent*>(targetForm), formName);
			break;
		case kFormType_ALCH:
			EditName_Ingestible(reinterpret_cast<AlchemyItem*>(targetForm), formName);
			break;
		case kFormType_KEYM:
			EditName_Key(reinterpret_cast<TempTESKey*>(targetForm), formName);
			break;
		default:
			bEdited = false;
	}
	_MESSAGE("    Edited Name: %s", targetForm->GetFullName());
}


// ---- Game Settings - GMST
void SAKEData::LoadGameSettings(json & settingOverrides)
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

