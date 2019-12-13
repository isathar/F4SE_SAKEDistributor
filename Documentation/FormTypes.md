# SAKE Distributor FormTypes

------------------------------------------------------------------------------------
## 1 - Leveled Lists

- **Variables**

	- *formType (int) - required*
		- 1 for Leveled Items, Leveled Actors, and *maybe* Leveled Spells (if they work in F4).
	- *formID (FormID String) - required*
		- The ID of the Leveled List to edit.
	- *comment/name/documentation (string or array(string)) - optional*
		- Can be used for organization.
	- *clear (bool) - optional*
		- Removes all items from the LeveledList before processing.
	- *delevel (bool) - optional*
		- Sets all entries' levels to 1.
	- *countMult (float) - optional*
		- Value to multiply Leveled List entry counts by.
	- *remove (array(llObject)) - optional*
		- The list of entries to remove from the original leveled list.
	- *add (array(llObject)) - optional*
		- The list of entries to add.
		- Added entries are affected by *delevel* and *multiplyCount*.
		- Can be used to replace the contents of existing leveled lists when *clear* is enabled.
		- *llObject:*
			- Used by *add* and *remove* lists.
			- *level* and *count* work the same as they do for editor-based leveled list entries.
			- *formID (FormID String) - required*
			- *level (int) - required*
			- *count (int) - required*

------------------------------------------------------------------------------------
## 2 - Races

- These should always be loaded before changes to Actors since they act as Actor templates and will reset them if edited.

- **Variables**

	- *formType (int) - required*
		- 2 for Races.
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
## 3 - Actors

- *Besides using formType 3, Actor override files are identical to Race overrides.*

------------------------------------------------------------------------------------
## 4 - Armors

- **Variables**

	- *formType (int) - required*
		- 4 for Armors.
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
## 5 - Weapons

- **Variables**

	- *formType (int) - required*
		- 5 for Weapons.
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
## 6 - Ammo

- **Variables**

	- *formType (int) - required*
		- 6 for Ammo.
	- *formID (FormID String) - required*
		- The ID of the Ammo item to edit.
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

------------------------------------------------------------------------------------
## 7 - MiscItems

- **Variables**

	- *formType (int) - required*
		- 7 for MiscItems.
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
