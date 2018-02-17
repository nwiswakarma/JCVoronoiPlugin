/*
 JCVoronoiPlugin 0.0.1
 -----
 
*/
using System.IO;
using System.Collections;
using UnrealBuildTool;

public class JCVoronoiPlugin: ModuleRules
{
    public JCVoronoiPlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "Engine"
            });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        string ThirdPartyPath = Path.Combine(ModuleDirectory, "../../ThirdParty");

        // -- JCV include and lib path

        string JCVPath = Path.Combine(ThirdPartyPath, "jcvoronoi");
        string JCVInclude = Path.Combine(JCVPath, "include");
        string JCVLib = Path.Combine(JCVPath, "lib");

        PublicIncludePaths.Add(Path.GetFullPath(JCVInclude));
        PublicLibraryPaths.Add(Path.GetFullPath(JCVLib));

        PublicAdditionalLibraries.Add("JCVoronoi.lib");
    }
}
