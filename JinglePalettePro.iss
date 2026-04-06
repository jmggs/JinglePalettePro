[Setup]
AppId={{A3F6A99F-7C1B-4D66-A5F8-4F44F8D1C201}
AppName=Jingle Palette Pro
AppVersion=0.3.0
AppVerName=Jingle Palette Pro v0.3.0
AppPublisher=jmggs
AppPublisherURL=https://github.com/jmggs/JinglePalettePro
AppSupportURL=https://github.com/jmggs/JinglePalettePro
AppUpdatesURL=https://github.com/jmggs/JinglePalettePro
DefaultDirName={autopf}\Jingle Palette Pro
DefaultGroupName=Jingle Palette Pro
DisableProgramGroupPage=yes
LicenseFile=
OutputDir=build\installer
OutputBaseFilename=JinglePalettePro_v0.3.0_Setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
SetupIconFile=logo.ico
UninstallDisplayIcon={app}\Jingle Palette Pro.exe

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"; Flags: unchecked

[Files]
Source: "..\build\Debug\Jingle Palette Pro.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Time_Announce.wav"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Debug\*.dll"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\Debug\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\Debug\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\Debug\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\Debug\multimedia\*"; DestDir: "{app}\multimedia"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\Debug\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\Debug\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\Debug\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\Debug\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\Debug\translations\*"; DestDir: "{app}\translations"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Jingle Palette Pro"; Filename: "{app}\Jingle Palette Pro.exe"; IconFilename: "{app}\Jingle Palette Pro.exe"
Name: "{group}\Uninstall Jingle Palette Pro"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Jingle Palette Pro"; Filename: "{app}\Jingle Palette Pro.exe"; IconFilename: "{app}\Jingle Palette Pro.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\Jingle Palette Pro.exe"; Description: "Launch Jingle Palette Pro"; Flags: nowait postinstall skipifsilent