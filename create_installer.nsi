# NSIS Installer Script for NEVRETEM-DER MBS
# This script creates a Windows installer using NSIS (Nullsoft Scriptable Install System)
# Download NSIS from: https://nsis.sourceforge.io/

!define APP_NAME "NEVRETEM-DER MBS"
!define APP_VERSION "1.1.1"
!define APP_PUBLISHER "NEVRETEM-DER"
!define APP_URL "https://nevretem-der.org"
!define APP_DESCRIPTION "Mezun Bilgi Sistemi - Student Management System"
!define APP_EXE "StudentManager.exe"
!define APP_ICON "logo.jpg"

# Installer settings
Name "${APP_NAME}"
OutFile "NEVRETEM-DER_MBS_Setup.exe"
InstallDir "$PROGRAMFILES64\${APP_PUBLISHER}\${APP_NAME}"
InstallDirRegKey HKLM "Software\${APP_PUBLISHER}\${APP_NAME}" "InstallDir"
RequestExecutionLevel admin

# Modern UI
!include "MUI2.nsh"

# Interface settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

# Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

# Uninstaller pages
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

# Languages
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Turkish"

# Version information
VIProductVersion "1.1.1.0"
VIAddVersionKey "ProductName" "${APP_NAME}"
VIAddVersionKey "ProductVersion" "${APP_VERSION}"
VIAddVersionKey "CompanyName" "${APP_PUBLISHER}"
VIAddVersionKey "FileDescription" "${APP_DESCRIPTION}"
VIAddVersionKey "FileVersion" "${APP_VERSION}"
VIAddVersionKey "LegalCopyright" "© 2025 ${APP_PUBLISHER}"

# Default section (required)
Section "!${APP_NAME} (required)" SecMain
  SectionIn RO
  
  # Set output path to the installation directory
  SetOutPath $INSTDIR
  
  # Copy application files
  File /r "deploy_windows\*.*"
  
  # Create desktop shortcut
  CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" 0
  
  # Create start menu shortcuts
  CreateDirectory "$SMPROGRAMS\${APP_PUBLISHER}"
  CreateShortCut "$SMPROGRAMS\${APP_PUBLISHER}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" 0
  CreateShortCut "$SMPROGRAMS\${APP_PUBLISHER}\Uninstall ${APP_NAME}.lnk" "$INSTDIR\Uninstall.exe"
  
  # Write registry keys
  WriteRegStr HKLM "Software\${APP_PUBLISHER}\${APP_NAME}" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "Software\${APP_PUBLISHER}\${APP_NAME}" "Version" "${APP_VERSION}"
  
  # Write uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  # Add to Add/Remove Programs
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayIcon" "$INSTDIR\${APP_EXE}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "${APP_PUBLISHER}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "URLInfoAbout" "${APP_URL}"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoRepair" 1
  
  # Calculate installed size
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "EstimatedSize" "$0"
SectionEnd

# Optional section for Visual C++ Redistributable
Section "Visual C++ Redistributable 2022" SecVCRedist
  # Download and install VC++ Redistributable if not present
  DetailPrint "Checking for Visual C++ Redistributable 2022..."
  
  # Check if already installed
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\X64" "Version"
  StrCmp $0 "" 0 vcredist_installed
  
  DetailPrint "Downloading Visual C++ Redistributable 2022..."
  NSISdl::download "https://aka.ms/vs/17/release/vc_redist.x64.exe" "$TEMP\vc_redist.x64.exe"
  Pop $R0
  StrCmp $R0 "success" +2
    MessageBox MB_OK "Failed to download Visual C++ Redistributable. Please install it manually."
    Goto vcredist_end
  
  DetailPrint "Installing Visual C++ Redistributable 2022..."
  ExecWait "$TEMP\vc_redist.x64.exe /quiet /norestart"
  Delete "$TEMP\vc_redist.x64.exe"
  Goto vcredist_end
  
  vcredist_installed:
    DetailPrint "Visual C++ Redistributable 2022 is already installed."
  
  vcredist_end:
SectionEnd

# Optional section for configuration
Section "Create Configuration File" SecConfig
  # Create default config.ini if it doesn't exist
  IfFileExists "$INSTDIR\config.ini" config_exists
    CopyFiles "$INSTDIR\config.example.ini" "$INSTDIR\config.ini"
    MessageBox MB_ICONINFORMATION "A default configuration file has been created at:$\n$INSTDIR\config.ini$\n$\nPlease edit this file with your Firebase credentials before running the application."
  config_exists:
SectionEnd

# Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMain} "Core application files (required)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecVCRedist} "Microsoft Visual C++ Redistributable 2022 (recommended)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecConfig} "Create default configuration file"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

# Uninstaller section
Section "Uninstall"
  # Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
  DeleteRegKey HKLM "Software\${APP_PUBLISHER}\${APP_NAME}"
  DeleteRegKey /ifempty HKLM "Software\${APP_PUBLISHER}"
  
  # Remove shortcuts
  Delete "$DESKTOP\${APP_NAME}.lnk"
  Delete "$SMPROGRAMS\${APP_PUBLISHER}\${APP_NAME}.lnk"
  Delete "$SMPROGRAMS\${APP_PUBLISHER}\Uninstall ${APP_NAME}.lnk"
  RMDir "$SMPROGRAMS\${APP_PUBLISHER}"
  
  # Remove files and directories
  RMDir /r "$INSTDIR\platforms"
  RMDir /r "$INSTDIR\plugins"
  RMDir /r "$INSTDIR\styles"
  RMDir /r "$INSTDIR\imageformats"
  RMDir /r "$INSTDIR\networkinformation"
  RMDir /r "$INSTDIR\tls"
  Delete "$INSTDIR\*.*"
  RMDir "$INSTDIR"
  
  # Remove installation directory if empty
  RMDir "$PROGRAMFILES64\${APP_PUBLISHER}"
SectionEnd

# Functions
Function .onInit
  # Check if application is already running
  System::Call 'kernel32::CreateMutexA(i 0, i 0, t "${APP_NAME}Installer") i .r1 ?e'
  Pop $R0
  StrCmp $R0 0 +3
    MessageBox MB_OK|MB_ICONEXCLAMATION "The installer is already running."
    Abort
FunctionEnd

# Include required libraries
!include "FileFunc.nsh"
!insertmacro GetSize
