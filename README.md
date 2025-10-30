# The Travelling Salesman — DSAI Group Project

Short, working README for the group project. This repository contains the group implementation and experiments for the Travelling Salesman-related assignments in the DSAI program.

## Status
Work in progress — the project is being developed using CMake and Visual Studio on Windows. This file is intentionally brief; more details and usage docs will be added as the project matures.

## Contributors
- Volodymyr Rodin — 20188
- Irfan Hadžić — 20013
- Kemal Sivro — 20015

## Credits
Special thanks to Prof. Izudin Džafić for guidance and for the natID components used in parts of this project.

## Prerequisites
- Visual Studio (2019/2022) with "Desktop development with C++" workload
- CMake (recommended >= 3.20)
- Git
- natID (if you are building GUI parts that depend on it) — ensure headers/libs are available or update CMake include paths accordingly

## Quick setup (recommended, PowerShell)
Clone the repository and build with CMake + Visual Studio from PowerShell. Replace the Visual Studio generator if you use a different version.

```powershell
# from a directory where you want the repo
git clone https://github.com/vrosi21/DSAI_AI_Project_The_Travelling_Salesman.git
cd DSAI_AI_Project_The_Travelling_Salesman

# create an out-of-source build and configure for Visual Studio 2022 x64
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# build (Release)
cmake --build build --config Release
``` 
