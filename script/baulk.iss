#if Ver < EncodeVer(6,1,0,0)
  #error This script requires Inno Setup 6 or later
#endif

#define AppVersion GetVersionNumbersString(AddBackslash(SourcePath) + "..\build\bin\baulk.exe")

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

#if "" == ArchitecturesInstallIn64BitMode
  #define BaseNameSuffix "ia32"
#else
  #define BaseNameSuffix ArchitecturesInstallIn64BitMode
#endif

[Setup]
AppId=83ab2204-bde5-4385-9e13-fa8b6276a57e
AppName=Baulk
AppVersion={#AppVersion}
AppPublisher=Baulk contributors
AppPublisherURL=https://github.com/baulk/baulk
AppSupportURL=https://github.com/baulk/baulk
LicenseFile=..\LICENSE
WizardStyle=modern
DefaultGroupName=Baulk
Compression=lzma2/ultra64
LZMAUseSeparateProcess=yes
SolidCompression=yes
OutputDir=..\build
SetupIconFile=..\res\screw-driver.ico
ChangesEnvironment=true
; "ArchitecturesAllowed=x64" specifies that Setup cannot run on
; anything but x64.
; "ArchitecturesInstallIn64BitMode=x64" requests that the install be
; done in "64-bit mode" on x64, meaning it should use the native
; 64-bit Program Files directory and the 64-bit view of the registry.
ArchitecturesAllowed={#ArchitecturesAllowed}
ArchitecturesInstallIn64BitMode={#ArchitecturesInstallIn64BitMode}
; version info
VersionInfoCompany=Baulk contributors
VersionInfoVersion={#AppVersion}
VersionInfoCopyright=Copyright © 2021. Baulk contributors

#if "user" == InstallTarget
AppVerName=Baulk (User)
VersionInfoDescription=Baulk User Installer
DefaultDirName={userpf}\Baulk
PrivilegesRequired=lowest
OutputBaseFilename=BaulkUserSetup-{#BaseNameSuffix}
VersionInfoOriginalFileName=BaulkUserSetup-{#BaseNameSuffix}.exe
#else
AppVerName=Baulk
VersionInfoDescription=Baulk System Installer
DefaultDirName={commonpf}\Baulk
OutputBaseFilename=BaulkSetup-{#BaseNameSuffix}
VersionInfoOriginalFileName=BaulkSetup-{#BaseNameSuffix}.exe
UsedUserAreasWarning=no
#endif

UninstallDisplayIcon={app}\bin\baulk.exe

[Components]
Name: windowsterminal; Description: "(NEW!) Add a Baulk Profile to Windows Terminal"; MinVersion: 10.0.18362

[Dirs]
Name: "{commonappdata}\Microsoft\Windows Terminal\Fragments\Baulk"; Components: windowsterminal; Check: IsAdminInstallMode
Name: "{localappdata}\Microsoft\Windows Terminal\Fragments\Baulk"; Components: windowsterminal; Check: not IsAdminInstallMode

[Files]
Source: "..\build\bin\baulk.exe"; DestDir: "{app}\bin"; DestName: "baulk.exe"
Source: "..\build\bin\baulk-dock.exe"; DestDir: "{app}\bin"; DestName: "baulk-dock.exe"
Source: "..\build\bin\baulk-exec.exe"; DestDir: "{app}\bin"; DestName: "baulk-exec.exe"
Source: "..\build\bin\baulk-lnk.exe"; DestDir: "{app}\bin"; DestName: "baulk-lnk.exe"
Source: "..\build\bin\baulk-update.exe"; DestDir: "{app}\bin"; DestName: "baulk-update.exe"
Source: "..\build\bin\baulkterminal.exe"; DestDir: "{app}"; DestName: "baulkterminal.exe"
Source: "..\config\baulk.json"; DestDir: "{app}\config"; DestName: "baulk.json"
Source: "..\LICENSE"; DestDir: "{app}\share"; DestName: "LICENSE"
Source: "..\res\screw-driver.ico"; DestDir: "{app}\share\baulk"; DestName: "baulk.ico"
Source: "..\script\Fragments.ps1"; DestDir: "{app}\script"; DestName: "Fragments.ps1"
Source: "..\script\Fragments.bat"; DestDir: "{app}\script"; DestName: "Fragments.bat"
Source: "..\script\FragmentsARM64.ps1"; DestDir: "{app}\script"; DestName: "FragmentsARM64.ps1"
Source: "..\script\FragmentsARM64.bat"; DestDir: "{app}\script"; DestName: "FragmentsARM64.bat"
Source: "..\script\FragmentsDel.bat"; DestDir: "{app}\script"; DestName: "FragmentsDel.bat"
#if "user" == InstallTarget
Source: "..\manifest\user-install.env"; DestDir: "{app}"; DestName: "baulk.env"
#else
Source: "..\manifest\system-install.env"; DestDir: "{app}"; DestName: "baulk.env"
#endif

[UninstallDelete]
; Delete Windows Terminal profile fragments
Type: files; Name: {commonappdata}\Microsoft\Windows Terminal\Fragments\Baulk\baulk.json
Type: files; Name: {localappdata}\Microsoft\Windows Terminal\Fragments\Baulk\baulk.json

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; Flags: unchecked
Name: "quicklaunchicon"; Description: "Create a &Quick Launch icon"; GroupDescription: "Additional icons:"; Flags: unchecked; OnlyBelowVersion: 0,6.1

[Icons]
Name: "{group}\Baulk Terminal"; Filename: "{app}\baulkterminal.exe"; Parameters: "--vs --clang"; AppUserModelID: "{#AppUserId}"
Name: "{autodesktop}\Baulk Terminal"; Filename: "{app}\baulkterminal.exe"; Parameters: "--vs --clang"; Tasks: desktopicon; AppUserModelID: "{#AppUserId}"
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\Baulk Terminal"; Filename: "{app}\baulkterminal.exe"; Parameters: "--vs --clang"; Tasks: quicklaunchicon; AppUserModelID: "{#AppUserId}"

[Registry]
; Aides installing
Root: HKLM; Subkey: Software\Baulk; ValueType: string; ValueName: CurrentVersion; ValueData: {#AppVersion}; Flags: uninsdeletevalue uninsdeletekeyifempty; Check: IsAdminInstallMode
Root: HKLM; Subkey: Software\Baulk; ValueType: string; ValueName: InstallPath; ValueData: {app}; Flags: uninsdeletevalue uninsdeletekeyifempty; Check: IsAdminInstallMode
Root: HKCU; Subkey: Software\Baulk; ValueType: string; ValueName: CurrentVersion; ValueData: {#AppVersion}; Flags: uninsdeletevalue uninsdeletekeyifempty; Check: not IsAdminInstallMode
Root: HKCU; Subkey: Software\Baulk; ValueType: string; ValueName: InstallPath; ValueData: {app}; Flags: uninsdeletevalue uninsdeletekeyifempty; Check: not IsAdminInstallMode

[Code]
procedure LogError(Msg:String);
begin
    SuppressibleMsgBox(Msg,mbError,MB_OK,IDOK);
    Log(Msg);
end;

procedure InstallWindowsTerminalFragment;
var
    AppPath,JSONDirectory,JSONPath:String;
begin
    if IsAdminInstallMode() then
        JSONDirectory:=ExpandConstant('{commonappdata}\Microsoft\Windows Terminal\Fragments\Baulk')
    else
        JSONDirectory:=ExpandConstant('{localappdata}\Microsoft\Windows Terminal\Fragments\Baulk');
    if not ForceDirectories(JSONDirectory) then begin
        LogError('Line {#__LINE__}: Unable to install Windows Terminal Fragment to '+JSONDirectory);
        Exit;
    end;
    JSONPath:=JSONDirectory+'\baulk.json';
    AppPath:=ExpandConstant('{app}');
    StringChangeEx(AppPath, '\', '/', True);
    if not SaveStringToFile(JSONPath,
        '{'+#10
        '  "profiles": ['#10+
        '    {'#10+
        '      "name": "Baulk",'#10+
        '      "guid": "{70972808-9457-5826-a04a-cf51f621d544}",'#10+
        '      "commandline": "\"'+AppPath+'/bin/baulk-exec.exe\" --vs --clang winsh",'#10+
        '      "icon": "'+AppPath+'/share/baulk/baulk.ico",'#10+
        '      "startingDirectory": "%USERPROFILE%",'#10+
        '      "tabTitle": "Windows Terminal \ud83d\udc96 Baulk"'#10+
        '    }'#10+
        '  ]'#10+
        '}',False) then begin;
        LogError('Line {#__LINE__}: Unable to install Windows Terminal Fragment to '+JSONPath)
    end;
end;

procedure CurStepChanged(CurStep:TSetupStep);
begin
    {
        Create the Windows Terminal integration
    }

    if WizardIsComponentSelected('windowsterminal') then
        InstallWindowsTerminalFragment();
end;