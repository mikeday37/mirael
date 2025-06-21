# Getting Started with Mirael
*(this file is a work-in-progress)*
```
git clone https://github.com/mikeday37/mirael.git
cd mirael
code .
```
## Recommended VS Code Extensions

  - **C/C++ Extension Pack** by Microsoft — should include:
    - **C/C++** by Microsoft
    - **CMake Tools** by Microsoft
  - (optional) **C++ Testmate** by Mate Pek — for running unit tests
  - (optional) **C/C++ Themes** by Microsoft — for better syntax coloring
  - (optional) **YAML** by Red Hat — for editing `.clang-format` and `.clang-tidy`

## Common Operations

Use the Command Palette (`Ctrl+Shift+P`) to run CMake tasks.
- **CMake: Select Configure Preset** *(do this first — choose `debug` or `release`)*
- **CMake: Build**
- Typically, `F5` to Run & Debug
- **CMake: Build Target** — for code quality tasks that run on all code except externals:
  - **clang-format-check** — validates code style
  - **clang-format-apply** — applies style automatically
  - **lint.bat** *(run from a terminal)* — runs `.clang-tidy` to perform linting.
    > ⚠️ This is a long-running operation.

## Troubleshooting

You need to have MSVC installed — typically via the **Community Edition of Visual Studio** with the **Desktop development with C++** workload.

Also: **Launch VS Code via a Developer Command Prompt**, such as "**x64 Native Tools Command Prompt for VS 2022**".  This helps ensure your environment variables are correctly configured for that **CMake** can detect and use MSVC.