#include "SAKELoader.h"


// ---------------- FormID/Identifier Utilities:

namespace SAKEUtilities
{
	// returns a form's plugin name
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
				(*g_dataHandler)->modList.loadedMods.GetNthItem(modIndex, tempMod);
				return tempMod->name;
			}
			else {
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
				else
					return "References";
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


	// reads a ValueModifier (ActorValue, amount) from the passed identifier string
	bool GetAVModiferFromIdentifer(const std::string & formIdentifier, TBO_InstanceData::ValueModifier & avMod)
	{
		if (formIdentifier.c_str() != "none") {
			std::size_t pos = formIdentifier.find_first_of(", ");
			std::string avIdentifier = formIdentifier.substr(0, pos);
			std::string avValueStr = formIdentifier.substr(pos + 2);

			ActorValueInfo *newAV = (ActorValueInfo*)GetFormFromIdentifier(avIdentifier.c_str());
			if (newAV) {
				avMod.avInfo = newAV;
				UInt32 newVal = std::stoi(avValueStr);
				if (newVal != 0) {
					avMod.unk08 = newVal;
					return true;
				}
			}
		}
		return false;
	}

	bool GetAVPropertyFromIdentifer(const std::string & formIdentifier, BGSPropertySheet::AVIFProperty & avMod)
	{
		if (formIdentifier.c_str() != "none") {
			std::size_t pos = formIdentifier.find_first_of(", ");
			std::string avIdentifier = formIdentifier.substr(0, pos);
			std::string avValueStr = formIdentifier.substr(pos + 2);

			ActorValueInfo *newAV = (ActorValueInfo*)GetFormFromIdentifier(avIdentifier.c_str());
			if (newAV) {
				avMod.actorValue = newAV;
				avMod.value = std::stof(avValueStr);
				return true;
			}
		}
		return false;
	}

	// reads a TBO_InstanceData::DamageTypes (DamageType, amount) from the passed identifier string
	bool GetDamageTypeFromIdentifier(const std::string & formIdentifier, TBO_InstanceData::DamageTypes & tempDT)
	{
		if (formIdentifier.c_str() != "none") {
			std::size_t pos = formIdentifier.find_first_of(", ");
			std::string avIdentifier = formIdentifier.substr(0, pos);
			std::string avValueStr = formIdentifier.substr(pos + 2);

			tempDT.damageType = (BGSDamageType*)GetFormFromIdentifier(avIdentifier.c_str());
			if (tempDT.damageType) {
				UInt32 newVal = std::stoi(avValueStr);
				if (newVal > 0) {
					tempDT.value = newVal;
					return true;
				}
			}
		}
		return false;
	}

}


// ----------------------- Config file loading

namespace SAKEData
{
	// cached general variables:

	bool bLoadWeapons = false;
	bool bLoadRaces = false;
	bool bLoadActors = false;
	bool bLoadArmors = false;

	bool bShowDebugInfo = false;
	bool bNPCsUseAmmo = false;

	int iWeaponCount = 0;
	int iRaceCount = 0;
	int iArmorCount = 0;
	int iActorCount = 0;

	tArray<TESRace*> editedRaces;
	tArray<TESObjectWEAP*> editedWeapons;
	tArray<TESObjectARMO*> editedArmors;


