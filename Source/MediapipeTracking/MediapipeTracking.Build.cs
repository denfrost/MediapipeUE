// OpenSeeUE Copyright (c) 2021 Hannes De Jager. This software is released under the MIT License.

using UnrealBuildTool;
using System.IO;

public class MediapipeTracking : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    public MediapipeTracking(ReadOnlyTargetRules Target) : base(Target)
	{
	    PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
						Path.Combine(ModuleDirectory, "Public"),
				// ... add public include paths required here ...
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "Private"),
				// ... add other private include paths required here ...
			}
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "CoreUObject",
                "Engine",
				"Sockets",
				"Networking"

                // ... add other public dependencies that you statically link with here ...
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore"
				// ... add private dependencies that you statically link with here ...	
			}
			);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
            }
            );

	}
}
