# Release Guide - Automatic GitHub Release

This guide explains how to automatically build and upload releases to GitHub.

## Overview

When you push a version tag (e.g., `v1.0.2`), GitHub Actions will automatically:
1. Build the project with Qt6 and MinGW
2. Package the application with all dependencies
3. Create a .zip file
4. Upload it to GitHub Releases

## Prerequisites

- Git repository pushed to GitHub
- GitHub Actions enabled (enabled by default for public repos)
- The workflow file `.github/workflows/release.yml` in your repository

## Step-by-Step Release Process

### 1. Update Version

Run the version update script with your new version number:

```bash
update_version.bat 1.0.2
```

This will update the version in:
- `CMakeLists.txt`
- `src/main.cpp`
- `create_installer.nsi`

### 2. Review Changes

Check that the version was updated correctly:

```bash
git diff
```

### 3. Commit Changes

Commit the version updates:

```bash
git add .
git commit -m "Bump version to 1.0.2"
```

### 4. Create and Push Tag

Create a version tag and push everything:

```bash
git tag v1.0.2
git push origin main
git push origin v1.0.2
```

**Important:** The tag must start with `v` (e.g., `v1.0.2`, `v2.0.0`)

### 5. Monitor the Build

1. Go to your GitHub repository
2. Click on the "Actions" tab
3. You'll see a workflow run for "Build and Release"
4. Click on it to see the build progress

The build typically takes 5-10 minutes.

### 6. Release is Created Automatically

Once the build completes:
1. Go to the "Releases" section of your GitHub repository
2. You'll see a new release with:
   - Release title: "Release 1.0.2"
   - The `NEVRETEM-DER_MBS_Windows.zip` file attached
   - Release notes template

### 7. Edit Release Notes (Optional)

You can edit the release to add:
- Changelog
- New features
- Bug fixes
- Known issues

## Workflow Details

### What the Workflow Does

The GitHub Actions workflow (`.github/workflows/release.yml`):

1. **Triggers:** On push of tags starting with `v*`
2. **Environment:** Runs on Windows (latest)
3. **Steps:**
   - Installs Qt 6.9.1 with MinGW
   - Configures CMake
   - Builds the project
   - Runs deployment (copies DLLs, Qt libraries)
   - Creates ZIP package
   - Creates GitHub Release
   - Uploads ZIP to the release

### Customizing the Workflow

To customize the Qt version, edit `.github/workflows/release.yml`:

```yaml
- name: Install Qt
  uses: jurplel/install-qt-action@v3
  with:
    version: '6.9.1'  # Change this to your Qt version
```

## Troubleshooting

### Build Fails

1. Check the Actions log in GitHub for errors
2. Common issues:
   - CMake configuration errors
   - Missing dependencies
   - Compilation errors

### Release Not Created

- Make sure the tag starts with `v` (e.g., `v1.0.2`)
- Check that GitHub Actions is enabled in repository settings
- Verify the workflow file is in `.github/workflows/release.yml`

### ZIP File Not Uploaded

- Check the Actions log for upload errors
- Verify that the ZIP file was created in the "Create ZIP archive" step
- Ensure `GITHUB_TOKEN` has proper permissions

## Testing Locally

Before pushing a tag, you can test the build process locally:

### Windows (Local Build)

```bash
# Build the project
build.bat

# Deploy (creates deploy_windows folder)
deploy_windows.bat

# This creates NEVRETEM-DER_MBS_Windows.zip
```

Test the resulting ZIP file to ensure everything works before creating a release.

## Auto-Update for Users

Once a release is published:

1. Users with the app installed will see an update notification (if you've implemented auto-check)
2. They can go to **Help → Check for Updates**
3. The app will download and install the update automatically

The update checker looks at `FurkanKaraketir/NEVRETEM-DER` repository for releases.

## Release Checklist

Before creating a new release:

- [ ] Test the application locally
- [ ] Update version with `update_version.bat`
- [ ] Update changelog/release notes
- [ ] Commit version changes
- [ ] Create and push tag
- [ ] Monitor GitHub Actions build
- [ ] Verify release was created
- [ ] Edit release notes on GitHub
- [ ] Test download and installation of ZIP
- [ ] Announce the release to users

## Quick Commands Reference

```bash
# Complete release in one go:
update_version.bat 1.0.2
git add .
git commit -m "Bump version to 1.0.2"
git tag v1.0.2
git push origin main && git push origin v1.0.2

# Delete a tag (if you made a mistake):
git tag -d v1.0.2              # Delete locally
git push origin --delete v1.0.2  # Delete remotely

# Create a pre-release tag:
git tag v1.0.2-beta
git push origin v1.0.2-beta
```

## Notes

- Tags starting with `v` are required for the workflow to trigger
- The workflow only runs on tag pushes, not on regular commits
- You can create draft releases manually and have the workflow update them
- The workflow uses `GITHUB_TOKEN` which is automatically provided by GitHub Actions

## Support

If you encounter issues:
1. Check the GitHub Actions logs
2. Review the workflow file
3. Verify all prerequisites are met
4. Test the build locally first

For more information:
- GitHub Actions Documentation: https://docs.github.com/actions
- Qt Installation Action: https://github.com/jurplel/install-qt-action
- Release Action: https://github.com/softprops/action-gh-release