	// loads weapon config data
	bool LoadData_Weapons(const std::string & configDir)
	{
		CSimpleIniA iniWeap;
		iniWeap.SetUnicode();
		iniWeap.SetMultiKey(true);

		std::string iniFileStr;
		iniFileStr.append(configDir);

		if (iniWeap.LoadFile(iniFileStr.c_str()) > -1) {
			CSimpleIni::TNamesDepend weapons, values;
			iniWeap.GetAllSections(weapons);
			if (weapons.size() > 0) {
				CSimpleIni::TNamesDepend::const_iterator weap = weapons.begin();
				for (; weap != weapons.end(); ++weap) {
					const char *weapID = weap->pItem;
					TESObjectWEAP *weaponBase = (TESObjectWEAP*)SAKEUtilities::GetFormFromIdentifier(weapID);

					if (weaponBase) {
						TESObjectWEAP::InstanceData *instanceData = &weaponBase->weapData;
						if (instanceData) {
							const char *keyID = "";

							// -- Damage
							int iDamage = iniWeap.GetLongValue(weapID, "iBaseDamage", -1);
							if (iDamage > -1) {
								instanceData->baseDamage = (UInt16)min(max(0, iDamage), 0xFF);
							}

							// -- DamageTypes
							if (iniWeap.GetAllValues(weapID, "sBaseDamageTypes", values)) {
								int dtCount = values.size();
								if (dtCount > 0) {
									if (!instanceData->damageTypes) {
										instanceData->damageTypes = new tArray<TBO_InstanceData::DamageTypes>();
									}
									int oldDTCount = instanceData->damageTypes->count;

									CSimpleIni::TNamesDepend::const_iterator i = values.begin();
									for (; i != values.end(); ++i) {
										TBO_InstanceData::DamageTypes tempDT;
										if (SAKEUtilities::GetDamageTypeFromIdentifier(i->pItem, tempDT)) {
											int iFound = -1;
											for (UInt16 j = 0; j < oldDTCount; j++) {
												TBO_InstanceData::DamageTypes checkDT;
												if (instanceData->damageTypes->GetNthItem(j, checkDT)) {
													if (checkDT.damageType == tempDT.damageType) {
														checkDT.value = tempDT.value;
														iFound = j;
														break;
													}
												}
											}
											if (iFound < 0) {
												instanceData->damageTypes->Push(tempDT);
											}
										}
									}
								}
							}
							values.clear();

							// -- Secondary Damage
							float fBaseSecDamage = iniWeap.GetDoubleValue(weapID, "fBaseSecDamage", -1.0);
							if (fBaseSecDamage >= 0.0) {
								instanceData->secondary = fBaseSecDamage;
							}

							// -- AP Cost
							float fAPCost = iniWeap.GetDoubleValue(weapID, "fAPCost", -1.0);
							if (fAPCost >= 0.0) {
								instanceData->actionCost = fAPCost;
							}

							// -- Critical Charge/Chance Multiplier
							float fCritChargeVal = iniWeap.GetDoubleValue(weapID, "fCritChargeBonus", -1.0);
							if (fCritChargeVal >= 0.0) {
								instanceData->critChargeBonus = fCritChargeVal;
							}

							// -- Range
							float fRangeMax = iniWeap.GetDoubleValue(weapID, "fBaseRangeMax", -1.0);
							if (fRangeMax >= 0.0) {
								instanceData->maxRange = fRangeMax;
							}
							float fRangeMin = iniWeap.GetDoubleValue(weapID, "fBaseRangeMin", -1.0);
							if (fRangeMin >= 0.0) {
								instanceData->minRange = fRangeMin;
							}

							// -- Out of Range Dmg Multiplier
							float fOoRMult = iniWeap.GetDoubleValue(weapID, "fOutOfRangeMult", -1.0);
							if (fOoRMult >= 0.0) {
								instanceData->outOfRangeMultiplier = fOoRMult;
							}

							// -- Melee Reach
							float fReach = iniWeap.GetDoubleValue(weapID, "fReach", -1.0);
							if (fReach >= 0.0) {
								instanceData->reach = fReach;
							}

							// -- Melee Stagger
							int iStaggerAmount = iniWeap.GetLongValue(weapID, "iStaggerAmount", -1);
							if ((iStaggerAmount > -1) && (iStaggerAmount < 5)) {
								instanceData->stagger = (UInt32)iStaggerAmount;
							}

							// -- Value
							int iCapsValue = iniWeap.GetDoubleValue(weapID, "iBaseValue", -1);
							if (iCapsValue >= 0) {
								instanceData->value = (UInt32)iCapsValue;
							}

							// -- Weight
							float fWeight = iniWeap.GetDoubleValue(weapID, "fWeight", -1.0);
							if (fWeight >= 0.0) {
								instanceData->weight = fWeight;
							}

							// ---- AimModel Edits:
							if (instanceData->aimModel) {
								ATAimModel *aimModel = (ATAimModel*)instanceData->aimModel;
								if (aimModel) {
									// -- Recoil Spring Force Multiplier
									float springForce = iniWeap.GetDoubleValue(weapID, "fRecoilSpringForce", -1.0);
									if (springForce >= 0.0) {
										aimModel->Rec_DimSpringForce = springForce;
									}
									// -- Min/Max Recoil
									float recoilMin = iniWeap.GetDoubleValue(weapID, "fRecoilMin", -1.0);
									if (recoilMin >= 0.0) {
										aimModel->Rec_MinPerShot = recoilMin;
									}
									float recoilMax = iniWeap.GetDoubleValue(weapID, "fRecoilMax", -1.0);
									if (recoilMax >= 0.0) {
										aimModel->Rec_MaxPerShot = recoilMax;
									}
									// -- Recoil Hip Multiplier
									float recoilHipMult = iniWeap.GetDoubleValue(weapID, "fRecoilHipMult", -1.0);
									if (recoilHipMult >= 0.0) {
										aimModel->Rec_HipMult = recoilHipMult;
									}

									// Min/Max Cone of Fire
									float cofMin = iniWeap.GetDoubleValue(weapID, "fConeOfFireMin", -1.0);
									if (cofMin >= 0.0) {
										aimModel->CoF_MinAngle = cofMin;
									}
									float cofMax = iniWeap.GetDoubleValue(weapID, "fConeOfFireMax", -1.0);
									if (cofMax >= 0.0) {
										aimModel->CoF_MaxAngle = cofMax;
									}
								}
							}

							// -- Ammo Type
							keyID = iniWeap.GetValue(weapID, "sAmmoID", "none");
							TESAmmo *ammoDefault = (TESAmmo*)SAKEUtilities::GetFormFromIdentifier(keyID);
							if (ammoDefault) {
								instanceData->ammo = ammoDefault;
							}

							// -- NPC Ammo List
							keyID = iniWeap.GetValue(weapID, "sAmmoLootList", "none");
							TESLevItem *ammoLoot = (TESLevItem*)SAKEUtilities::GetFormFromIdentifier(keyID);
							if (ammoLoot) {
								instanceData->addAmmoList = ammoLoot;
							}

							// -- Projectile Override
							if (instanceData->firingData) {
								keyID = iniWeap.GetValue(weapID, "sProjectile", "none");
								BGSProjectile *projOvrd = (BGSProjectile*)SAKEUtilities::GetFormFromIdentifier(keyID);
								if (projOvrd) {
									instanceData->firingData->projectileOverride = projOvrd;
								}
							}

							// -- ImpactDataSet
							keyID = iniWeap.GetValue(weapID, "sImpactDataSet", "none");
							BGSImpactDataSet *tempImpactData = (BGSImpactDataSet*)SAKEUtilities::GetFormFromIdentifier(keyID);
							if (tempImpactData) {
								instanceData->unk58 = tempImpactData;
							}

							// ---- Flags:
							if (instanceData->flags) {
								// -- NPCsUseAmmo
								if (bNPCsUseAmmo) {
									bool bUseAmmo = iniWeap.GetBoolValue(weapID, "bNPCsUseAmmo", false);
									if (bUseAmmo)
										instanceData->flags |= wFlag_NPCsUseAmmo;
									else
										instanceData->flags &= wFlag_NPCsUseAmmo;
								}
							}

							// -- ActorValue Modifiers
							if (iniWeap.GetAllValues(weapID, "sActorValues", values)) {
								if (values.size() > 0) {
									if (!instanceData->modifiers) {
										instanceData->modifiers = new tArray<TBO_InstanceData::ValueModifier>();
									}
									CSimpleIni::TNamesDepend::const_iterator i = values.begin();
									for (; i != values.end(); ++i) {
										keyID = i->pItem;
										TBO_InstanceData::ValueModifier newAVMod;
										if (SAKEUtilities::GetAVModiferFromIdentifer(keyID, newAVMod)) {
											instanceData->modifiers->Push(newAVMod);
										}
									}
								}
							}
							values.clear();

							// -- Enchantments
							if (iniWeap.GetAllValues(weapID, "sEnchantments", values)) {
								if (values.size() > 0) {
									if (!instanceData->enchantments) {
										instanceData->enchantments = new tArray<EnchantmentItem*>();
									}
									CSimpleIni::TNamesDepend::const_iterator i = values.begin();
									for (; i != values.end(); ++i) {
										keyID = i->pItem;
										EnchantmentItem *tempEnch = (EnchantmentItem*)SAKEUtilities::GetFormFromIdentifier(keyID);
										if (tempEnch) {
											instanceData->enchantments->Push(tempEnch);
										}
									}
								}
							}
							values.clear();

							// -- Keywords
							if (iniWeap.GetAllValues(weapID, "sKeywords", values)) {
								int kwCount = values.size();
								if (kwCount > 0) {
									int baseKWCount = weaponBase->keyword.numKeywords;
									tArray<BGSKeyword*> tempKWs;
									for (UInt32 j = 0; j < baseKWCount; j++) {
										if (weaponBase->keyword.keywords[j]) {
											tempKWs.Push(weaponBase->keyword.keywords[j]);
										}
									}

									CSimpleIni::TNamesDepend::const_iterator i = values.begin();
									for (; i != values.end(); ++i) {
										keyID = i->pItem;
										BGSKeyword *tempKW = (BGSKeyword*)SAKEUtilities::GetFormFromIdentifier(keyID);
										if (tempKW) {
											if (tempKWs.GetItemIndex(tempKW) > -1) {
												tempKWs.Push(tempKW);
											}
										}
									}
									values.clear();

									kwCount = tempKWs.count;
									if (kwCount > 0) {
										weaponBase->keyword.numKeywords = kwCount;
										weaponBase->keyword.keywords = new BGSKeyword*[kwCount];

										for (UInt8 j = 0; j < baseKWCount; j++) {
											weaponBase->keyword.keywords[j] = tempKWs[j];
										}
									}

								}
							}
							values.clear();
						}
						editedWeapons.Push(weaponBase);
					}
				}
				return true;
			}
		}
		return false;
	}



