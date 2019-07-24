F4SE SAKE Distributor
======================


- WIP plugin- and load-order independent patcher for Fallout 4's Races, Weapons, Armors, Actors, and leveled items. 
- Allows distributing Spells, ActorValues, Keywords to actors and races via ini files. Also allows editing base weapon and armor instance stats and enchantments.
- Could be used to make simple plugin-less mods and compatibility patches.
- Created for AmmoTweaks 2.0. It will be included in AmmoTweaks, and also released separately.


Notes for compiling:

- The contents of the Plugin\src\SAKE folder need to be with the F4SE source files to compile ((Fallout 4 directory)\src\SAKE\)


Credits:

- Uses SimpleINI (https://github.com/brofield/simpleini) for config file read operations.
- Created with Visual Studio 2017 (so you'll most likely need the Visual C++ Redistributable 2017 to run this).
- Many thanks to the F4SE team.



*********************************** Very WIP Config Files Documentation *********************************

Some example config file templates can be found in (bin/Fallout4 directory)\Data\F4SE\Plugins\DefaultTemplates.

Installing templates:
- Copy any template folders you want SAKE to load into the _Active folder
- Templates are processed in alphanumerical order. Rename folders if you need a specific load order.


Included Templates:
- Note: None of these cover everything, and are very much still works in progress.

*	AdditionalResistances			(no requirements)
--		Distributes resistances to Fallout 4's neglected damage types to races, actors and armors.

*	zNPCsUseAmmo					(no requirements)
--		Enables NPC ammo usage for the specified weapons.
-- 		Starts with a z because it needs to load after any other weapon edits to make sure the flag is set.

*	zReducedAmmoLoot				(no requirements)
--		Reduces the amount of ammo found in base loot/vendor leveled lists.

*	DTF_Default						(requires Damage Threshold Framework)
-- 		Distributes DTF's damage threshold to races, actors and armors.
--		Default version (do not use with AmmoTweaks)




Custom templates:

	Notes:
		- Any values that are not set are ignored.
		
	
	FormID formats:

		Standard FormID String: 			sFormID = PluginName.esp|000000
		- replace the first byte of the FormID with its plugin name and extension:

		ActorValue String:			
							Actor/Race:		sActorValue = PluginName.esp|000000, 99.0
							Weapon/Armor:	sActorValue = PluginName.esp|000000, 99
		- FormID followed by , value
		- use floating point values for actors and races, integer values for weapon and armor
		- (the ini reader can switch between them automatically, but those are the formats used internally)
		
		DamageType String:					sDamageType = PluginName.esp|000000, 99
		- FormID followed by , value
		
		LeveledItem String:					sLeveledItemX = PluginName.esp|000000, 1, 1
		- item FormID followed by , level, count
	
	
	
	Template Types:
	
	Races:
	
		- ini files with names starting with "Races_"
		- Section names are race FormIDs	
		
		Available Variables:
		
			sAbilities (FormID string)
				- default spells/abilities to add
				- multiple values are possible
				
			sActorValues (ActorValue String)
				- default ActorValues to add
			
			sKeywords (FormID String)
				- default keywords to add
		
	Actors:
	
		- ini files with names starting with "Actors_"
		- Section names are Actor/ActorBase FormIDs
		- same variables as Races
		
		
	Armors:
	
		- ini files with names beginning with "Armors_"
		- section names are armor FormIDs
		
		Available Variables:
			
			sBaseName (string)
			
			sNamingRules (FormID String)
			- instance naming rules
			
			iArmorRating (int)
			
			sDamageTypes (DamageType String)
			- resistances to different damage types
			
			iBaseValue (int)
			
			fBaseWeight (float)
			
			iBaseHealth (int)
			- only used by power armor by default
			
			sKeywords (FormID String)
				- default keywords to add
			
			sKeywordsRemove (FormID String)
				- default keywords to remove
			
			
	Weapons:
	
		- ini files with names beginning with "Weapons_"
		- section names are weapon FormIDs
		
		Available Variables:
			
			sBaseName (string)
			sNamingRules (FormID String)
			- instance naming rules
			iBaseDamage (int)
			sBaseDamageTypes (DamageType Strings)
			fBaseSecDamage (float)
			fAPCost (float)
			fCritChargeBonus (float)
			fBaseRangeMax (float)
			fBaseRangeMin (float)
			fOutOfRangeMult (float)
			fAttackDelay (float)
			fSpeedMult (float)
			fReach (float)
			iStaggerAmount (int)
			iBaseValue (int)
			fWeight (float)
			fRecoilSpringForce (float)
			fRecoilMin (float)
			fRecoilMax (float)
			fRecoilHipMult (float)
			fConeOfFireMin (float)
			fConeOfFireMax (float)
			sAmmoID (FormID string)
			sAmmoLootList (FormID string)
			sProjectile (FormID string)
			sImpactDataSet (FormID string)
			bNPCsUseAmmo (bool)
			
			sActorValues (ActorValue String)
			- list of ActorValue modifiers for this weapon
			
			sEnchantments (FormID String)
			- list of enchantments
			
			sKeywords (FormID String)
				- default keywords to add
			
			sKeywordsRemove (FormID String)
				- default keywords to remove
			
			
	Leveled Items:
	
		- ini files with names beginning with "LeveledItems_"
		- section names are Lveled Item FormIDs
		
		Available Variables:
			
			sLeveledItemsRep (LeveledItem string)
			- destructive:
			- replaces the existing base leveled item's list (with all sLeveledItemsRep entries)
			
			sLeveledItemsAdd (LeveledItem string)
			- added to the existing base leveled list
		
