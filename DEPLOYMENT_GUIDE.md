# NEVRETEM-DER MBS - Windows Deployment Guide

This guide explains how to prepare and distribute the NEVRETEM-DER MBS application for Windows users.

## 🚀 Quick Start

### Method 1: Using the Deployment Script (Recommended)

```batch
# 1. Build the application first
build.bat

# 2. Run the deployment script
deploy_windows.bat
```

### Method 2: Using CMake

```batch
# 1. Build the application
cmake --build build --config Release

# 2. Deploy using CMake target
cmake --build build --target deploy_windows
```

## 📋 Prerequisites

### Development Environment

- **Qt 6.5+** with MSVC compiler
- **Visual Studio 2022** or **Build Tools for Visual Studio 2022**
- **CMake 3.16+**
- **Git** (for cloning dependencies)

### For Distribution

- **NSIS (Nullsoft Scriptable Install System)** - for creating installers
  - Download from: https://nsis.sourceforge.io/

## 🛠️ Deployment Methods

### 1. Portable Distribution (ZIP)

The `deploy_windows.bat` script creates a portable version:

```
deploy_windows/
├── StudentManager.exe          # Main application
├── Qt6Core.dll                # Qt libraries
├── Qt6Gui.dll
├── Qt6Widgets.dll
├── Qt6Network.dll
├── platforms/                 # Qt platform plugins
├── plugins/                   # Additional Qt plugins
├── config.example.ini         # Configuration template
├── universities.json          # University data
├── logo.jpg                   # Application logo
├── run.bat                    # Launcher script
└── README.txt                 # User instructions
```

**Advantages:**

- No installation required
- Can run from USB drive
- Easy to distribute via ZIP

**Usage:**

1. Run `deploy_windows.bat`
2. Zip the `deploy_windows` folder
3. Distribute the ZIP file
4. Users extract and run `StudentManager.exe`

### 2. Professional Installer (NSIS)

Create a professional Windows installer:

```batch
# 1. Create deployment package
deploy_windows.bat

# 2. Compile installer (requires NSIS)
makensis create_installer.nsi
```

**Features:**

- Professional installation experience
- Start Menu shortcuts
- Desktop shortcut
- Add/Remove Programs entry
- Automatic Visual C++ Redistributable installation
- Uninstaller

## 🔧 Configuration

### Firebase Setup

Users need to configure Firebase credentials:

1. Copy `config.example.ini` to `config.ini`
2. Edit `config.ini` with Firebase project details:

```ini
[firestore]
projectId=your-firebase-project-id
apiKey=your-firebase-api-key
```

### System Requirements

- **OS:** Windows 10 or later (64-bit)
- **RAM:** 4 GB minimum, 8 GB recommended
- **Storage:** 100 MB for application + data
- **Network:** Internet connection required for Firebase

## 📦 Distribution Options

### Option 1: Direct Download

- Upload ZIP file to website/cloud storage
- Provide download link to users
- Include setup instructions

### Option 2: Professional Installer

- Use NSIS installer for enterprise deployment
- Can be signed with code signing certificate
- Supports silent installation: `setup.exe /S`

### Option 3: Microsoft Store (Future)

- Package as MSIX for Microsoft Store
- Automatic updates
- Trusted installation source

## 🔍 Troubleshooting

### Common Issues

**"libgcc_s_seh-1.dll not found" Error (MinGW):**

- This occurs when the application was built with MinGW but runtime libraries are missing
- **Quick Fix:** Run `fix_mingw_dlls.bat` in the application directory
- **Manual Fix:** Copy these files from your MinGW installation:
  - `libgcc_s_seh-1.dll`
  - `libstdc++-6.dll`
  - `libwinpthread-1.dll`
- **Prevention:** Use the updated `deploy_windows.bat` script which automatically includes these

**"Application failed to start" Error:**

- Install Visual C++ Redistributable 2022 (for MSVC builds)
- Check Qt libraries are present
- Verify config.ini is properly configured
- For MinGW builds, ensure MinGW runtime DLLs are present

**"Qt platform plugin not found" Error:**

- Ensure `platforms` folder is present
- Check `platforms/qwindows.dll` exists
- Verify deployment was completed successfully

**Network/Firebase Errors:**

- Check internet connection
- Verify Firebase credentials in config.ini
- Ensure Firebase project is properly configured

### Debugging Deployment

1. **Test on Clean System:**

   - Use virtual machine without Qt installed
   - Verify all dependencies are included
2. **Check Dependencies:**

   ```batch
   # Use Dependency Walker or similar tool
   depends.exe StudentManager.exe
   ```
3. **Verify Qt Plugins:**

   - Ensure all required plugins are deployed
   - Check plugin directories are correct

## 🔐 Security Considerations

### Code Signing (Recommended)

```batch
# Sign the executable (requires certificate)
signtool sign /f certificate.p12 /p password /t http://timestamp.digicert.com StudentManager.exe
```

### Antivirus Compatibility

- Some antivirus software may flag unsigned executables
- Consider code signing for production releases
- Test with major antivirus solutions

## 📊 Deployment Checklist

### Pre-Deployment

- [ ] Application builds successfully
- [ ] All features tested on development machine
- [ ] Configuration template created
- [ ] Logo and assets included
- [ ] Version number updated

### Deployment Process

- [ ] Run deployment script
- [ ] Verify all Qt libraries included
- [ ] Test on clean Windows system
- [ ] Check file sizes are reasonable
- [ ] Verify shortcuts work correctly

### Post-Deployment

- [ ] Create user documentation
- [ ] Test installation process
- [ ] Verify uninstallation works
- [ ] Prepare support materials
- [ ] Plan update mechanism

## 📞 Support

For deployment issues or questions:

**Developer:** FURKAN KARAKETİR
**Phone:** +90 551 145 09 68
**Organization:** NEVRETEM-DER

## 📄 License

This software is licensed under the MIT License. See `LICENSE.txt` for details.

---

*Last updated: January 2025*