	// loads armor config data
	bool LoadData_Armors(const std::string & configDir)
	{
		CSimpleIniA iniArmor;
		iniArmor.SetUnicode();
		iniArmor.SetMultiKey(true);

		std::string iniFileStr;
		iniFileStr.append(configDir);

		if (iniArmor.LoadFile(iniFileStr.c_str()) > -1) {
			CSimpleIni::TNamesDepend armors, values;
			iniArmor.GetAllSections(armors);
			if (armors.size() > 0) {
				CSimpleIni::TNamesDepend::const_iterator armor = armors.begin();
				for (; armor != armors.end(); ++armor) {
					const char *armorID = armor->pItem;
					TESObjectARMO *armorBase = (TESObjectARMO*)SAKEUtilities::GetFormFromIdentifier(armorID);

					if (armorBase) {
						TESObjectARMO::InstanceData *instanceData = &armorBase->instanceData;
						if (instanceData) {
							// -- Armor Rating (Phys Resistance)
							int iArmorRating = iniArmor.GetLongValue(armorID, "iArmorRating", -1);
							if (iArmorRating > -1) {
								instanceData->armorRating = (UInt16)min(iArmorRating, 0xFFFF);
							}

							// -- DamageType Resistances
							if (iniArmor.GetAllValues(armorID, "sDamageTypes", values)) {
								int dtCount = values.size();
								if (dtCount > 0) {
									if (!instanceData->damageTypes) {
										instanceData->damageTypes = new tArray<TBO_InstanceData::DamageTypes>();
									}
									int oldDTCount = instanceData->damageTypes->count;

									CSimpleIni::TNamesDepend::const_iterator i = values.begin();
									for (; i != values.end(); ++i) {
										TBO_InstanceData::DamageTypes tempDT;
										if (SAKEUtilities::GetDamageTypeFromIdentifier(i->pItem, tempDT)) {
											int iFound = -1;
											for (UInt16 j = 0; j < oldDTCount; j++) {
												TBO_InstanceData::DamageTypes checkDT;
												if (instanceData->damageTypes->GetNthItem(j, checkDT)) {
													if (checkDT.damageType == tempDT.damageType) {
														checkDT.value = tempDT.value;
														iFound = j;
														break;
													}
												}
											}
											if (iFound < 0) {
												instanceData->damageTypes->Push(tempDT);
											}
										}
									}
								}
							}
							values.clear();

							// -- Value
							int iBaseValue = iniArmor.GetLongValue(armorID, "iBaseValue", -1);
							if (iBaseValue > -1) {
								instanceData->value = (UInt32)iBaseValue;
							}

							// -- weight
							float fWeight = iniArmor.GetDoubleValue(armorID, "fBaseWeight", -1.0);
							if (fWeight > -1.0) {
								instanceData->weight = fWeight;
							}

							// -- health
							int iHealth = iniArmor.GetLongValue(armorID, "iBaseHealth", -1);
							if (iHealth > -1) {
								instanceData->health = (UInt32)iHealth;
							}

							// -- Keywords
							if (iniArmor.GetAllValues(armorID, "sKeywords", values)) {
								int kwCount = values.size();
								if (kwCount > 0) {
									int baseKWCount = instanceData->keywords->numKeywords;
									tArray<BGSKeyword*> tempKWs;
									for (UInt32 j = 0; j < baseKWCount; j++) {
										if (instanceData->keywords->keywords[j]) {
											tempKWs.Push(instanceData->keywords->keywords[j]);
										}
									}

									CSimpleIni::TNamesDepend::const_iterator i = values.begin();
									for (; i != values.end(); ++i) {
										const char * keyID = i->pItem;
										BGSKeyword *tempKW = (BGSKeyword*)SAKEUtilities::GetFormFromIdentifier(keyID);
										if (tempKW) {
											if (tempKWs.GetItemIndex(tempKW) > -1) {
												tempKWs.Push(tempKW);
											}
										}
									}
									values.clear();

									kwCount = tempKWs.count;
									if (kwCount > 0) {
										instanceData->keywords->numKeywords = kwCount;
										instanceData->keywords->keywords = new BGSKeyword*[kwCount];

										for (UInt8 j = 0; j < baseKWCount; j++) {
											instanceData->keywords->keywords[j] = tempKWs[j];
										}
									}

								}
							}
							values.clear();

							editedArmors.Push(armorBase);
						}
					}
				}
			}
		}
		return false;
	}


