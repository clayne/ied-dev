#pragma once

#include "Localization/Common.h"

namespace IED
{
	namespace UI
	{
		enum class UITip : Localization::StringID
		{
			SyncReferenceNode                   = 1000,
			DropOnDeath                         = 1001,
			TargetNode                          = 1002,
			HideInFurniture                     = 1003,
			AttachmentMode                      = 1004,
			Position                            = 1005,
			Rotation                            = 1006,
			CustomInventoryItem                 = 1007,
			CustomFormModelSwap                 = 1008,
			CustomCountRange                    = 1009,
			CustomForm                          = 1010,
			PreferredItem                       = 1011,
			SkipTempRefs                        = 1012,
			Invisible                           = 1013,
			PropagateMemberToEquipmentOverrides = 1014,
			AlwaysUnloadSlot                    = 1015,
			HideEquipped                        = 1016,
			LeftWeapon                          = 1017,
			AlwaysUnloadCustom                  = 1018,
			HideLayingDown                      = 1019,
			RemoveScabbard                      = 1020,
			Load1pWeaponModel                   = 1021,
			CheckCannotWear                     = 1022,
			CustomChance                        = 1023,
			CustomEquipmentMode                 = 1024,
			IgnoreRaceEquipTypes                = 1025,
			DisableIfEquipped                   = 1026,
			ModelCache                          = 1027,
			CloseOnESC                          = 1028,
			EnableRestrictions                  = 1029,
			ControlLock                         = 1030,
			UIScale                             = 1031,
			AnimSupportWarning                  = 1032,
			CacheInfo                           = 1033,
			ImportMode                          = 1034,
			LoadARMA                            = 1035,
			SelectCrosshairActor                = 1036,
			KeepTorchFlame                      = 1037,
			DisableHavok                        = 1038,
			MatchSkin                           = 1039,
			SyncSexes                           = 1040,
			IsFavorited                         = 1041,
			NoCheckFav                          = 1042,
			FreezeTime                          = 1043,
			UseWorldModel                       = 1044,
			ReleaseFontData                     = 1045,
			IgnoreRaceEquipTypesSlot            = 1046,
			MatchChildLoc                       = 1047,
			MatchEitherFormKW                   = 1048,
			MatchWorldspaceParent               = 1049,
			PlayAnimation                       = 1050,
			EquippedConditionsEquipment         = 1051,
			DeadScatter                         = 1052,
			XP32AA                              = 1053,
			XP32AA_FF                           = 1054,
			RandPlacement                       = 1055,
			BhkAnims                            = 1056,
			DisableWeaponAnims                  = 1057,
			DisableAnimEventForwarding          = 1058,
			AnimEventForwarding                 = 1059,
			AnimationEvent                      = 1060,
			EffectShadersParallelUpdates        = 1061,
			EquippedConditionsGearPositioning   = 1062,
			CustomLastEquipped                  = 1063,
			Presence                            = 1064,
			IdleCondition                       = 1065,
			CustomFormLastEquipped              = 1066,
			OverrideModel                       = 1067,
			EquipmentOverrideGroupContinue      = 1068,
			EffectShadersTargetRoot             = 1069,
			PVStiffness2                        = 1070,
			PVSpringSlackOffset                 = 1071,
			PVSpringSlackMag                    = 1072,
			PVDamping                           = 1073,
			PVGravityBias                       = 1074,
			PVGravityCorrection                 = 1075,
			PVResistance                        = 1076,
			PVMass                              = 1077,
			PVMaxVelocity                       = 1078,
			PVLinearScale                       = 1079,
			PVRotationScale                     = 1080,
			PVRotationAdjust                    = 1081,
			PVCogOffset                         = 1082,
			PVRotGravityCorrection              = 1083,
			PVVelocityResponseScale             = 1084,
			PVPenBiasDepthLimit                 = 1085,
			PVPenBiasFactor                     = 1086,
			PVRestitutionCoefficient            = 1087,
			PVSphereOffset                      = 1088,
			PVSphereRadius                      = 1089,
			PVBoxMin                            = 1090,
			PVBoxMax                            = 1091,
			PVStiffness                         = 1094,
			EquipmentPhysics                    = 1095,
			PhysicsOffWarning                   = 1096,
			PVConstraintFriction                = 1097,
			AttachLight                         = 1098,
			HideLight                           = 1099,
			RemoveProjectileTracers             = 1100,
		};

	}
}