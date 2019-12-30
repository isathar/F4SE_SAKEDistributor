# SAKE Distributor FormTypes

------------------------------------------------------------------------------------
## Shared Variables

- **comment** *(String - line)*/**comments** *(Array(String) - multi-line)*
	- Can be used for organization. 
	- One comment can be added into each object (anything between {...} )
	- These are loaded with the rest of the file, so don't overdo it.
	- Used in all form types and objects.
- **formID** *(FormID String) - required*
	- The FormID of the item/object to edit (see Configuration.md for the format).
	- Used in all form types.
- **name** *(String) - optional*
	- Edits the Form's base name.
	- Used for most forms. Not used for: LeveledItems, LeveledActors, Races, EncounterZones.
- **value** *(int) - optional*
	- Base caps value.
	- Only for items: Armor, Weapon, MiscItem, Component, Ingestible, Ammo.
- **weight** *(float) - optional*
	- Base weight.
	- Only for items: Armor, Weapon, MiscItem, Ingestible, Ammo.
- **spells** *(Array(Object)) - optional*
	- List of spells/abilities to add.
	- Used for Races and Actors.
	- **Variables:**
		- **formID** *(FormID String) - required*
		- Room for comments.
- **actorValues** *(Array(Object)) - optional*
	- List of ActorValues to modify.
	- Used for Races and Actors. NOTE: See Weapons definition for the differences compared to weapon ActorValues.
	- **Variables:**
		- **formID** *(FormID String) - required*
		- **set** *(float) - optional*
			- the amount to replace this AV's value with.
		- **add** *(float) - optional*
			- The amount to add to this AV.
			- Processed after *set*.
- **keywords** *(Object) - optional*
	- The lists of keywords to add/remove.
	- Used for most forms. Not used for: LeveledItems, LeveledActors, Races, Components, EncounterZones.
	- **Variables:**
		- **add/remove** *(Array(FormID String)) - both optional*
- **instanceNamingRules** *(FormID String) - optional*
	- Replaces the item's Instance Naming Rules form.
	- Used by Weapons, Armors.

------------------------------------------------------------------------------------
## Leveled Items

- **Shared:** *formID*
- **chanceNone** *(int) - optional*
	- The chance this list won't create anything when used.
- **useChanceGlobal** *(FormID String) - optional*
	- FormID of a GlobalVariable used to set the chanceNone of this list. Same as UseGlobal in the CK.
- **clear** *(bool) - optional*
	- Removes all items from the LeveledList before processing.
- **delevel** *(bool) - optional*
	- Sets all entries' levels to 1.
- **countMult** *(float) - optional*
	- Value to multiply Leveled List entry counts by.
- **remove/add** *(array(Object)) - both optional*
	- The lists of entries to add or remove from the original leveled list.
	- Added entries are affected by *delevel* and *multiplyCount*.
	- Add can be used to replace the contents of existing leveled lists when *clear* is enabled.
	- **Variables:**
		- **formID** *(FormID String) - required*
		- **level** *(int) - required*
		- **count** *(int) - required*
		- **chanceNone** *(int) - optional*
			- The chance of this entry not creating an item when the leveled list is used.

------------------------------------------------------------------------------------
## Leveled Actors

- **Shared:** *formID*
- **clear** *(bool) - optional*
	- Removes all items from the LeveledList before processing.
- **delevel** *(bool) - optional*
	- Sets all entries' levels to 1.
- **remove/add** *(array(Object)) - both optional*
	- The lists of entries to add or remove from the original leveled list.
	- Added entries are affected by *delevel*.
	- Add can be used to replace the contents of existing leveled lists when *clear* is enabled.
	- **Variables:**
		- **formID** *(FormID String) - required*
		- **level** *(int) - required*

------------------------------------------------------------------------------------
## Races

- **Shared:** *formID, spells, actorValues, keywords*

------------------------------------------------------------------------------------
## Actors

- **Shared:** *formID, name, spells, actorValues, keywords*
- **npcClass** *(FormID String) - optional*
	- The the FormID of the actor's Class.
- **combatStyle** *(FormID String) - optional*
	- The FormID of the actor's Combat Style.
- **outfitDefault** *(FormID String) - optional*
	- The FormID of the Actor's default outfit.
- **outfitSleep** *(FormID String) - optional*
	- The FormID of the outfit the Actor wears while sleeping.

------------------------------------------------------------------------------------
## Armors

- **Shared:** *formID, name, keywords, value, weight, instanceNamingRules*
- **armorRating** *(int) - optional*
	- Base armor rating.
- **damageResistances** *(array(Object)) - optional*
	- Base typed damage resistances.
	- **Variables:**
		- **formID** *(FormID String) - required*
			- The damageType's formID.
		- **set** *(int) - optional*
			- The amount of resistance to set.
		- **add** *(int) - optional*
			- The amount of resistance to add (after *set*).
- **health** *(int) - optional*
	- This armor's maximum condition - used by power armor.

------------------------------------------------------------------------------------
## Weapons

- **Shared:** *formID, name, keywords, value, weight, instanceNamingRules*
- **damage** *(int) - optional*
	- Base weapon damage.
- **damageTypes** *(array(Object)) - optional*
	- Base typed damage amounts.
	- **Variables:**
		- **formID** *(FormID String) - required*
			- The damageType's formID.
		- **set** *(int) - optional*
			- The amount of damage to set.
		- **add** *(int) - optional*
			- The amount of damage to add (after *set*).
- **secondaryDamage** *(float) - optional*
	- Base bashing damage.
- **enchantments** *(Array(Object)) - optional*
	- The list of enchantments to add to this weapon.
	- **Variables:**
		- **formID** *(FormID String) - optional*
			- Enchantment FormID.
		- Room for comments.
- **actorvalues** *(Array(Object) - optional*
	- The list of ActorValue modifiers
	- Works the same way as actorValues variables on Races and Actors, except using integers instead of floats.
	- **Variables:**
		- **formID** *(FormID String) - required*
		- **set** *(int) - optional*
			- the amount to replace this AV modifier's value with.
		- **add** *(int) - optional*
			- The amount to add to this AV modifier.
- **aimModel** *(Object) - optional*
	- **Variables:**
		- **formID** *(FormID String) - optional*
			- The weapon's base AimModel Form.
		- **recoil** *(Object) - optional*
			- **Variables:**
				- **minAngle/maxAngle** *(float) - both optional*
					- min./max. spread angle (crosshair size)
				- **increasePerShot** *(float) - optional*
					- spread increase per shot
				- **decreasePerSec** *(float) - optional*
					- spread decrease per second (after delay)
				- **decreaseDelayMS** *(int) - optional*
					- delay in ms before spread starts to decrease after firing
				- **sneakMult** *(float) - optional*
					- multiplier applied to spread while sneaking/crouched
				- **ironSightsMult** *(float) - optional*
					- multiplier applied to spread while aiming without a scope
		- **coneOfFire** *(Object) - optional*
			- **Variables:**
				- **arcDegrees** *(float) - optional*
					- max. difference from the base recoil angle per shot in degrees
				- **arcRotate** *(float) - optional*
					- angle for the recoil direction (clock-wise from 12:00)
				- **diminishSpringForce** *(float) - optional*
					- amount of automatic aim correction after recoil
				- **diminishSightsMult** *(float) - optional*
					- multiplier applied to diminishSpringForce while aiming
				- **minPerShot/maxPerShot** *(float) - both optional*
					- min./max. amount of recoil per shot
				- **hipMult** *(float) - optional*
					- multiplier applied to recoil while firing from the hip
		- **runawayRecoilShots** *(int) - optional*
			- Number of shots before recoil starts accumulating.
		- **baseStability** *(float) optional*
			- Multiplier for sway while aiming.
- **ammo** *(FormID String) - optional*
	- This weapon's equipped ammo's FormID.
- **npcAmmoLeveledList** *(FormID String) - optional*
	- The FormID of the leveled list used to supply NPCs/Containers with ammo when created.
- **rangeMin/rangeMax** *(float) - both optional*
	- The weapon's base min./max. range.
- **outOfRangeMult** *(float) - optional*
	- Base multiplier for damage depending on min/max range.
- **critChargeMult** *(float) - optional*
	- Base Critical charge multiplier.
- **critDamageMult** *(float) - optional*
	- Base critical damage multiplier.
- **flags** *(Object) - optional*
	- Edits weapon flags.
	- **Variables:**
		- **npcsUseAmmo** *(bool) - optional*
			- Enables NPC ammo depletion.

------------------------------------------------------------------------------------
## Ammo

- **Shared:** *formID, name, keywords, value, weight*
- **projectile** *(FormID String) - optional*
	- Overrides the ammo form's Projectile. Only used if the weapon doesn't have an OverrideProjectile set.
- **health** *(int) - optional*
	- The number of charges for this ammo type. Only used by Fusion Cores by default.
- **damage** *(float) - optional*
	- Damage added by this ammo type. Only used by AmmoBloodBug by default (Does nothing for normal weapons).
	- Probably only works with the 'Ignores Normal Weapon Resistance' flag and/or specific weapons.

------------------------------------------------------------------------------------
## MiscItems

- **Shared:** *formID, name, keywords, value, weight*
- **components** *(Object) - optional*
	- The list of components to add/remove.
	- **Variables:**
		- **clear** *(bool) - optional*
			- Whether or not to remove all existing components.
		- **add/remove** *(array(Object)) - both optional*
			- *formID (FormID String) - required*
			- *count (int) - required*

------------------------------------------------------------------------------------
## Crafting Components

- **Shared:** *formID, name, value*
- **scrapScalarGlobal** *(FormID String) - optional*
	- ID of the GlobalVariable to use for the scrap scalar (rarity value)
- **scrapMiscItem** *(FormID String) - optional*
	- This component's scrap MiscItem. The specified item should only include this component (x1) in its components list.

------------------------------------------------------------------------------------
## Chems/Ingestibles

- **Shared:** *formID, name, keywords, weight*

------------------------------------------------------------------------------------
## Encounter Zones

- **Shared:** *formID*
- **levelMin/levelMax** *(int) - both required*
	- Min/Max levels for things generated in this encounter zone.

------------------------------------------------------------------------------------