	// loads race config data
	bool LoadData_Races(const std::string & configDir)
	{
		CSimpleIniA iniRaces;
		iniRaces.SetUnicode();
		iniRaces.SetMultiKey(true);

		if (iniRaces.LoadFile(configDir.c_str()) > -1) {
			CSimpleIni::TNamesDepend sections, abilities, actorvalues;
			const char *spellID = "", *raceID = "", *avID = "";
			SpellItem * newSpell = nullptr;
			TESRace * targetRace = nullptr;
			UInt32 iNumSpells = 0;

			iniRaces.GetAllSections(sections);
			CSimpleIni::TNamesDepend::const_iterator p = sections.begin();
			for (; p != sections.end(); ++p) {
				raceID = p->pItem;
				targetRace = (TESRace*)SAKEUtilities::GetFormFromIdentifier(raceID);
				if (targetRace) {
					if (!targetRace->spellList.unk08) {
						// no spells set for this race by default - create new spellData
						targetRace->spellList.unk08 = new ATSpellListEntries();
					}
					ATSpellListEntries * spellData = (ATSpellListEntries*)targetRace->spellList.unk08;
					
					if (spellData) {
						iNumSpells = spellData->numSpells;
						tArray<SpellItem*> tempSpells;
						for (UInt32 i = 0; i < iNumSpells; i++) {
							tempSpells.Push(spellData->spells[i]);
						}

						// Abilities list:
						if (iniRaces.GetAllValues(raceID, "sAbilities", abilities)) {
							CSimpleIni::TNamesDepend::const_iterator k = abilities.begin();
							for (; k != abilities.end(); ++k) {
								spellID = k->pItem;
								newSpell = (SpellItem*)SAKEUtilities::GetFormFromIdentifier(spellID);
								if (newSpell) {
									// make sure the spell isn't already in the list
									if (tempSpells.GetItemIndex(newSpell) == -1) {
										tempSpells.Push(newSpell);
									}
								}
							}
						}
						abilities.clear();

						if (iNumSpells != tempSpells.count) {
							iNumSpells = tempSpells.count;
							spellData->numSpells = iNumSpells;
							spellData->spells = new SpellItem*[iNumSpells];
							for (UInt32 j = 0; j < iNumSpells; j++) {
								spellData->spells[j] = tempSpells[j];
							}
						}
					}

					// ActorValues:
					if (targetRace->propertySheet.sheet) {
						if (iniRaces.GetAllValues(raceID, "sActorValues", actorvalues)) {
							CSimpleIni::TNamesDepend::const_iterator l = actorvalues.begin();
							for (; l != actorvalues.end(); ++l) {
								avID = l->pItem;
								BGSPropertySheet::AVIFProperty newAVProp;
								if (SAKEUtilities::GetAVPropertyFromIdentifer(avID, newAVProp)) {
									// check if the AV is already in the list, replace value if it does
									bool bFound = false;
									for (UInt32 i = 0; i < targetRace->propertySheet.sheet->count; i++) {
										BGSPropertySheet::AVIFProperty checkAVProp;
										if (targetRace->propertySheet.sheet->GetNthItem(i, checkAVProp)) {
											if (checkAVProp.actorValue == newAVProp.actorValue) {
												bFound = true;
												checkAVProp.value = newAVProp.value;
												break;
											}
										}
									}
									// new AV - add to the property sheet
									if (!bFound) {
										targetRace->propertySheet.sheet->Push(newAVProp);
									}
								}
							}
						}
						actorvalues.clear();
					}

					// add the race to the log index if needed
					if (bShowDebugInfo) {
						if (editedRaces.GetItemIndex(targetRace) == -1) {
							editedRaces.Push(targetRace);
						}
					}
				}
			}
			return true;
		}
		return false;
	}


