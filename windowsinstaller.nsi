; Copyright 2008 Jason A. Donenfeld <Jason@zx2c4.com>

SetCompressor /SOLID /FINAL lzma

!define PRODUCT_NAME "Torora"
!define /date PRODUCT_VERSION "Snapshot (%#m-%#d-%#Y)"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\torora.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

!include "MUI.nsh"
!define MUI_ABORTWARNING
!define MUI_ICON ".\src\browser.ico"
!define MUI_UNICON ".\src\browser.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\torora.exe"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${PRODUCT_NAME} ${PRODUCT_VERSION} Installer.exe"
InstallDir "$PROGRAMFILES\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Section "Main Components"
  KillProcDLL::KillProc "torora.exe"
  Sleep 100
  SetOverwrite on

  SetOutPath "$INSTDIR"
  File "torora.exe"
  File "C:\Qt\2009.01\mingw\bin\mingwm10.dll"
  File "C:\Qt\2009.01\qt\bin\QtCore4.dll"
  File "C:\Qt\2009.01\qt\bin\QtGui4.dll"
  File "C:\Qt\2009.01\qt\bin\QtNetwork4.dll"
  File "C:\Qt\2009.01\qt\bin\QtWebKit4.dll"
  File "C:\Qt\2009.01\bin\phonon4.dll"
  File "C:\Users\robert\Documents\Development\openssl-0.9.8j.tar.gz\openssl-0.9.8j\out32dll\ssleay32.dll"
  File "C:\Users\robert\Documents\Development\openssl-0.9.8j.tar.gz\openssl-0.9.8j\out32dll\libeay32.dll"

  SetOutPath "$INSTDIR\locale"
  File "src\.qm\locale\*.qm"

  SetOutPath "$INSTDIR\imageformats"
  File "C:\Qt\2009.01\qt\plugins\imageformats\qtiff4.dll"
  File "C:\Qt\2009.01\qt\plugins\imageformats\qsvg4.dll"
  File "C:\Qt\2009.01\qt\plugins\imageformats\qmng4.dll"
  File "C:\Qt\2009.01\qt\plugins\imageformats\qjpeg4.dll"
  File "C:\Qt\2009.01\qt\plugins\imageformats\qico4.dll"
  File "C:\Qt\2009.01\qt\plugins\imageformats\qgif4.dll"

  SetOutPath "$INSTDIR\iconengines"
  File "C:\Qt\2009.01\qt\plugins\iconengines\qsvgicon4.dll"

  SetOutPath "$INSTDIR\codecs"
  File "C:\Qt\2009.01\qt\plugins\codecs\qtwcodecs4.dll"
  File "C:\Qt\2009.01\qt\plugins\codecs\qkrcodecs4.dll"
  File "C:\Qt\2009.01\qt\plugins\codecs\qjpcodecs4.dll"
  File "C:\Qt\2009.01\qt\plugins\codecs\qcncodecs4.dll"

  ;SetOutPath "$INSTDIR\phonon_backend"
  ;File "C:\Qt\2009.01\qt\plugins\phonon_backend\phonon_ds94.dll"
SectionEnd

Section Icons
  CreateShortCut "$SMPROGRAMS\Torora.lnk" "$INSTDIR\torora.exe"
SectionEnd

Section Uninstaller
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\torora.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\torora.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
SectionEnd

;Section MSVC
;  InitPluginsDir
;  SetOutPath $PLUGINSDIR
;  File "C:\Program Files\Microsoft Visual Studio 8\SDK\v2.0\BootStrapper\Packages\vcredist_x86\vcredist_x86.exe"
;  DetailPrint "Installing Visual C++ 2005 Libraries"
;  ExecWait '"$PLUGINSDIR\vcredist_x86.exe" /q:a /c:"msiexec /i vcredist.msi /quiet"'
;SectionEnd

Section Uninstall
  KillProcDLL::KillProc "torora.exe"
  Sleep 100
  Delete $SMPROGRAMS\Torora.lnk
  RMDir /r "$INSTDIR"
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
SectionEnd

BrandingText "torora.net"
