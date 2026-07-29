[Setup]
AppName=BookClub Server
AppVersion=4.0
AppPublisher=KKK
DefaultDirName={autopf}\BookClub Server
DefaultGroupName=BookClub Server
OutputDir=C:\Users\amirali\Desktop\installer-client
OutputBaseFilename=BookClubServer_Setup_v4
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
SetupIconFile=C:\Users\amirali\Desktop\favicon.ico

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "C:\Users\amirali\Desktop\THEEND\build\Desktop_Qt_6_11_1_MSVC2022_64bit_Release\bin\BookClubServer.exe"; \
    DestDir: "{app}"; \
    Flags: ignoreversion

Source: "C:\Users\amirali\Desktop\THEEND\build\Desktop_Qt_6_11_1_MSVC2022_64bit_Release\bin\Qt6Core.dll"; \
    DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\amirali\Desktop\THEEND\build\Desktop_Qt_6_11_1_MSVC2022_64bit_Release\bin\Qt6Network.dll"; \
    DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\amirali\Desktop\THEEND\build\Desktop_Qt_6_11_1_MSVC2022_64bit_Release\bin\Qt6Sql.dll"; \
    DestDir: "{app}"; Flags: ignoreversion

Source: "C:\Users\amirali\Desktop\THEEND\build\Desktop_Qt_6_11_1_MSVC2022_64bit_Release\bin\sqldrivers\*"; \
    DestDir: "{app}\sqldrivers"; Flags: ignoreversion
Source: "C:\Users\amirali\Desktop\THEEND\build\Desktop_Qt_6_11_1_MSVC2022_64bit_Release\bin\tls\*"; \
    DestDir: "{app}\tls"; Flags: ignoreversion
Source: "C:\Users\amirali\Desktop\THEEND\build\Desktop_Qt_6_11_1_MSVC2022_64bit_Release\bin\networkinformation\*"; \
    DestDir: "{app}\networkinformation"; Flags: ignoreversion

Source: "C:\Users\amirali\Desktop\favicon.ico"; \
    DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\BookClub Server"; Filename: "{app}\BookClubServer.exe"; IconFilename: "{app}\favicon.ico"
Name: "{userdesktop}\BookClub Server"; Filename: "{app}\BookClubServer.exe"; IconFilename: "{app}\favicon.ico"

[Run]
Filename: "{app}\BookClubServer.exe"; \
    Description: "Launch BookClub Server"; \
    Flags: nowait postinstall skipifsilent
    