	// TBD: loads ActorBase config data
	bool LoadData_Actors(const std::string & configDir)
	{
		return false;
	}



	void LogDebugInfo()
	{
		_MESSAGE("\nRaces:");
		for (UInt32 i = 0; i < editedRaces.count; i++) {
			_MESSAGE("\n  %i: 0x%08X - %s", i, editedRaces[i]->formID, editedRaces[i]->fullName.name.c_str());

			ATSpellListEntries * spellData = (ATSpellListEntries*)editedRaces[i]->spellList.unk08;
			if (spellData) {
				int iNumSpells = spellData->numSpells;

				_MESSAGE("\n    Abilities:");
				for (UInt32 j = 0; j < iNumSpells; j++)
					_MESSAGE("        %i: 0x%08X - %s", j, spellData->spells[j]->formID, spellData->spells[j]->name.name.c_str());
			}

			if (editedRaces[i]->propertySheet.sheet->count > 0) {
				_MESSAGE("\n    ActorValues:");
				BGSPropertySheet::AVIFProperty tempAVProp;
				for (UInt32 j = 0; j < editedRaces[i]->propertySheet.sheet->count; j++) {
					if (editedRaces[i]->propertySheet.sheet->GetNthItem(j, tempAVProp)) {
						if (tempAVProp.actorValue)
							_MESSAGE("        %i: 0x%08X - %s, %f", j, tempAVProp.actorValue->formID, tempAVProp.actorValue->avName, tempAVProp.value);
						else
							_MESSAGE("        %i: none, %f", j, tempAVProp.value);
					}
				}
			}
		}

		_MESSAGE("\nWeapons:");
		for (UInt32 i = 0; i < editedWeapons.count; i++) {
			_MESSAGE("  %i: %08x - %s", i, editedWeapons[i]->formID, editedWeapons[i]->fullName.name.c_str());
		}

		_MESSAGE("\nArmors:");
		for (UInt32 i = 0; i < editedArmors.count; i++) {
			_MESSAGE("  %i: %08x - %s", i, editedArmors[i]->formID, editedArmors[i]->fullName.name.c_str());
		}

	}
}


