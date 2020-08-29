#pragma once
#include "f4se/GameData.h"
#include "f4se/GameFormComponents.h"
#include "f4se/GameForms.h"
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
	UInt32						soundLevel;				// 180 - 0=loud, 1=normal, 2=silent, 3=very loud, 4=quiet
};


/* BGSExplosion
144? */
class TempBGSExplosion : public TESBoundObject
{
public:
	enum { 
		kTypeID = kFormType_EXPL,

		xFlag_Unknown0 = 0x0001,
		xFlag_AlwaysUseWorldOrientation = 0x0002,
		xFlag_KnockDownAlways = 0x0004,
		xFlag_KnockDownByFormula = 0x0008,
		xFlag_IgnoreLOSCheck = 0x0010,
		xFlag_PushExplSourceRefOnly = 0x0020,
		xFlag_IgnoreImageSpaceSwap = 0x0040,
		xFlag_Chain = 0x0080,
		xFlag_NoControllerVibration = 0x0100,
		xFlag_PlacedObjectPersists = 0x0200,
		xFlag_SkipUnderwaterTests = 0x0400
	};

	TESFullName				fullName;				// 68
	TESModel				model;					// 78
	BGSPreloadable			preloadable;			// A8

	EnchantmentItem *		objectEffect;			// B0
	UInt64					unkB8;					// B8
	UInt64					unkC0;					// C0
	UInt64					unkC8;					// C8

	TESImageSpaceModifier * imageSpaceModifier;		// D0

	// 6C
	struct ExplosionData
	{
		TESForm					* light;			// 00
		BGSSoundDescriptorForm	* sound1;			// 08
		BGSSoundDescriptorForm	* sound2;			// 10
		TESForm					* impactDataSet;	// 18
		TESForm					* placedObject;		// 20

		BGSProjectile			* spawnProjectile;	// 28
		float					spawnX;				// 30
		float					spawnY;				// 34
		float					spawnZ;				// 38
		float					spawnSpreadDeg;		// 3C
		UInt32					spawnCount;			// 40

		float					force;				// 44
		float					damage;				// 48
		float					innerRadius;		// 4C
		float					outerRadius;		// 50
		float					isRadius;			// 54
		float					verticalOffset;		// 58

		UInt32					flags;				// 5C
		UInt32					soundLevel;			// 60
		float					placedObjFadeDelay;	// 64
		UInt32					stagger;			// 68
	};

	ExplosionData			data;					// D8
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


// 150
struct TempWEAPInstanceData : public TBO_InstanceData
{
public:
	BGSSoundDescriptorForm		* unk10;					// 10 BGSSoundDescriptorForm *
	UInt64						unk18;						// 18
	UInt64						unk20;						// 20
	BGSSoundDescriptorForm		*unk28;						// 28 BGSSoundDescriptorForm *
	BGSSoundDescriptorForm		* unk30;					// 30 BGSSoundDescriptorForm *
	BGSSoundDescriptorForm		* unk38;					// 38 BGSSoundDescriptorForm *
	BGSSoundDescriptorForm		* unk40;					// 40 BGSSoundDescriptorForm * 
	BGSSoundDescriptorForm		* unk48;					// 48 BGSSoundDescriptorForm *
	UInt64						unk50;						// 50
	BGSImpactDataSet			* impactDataSet;			// 58 BGSImpactDataSet*
	TESLevItem					* addAmmoList;				// 60 TESLevItem *
	TESAmmo						* ammo;						// 68 TESAmmo *
	BGSEquipSlot				* equipSlot;				// 70 BGSEquipSlot*
	SpellItem					* unk78;					// 78 SpellItem*
	BGSKeywordForm				* keywords;					// 80
	BGSAimModel					* aimModel;					// 88 BGSAimModel *
	BGSZoomData					* zoomData;					// 90 BGSZoomData*

	struct FiringData
	{
		BGSProjectile	* projectileOverride;	// 00
		float			unk00;					// 08
		float			leftMotorStrength;		// 0C
		float			rightMotorStrength;		// 10
		float			duration;				// 14
		float			unk18;					// 18
		float			unk1C;					// 1C
		float			sightedTransition;		// 20
		UInt32			period;					// 24
		UInt32			unk28;					// 28

		UInt16			unk2A;					// 2C - start of old numProjectiles
		UInt8			unk2B;					// 2E
		UInt8			numProjectiles;			// 2F
	};

	FiringData					* firingData;				// 98
	tArray<EnchantmentItem*>	* enchantments;				// A0
	tArray<BGSMaterialSwap*>	* materialSwaps;			// A8
	tArray<DamageTypes>			* damageTypes;				// B0
	tArray<ValueModifier>		* modifiers;				// B8
	float						unkC0;						// C0
	float						reloadSpeed;				// C4
	float						speed;						// C8
	float						reach;						// CC
	float						minRange;					// D0
	float						maxRange;					// D4
	float						attackDelay;				// D8
	float						unkD8;						// DC
	float						outOfRangeMultiplier;		// E0
	float						secondary;					// E4
	float						critChargeBonus;			// E8
	float						weight;						// EC
	float						unkEC;						// F0
	float						actionCost;					// F4
	float						fullPowerSeconds;			// F8
	float						minPowerShot;				// FC
	UInt32						unk100;						// 100
	float						critDamageMult;				// 104
	UInt32						stagger;					// 108
	UInt32						value;						// 10C

