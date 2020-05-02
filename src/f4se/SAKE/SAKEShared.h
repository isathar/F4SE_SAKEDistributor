#pragma once
#include "f4se/GameData.h"
#include "f4se/GameFormComponents.h"
#include "f4se/GameSettings.h"
#include "f4se/GameTypes.h"
#include "f4se/GameRTTI.h"
#include "nlohmann/json.hpp"
#include <Windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

using json = nlohmann::json;


/* BGSAimModel
- affects spread, recoil and aiming stability
- any AimModel edits require affected weapons to be re-equipped to take effect
60 */
class TempBGSAimModel : public BaseFormComponent
{
public:
	virtual			~TempBGSAimModel();

	// not sure about these first few...
	// - they seem to be similar to TESForm's first few variables
	// - using UInt32s to fill in the space before the useful variables start

	UInt32				unk08;						// 08
	UInt32				unk0C;						// 0C
	UInt32				unk10;						// 10
	UInt32				formID;						// 14 - resets to 0 when any values are edited by omods or instanceData mods
	UInt32				unk18;						// 18
	UInt32				unk1C;						// 1C

	// CoF_ = spread/cone of fire, Rec_ = recoil

	float				CoF_MinAngle;				// 20 - min. spread angle (crosshair size)
	float				CoF_MaxAngle;				// 24 - max. spread angle
	float				CoF_IncrPerShot;			// 28 - spread increase per shot
	float				CoF_DecrPerSec;				// 2C - spread decrease per second (after delay)
	UInt32				CoF_DecrDelayMS;			// 30 - delay in ms before spread starts to decrease after firing
	float				CoF_SneakMult;				// 34 - multiplier applied to spread while sneaking/crouched
	float				Rec_DimSpringForce;			// 38 - amount of automatic aim correction after recoil
	float				Rec_DimSightsMult;			// 3C - amount of automatic aim correction after recoil while aiming
	float				Rec_MaxPerShot;				// 40 - max. amount of recoil per shot
	float				Rec_MinPerShot;				// 44 - min. amount of recoil per shot
	float				Rec_HipMult;				// 48 - multiplier applied to recoil while firing from the hip
	UInt32				Rec_RunawayShots;			// 4C - the number of shots before recoil becomes unbearable?
	float				Rec_ArcMaxDegrees;			// 50 - max. difference from the base recoil angle per shot in degrees
	float				Rec_ArcRotate;				// 54 - angle for the recoil direction (clock-wise from 12:00)
	float				CoF_IronSightsMult;			// 58 - multiplier applied to spread while aiming without a scope
	float				BaseStability;				// 5C - multiplier applied to camera sway while using a scope
};


/* TESSpellList's unk08
- used by TESRace and TESActorBase for default abilities
- edits need to be done early (or require reloading a save) since TESRace, Actor are used as templates by the game
1C or 20? */
class TempSpellListEntries
{
public:
	SpellItem			** spells;				//00 - array of SpellItems
	void				* unk08;				//08
	void				* unk10;				//10
	UInt32				numSpells;				//18 - length of the spells array - set manually
};


/* TESKey
- Based on TESObjectMISC in Skyrim
- casting to it seems to work, so maybe it's the same here?
- MiscItem differences in Fallout 4: components, featured item msg
100 */
class TempTESKey : public TESObjectMISC
{
public:
	enum { kTypeID = kFormType_KEYM };
};


/* BGSProjectile
- mostly the same layout as Skyrim's BGSProjectile with some additions
188 */
class TempBGSProjectile : public TESBoundObject
{
public:
	enum { 
		kTypeID = kFormType_PROJ,

		pFlag_Hitscan = 0x0001,
		pFlag_Explosion = 0x0002,
		pFlag_AltTrigger = 0x0004,
		pFlag_MuzzleFlash = 0x0008,
		pFlag_Unknown4 = 0x0010,
		pFlag_CanBeDisabled = 0x0020,
		pFlag_CanBePickedUp = 0x0040,
		pFlag_Supersonic = 0x0080,
		pFlag_PinsLimbs = 0x0100,
		pFlag_PassThroughSmallTransparent = 0x0200,
		pFlag_DisableCombatAimCorrection = 0x0400,
		pFlag_PenetratesGeometry = 0x0800,
		pFlag_ContinuousUpdate = 0x1000,
		pFlag_SeeksTarget = 0x2000
	};

	TESFullName					fullName;				// 68
	TESModel					model;					// 78
	BGSPreloadable				preloadable;			// A8
	BGSDestructibleObjectForm	destructible;			// B0

	// 90
	struct ProjectileData
	{
		UInt16					flags;					// 00
		UInt16					type;					// 02 - 1=missile, 2=lobber, 4=beam, 8=flame, 16=cone, 32=barrier, 64=arrow
		float					gravity;				// 04
		float					speed;					// 08
		float					range;					// 0C
		TESForm					* light;				// 10
		TESForm					* muzFlashLight;		// 18
		float					explosionProximity;		// 20
		float					explosionTimer;			// 24
		TESForm					* explosion;			// 28
		BGSSoundDescriptorForm	* sound;				// 30
		float					muzflashDuration;		// 38
		float					fadeDuration;			// 3C
		float					impactForce;			// 40
		UInt32					unk44;					// 44 - always 0
		BGSSoundDescriptorForm	* countdownSound;		// 48
		BGSSoundDescriptorForm	* disableSound;			// 50
		TESObjectWEAP			* weaponSource;			// 58
		float					coneSpread;				// 60
		float					collisionRadius;		// 64
		float					lifeTime;				// 68
		float					relaunchInterval;		// 6C
		TESForm					* decalData;			// 70
		TESForm					* collisionLayer;		// 78
		BGSProjectile			* vatsProjectile;		// 80
		UInt32					tracerFrequency;		// 88
	};

