#if Ver < EncodeVer(6,1,0,0)
  #error This script requires Inno Setup 6 or later
#endif

#define AppVersion GetVersionNumbersString(AddBackslash(SourcePath) + "..\build\bin\baulk.exe")

#ifndef BAULK_BASENAME
  #define BAULK_BASENAME "BaulkSetup"
#endif

#ifndef ArchitecturesAllowed
  #define ArchitecturesAllowed "x64"
#endif

#ifndef ArchitecturesInstallIn64BitMode
  #define ArchitecturesInstallIn64BitMode "x64"
#endif

#ifndef InstallTarget
  #define InstallTarget "user"
#endif

#ifndef AppUserId
  #define AppUserId "Baulk"
#endif


[Setup]
AppId=83ab2204-bde5-4385-9e13-fa8b6276a57e
AppName=baulk
AppVersion={#AppVersion}
AppPublisher=Baulk contributors
AppPublisherURL=https://github.com/baulk/baulk
AppSupportURL=https://github.com/baulk/baulk
LicenseFile=..\LICENSE
WizardStyle=modern
DefaultGroupName=baulk
Compression=lzma2
SolidCompression=yes
OutputDir=..\build
SetupIconFile=baulk-setup.ico
ChangesEnvironment=true
OutputBaseFilename={#BAULK_BASENAME}
; "ArchitecturesAllowed=x64" specifies that Setup cannot run on
; anything but x64.
; "ArchitecturesInstallIn64BitMode=x64" requests that the install be
; done in "64-bit mode" on x64, meaning it should use the native
; 64-bit Program Files directory and the 64-bit view of the registry.
ArchitecturesAllowed={#ArchitecturesAllowed}
ArchitecturesInstallIn64BitMode={#ArchitecturesInstallIn64BitMode}
; version info
VersionInfoCompany=Baulk contributors
VersionInfoDescription=baulk - Minimal Package Manager for Windows
VersionInfoVersion={#AppVersion}
VersionInfoCopyright=Copyright © 2021. Baulk contributors

#if "user" == InstallTarget
DefaultDirName={userpf}\baulk
PrivilegesRequired=lowest
#else
DefaultDirName={pf}\baulk
#endif

UninstallDisplayIcon={app}\bin\baulk.exe

[Files]
Source: "..\build\bin\baulk.exe"; DestDir: "{app}\bin"; DestName: "baulk.exe"
Source: "..\build\bin\baulk-dock.exe"; DestDir: "{app}\bin"; DestName: "baulk-dock.exe"
Source: "..\build\bin\baulk-exec.exe"; DestDir: "{app}\bin"; DestName: "baulk-exec.exe"
Source: "..\build\bin\baulk-lnk.exe"; DestDir: "{app}\bin"; DestName: "baulk-lnk.exe"
Source: "..\build\bin\baulk-update.exe"; DestDir: "{app}\bin"; DestName: "baulk-update.exe"
Source: "..\build\bin\ssh-askpass-baulk.exe"; DestDir: "{app}\bin"; DestName: "ssh-askpass-baulk.exe"
Source: "..\build\bin\baulkterminal.exe"; DestDir: "{app}"; DestName: "baulkterminal.exe"
Source: "..\LICENSE"; DestDir: "{app}\share"; DestName: "LICENSE"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; Flags: unchecked
Name: "quicklaunchicon"; Description: "Create a &Quick Launch icon"; GroupDescription: "Additional icons:"; Flags: unchecked; OnlyBelowVersion: 0,6.1

[Icons]
Name: "{group}\Baulk Terminal"; Filename: "{app}\baulkterminal.exe"; AppUserModelID: "{#AppUserId}"
Name: "{autodesktop}\Baulk Terminal"; Filename: "{app}\baulkterminal.exe"; Tasks: desktopicon; AppUserModelID: "{#AppUserId}"
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\Baulk Terminal"; Filename: "{app}\baulkterminal.exe"; Tasks: quicklaunchicon; AppUserModelID: "{#AppUserId}"
