# Obidji Jugu installers

Display name: **Obidji Jugu**. Authors: **Hadzic, Rodin, Sivro**.
Executable/package ID: `obidji-jugu` (valid Debian name, no underscores).

Uses natID SetupCollector, as in the natidqp solver project. Use matching SDK
headers, binaries and SetupCollector for the target OS/architecture. No SDK
headers or binaries are patched. Windows packaging uses the reference project's
GTK4 package manifest. Windows, macOS and Linux packaging must run on their own OS.

## Build preparation

Install Python 3 and Pillow (`python -m pip install Pillow`), then run:

```
python tools/generate_icons.py
cmake -S . -B build -DNATID_SDK_ROOT=/absolute/path/to/natID.SDK -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target obidji-jugu
```

On Windows use the x64 Visual Studio generator. natID's normal RAMDisk/output
layout must be configured, just as in its examples. The icon generator converts
`res/flag-bhs.png` to ICO, ICNS and Linux PNG icons, preserving the flag's aspect
ratio on a transparent square. Generate icons BEFORE configuring CMake.

## Package the Release output

```
python tools/package_installer.py --sdk /absolute/path/to/natID.SDK --collector /absolute/path/to/SetupCollector --executable-dir /absolute/path/to/Release --license /absolute/path/to/PROJECT-LICENSE.txt --output installer-output
```

Windows collector is `natID.Utils/windows/SetupCollector.exe`; use the native
macOS/Linux utility on those systems. The `--license` argument is optional; omit
it to generate the configuration without an application license file.
The collector stages natGUIALL and Windows GTK4, regional resources and both XML
translation catalogs. Keep all generated Windows bootstrapper/MSI files together.
Unsigned installers may require platform trust approval; macOS distribution
signing/notarization requires the authors' Apple credentials.

## Localization

`res/DevRes.xml` includes `res/tr/EN/main.xml` and `res/tr/BA/main.xml` through
natID's `%s` translation config. They are packaged resources, not registry-only
translations. The language toolbar saves the language preference and restarts.
The package script resolves development SDK resource paths in its staging copy.

## GitHub Actions (Windows, macOS, Linux)

Commit and push `.github/workflows/build-installer.yml` to the repository's default
branch. In Actions, select **Build Obidji Jugu installers**, then **Run
workflow**. Leave the license path empty to omit the application license, or supply
the path to a committed license file. The workflow installs the matching natID v4.2.1
headers and Windows binaries (20260710), compiles Release, packages the installer,
and uploads **Obidji-Jugu-Windows** as an artifact. Download and extract the entire
artifact so bootstrapper and MSI files stay together. Like the reference project,
`v*` tags publish a release; manual runs publish only when `publish_release` is
`yes` and `release_tag` is supplied.

The same workflow also builds macOS Apple Silicon and Intel app ZIPs (ad-hoc signed), and a Linux Ubuntu 24.04 DEB ZIP. Artifacts: Obidji-Jugu-macOS-Silicon, Obidji-Jugu-macOS-Intel, Obidji-Jugu-Linux. Application name: Obidji Jugu. Authors: Hadzic, Rodin, Sivro.
