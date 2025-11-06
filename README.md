## Quick Start (Linux, Vulkan + SensorConnector + iPhone USB)

### Dependencies (Ubuntu 22.04/24.04)
```bash
sudo apt update
sudo apt install -y cmake build-essential pkg-config \
  libglfw3-dev libglm-dev \
  libvulkan-dev vulkan-tools vulkan-validationlayers vulkan-utility-libraries-dev \
  libturbojpeg-dev libjpeg-turbo8-dev \
  libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev \
  ipheth-utils usbmuxd libimobiledevice6 libimobiledevice-utils
# optional shader tools
sudo apt install -y glslang-tools spirv-tools libshaderc-dev
```

Qt 6 via Qt Maintenance Tool (example path): `/home/savva/Qt/6.5.3/gcc_64`

### Build SensorConnector (Qt6)
```bash
cd /home/savva/Documents/ARLauncherProject/SensorConnector
/home/savva/Qt/6.5.3/gcc_64/bin/qmake6 SensorConnector.pro CONFIG+=release
make -j$(nproc)
```

### Build ARLauncher (Vulkan + SensorConnector)
```bash
cd /home/savva/Documents/ARLauncherProject/ARLauncher
rm -rf build && mkdir build
cmake -S /home/savva/Documents/ARLauncherProject/ARLauncher \
      -B /home/savva/Documents/ARLauncherProject/ARLauncher/build \
      -DUSE_VULKAN=ON -DUSE_OPENGL=OFF \
      -DUSE_SENSOR_CONNECTOR=ON \
      -DQt6_DIR=/home/savva/Qt/6.5.3/gcc_64/lib/cmake/Qt6
cmake --build /home/savva/Documents/ARLauncherProject/ARLauncher/build -j$(nproc)
```

### Enable iPhone USB Tethering
1) In VM (if used): VM → Removable Devices → Apple iPhone → Connect (Disconnect from Host). USB Controller set to USB 3.0/3.1.
2) On iPhone: Settings → Personal Hotspot → enable (keep screen open) and Trust this Computer.
3) Verify and bring up interface:
```bash
sudo systemctl status usbmuxd
lsmod | grep ipheth || sudo modprobe ipheth
lsusb | grep -i apple
nmcli device status            # find iPhone iface (enx...)
IF=<IPHONE_IFACE>
nmcli device set $IF managed yes
nmcli con add type ethernet ifname $IF con-name iPhoneUSB ipv4.method auto ipv6.method ignore connection.autoconnect yes
nmcli con up iPhoneUSB
ip addr show $IF
ping -c1 172.20.10.1           # often iPhone is 172.20.10.1
```
Note: Host often gets 172.20.10.2. Do not hardcode 172.20.10.3.

Fallbacks: if USB–Ethernet won’t appear, use `iproxy 9000 9000` (usbmuxd port forwarding) or Wi‑Fi (listen on 0.0.0.0:9000).

### Run
```bash
/home/savva/Documents/ARLauncherProject/ARLauncher/build/bin/ARLauncher
```
Optional Vulkan validation:
```bash
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation /home/savva/Documents/ARLauncherProject/ARLauncher/build/bin/ARLauncher
```

### Expected
- VulkanRenderer initialized; swapchain size printed; demo scene created.
- SensorConnector servers on TCP/UDP 9000; when frames arrive from iPhone, they appear as background; camera pose mirrored via LensEngine.