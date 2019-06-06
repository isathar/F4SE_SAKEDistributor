#pragma once
#include "f4se/GameData.h"
#include "f4se/GameFormComponents.h"
#include "f4se/GameRTTI.h"
#include "SimpleIni/SimpleIni.h"
#include <string>



/* (AimModel)
- affects spread, recoil and aiming stability
- somewhat safe usage: cast AimModel as ATAimModel, edit result
- any AimModel edits require affected weapons to be re-equipped to take effect
110 */
class ATAimModel : public BaseFormComponent
{
public:
	virtual			~ATAimModel();

	// not sure about these first few...
	// - they seem to be similar to TESForm's first few variables
	// - using UInt32s to fill in the bytes before the actual AimModel variables start

	UInt32				unk00;						//00
	UInt32				unk08;						//08
	UInt32				unk10;						//10
	UInt32				formID;						//18 - gets set to 0 when any values are edited by omods or plugins
	UInt32				unk20;						//20
	UInt32				unk28;						//28

	// *cof* = spread/cone of fire, *rec* = recoil:

	float				CoF_MinAngle;				//30 - min. spread angle (crosshair size)
	float				CoF_MaxAngle;				//38 - max. spread angle
	float				CoF_IncrPerShot;			//40 - spread increase per shot
	float				CoF_DecrPerSec;				//48 - spread decrease per second (after delay)
	UInt32				CoF_DecrDelayMS;			//50 - delay in ms before spread starts to decrease after firing
	float				CoF_SneakMult;				//58 - multiplier applied to spread while sneaking/crouched
	float				Rec_DimSpringForce;			//60 - amount of automatic aim correction after recoil
	float				Rec_DimSightsMult;			//68 - amount of automatic aim correction after recoil while aiming
	float				Rec_MaxPerShot;				//70 - max. amount of recoil per shot
	float				Rec_MinPerShot;				//78 - min. amount of recoil per shot
	float				Rec_HipMult;				//80 - multiplier applied to recoil while firing from the hip
	UInt32				Rec_RunawayShots;			//88 - the number of shots before recoil becomes unbearable?
	float				Rec_ArcMaxDegrees;			//90 - max. difference from the base recoil angle per shot in degrees
	float				Rec_ArcRotate;				//98 - angle for the recoil direction (clock-wise from 12:00)
	float				CoF_IronSightsMult;			//100 - multiplier applied to spread while aiming without a scope
	float				BaseStability;				//108 - multiplier applied to camera sway while using a scope
};


/* TESSpellList's unk08
- used by TESRace and TESActorBase for default abilities
- probably completely unsafe usage: cast TESSpellList's unk08 to this, edit result
- edits need to be done early (or require reloading a save) since TESRace, TESActorBase are templates for Actors
20 */
class ATSpellListEntries
{
public:
	SpellItem			** spells;				//00 - array of SpellItems
	void				* unk08;				//08
	void				* unk10;				//10
	UInt32				numSpells;				//18 - length of spells - set manually
};


namespace SAKEData
{
	// updated weapon flags
	enum SDWeaponFlags
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

	// starts the loading process
	void LoadGameData();
}

