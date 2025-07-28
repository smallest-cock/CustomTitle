# Custom Title (BakkesMod plugin)
Enables client-side title customization (and more) in Rocket League

🎥 Video showcase: https://youtu.be/qGLaQm3ruLg

<img src="./assets/images/screenshots/spawn_ss.png" width="70%"/>
<img src="./assets/images/screenshots/menu_ss.png" width="70%"/>
<!-- <img src="./assets/images/screenshots/in_game_ss.png" width="70%"/> -->

## ✨ Features
- Edit text, text color & glow color
- Edit tournament rank icons using custom images
- Create title presets to save your designs
- Apply the appearance of any existing RL title
- Spawn your custom title
  - Can also spawn any item based on its product ID
- Make your custom title visible to other players with the mod (optional)
- Censor the custom titles of other players with the mod (optional)

## 🔧 Installation
Download the latest release from the [Releases page](https://github.com/smallest-cock/CustomTitle/releases/latest).

## 💻 Console Commands
You can use the following commands in the BakkesMod console (`F6`) or bind them to keys:

| Command | Description | Best Used With |
|---------|-------------|:--------------:|
| `customtitle_spawn_custom_title` | Spawn current title preset | Key bind |
| `customtitle_spawn_item <id>` | Spawn item based on its product ID | Console |

<br>

## 🛠️ Building
> [!NOTE]  
> Building this plugin requires **64-bit Windows** and the **MSVC** toolchain
> - Due to reliance on the Windows SDK and the need for ABI compatibility with Rocket League

### 1. Initialize Submodules
Run `./scripts/init-submodules.bat` after cloning the repo to initialize the submodules optimally.

<details> <summary>🔍 Why this instead of <code>git submodule update --init --recursive</code> ?</summary>
<li>Avoids downloading 200MB of history for the <strong>nlohmann/json</strong> library</li>
<li>Ensures Git can track submodule updates</li>
</details>

### 2. Set up vcpkg
This project uses [DirectXTex](https://github.com/microsoft/DirectXTex) and [MinHook](https://github.com/TsudaKageyu/minhook) which are installed via vcpkg. Do one of the following:

- **Install vcpkg** (if you dont already have it):
  ```bash
  git clone --recurse-submodules https://github.com/microsoft/vcpkg.git ./vcpkg
  ./vcpkg/bootstrap-vcpkg.bat
  ```
- **Use an existing vcpkg installation** by specifying it in a `./CMakeUserPresets.json`:
  ```json
  {
      "version": 10,
      "configurePresets": [
          {
              "name": "windows-x64-msvc-with-my-vcpkg",
              "inherits": "windows-x64-msvc",
              "cacheVariables": {
                  "CMAKE_TOOLCHAIN_FILE": "C:/<your vcpkg path>/scripts/buildsystems/vcpkg.cmake"
              }
          }
      ]
  }
    ```

➡️ Now when you build the project for the first time, vcpkg will build/install the dependencies listed in `./vcpkg.json`.

More info: [vcpkg manifest mode](https://learn.microsoft.com/en-us/vcpkg/consume/manifest-mode?tabs=msbuild%2Cbuild-MSBuild#2---integrate-vcpkg-with-your-build-system)

### 3. Build with CMake
1. Install [CMake](https://cmake.org/download) and [Ninja](https://github.com/ninja-build/ninja/releases) (or another build system if you're not using Ninja)
2. Run `cmake --preset windows-x64-msvc` (or a custom preset in `CMakeUserPresets.json`) to generate build files in `./build`
3. Run `cmake --build build`
   - The built binaries will be in `./plugins`

<br>

## ❤️ Support
If you found this plugin helpful and want to support future development:

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/sslowdev)
