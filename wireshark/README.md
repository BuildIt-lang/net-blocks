# Net-Blocks Wireshark Plugin

This plugin lets Wireshark analyze custom protocols created with Net-Blocks
and display them as **NBX**.

Debian/Ubuntu and, Wireshark **3.6.2**, Python **3.10**.

## 1. Prepare the source directories

The commands below assume that Net-Blocks is already available at
`$HOME/workspace/net-blocks` and use the following directory layout:

```text
~/workspace/
├── net-blocks/
│   └── wireshark/       # NBX plugin sources (this directory)
├── wireshark/           # Wireshark 3.6.2 source tree
└── wireshark-build/     # Out-of-source build directory
```

Clone Wireshark alongside Net-Blocks.

```sh
cd "$HOME/workspace"
git clone -b v3.6.2 --depth=1 https://github.com/wireshark/wireshark.git
```

## 2. Install build dependencies

Install the development libraries and tools needed in this environment.

```sh
sudo apt update
sudo apt install \
    libglib2.0-dev \
    libgcrypt20-dev \
    libc-ares-dev \
    flex bison \
    qtbase5-dev qt5-qmake \
    qttools5-dev qttools5-dev-tools \
    qtmultimedia5-dev libqt5multimedia5-plugins
```

## 3. Generate the custom protocol dissector
 
Add the following include to `src/impls/simple.cpp`:

```cpp
#include "core/genplugin.h"
```

In `main`, after `net_packet.fix_layout()` and before `return 0`, add the
following code to specify the generated file's destination:

```cpp
std::ofstream plugin_output("wireshark/packet-net_blocks-generated.c");
if (!plugin_output) {
    std::cerr << "Failed to open the Wireshark plugin output file" << std::endl;
    return 1;
}
net_blocks::plugin::generate_wireshark_plugin(plugin_output);
```

Build the BuildIt dependency and Net-Blocks, then run the generator with
the two output paths required by its command-line interface:

```sh
make 
./build/impls/simple scratch/gen_headers.h scratch/nb_simple.c
```

Running the generator creates `wireshark/packet-net_blocks-generated.c`. After changing the protocol in `src/impls/simple.cpp`, rebuild and rerun the generator, then rebuild and reinstall the plugin.

## 4. Link the plugin into Wireshark

Create a symbolic link so that Wireshark can build the plugin directly
from this directory:

```sh
cd "$HOME/workspace/net-blocks"
ln -s "$HOME/workspace/net-blocks/wireshark" ../wireshark/plugins/epan/nbxplugin
```

The plugin consists of:

| File | Purpose |
| --- | --- |
| `CMakeLists.txt` | Defines the `nbxplugin` build target and registers the plugin sources. |
| `nbx_protocol.c` | Registers NBX and dissector with Wireshark. |
| `packet-net_blocks-generated.c` | Contains the generated code that decodes the custom protocol's fields. |

## 5. Configure and build the plugin

```sh
cd "$HOME/workspace/net-blocks"
cmake -S ../wireshark -B ../wireshark-build \
    -DPython3_EXECUTABLE="/usr/bin/python3.10" \
    -DPYTHON_EXECUTABLE="/usr/bin/python3.10" \
    -DCUSTOM_PLUGIN_SRC_DIR="plugins/epan/nbxplugin"

cmake --build ../wireshark-build --target nbxplugin 
```

For this Wireshark version, the resulting shared library is:

```text
~/workspace/wireshark-build/run/plugins/3.6/epan/nbxplugin.so
```

After regenerating `packet-net_blocks-generated.c` for a changed protocol, run the build command again.

## 6. Install and verify the plugin

Use the plugin with a compatible Wireshark 3.6 installation. The command below uses the local x86-64 Linux path; 

```sh
sudo install -d /usr/lib/x86_64-linux-gnu/wireshark/plugins/3.6/epan
sudo install -m 755 \
    "$HOME/workspace/wireshark-build/run/plugins/3.6/epan/nbxplugin.so" \
    /usr/lib/x86_64-linux-gnu/wireshark/plugins/3.6/epan/nbxplugin.so
```

Open a capture containing the matching Net-Blocks protocol
and use the filter `nbx` .