using System.IO;
using UnrealBuildTool;

public class SteamVrTrackExt : ModuleRules
{
	private string PluginsPath
	{
		get { return Path.GetFullPath(BuildConfiguration.RelativeEnginePath) + "Plugins/Runtime/"; }
	}

	public SteamVrTrackExt(TargetInfo Target)
	{
		MinFilesUsingPrecompiledHeaderOverride = 1;
		bFasterWithoutUnity = true;
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnforceIWYU = true;

		PublicIncludePaths.AddRange(
			new string[] {
				"SteamVrTrackExt/Public"
			}
			);

		PrivateIncludePaths.AddRange(
			new string[] {
				"SteamVrTrackExt/Private",
				"SteamVR/Private",
			}
			);

		PublicDependencyModuleNames.AddRange(
			new string[] 
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"PhysX",
				"HeadMountedDisplay",
				"RHI",
				"RenderCore",
				"ShaderCore",
				"SteamVR"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"SteamVR"
			}
			);
		// Locking steamVR out of non windows builds
		if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64)
		{

			PublicIncludePaths.AddRange(
			new string[] {
						"../Plugins/Runtime/Steam/SteamVR/Source/SteamVR/Private" // This is dumb but it isn't very open

				// ... add public include paths required here ...
			}
			);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
				"SteamVR",
				"OpenVR",
				"SteamVRController"
				});
		}

	}
}