	ProjectileData				data;					// C0
	TESModel					muzFlashModel;			// 150
	UInt32						soundLevel;				// 180
};


/* TESAmmo
1B0 */
class TempTESAmmo : public TESBoundObject
{
public:
	enum
	{
		kTypeID = kFormType_AMMO,
		aFlag_IgnoreNormalResistance = 0x00000001,
		aFlag_NonPlayable = 0x00000002,
		aFlag_CountBased3D = 0x00000004
	};

	TESFullName					fullName;			// 068
	BGSModelMaterialSwap		materialSwap;		// 078
	TESIcon						icon;				// 0B8
	BGSMessageIcon				messageIcon;		// 0C8
	TESValueForm				value;				// 0E0
	BGSDestructibleObjectForm	destructible;		// 0F0
	BGSPickupPutdownSounds		pickupSounds;		// 100
	TESDescription				description;		// 118
	BGSKeywordForm				keywordForm;		// 130
	TESWeightForm				weight;				// 150

	BGSProjectile				* projectile;		// 160
	UInt32						health;				// 168 - could be 2 16 bit ints since 32 seems like overkill for ammo count
	UInt32						flags;				// 16C - there are only 3 flags - could also be multiple 8 or 16 bit ints
	float						damage;				// 170
	UInt32						unk174;				// 174 - always 0
	BSFixedString				shortName;			// 178
	TESModel					casingModel;		// 180
};



namespace SAKEUtilities
{
	const char * GetPluginNameFromFormID(UInt32 formID);
	const char * GetIdentifierFromFormID(UInt32 formID);
	UInt32 GetFormIDFromIdentifier(const std::string & formIdentifier);
	TESForm * GetFormFromIdentifier(const std::string & formIdentifier);
	bool IsModLoaded(const std::string & modName);
	std::vector<std::string> GetFileNames(const std::string & folder);
	bool IsPathValid(const std::string & path);
}


namespace SAKEData
{
	// updated weapon flags
	enum
	{
		wFlag_PlayerOnly = 0x0000001,
		wFlag_NPCsUseAmmo = 0x0000002,
		wFlag_NoJamAfterReload = 0x0000004,
		wFlag_ChargingReload = 0x0000008,
		wFlag_MinorCrime = 0x0000010,
		wFlag_FixedRange = 0x0000020,
		wFlag_NotUsedInNormalCombat = 0x0000040,
		wFlag_CritEffectonDeath = 0x0000100,
		wFlag_ChargingAttack = 0x0000200,
		wFlag_HoldInputToPower = 0x0000800,
		wFlag_NonHostile = 0x0001000,
		wFlag_BoundWeapon = 0x0002000,
		wFlag_IgnoresNormalResist = 0x0004000,
		wFlag_Automatic = 0x0008000,
		wFlag_RepeatableSingleFire = 0x0010000,
		wFlag_CantDrop = 0x0020000,
		wFlag_HideBackpack = 0x0040000,
		wFlag_EmbeddedWeapon = 0x0080000,
		wFlag_NotPlayable = 0x0100000,
		wFlag_HasScope = 0x0200000,
		wFlag_BoltAction = 0x0400000,
		wFlag_SecondaryWeapon = 0x0800000,
		wFlag_DisableShells = 0x1000000
	};

	void LoadOverrides_Weapon(TESObjectWEAP * weapForm, json & weaponOverride);
	void LoadOverrides_Armor(TESObjectARMO * armorForm, json & armorOverride);
	void LoadOverrides_Race(TESRace * raceForm, json & raceOverride);
	void LoadOverrides_Actor(TESNPC * actorForm, json & actorOverride);

	void LoadOverrides_LeveledItem(TESLevItem * lliForm, json & llOverride);
	void LoadOverrides_LeveledActor(TESLevCharacter * llcForm, json & llOverride);

	void LoadOverrides_Ammo(TempTESAmmo * ammoForm, json & ammoOverride);
	void LoadOverrides_Misc(TESObjectMISC * miscForm, json & miscOverride);
	void LoadOverrides_Key(TempTESKey * keyForm, json & miscOverride);
	void LoadOverrides_Component(BGSComponent * compoForm, json & compoOverride);
	void LoadOverrides_Ingestible(AlchemyItem * alchForm, json & alchOverride);

	void LoadOverrides_Projectile(TempBGSProjectile * projForm, json & projOverride);
	void LoadOverrides_EncounterZone(BGSEncounterZone * enczForm, json & enczOverride);

	void LoadNamePrefix(TESForm * targetForm, const std::string & prefixStr);

	void LoadGameSettings(json & settingOverrides);
}


namespace SAKEFileReader
{
	// starts the loading process
	void LoadGameData();
}
