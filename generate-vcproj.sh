#!/bin/sh

if which unix2dos > /dev/null 2>&1; then
    UNIX2DOS=$(which unix2dos)
elif which fromdos > /dev/null 2>&1 && which todos > /dev/null 2>&1; then
    # wouldn't trust just looking for todos because that's also todo s
    UNIX2DOS=$(which todos)
elif which dos2unix > /dev/null 2>&1 && (echo "testing" | dos2unix --u2d)> /dev/null 2>&1; then
    # Ah, we have one of the dos2unix binaries that takes the u2d parameter. Interesting.
    UNIX2DOS="$(which dos2unix) --u2d"
else
    UNIX2DOS="echo Could not find a unix2dos-type tool so line endings might be wrong on "
fi

function IncludeDirectories()
{
cat <<'EOS'
				AdditionalIncludeDirectories="&quot;$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Include&quot;;../dtrack;quat;../isense;../Dtrack;../libfreespace/include;&quot;$(SYSTEMDRIVE)\Program Files\National Instruments\NI-DAQ\DAQmx ANSI C Dev\include&quot;;&quot;$(SYSTEMDRIVE)\Program Files\National Instruments\NI-DAQ\Include&quot;;&quot;$(SYSTEMDRIVE)\sdk\cpp&quot;;&quot;$(SYSTEMDRIVE)\Program Files\boost\boost_1_34_1&quot;;./submodules/hidapi/hidapi;&quot;$(SYSTEMDRIVE)\Program Files\libusb-1.0\libusb&quot;;&quot;$(SYSTEMDRIVE)\Program Files (x86)\Polhemus\PDI\PDI_90\Inc&quot;"
EOS
}

function LinkDirectories()
{
cat <<'EOS'
				AdditionalLibraryDirectories="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Lib;$(SYSTEMDRIVE)\sdk\cpp;$(SYSTEMDRIVE)\Program Files\boost\boost_1_34_1\lib;&quot;$(SYSTEMDRIVE)\Program Files (x86)\Polhemus\PDI\PDI_90\Lib\Win32&quot;"
EOS
}

function SourceFile()
{
if [ "x$2" == "xc" ]; then
	COMPILEAS=1 # If we pass a second argument "c", then compile this as C
else
	COMPILEAS=2 # Otherwise force compilation as C++
fi

cat <<EOS
			<File
				RelativePath="${1}"
				>
				<FileConfiguration
					Name="Release|Win32"
					>
					<Tool
						Name="VCCLCompilerTool"
						CompileAs="${COMPILEAS}"
					/>
				</FileConfiguration>
				<FileConfiguration
					Name="Debug|Win32"
					>
					<Tool
						Name="VCCLCompilerTool"
						CompileAs="${COMPILEAS}"
					/>
				</FileConfiguration>
			</File>
EOS

}

function HeaderFile()
{
cat <<EOS
			<File
				RelativePath="${1}"
				>
			</File>
EOS
}

function CleanFileList()
{
	sort -f | grep --invert-match "Android" | grep --invert-match "Atmel"
}

function GenerateFilesSection()
{
	cat << EOS
	<Files>
		<Filter
			Name="Source Files"
			Filter="cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
			>
EOS

	for src in $(ls vrpn_*.C vrpn_*.cpp | CleanFileList)
	do
		if (echo "$src" | grep -i "Local_HIDAPI" >/dev/null); then
			# This is the HIDAPI source, which must be compiled as C or signal11 gets grumpy.
			SourceFile $src c
		else
			SourceFile $src
		fi
	done

cat <<EOS
		</Filter>
		<Filter
			Name="Header Files"
			Filter="h;hpp;hxx;hm;inl"
			>
EOS

	for header in $(ls vrpn_*.h | CleanFileList)
	do
		HeaderFile $header
	done

cat <<EOS
		</Filter>
	</Files>
EOS
}

