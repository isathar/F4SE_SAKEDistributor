# SAKE Distributor FormTypes

------------------------------------------------------------------------------------
## Leveled Items

- *formID (FormID String) - required*
	- The ID of the Leveled Item to edit.
- *comment/name/documentation (string or array(string)) - optional*
	- Can be used for organization.
- *chanceNone (int) - optional*
	- The chance this list won't create anything when used.
- *useChanceGlobal (FormID String) - optional*
	- FormID of a GlobalVariable used to set the chanceNone of this list. Same as UseGlobal in the CK.
- *clear (bool) - optional*
	- Removes all items from the LeveledList before processing.
- *delevel (bool) - optional*
	- Sets all entries' levels to 1.
- *countMult (float) - optional*
	- Value to multiply Leveled List entry counts by.
- *remove, add (array(llObject)) - optional*
	- The lists of entries to add or remove from the original leveled list.
	- Added entries are affected by *delevel* and *multiplyCount*.
	- Add can be used to replace the contents of existing leveled lists when *clear* is enabled.
	- Variables:
		- *level* and *count* work the same as they do for editor-based leveled list entries.
		- *formID (FormID String) - required*
		- *level (int) - required*
		- *count (int) - required*
		- *chanceNone (int) - optional*
			- The chance of this entry not creating an item when the leveled list is used.
			- Not used for LeveledActors in the base game - could work, but it's untested.

------------------------------------------------------------------------------------
## Leveled Actors

- Mostly identical to Leveled Items.
- They do not support countMult, count, chanceNone and useChanceGlobal.

------------------------------------------------------------------------------------
## Races

- These are always loaded before changes to Actors since they act as Actor templates and will reset them if edited.
- *formID (FormID String) - required*
	- The ID of the Race to edit.
- *comment/name/documentation (string or array(string)) - optional*
	- Can be used for organization.
- *actorValues (Array(avObject)) - optional*
	- List of ActorValues to modify.
	- *avObject:*
		- *formID (FormID String) - required*
		- *set (float) - optional*
			- the amount to replace this AV's value with.
		- *add (float) - optional*
			- The amount to add to this AV.
			- Processed after *set*.
		- *name/comment/description (string) - optional*
			- used for organization.
- *spells (Array(spellObject)) - optional*
	- List of spells/abilities to add.
	- *spellObject:*
		- *formID (FormID String) - required*
		- *name/comment/description (string) - optional*
- *keywords (Object) - optional*
	- Contains the lists of keywords to add/remove.
	- *add/remove (array(FormID String)) - optional*

------------------------------------------------------------------------------------
## Actors

- *Actor override files are identical to Race overrides.*

------------------------------------------------------------------------------------
## Armors

- *formID (FormID String) - required*
	- The ID of the Armor (ARMO) to edit.
- *comment/documentation (string or array(string)) - optional*
	- Can be used for organization.
- *name (string) - optional*
	- Replaces the Form's base name.
- *instanceNamingRules (FormID String) - optional*
	- Replaces the armor's Instance Naming Rules form.
- *armorRating (int) - optional*
	- Base armor rating.
- *value (int) - optional*
	- Base caps value.
- *weight (float) - optional*
	- Base weight.
- *keywords (Object) - optional*
	- Contains the lists of keywords to add/remove.
	- *add/remove (array(FormID String)) - optional*

------------------------------------------------------------------------------------
## Weapons

- *formID (FormID String) - required*
	- The ID of the Weapon to edit.
- *comment/documentation (string or array(string)) - optional*
	- Can be used for organization.
- *name (string) - optional*
	- Replaces the Form's base name.
- *value (int) - optional*
	- Base caps value.
- *weight (float) - optional*
	- Base weight.
- *instanceNamingRules (FormID String) - optional*
	- Replaces the weapon's Instance Naming Rules form.
- *keywords (Object) - optional*
	- Contains the lists of keywords to add/remove.
	- *add/remove (array(FormID String)) - optional*

------------------------------------------------------------------------------------
## Ammo

- *formID (FormID String) - required*
	- The ID of the Ammo item to edit.
- *comment/documentation (string or array(string)) - optional*
	- Can be used for organization.
- *name (string) - optional*
	- Replaces the Form's base name.
- *projectile (FormID String) - optional*
	- Overrides the ammo form's Projectile. Only used if the weapon doesn't have an OverrideProjectile set.
- *health (int) - optional*
	- The number of charges for this ammo type. Only used by Fusion Cores by default.
- *damage (float) - optional*
	- Damage added by this ammo type. Only used by AmmoBloodBug by default (Does nothing for normal weapons).
	- Probably only works with the 'Ignores Normal Weapon Resistance' flag and/or specific weapons.
- *value (int) - optional*
	- Base caps value.
- *weight (float) - optional*
	- Base weight.
- *keywords (Object) - optional*
	- Contains the lists of keywords to add/remove.
	- *add/remove (array(FormID String)) - optional*

------------------------------------------------------------------------------------
## MiscItems

- *formID (FormID String) - required*
	- The ID of the MiscItem to edit.
- *comment/documentation (string or array(string)) - optional*
	- Can be used for organization.
- *name (string) - optional*
	- Replaces the Form's base name.
- *value (int) - optional*
	- Base caps value.
- *weight (float) - optional*
	- Base weight.
- *keywords (Object) - optional*
	- Contains the lists of keywords to add/remove.
	- *add/remove (array(FormID String)) - optional*
- *components (Object) - optional*
	- The list of components to add/remove.
	- *clearList (bool) - optional*
		- Whether or not to remove all existing components.
	- *add/remove (array(Object)) - optional*
		- *formID (FormID String) - required*
		- *count (int) - required*

------------------------------------------------------------------------------------
## Crafting Components

- *formID (FormID String) - required*
	- The ID of the Component to edit.
- *comment/documentation (string or array(string)) - optional*
	- Can be used for organization.
- *name (string) - optional*
	- Replaces the Form's base name.
- *value (int) - optional*
	- Base caps value. This should match the value of the component's scrap MiscItem.
- *scrapScalarGlobal (FormID String) - optional*
	- ID of the GlobalVariable to use for the scrap scalar (rarity value)
- *scrapMiscItem (FormID String) - optional*
	- This component's scrap MiscItem. The specified item should only include this component (x1) in its components list.

------------------------------------------------------------------------------------
## Chems/Ingestibles

- *formID (FormID String) - required*
	- The ID of the Ingestible to edit.
- *comment/documentation (string or array(string)) - optional*
	- Can be used for organization.
- *name (string) - optional*
	- Replaces the Form's base name.
- *weight (float) - optional*
	- Base weight.
- *keywords (Object) - optional*
	- Contains the lists of keywords to add/remove.
	- *add/remove (array(FormID String)) - optional*

------------------------------------------------------------------------------------
