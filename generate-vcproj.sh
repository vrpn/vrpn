#!/bin/sh
function SourceFile()
{
cat <<EOS
			<File
				RelativePath="${1}"
				>
				<FileConfiguration
					Name="Release|Win32"
					>
					<Tool
						Name="VCCLCompilerTool"
						CompileAs="2"
					/>
				</FileConfiguration>
				<FileConfiguration
					Name="Debug|Win32"
					>
					<Tool
						Name="VCCLCompilerTool"
						CompileAs="2"
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
		SourceFile $src
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
	Version="8.00"
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
				AdditionalIncludeDirectories="&quot;$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Include&quot;;../dtrack;quat;../isense;../Dtrack;../libfreespace/include;&quot;$(SYSTEMDRIVE)\Program Files\National Instruments\NI-DAQ\DAQmx ANSI C Dev\include&quot;;&quot;$(SYSTEMDRIVE)\Program Files\National Instruments\NI-DAQ\Include&quot;;&quot;$(SYSTEMDRIVE)\sdk\cpp&quot;;&quot;$(SYSTEMDRIVE)\Program Files\boost\boost_1_34_1&quot;;./submodules/hidapi/hidapi;&quot;$(SYSTEMDRIVE)\Program Files\libusb-1.0\libusb&quot;"
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
				AdditionalIncludeDirectories="&quot;$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Include&quot;;../dtrack;quat;../isense;../Dtrack;../libfreespace/include;&quot;$(SYSTEMDRIVE)\Program Files\National Instruments\NI-DAQ\DAQmx ANSI C Dev\include&quot;;&quot;$(SYSTEMDRIVE)\Program Files\National Instruments\NI-DAQ\Include&quot;;&quot;$(SYSTEMDRIVE)\sdk\cpp&quot;;&quot;$(SYSTEMDRIVE)\Program Files\boost\boost_1_34_1&quot;;./submodules/hidapi/hidapi;&quot;$(SYSTEMDRIVE)\Program Files\libusb-1.0\libusb&quot;"
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
	Version="8.00"
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
				AdditionalIncludeDirectories="&quot;$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Include&quot;;quat;../isense;../Dtrack;../libfreespace/include;&quot;$(SYSTEMDRIVE)\Program Files\National Instruments\NI-DAQ\DAQmx ANSI C Dev\include&quot;;&quot;$(SYSTEMDRIVE)\Program Files\National Instruments\NI-DAQ\Include&quot;;&quot;$(SYSTEMDRIVE)\sdk\cpp&quot;;&quot;$(SYSTEMDRIVE)\Program Files\boost\boost_1_34_1&quot;;./submodules/hidapi/hidapi;&quot;$(SYSTEMDRIVE)\Program Files\libusb-1.0\libusb&quot;"
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
				AdditionalLibraryDirectories="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Lib,$(SYSTEMDRIVE)\sdk\cpp,$(SYSTEMDRIVE)\Program Files\boost\boost_1_34_1\lib"
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
				AdditionalIncludeDirectories="&quot;$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Include&quot;;quat;../isense;../Dtrack;../libfreespace/include;&quot;$(SYSTEMDRIVE)\Program Files\National Instruments\NI-DAQ\DAQmx ANSI C Dev\include&quot;;&quot;$(SYSTEMDRIVE)\Program Files\National Instruments\NI-DAQ\Include&quot;;&quot;$(SYSTEMDRIVE)\sdk\cpp&quot;;&quot;$(SYSTEMDRIVE)\Program Files\boost\boost_1_34_1&quot;;./submodules/hidapi/hidapi;&quot;$(SYSTEMDRIVE)\Program Files\libusb-1.0\libusb&quot;"
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
				AdditionalLibraryDirectories="$(SYSTEMDRIVE)\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Lib,$(SYSTEMDRIVE)\sdk\cpp,$(SYSTEMDRIVE)\Program Files\boost\boost_1_34_1\lib"
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
dos2unix --u2d vrpn.vcproj

GenerateVrpnDLLVCPROJ > vrpndll.vcproj
dos2unix --u2d vrpndll.vcproj
# | sed 's|^\(.*\)$|<File RelativePath="\1"></File>|'
