#include "SAKELoader.h"
#include <vector>
#include <Windows.h>


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


	// credit: stackoverflow user herohuyongtao:

	std::vector<std::string> GetDirFileNames(const std::string folder)
	{
		std::vector<std::string> names;
		std::string search_path = folder + "/*.*";
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

	std::vector<std::string> GetDirFolderNames(const std::string folder)
	{
		std::vector<std::string> names;
		std::string search_path = folder + "/*.*";
		WIN32_FIND_DATA fd;
		std::string tempName;
		std::size_t pos;
		HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					tempName = fd.cFileName;
					pos = tempName.find_first_of(".");
					if (pos != (std::size_t)0) {
						names.push_back(fd.cFileName);
					}
				}
			} while (::FindNextFile(hFind, &fd));
			::FindClose(hFind);
		}
		return names;
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
	bool bVerboseDebugInfo = false;


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
							// -- Base Name
							const char * sBaseName = iniWeap.GetValue(weapID, "sBaseName", "none");
							if (sBaseName != "none") {
								weaponBase->fullName.name = BSFixedString(sBaseName);
							}

							// -- instance naming rules
							keyID = iniWeap.GetValue(weapID, "sNamingRules", "none");
							BGSInstanceNamingRules * newNamingRules = (BGSInstanceNamingRules*)SAKEUtilities::GetFormFromIdentifier(keyID);
							if (newNamingRules) {
								weaponBase->namingRules.rules = newNamingRules;
							}

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
											bool bFound = tempDT.value != 0;
											for (UInt16 j = 0; j < oldDTCount; j++) {
												TBO_InstanceData::DamageTypes checkDT;
												if (instanceData->damageTypes->GetNthItem(j, checkDT)) {
													if (checkDT.damageType == tempDT.damageType) {
														instanceData->damageTypes->Remove(j);
														break;
													}
												}
											}
											if (bFound) {
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

							// -- Attack Delay
							float fAttackDelay = iniWeap.GetDoubleValue(weapID, "fAttackDelay", -1.0);
							if (fAttackDelay >= 0.0) {
								instanceData->attackDelay = fAttackDelay;
							}

							// -- Speed Multiplier
							float fSpeedMult = iniWeap.GetDoubleValue(weapID, "fSpeedMult", -1.0);
							if (fSpeedMult >= 0.0) {
								instanceData->speed = fSpeedMult;
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

									// -- Min/Max Cone of Fire
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

							// -- Flags:
							if (instanceData->flags) {
								// -- NPCsUseAmmo
								bool bUseAmmo = iniWeap.GetBoolValue(weapID, "bNPCsUseAmmo", false);
								if (bUseAmmo)
									instanceData->flags |= wFlag_NPCsUseAmmo;
								else
									instanceData->flags &= ~wFlag_NPCsUseAmmo;
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

							// ---- Keywords:
							tArray<BGSKeyword*> keywordsAdd, keywordsRemove;

							// Keywords to Remove
							if (iniWeap.GetAllValues(weapID, "sKeywordsRemove", values)) {
								if (values.size() > 0) {
									CSimpleIni::TNamesDepend::const_iterator i = values.begin();
									for (; i != values.end(); ++i) {
										keyID = i->pItem;
										BGSKeyword *tempKW = (BGSKeyword*)SAKEUtilities::GetFormFromIdentifier(keyID);
										if (tempKW) {
											keywordsRemove.Push(tempKW);
										}
									}
								}
							}
							values.clear();

							// Keywords to Add
							if (iniWeap.GetAllValues(weapID, "sKeywords", values)) {
								if (values.size() > 0) {
									CSimpleIni::TNamesDepend::const_iterator i = values.begin();
									for (; i != values.end(); ++i) {
										keyID = i->pItem;
										BGSKeyword *tempKW = (BGSKeyword*)SAKEUtilities::GetFormFromIdentifier(keyID);
										if (tempKW) {
											if (keywordsAdd.GetItemIndex(tempKW) < 0) {
												keywordsAdd.Push(tempKW);
											}
										}
									}
								}
							}
							values.clear();

							// rebuild keywords list if needed
							if (keywordsAdd.count > 0 || keywordsRemove.count > 0) {
								int baseKWCount = weaponBase->keyword.numKeywords;
								for (UInt32 j = 0; j < baseKWCount; j++) {
									BGSKeyword * tempKW = weaponBase->keyword.keywords[j];
									if (tempKW) {
										if (keywordsAdd.GetItemIndex(tempKW) < 0) {
											if (keywordsRemove.GetItemIndex(tempKW) < 0) {
												keywordsAdd.Push(tempKW);
											}
										}
									}
								}

								int kwCount = keywordsAdd.count;
								if (kwCount > 0) {
									weaponBase->keyword.numKeywords = kwCount;
									weaponBase->keyword.keywords = new BGSKeyword*[kwCount];
									for (UInt8 j = 0; j < kwCount; j++) {
										weaponBase->keyword.keywords[j] = keywordsAdd[j];
									}
								}
							}
							keywordsAdd.Clear();
							keywordsRemove.Clear();

							if (bShowDebugInfo) {
								_MESSAGE("      0x%08x - %s", weaponBase->formID, weaponBase->fullName.name.c_str());
							}
							
						}
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
				tArray<BGSKeyword*> keywordsAdd, keywordsRemove;
				CSimpleIni::TNamesDepend::const_iterator armor = armors.begin();
				for (; armor != armors.end(); ++armor) {
					const char *armorID = armor->pItem;
					TESObjectARMO *armorBase = (TESObjectARMO*)SAKEUtilities::GetFormFromIdentifier(armorID);
					if (armorBase) {
						TESObjectARMO::InstanceData *instanceData = &armorBase->instanceData;
						if (instanceData) {
							const char * keyID = "";

							// -- Base Name
							const char * sBaseName = iniArmor.GetValue(armorID, "sBaseName", "none");
							if (sBaseName != "none") {
								armorBase->fullName.name = BSFixedString(sBaseName);
							}

							// -- instance naming rules
							keyID = iniArmor.GetValue(armorID, "sNamingRules", "none");
							BGSInstanceNamingRules * newNamingRules = (BGSInstanceNamingRules*)SAKEUtilities::GetFormFromIdentifier(keyID);
							if (newNamingRules) {
								armorBase->namingRules.rules = newNamingRules;
							}

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
											bool bFound = false;
											for (UInt16 j = 0; j < oldDTCount; j++) {
												TBO_InstanceData::DamageTypes checkDT;
												if (instanceData->damageTypes->GetNthItem(j, checkDT)) {
													bFound = (checkDT.damageType == tempDT.damageType);
													if (checkDT.damageType == tempDT.damageType) {

														checkDT.value = tempDT.value;
														bFound = true;
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
							
							// Keywords to Remove
							if (iniArmor.GetAllValues(armorID, "sKeywordsRemove", values)) {
								if (values.size() > 0) {
									CSimpleIni::TNamesDepend::const_iterator i = values.begin();
									for (; i != values.end(); ++i) {
										keyID = i->pItem;
										BGSKeyword *tempKW = (BGSKeyword*)SAKEUtilities::GetFormFromIdentifier(keyID);
										if (tempKW) {
											keywordsRemove.Push(tempKW);
										}
									}
								}
							}
							values.clear();

							// Keywords to Add
							if (iniArmor.GetAllValues(armorID, "sKeywords", values)) {
								if (values.size() > 0) {
									CSimpleIni::TNamesDepend::const_iterator i = values.begin();
									for (; i != values.end(); ++i) {
										keyID = i->pItem;
										BGSKeyword *tempKW = (BGSKeyword*)SAKEUtilities::GetFormFromIdentifier(keyID);
										if (tempKW) {
											if (keywordsAdd.GetItemIndex(tempKW) < 0) {
												keywordsAdd.Push(tempKW);
											}
										}
									}
								}
							}
							values.clear();

							// rebuild keywords list if needed
							if (keywordsAdd.count > 0 || keywordsRemove.count > 0) {
								int baseKWCount = armorBase->keywordForm.numKeywords;
								for (UInt32 j = 0; j < baseKWCount; j++) {
									BGSKeyword * tempKW = armorBase->keywordForm.keywords[j];
									if (tempKW) {
										if (keywordsAdd.GetItemIndex(tempKW) < 0) {
											if (keywordsRemove.GetItemIndex(tempKW) < 0) {
												keywordsAdd.Push(tempKW);
											}
										}
									}
								}
								int kwCount = keywordsAdd.count;
								if (kwCount > 0) {
									armorBase->keywordForm.numKeywords = kwCount;
									armorBase->keywordForm.keywords = new BGSKeyword*[kwCount];
									for (UInt8 j = 0; j < kwCount; j++) {
										armorBase->keywordForm.keywords[j] = keywordsAdd[j];
									}
								}
							}
							keywordsAdd.Clear();
							keywordsRemove.Clear();


							// debug log output
							if (bShowDebugInfo) {
								_MESSAGE("      0x%08x - %s", armorBase->formID, armorBase->fullName.name.c_str());
							}
							
						}
					}
				}
				armors.clear();
				return true;
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
			CSimpleIni::TNamesDepend sections, abilities, actorvalues, keywords;
			const char *spellID = "", *actorID = "", *avID = "", *kwID;
			SpellItem * newSpell = nullptr;
			TESRace * targetRace = nullptr;
			UInt32 iNumSpells = 0;

			iniRaces.GetAllSections(sections);
			if (sections.size() > 0) {
				tArray<SpellItem*> tempSpells;
				tArray<BGSKeyword*> tempKeywords;

				CSimpleIni::TNamesDepend::const_iterator p = sections.begin();
				for (; p != sections.end(); ++p) {
					actorID = p->pItem;
					targetRace = (TESRace*)SAKEUtilities::GetFormFromIdentifier(actorID);
					if (targetRace) {
						// Abilities list:
						if (iniRaces.GetAllValues(actorID, "sAbilities", abilities)) {
							if (abilities.size() > 0) {
								if (!targetRace->spellList.unk08) {
									targetRace->spellList.unk08 = new ATSpellListEntries();
								}
								ATSpellListEntries * spellData = (ATSpellListEntries*)targetRace->spellList.unk08;
								if (spellData) {
									iNumSpells = spellData->numSpells;
									for (UInt32 i = 0; i < iNumSpells; i++) {
										tempSpells.Push(spellData->spells[i]);
									}
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
								if (iNumSpells != tempSpells.count) {
									iNumSpells = tempSpells.count;
									spellData->numSpells = iNumSpells;
									spellData->spells = new SpellItem*[iNumSpells];
									for (UInt32 j = 0; j < iNumSpells; j++) {
										spellData->spells[j] = tempSpells[j];
									}
								}
								tempSpells.Clear();
							}
						}
						abilities.clear();

						// ActorValues:
						if (targetRace->propertySheet.sheet) {
							if (iniRaces.GetAllValues(actorID, "sActorValues", actorvalues)) {
								if (actorvalues.size() > 0) {
									CSimpleIni::TNamesDepend::const_iterator l = actorvalues.begin();
									for (; l != actorvalues.end(); ++l) {
										avID = l->pItem;
										BGSPropertySheet::AVIFProperty newAVProp;
										if (SAKEUtilities::GetAVPropertyFromIdentifer(avID, newAVProp)) {
											bool bFound = false;
											for (UInt32 i = 0; i < targetRace->propertySheet.sheet->count; i++) {
												BGSPropertySheet::AVIFProperty checkAVProp;
												if (targetRace->propertySheet.sheet->GetNthItem(i, checkAVProp)) {
													if (checkAVProp.actorValue == newAVProp.actorValue) {
														checkAVProp.value = newAVProp.value;
														bFound = true;
														break;
													}
												}
											}
											if (!bFound) {
												targetRace->propertySheet.sheet->Push(newAVProp);
											}
										}
									}
								}
							}
							actorvalues.clear();
						}

						// Keywords:
						if (iniRaces.GetAllValues(actorID, "sKeywords", keywords)) {
							if (keywords.size() > 0) {
								if (targetRace->keywordForm.numKeywords > 0) {
									for (UInt32 i = 0; i < targetRace->keywordForm.numKeywords; i++) {
										if (targetRace->keywordForm.keywords[i]) {
											tempKeywords.Push(targetRace->keywordForm.keywords[i]);
										}
									}
								}
								CSimpleIni::TNamesDepend::const_iterator k = keywords.begin();
								for (; k != keywords.end(); ++k) {
									kwID = k->pItem;
									BGSKeyword * newKW = (BGSKeyword*)SAKEUtilities::GetFormFromIdentifier(kwID);
									if (newKW) {
										tempKeywords.Push(newKW);
									}
								}
								int kwCount = tempKeywords.count;
								if (kwCount > 0) {
									targetRace->keywordForm.numKeywords = kwCount;
									targetRace->keywordForm.keywords = new BGSKeyword*[kwCount];
									for (UInt8 j = 0; j < kwCount; j++) {
										targetRace->keywordForm.keywords[j] = tempKeywords[j];
									}
								}
								tempKeywords.Clear();
							}
						}
						keywords.clear();

						// write debug info to log
						if (bShowDebugInfo) {
							if (bVerboseDebugInfo) {
								_MESSAGE("\n      0x%08X - %s", targetRace->formID, targetRace->fullName.name.c_str());
								ATSpellListEntries * spellData = (ATSpellListEntries*)targetRace->spellList.unk08;
								if (spellData) {
									int iNumSpells = spellData->numSpells;
									_MESSAGE("        Abilities:");
									for (UInt32 j = 0; j < iNumSpells; j++)
										_MESSAGE("          %i: 0x%08X - %s", j, spellData->spells[j]->formID, spellData->spells[j]->name.name.c_str());
								}
								if (targetRace->propertySheet.sheet->count > 0) {
									_MESSAGE("        ActorValues:");
									BGSPropertySheet::AVIFProperty tempAVProp;
									for (UInt32 j = 0; j < targetRace->propertySheet.sheet->count; j++) {
										if (targetRace->propertySheet.sheet->GetNthItem(j, tempAVProp)) {
											if (tempAVProp.actorValue)
												_MESSAGE("          %i: 0x%08X - %s, %f", j, tempAVProp.actorValue->formID, tempAVProp.actorValue->avName, tempAVProp.value);
											else
												_MESSAGE("          %i: none, %f", j, tempAVProp.value);
										}
									}
								}
							}
							else {
								_MESSAGE("      0x%08X - %s", targetRace->formID, targetRace->fullName.name.c_str());
							}
						}
					}
				}
				return true;
			}
		}
		return false;
	}


	// loads ActorBase config data
	bool LoadData_Actors(const std::string & configDir)
	{
		CSimpleIniA iniActors;
		iniActors.SetUnicode();
		iniActors.SetMultiKey(true);

		if (iniActors.LoadFile(configDir.c_str()) > -1) {
			CSimpleIni::TNamesDepend sections, abilities, actorvalues, keywords;
			const char *spellID = "", *raceID = "", *avID = "", *kwID;
			SpellItem * newSpell = nullptr;
			TESActorBase * targetActor = nullptr;
			UInt32 iNumSpells = 0;

			iniActors.GetAllSections(sections);
			if (sections.size() > 0) {
				tArray<BGSKeyword*> tempKeywords;
				tArray<SpellItem*> tempSpells;
				CSimpleIni::TNamesDepend::const_iterator p = sections.begin();
				for (; p != sections.end(); ++p) {
					raceID = p->pItem;
					targetActor = (TESActorBase*)SAKEUtilities::GetFormFromIdentifier(raceID);
					if (targetActor) {
						// Spells/Abilities
						if (iniActors.GetAllValues(raceID, "sAbilities", abilities)) {
							if (abilities.size() > 0) {
								if (!targetActor->spellList.unk08) {
									targetActor->spellList.unk08 = new ATSpellListEntries();
								}
								ATSpellListEntries * spellData = (ATSpellListEntries*)targetActor->spellList.unk08;
								if (spellData) {
									iNumSpells = spellData->numSpells;
									for (UInt32 i = 0; i < iNumSpells; i++) {
										tempSpells.Push(spellData->spells[i]);
									}
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
									if (iNumSpells != tempSpells.count) {
										iNumSpells = tempSpells.count;
										spellData->numSpells = iNumSpells;
										spellData->spells = new SpellItem*[iNumSpells];
										for (UInt32 j = 0; j < iNumSpells; j++) {
											spellData->spells[j] = tempSpells[j];
										}
									}
								}
								tempSpells.Clear();
							}
							
						}
						abilities.clear();
						
						// ActorValues:
						if (targetActor->propertySheet.sheet) {
							if (iniActors.GetAllValues(raceID, "sActorValues", actorvalues)) {
								if (actorvalues.size() > 0) {
									CSimpleIni::TNamesDepend::const_iterator l = actorvalues.begin();
									for (; l != actorvalues.end(); ++l) {
										avID = l->pItem;
										BGSPropertySheet::AVIFProperty newAVProp;
										if (SAKEUtilities::GetAVPropertyFromIdentifer(avID, newAVProp)) {
											bool bFound = false;
											for (UInt32 i = 0; i < targetActor->propertySheet.sheet->count; i++) {
												BGSPropertySheet::AVIFProperty checkAVProp;
												if (targetActor->propertySheet.sheet->GetNthItem(i, checkAVProp)) {
													if (checkAVProp.actorValue == newAVProp.actorValue) {
														checkAVProp.value = newAVProp.value;
														bFound = true;
														break;
													}
												}
											}
											if (!bFound) {
												targetActor->propertySheet.sheet->Push(newAVProp);
											}
										}
									}
								}
							}
							actorvalues.clear();
						}
						
						// Keywords:
						if (iniActors.GetAllValues(raceID, "sKeywords", keywords)) {
							if (keywords.size() > 0) {
								if (targetActor->keywords.numKeywords > 0) {
									for (UInt32 i = 0; i < targetActor->keywords.numKeywords; i++) {
										if (targetActor->keywords.keywords[i]) {
											tempKeywords.Push(targetActor->keywords.keywords[i]);
										}
									}
								}
								CSimpleIni::TNamesDepend::const_iterator k = keywords.begin();
								for (; k != keywords.end(); ++k) {
									kwID = k->pItem;
									BGSKeyword * newKW = (BGSKeyword*)SAKEUtilities::GetFormFromIdentifier(kwID);
									if (newKW) {
										tempKeywords.Push(newKW);
									}
								}
								int kwCount = tempKeywords.count;
								if (kwCount > 0) {
									targetActor->keywords.numKeywords = kwCount;
									targetActor->keywords.keywords = new BGSKeyword*[kwCount];
									for (UInt8 j = 0; j < kwCount; j++) {
										targetActor->keywords.keywords[j] = tempKeywords[j];
									}
								}
								tempKeywords.Clear();
							}
						}
						keywords.clear();


						if (bShowDebugInfo) {
							if (bVerboseDebugInfo) {
								_MESSAGE("\n      0x%08X - %s", targetActor->formID, targetActor->fullName.name.c_str());
								ATSpellListEntries * spellData = (ATSpellListEntries*)targetActor->spellList.unk08;
								if (spellData) {
									int iNumSpells = spellData->numSpells;
									_MESSAGE("        Abilities:");
									for (UInt32 j = 0; j < iNumSpells; j++)
										_MESSAGE("          %i: 0x%08X - %s", j, spellData->spells[j]->formID, spellData->spells[j]->name.name.c_str());
								}
								if (targetActor->propertySheet.sheet->count > 0) {
									_MESSAGE("        ActorValues:");
									BGSPropertySheet::AVIFProperty tempAVProp;
									for (UInt32 j = 0; j < targetActor->propertySheet.sheet->count; j++) {
										if (targetActor->propertySheet.sheet->GetNthItem(j, tempAVProp)) {
											if (tempAVProp.actorValue)
												_MESSAGE("          %i: 0x%08X - %s, %f", j, tempAVProp.actorValue->formID, tempAVProp.actorValue->avName, tempAVProp.value);
											else
												_MESSAGE("          %i: none, %f", j, tempAVProp.value);
										}
									}
								}
							}
							else {
								_MESSAGE("      0x%08X - %s", targetActor->formID, targetActor->fullName.name.c_str());
							}
						}
					}
				}

				
				return true;
			}
		}
		return false;
	}

}


// startup
void SAKEData::LoadGameData()
{
	CSimpleIniA iniMain;
	iniMain.SetUnicode();
	iniMain.SetMultiKey(true);

	if (iniMain.LoadFile(".\\Data\\F4SE\\Plugins\\SAKE.ini") > -1) {
		const char *configDataPath = iniMain.GetValue("General", "sConfigFoldersPath", "");
		if (configDataPath != "") {
			std::vector<std::string> templateDirs = SAKEUtilities::GetDirFolderNames(configDataPath);
			if (templateDirs.size() > 0) {
				const char * dirName = "";
				std::size_t tempZero = 0;
				std::vector<std::string> templateFiles;
				std::string tempDirStr, fileName, filePath;

				bShowDebugInfo = iniMain.GetBoolValue("General", "bShowDebugInfo", false);
				bVerboseDebugInfo = iniMain.GetBoolValue("General", "bVerboseDebugLog", false);
				
				for (std::vector<std::string>::iterator itDir = templateDirs.begin(); itDir != templateDirs.end(); ++itDir) {
					dirName = itDir->c_str();
					tempDirStr = configDataPath;
					tempDirStr.append(dirName);
					tempDirStr.append("\\\\");
					
					templateFiles = SAKEUtilities::GetDirFileNames(tempDirStr.c_str());
					if (templateFiles.size() > 0) {
						if (bShowDebugInfo) {
							_MESSAGE("\nLoading Template: %s", dirName);
						}
						for (std::vector<std::string>::iterator itFile = templateFiles.begin(); itFile != templateFiles.end(); ++itFile) {
							fileName = itFile->c_str();
							filePath = tempDirStr.c_str();
							filePath.append(fileName);

							if (bShowDebugInfo) {
								_MESSAGE("\n    %s:", fileName.c_str());
							}
							if (fileName.find("Actors_") == tempZero) {
								if (!LoadData_Actors(filePath.c_str()))
									_MESSAGE("      Failed to load Actors from %s", filePath.c_str());
							}
							else if (fileName.find("Armors_") == tempZero) {
								if (!LoadData_Armors(filePath.c_str()))
									_MESSAGE("      Failed to load Armors from %s", filePath.c_str());
							}
							else if (fileName.find("Races_") == tempZero) {
								if (!LoadData_Races(filePath.c_str()))
									_MESSAGE("      Failed to load Races from %s", filePath.c_str());
							}
							else if (fileName.find("Weapons_") == tempZero) {
								if (!LoadData_Weapons(filePath.c_str()))
									_MESSAGE("      Failed to load Weapons from %s", filePath.c_str());
							}
							else {
								_MESSAGE("      Not a valid config file: %s", tempDirStr.c_str());
							}
						}
						templateFiles.clear();
					}
					else {
						_MESSAGE("\n    %s is empty", fileName.c_str());
					}
				}
				tempDirStr.clear();
				fileName.clear();
				filePath.clear();
			}
			
			templateDirs.clear();
			iniMain.Reset();
		}
		else {
			_MESSAGE("\n!!Unable to load config path.");
		}
	}

}

