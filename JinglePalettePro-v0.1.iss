; Inno Setup Script for JinglePalettePro v0.1
#define MyAppName "JinglePalettePro"
#define MyAppVersion "v0.1"
#define MyAppPublisher "jmggs"
#define MyAppExeName "JinglePalette.exe"


[Setup]
AppId={{A1B2C3D4-E5F6-47A8-9B0C-1234567890AB}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputBaseFilename={#MyAppName}-{#MyAppVersion}-Setup
Compression=lzma
SolidCompression=yes
DisableDirPage=no
DisableProgramGroupPage=no
OutputDir=.
SetupIconFile=logo.ico

[Languages]
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"

[Files]
Source: "build\Debug\JinglePalette.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\Debug\Qt6*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\Debug\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\Debug\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\Debug\multimedia\*"; DestDir: "{app}\multimedia"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "resources\*"; DestDir: "{app}\resources"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "logo.ico"; DestDir: "{app}"; Flags: ignoreversion


[Icons]
Name: "{group}\JinglePalettePro"; Filename: "{app}\JinglePalette.exe"; IconFilename: "{app}\logo.ico"
Name: "{commondesktop}\JinglePalettePro"; Filename: "{app}\JinglePalette.exe"; IconFilename: "{app}\logo.ico"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Criar atalho no Ambiente de Trabalho"; GroupDescription: "Opções adicionais:"

[Run]
Filename: "{app}\JinglePalette.exe"; Description: "Executar o JinglePalettePro"; Flags: nowait postinstall skipifsilent