function GenerateVrpnVCPROJ()
{

cat <<'EOS'
<?xml version="1.0" encoding="Windows-1252"?>
<VisualStudioProject
	ProjectType="Visual C++"
	Version="9.00"
	Name="vrpn"
	ProjectGUID="{7EE1DB03-16A5-4465-9164-061BE0C88B8C}"
	RootNamespace="vrpn"
	>
	<Platforms>
		<Platform
			Name="Win32"
		/>
	</Platforms>
	<ToolFiles>
	</ToolFiles>
	<Configurations>
		<Configuration
			Name="Release|Win32"
			OutputDirectory=".\pc_win32/Release"
			IntermediateDirectory=".\pc_win32/Release"
			ConfigurationType="4"
			InheritedPropertySheets="$(VCInstallDir)VCProjectDefaults\UpgradeFromVC60.vsprops"
			UseOfMFC="0"
			ATLMinimizesCRunTimeLibraryUsage="false"
			CharacterSet="2"
			>
			<Tool
				Name="VCPreBuildEventTool"
			/>
			<Tool
				Name="VCCustomBuildTool"
			/>
			<Tool
				Name="VCXMLDataGeneratorTool"
			/>
			<Tool
				Name="VCWebServiceProxyGeneratorTool"
			/>
			<Tool
				Name="VCMIDLTool"
			/>
			<Tool
				Name="VCCLCompilerTool"
				Optimization="2"
				InlineFunctionExpansion="1"
EOS
IncludeDirectories
cat <<'EOS'
				PreprocessorDefinitions="_CRT_SECURE_NO_WARNINGS,VRPNDLL_NOEXPORTS"
				StringPooling="true"
				RuntimeLibrary="2"
				EnableFunctionLevelLinking="true"
				PrecompiledHeaderFile=".\pc_win32/Release/vrpn.pch"
				AssemblerListingLocation=".\pc_win32/Release/"
				ObjectFile=".\pc_win32/Release/"
				ProgramDataBaseFileName=".\pc_win32/Release/"
				BrowseInformation="1"
				WarningLevel="3"
				SuppressStartupBanner="true"
			/>
			<Tool
				Name="VCManagedResourceCompilerTool"
			/>
			<Tool
				Name="VCResourceCompilerTool"
				Culture="1033"
			/>
			<Tool
				Name="VCPreLinkEventTool"
			/>
			<Tool
				Name="VCLibrarianTool"
				OutputFile=".\pc_win32/Release\vrpn.lib"
EOS
LinkDirectories
cat <<'EOS'
				SuppressStartupBanner="true"
			/>
			<Tool
				Name="VCALinkTool"
			/>
			<Tool
				Name="VCXDCMakeTool"
			/>
			<Tool
				Name="VCBscMakeTool"
				SuppressStartupBanner="true"
				OutputFile=".\pc_win32/Release/vrpn.bsc"
			/>
			<Tool
				Name="VCFxCopTool"
			/>
			<Tool
				Name="VCPostBuildEventTool"
			/>
		</Configuration>
		<Configuration
			Name="Debug|Win32"
			OutputDirectory=".\pc_win32/Debug"
			IntermediateDirectory=".\pc_win32/Debug"
			ConfigurationType="4"
			InheritedPropertySheets="$(VCInstallDir)VCProjectDefaults\UpgradeFromVC60.vsprops"
			UseOfMFC="0"
			ATLMinimizesCRunTimeLibraryUsage="false"
			CharacterSet="2"
			>
			<Tool
				Name="VCPreBuildEventTool"
			/>
			<Tool
				Name="VCCustomBuildTool"
			/>
			<Tool
				Name="VCXMLDataGeneratorTool"
			/>
			<Tool
				Name="VCWebServiceProxyGeneratorTool"
			/>
			<Tool
				Name="VCMIDLTool"
			/>
			<Tool
				Name="VCCLCompilerTool"
				Optimization="0"
EOS
IncludeDirectories
cat <<'EOS'
				PreprocessorDefinitions="_CRT_SECURE_NO_WARNINGS,VRPNDLL_NOEXPORTS"
				RuntimeLibrary="3"
				PrecompiledHeaderFile=".\pc_win32/Debug/vrpn.pch"
				AssemblerListingLocation=".\pc_win32/Debug/"
				ObjectFile=".\pc_win32/Debug/"
				ProgramDataBaseFileName=".\pc_win32/Debug/"
				BrowseInformation="1"
				WarningLevel="3"
				SuppressStartupBanner="true"
				DebugInformationFormat="3"
			/>
			<Tool
				Name="VCManagedResourceCompilerTool"
			/>
			<Tool
				Name="VCResourceCompilerTool"
				Culture="1033"
			/>
			<Tool
				Name="VCPreLinkEventTool"
			/>
			<Tool
				Name="VCLibrarianTool"
				OutputFile=".\pc_win32/Debug\vrpn.lib"
EOS
LinkDirectories
cat <<'EOS'
				SuppressStartupBanner="true"
			/>
			<Tool
				Name="VCALinkTool"
			/>
			<Tool
				Name="VCXDCMakeTool"
			/>
			<Tool
				Name="VCBscMakeTool"
				SuppressStartupBanner="true"
				OutputFile=".\pc_win32/Debug/vrpn.bsc"
			/>
			<Tool
				Name="VCFxCopTool"
			/>
			<Tool
				Name="VCPostBuildEventTool"
			/>
		</Configuration>
	</Configurations>
	<References>
	</References>
EOS

GenerateFilesSection

cat <<EOS
	<Globals>
	</Globals>
</VisualStudioProject>
EOS
}