// startup
void SAKEData::LoadGameData()
{
	CSimpleIniA iniMain;
	iniMain.SetUnicode();
	iniMain.SetMultiKey(true);

	CSimpleIniA iniIndex;
	iniIndex.SetUnicode();
	iniIndex.SetMultiKey(true);

	// load global config
	if (iniMain.LoadFile(".\\Data\\F4SE\\Plugins\\SAKE.ini") > -1) {
		const char *configDataPath = iniMain.GetValue("General", "sConfigFoldersPath", "");
		if (configDataPath != "") {
			bLoadWeapons = iniMain.GetBoolValue("General", "bLoadWeapons", false);
			bLoadArmors = iniMain.GetBoolValue("General", "bLoadArmors", false);
			bLoadRaces = iniMain.GetBoolValue("General", "bLoadRaces", false);
			bLoadActors = iniMain.GetBoolValue("General", "bLoadActors", false);

			bShowDebugInfo =	iniMain.GetBoolValue("General", "bShowDebugInfo", false);
			bNPCsUseAmmo =		iniMain.GetBoolValue("General", "bNPCsUseAmmo", false);

			std::string tempDirStr;
			const char * templateToLoad = "";

			CSimpleIni::TNamesDepend templates, configPaths;
			if (iniMain.GetAllValues("General", "sTemplates", templates)) {
				bool bWeapsLoaded = false, bRacesloaded = false, bArmorsLoaded = false;
				std::string tempLoadPath;
				const char * tempPath = "";

				CSimpleIni::TNamesDepend::const_iterator i = templates.begin();
				for (; i != templates.end(); ++i) {
					templateToLoad = i->pItem;
					tempDirStr.append(configDataPath);
					tempDirStr.append(templateToLoad);
					tempDirStr.append("\\_Index.ini");

					if (iniIndex.LoadFile(tempDirStr.c_str()) > -1) {
						bWeapsLoaded = false;
						bRacesloaded = false;
						bArmorsLoaded = false;

						// weapon edits
						if (bLoadWeapons) {
							if (iniIndex.GetAllValues("Index", "sWeaponPaths", configPaths)) {
								CSimpleIni::TNamesDepend::const_iterator j = configPaths.begin();
								for (; j != configPaths.end(); ++j) {
									tempPath = j->pItem;
									tempLoadPath.append(configDataPath);
									tempLoadPath.append(templateToLoad);
									tempLoadPath.append("\\");
									tempLoadPath.append(tempPath);

									bWeapsLoaded = LoadData_Weapons(tempLoadPath);
									tempLoadPath.clear();
								}
							}
							configPaths.clear();
						}

						// race abilities/avs
						if (bLoadRaces) {
							if (iniIndex.GetAllValues("Index", "sRacePaths", configPaths)) {
								CSimpleIni::TNamesDepend::const_iterator j = configPaths.begin();
								for (; j != configPaths.end(); ++j) {
									tempPath = j->pItem;
									tempLoadPath.append(configDataPath);
									tempLoadPath.append(templateToLoad);
									tempLoadPath.append("\\");
									tempLoadPath.append(tempPath);

									bRacesloaded = LoadData_Races(tempLoadPath);
									tempLoadPath.clear();
								}
							}
						}

						if (bLoadArmors) {
							if (iniIndex.GetAllValues("Index", "sArmorPaths", configPaths)) {
								CSimpleIni::TNamesDepend::const_iterator j = configPaths.begin();
								for (; j != configPaths.end(); ++j) {
									tempPath = j->pItem;
									tempLoadPath.append(configDataPath);
									tempLoadPath.append(templateToLoad);
									tempLoadPath.append("\\");
									tempLoadPath.append(tempPath);

									bArmorsLoaded = LoadData_Armors(tempLoadPath);
									tempLoadPath.clear();
								}
							}
						}

						_MESSAGE("Loaded Template: %s...", templateToLoad);
						
					}
					iniIndex.Reset();
					tempDirStr.clear();
				}

				if (bShowDebugInfo) {
					LogDebugInfo();
				}

				editedRaces.Clear();
				editedWeapons.Clear();
			}
			templates.clear();
		}
		else {
			_MESSAGE("\n!!Unable to load config path.");
		}
	}

}