	// updated flags
	enum WeaponFlags
	{
		kFlag_PlayerOnly = 0x00000001,
		kFlag_NPCsUseAmmo = 0x00000002,
		kFlag_NoJamAfterReload = 0x00000004,
		kFlag_ChargingReload = 0x00000008,
		kFlag_MinorCrime = 0x00000010,
		kFlag_FixedRange = 0x00000020,
		kFlag_NotUsedInNormalCombat = 0x00000040,
		kFlag_Unknown8 = 0x00000080,
		kFlag_CritEffectonDeath = 0x00000100,
		kFlag_ChargingAttack = 0x00000200,
		kFlag_Unknown11 = 0x00000400,
		kFlag_HoldInputToPower = 0x00000800,
		kFlag_NonHostile = 0x00001000,
		kFlag_BoundWeapon = 0x00002000,
		kFlag_IgnoresNormalResist = 0x00004000,
		kFlag_Automatic = 0x00008000,
		kFlag_RepeatableSingleFire = 0x00010000,
		kFlag_CantDrop = 0x00020000,
		kFlag_HideBackpack = 0x00040000,
		kFlag_EmbeddedWeapon = 0x00080000,
		kFlag_NotPlayable = 0x00100000,
		kFlag_HasScope = 0x00200000,
		kFlag_BoltAction = 0x00400000,
		kFlag_SecondaryWeapon = 0x00800000,
		kFlag_DisableShells = 0x01000000,
		kFlag_Unknown26 = 0x02000000,
		kFlag_Unknown27 = 0x04000000,
		kFlag_Unknown28 = 0x08000000,
		kFlag_Unknown29 = 0x10000000,
		kFlag_Unknown30 = 0x20000000,
		kFlag_Unknown31 = 0x40000000,
		kFlag_Unknown32 = 0x80000000
	};

	UInt32						flags;						// 110
	UInt32						unk114;						// 114
	UInt32						unk118;						// 118
	UInt32						unk11C;						// 11C
	ActorValueInfo				* skill;					// 120
	ActorValueInfo				* damageResist;				// 128
	UInt16						ammoCapacity;				// 130
	UInt16						baseDamage;					// 132
	UInt16						unk134;						// 134
	UInt8						accuracyBonus;				// 136
	UInt8						unk137;						// 137

	BGSModelMaterialSwap*		swap138;					// 138
	UInt64						unk140;						// 140
	BGSMod::Attachment::Mod*	embeddedMod;				// 148
};


// 58
struct TempARMOInstanceData : public TBO_InstanceData
{
public:
	tArray<EnchantmentItem*> * enchantments;	// 10
	UInt64 unk18;								// 18
	UInt64 unk20;								// 20
	BGSKeywordForm * keywords;					// 28
	tArray<DamageTypes>	* damageTypes;			// 30
	tArray<TBO_InstanceData::ValueModifier> * modifiers; // 38
	float weight;								// 40
	SInt32 pad44;								// 44
	UInt32 value;								// 48
	UInt32 health;								// 4C
	UInt32 unk50;								// 50
	UInt16 armorRating;							// 54
	UInt16 unk56;								// 56
};


// BGSPerkRankArray
// 18
class TempBGSPerkRankArray : public BaseFormComponent
{
public:
	UInt64 * unk08;		// 08 - an array of Perk, Rank, Perk, Rank, etc. - could be a hashset, but a tHashSet won't work here
	UInt32	unk10;		// 10
	UInt32	pad14;		// 14
};


// not a base game struct - used to edit perk lists
struct TempPerkRankEntry
{
	BGSPerk * perk = nullptr;
	UInt32 rank = 1;
};




namespace SAKELoader
{
	// file and form operations:

	const char * GetPluginNameFromFormID(UInt32 formID);
	const char * GetIdentifierFromFormID(UInt32 formID);
	UInt32 GetFormIDFromIdentifier(const std::string & formIdentifier);
	TESForm * GetFormFromIdentifier(const std::string & formIdentifier);
	bool IsModLoaded(const std::string & modName);
	std::vector<std::string> GetFileNames(const std::string & folder);
	bool IsPathValid(const std::string & path);

	// form overrides:

	void LoadOverrides_Weapon(TESObjectWEAP * weapForm, json & weaponOverride);
	void LoadOverrides_Armor(TESObjectARMO * armorForm, json & armorOverride);
	void LoadOverrides_Race(TESRace * raceForm, json & raceOverride);
	void LoadOverrides_Actor(TESNPC * actorForm, json & actorOverride);

	void LoadOverrides_LeveledItem(TESLevItem * lliForm, json & llOverride);
	void LoadOverrides_LeveledActor(TESLevCharacter * llcForm, json & llOverride);

	void LoadOverrides_Container(TESObjectCONT * contForm, json & contOverride);

	void LoadOverrides_Ammo(TempTESAmmo * ammoForm, json & ammoOverride);
	void LoadOverrides_Misc(TESObjectMISC * miscForm, json & miscOverride);
	void LoadOverrides_Key(TempTESKey * keyForm, json & miscOverride);
	void LoadOverrides_Component(BGSComponent * compoForm, json & compoOverride);
	void LoadOverrides_Ingestible(AlchemyItem * alchForm, json & alchOverride);

	void LoadOverrides_Projectile(TempBGSProjectile * projForm, json & projOverride);
	void LoadOverrides_Explosion(TempBGSExplosion * explForm, json & explOverride);
	void LoadOverrides_EncounterZone(BGSEncounterZone * enczForm, json & enczOverride);

	void LoadNamePrefix(TESForm * targetForm, const std::string & prefixStr);

	void LoadGameSettings(json & settingOverrides);


	// starts the loading process
	void LoadGameData();
}

