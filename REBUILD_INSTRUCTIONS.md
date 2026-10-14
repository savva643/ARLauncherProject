# Rebuild Instructions

## ARLauncher (Linux/PC)

✅ **Already rebuilt with full clean:**
```bash
cd /home/savva/Documents/ARLauncherProject/ARLauncher
rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
./bin/ARLauncher
```

## ARStreamer (iPhone/Xcode)

⚠️ **IMPORTANT: Clean and rebuild in Xcode:**

1. **Clear Xcode cache:**
   ```bash
   rm -rf ~/Library/Developer/Xcode/DerivedData/*
   ```
   ✅ Already done!

2. **In Xcode:**
   - Product → Clean Build Folder (Cmd+Shift+K)
   - Product → Build (Cmd+B)
   - Run on iPhone (Cmd+R)

3. **Or from command line:**
   ```bash
   cd /home/savva/Documents/ARLauncherProject/ARStreamer
   xcodebuild clean -scheme ARStreamer
   xcodebuild build -scheme ARStreamer -configuration Debug
   ```

## Changes Made:

✅ **ARStreamer.swift**
- Fixed LiDAR duplication by checking displayMode in depthDataOutput
- Only sends depth when displayMode is .depthOnly or .both
- Skips depth in .rgbOnly mode

✅ **ContentView.swift**
- Removed IMU display from top-left corner
- Added IMU and position to debug panel (gauge button)

✅ **NetworkConnectViewModel.swift**
- Added imuText and positionText variables
- Added imuUpdateCallback to ARStreamer

## Expected Results:

1. ✅ LiDAR shows only once in each mode
2. ✅ IMU and position visible in debug panel
3. ✅ No flickering
4. ✅ Proper camera rotation (180°)

---

**After rebuilding, all issues should be resolved!**