function GenerateVrpnDLLVCPROJ()
{
cat <<'EOS'
<?xml version="1.0" encoding="Windows-1252"?>
<VisualStudioProject
	ProjectType="Visual C++"
	Version="9.00"
	Name="vrpndll"
	ProjectGUID="{5F38B32E-B5AE-4316-B15E-9E21D033B1BE}"
	RootNamespace="vrpndll"
	>
	<Platforms>
		<Platform
			Name="Win32"
		/>
	</Platforms>
	<ToolFiles>
	</ToolFiles>
	<Configurations>
		<Configuration
			Name="Debug|Win32"
			OutputDirectory=".\pc_win32/DLL/Debug"
			IntermediateDirectory=".\pc_win32/DLL/Debug"
			ConfigurationType="2"
			InheritedPropertySheets="$(VCInstallDir)VCProjectDefaults\UpgradeFromVC60.vsprops"
			UseOfMFC="0"
			ATLMinimizesCRunTimeLibraryUsage="false"
			CharacterSet="2"
			>
			<Tool
				Name="VCPreBuildEventTool"
			/>
			<Tool
				Name="VCCustomBuildTool"
			/>
			<Tool
				Name="VCXMLDataGeneratorTool"
			/>
			<Tool
				Name="VCWebServiceProxyGeneratorTool"
			/>
			<Tool
				Name="VCMIDLTool"
				PreprocessorDefinitions="_DEBUG"
				MkTypLibCompatible="true"
				SuppressStartupBanner="true"
				TargetEnvironment="1"
				TypeLibraryName=".\pc_win32/DLL/Debug/vrpndll.tlb"
				HeaderFileName=""
			/>
			<Tool
				Name="VCCLCompilerTool"
				Optimization="0"
EOS
IncludeDirectories
cat <<'EOS'
				PreprocessorDefinitions="VRPNDLL_EXPORTS,_CRT_SECURE_NO_WARNINGS"
				MinimalRebuild="true"
				BasicRuntimeChecks="3"
				RuntimeLibrary="3"
				PrecompiledHeaderFile=".\pc_win32/DLL/Debug/vrpndll.pch"
				AssemblerListingLocation=".\pc_win32/DLL/Debug/"
				ObjectFile=".\pc_win32/DLL/Debug/"
				ProgramDataBaseFileName=".\pc_win32/DLL/Debug/"
				WarningLevel="3"
				SuppressStartupBanner="true"
				DebugInformationFormat="4"
			/>
			<Tool
				Name="VCManagedResourceCompilerTool"
			/>
			<Tool
				Name="VCResourceCompilerTool"
				PreprocessorDefinitions="_DEBUG"
				Culture="1033"
			/>
			<Tool
				Name="VCPreLinkEventTool"
			/>
			<Tool
				Name="VCLinkerTool"
				IgnoreImportLibrary="true"
				OutputFile=".\pc_win32/DLL/Debug/vrpndll.dll"
				LinkIncremental="2"
				SuppressStartupBanner="true"
EOS
LinkDirectories
cat <<'EOS'
				GenerateDebugInformation="true"
				ProgramDatabaseFile=".\pc_win32/DLL/Debug/vrpndll.pdb"
				ImportLibrary=".\pc_win32\DLL\Debug\vrpndll.lib"
				TargetMachine="1"
			/>
			<Tool
				Name="VCALinkTool"
			/>
			<Tool
				Name="VCManifestTool"
			/>
			<Tool
				Name="VCXDCMakeTool"
			/>
			<Tool
				Name="VCBscMakeTool"
				SuppressStartupBanner="true"
				OutputFile=".\pc_win32/DLL/Debug/vrpndll.bsc"
			/>
			<Tool
				Name="VCFxCopTool"
			/>
			<Tool
				Name="VCAppVerifierTool"
			/>
			<Tool
				Name="VCWebDeploymentTool"
			/>
			<Tool
				Name="VCPostBuildEventTool"
			/>
		</Configuration>
		<Configuration
			Name="Release|Win32"
			OutputDirectory=".\pc_win32/DLL/Release"
			IntermediateDirectory=".\pc_win32/DLL/Release"
			ConfigurationType="2"
			InheritedPropertySheets="$(VCInstallDir)VCProjectDefaults\UpgradeFromVC60.vsprops"
			UseOfMFC="0"
			ATLMinimizesCRunTimeLibraryUsage="false"
			CharacterSet="2"
			>
			<Tool
				Name="VCPreBuildEventTool"
			/>
			<Tool
				Name="VCCustomBuildTool"
			/>
			<Tool
				Name="VCXMLDataGeneratorTool"
			/>
			<Tool
				Name="VCWebServiceProxyGeneratorTool"
			/>
			<Tool
				Name="VCMIDLTool"
				PreprocessorDefinitions="NDEBUG"
				MkTypLibCompatible="true"
				SuppressStartupBanner="true"
				TargetEnvironment="1"
				TypeLibraryName=".\pc_win32/DLL/Release/vrpndll.tlb"
				HeaderFileName=""
			/>
			<Tool
				Name="VCCLCompilerTool"
				Optimization="2"
				InlineFunctionExpansion="1"
EOS
IncludeDirectories
cat <<'EOS'
				PreprocessorDefinitions="VRPNDLL_EXPORTS,_CRT_SECURE_NO_WARNINGS"
				StringPooling="true"
				RuntimeLibrary="2"
				EnableFunctionLevelLinking="true"
				PrecompiledHeaderFile=".\pc_win32/DLL/Release/vrpndll.pch"
				AssemblerListingLocation=".\pc_win32/DLL/Release/"
				ObjectFile=".\pc_win32/DLL/Release/"
				ProgramDataBaseFileName=".\pc_win32/DLL/Release/"
				WarningLevel="3"
				SuppressStartupBanner="true"
			/>
			<Tool
				Name="VCManagedResourceCompilerTool"
			/>
			<Tool
				Name="VCResourceCompilerTool"
				PreprocessorDefinitions="NDEBUG"
				Culture="1033"
			/>
			<Tool
				Name="VCPreLinkEventTool"
			/>
			<Tool
				Name="VCLinkerTool"
				IgnoreImportLibrary="true"
				OutputFile=".\pc_win32/DLL/Release/vrpndll.dll"
				LinkIncremental="1"
				SuppressStartupBanner="true"
EOS
LinkDirectories
cat <<'EOS'
				ProgramDatabaseFile=".\pc_win32/DLL/Release/vrpndll.pdb"
				ImportLibrary=".\pc_win32\DLL\Release\vrpndll.lib"
				TargetMachine="1"
			/>
			<Tool
				Name="VCALinkTool"
			/>
			<Tool
				Name="VCManifestTool"
			/>
			<Tool
				Name="VCXDCMakeTool"
			/>
			<Tool
				Name="VCBscMakeTool"
				SuppressStartupBanner="true"
				OutputFile=".\pc_win32/DLL/Release/vrpndll.bsc"
			/>
			<Tool
				Name="VCFxCopTool"
			/>
			<Tool
				Name="VCAppVerifierTool"
			/>
			<Tool
				Name="VCWebDeploymentTool"
			/>
			<Tool
				Name="VCPostBuildEventTool"
			/>
		</Configuration>
	</Configurations>
	<References>
	</References>
EOS
GenerateFilesSection

cat <<EOS
	<Globals>
	</Globals>
</VisualStudioProject>
EOS

}

GenerateVrpnVCPROJ > vrpn.vcproj
$UNIX2DOS vrpn.vcproj

GenerateVrpnDLLVCPROJ > vrpndll.vcproj
$UNIX2DOS vrpndll.vcproj
# | sed 's|^\(.*\)$|<File RelativePath="\1"></File>|'
