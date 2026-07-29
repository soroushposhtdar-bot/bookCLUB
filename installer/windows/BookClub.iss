[Setup]
AppName=BookClub
AppVersion=4.0
AppPublisher=KKK
DefaultDirName={autopf}\BookClub
DefaultGroupName=BookClub
OutputDir=C:\Users\amirali\Desktop\installer-client
OutputBaseFilename=BookClub_Setup_v4
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
SetupIconFile=C:\Users\amirali\Desktop\favicon.ico

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "C:\Users\amirali\Desktop\THEEND\build\Desktop_Qt_6_11_1_MSVC2022_64bit_Release\bin\*"; \
    DestDir: "{app}"; \
    Flags: recursesubdirs createallsubdirs

Source: "C:\Users\amirali\Desktop\favicon.ico"; \
    DestDir: "{app}"; \
    Flags: ignoreversion

[Icons]
Name: "{group}\BookClub Client"; Filename: "{app}\BookClubClient.exe"; IconFilename: "{app}\favicon.ico"
Name: "{group}\BookClub Server"; Filename: "{app}\BookClubServer.exe"
Name: "{userdesktop}\BookClub"; Filename: "{app}\BookClubClient.exe"; IconFilename: "{app}\favicon.ico"

[Run]
Filename: "{app}\BookClubClient.exe"; \
    Description: "Launch BookClub"; \
    Flags: nowait postinstall skipifsilent