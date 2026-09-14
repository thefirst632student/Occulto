# Occulto Middleware Missing Exports Patch

Your check log shows the middleware is now installed correctly:

- `GameAssembly.dll` = middleware, size `197632`
- `GameAssembly.Real.dll` = original Unity GameAssembly, size `12766208`
- `GameAssembly.dll.orig` exists

But the middleware is missing 11 exports that exist in the original GameAssembly:

- `CloseZStream`
- `CreateZStream`
- `Flush`
- `ReadZStream`
- `WriteZStream`
- `UnityPalGetLocalTimeZoneData`
- `UnityPalGetTimeZoneDataForID`
- `UnityPalTimeZoneInfoGetTimeZoneIDs`
- `UseUnityPalForTimeZoneInformation`
- `il2cpp_unity_liveness_calculation_begin`
- `il2cpp_unity_liveness_calculation_end`

Unity may fail with `Failed to load il2cpp` if any required GameAssembly export is absent.

## Install

1. Copy `Occulto_MissingUnity2018Exports.cpp` into:

   `D:\tools\Occulto-main-member104\Middleware\`

2. Open:

   `D:\tools\Occulto-main-member104\Middleware\Middleware.sln`

3. In Visual Studio, add the file to the Middleware project:

   `Middleware project -> Add -> Existing Item -> Occulto_MissingUnity2018Exports.cpp`

4. Build:

   `Release | x64`

5. Copy the new middleware DLL:

   From:

   `D:\tools\Occulto-main-member104\Middleware\x64\Release\GameAssembly.dll`

   To:

   `D:\tools\Occulto-bin\Middleware\GameAssembly.dll`

6. Restore and apply again:

```powershell
python "D:\tools\apply_occulto_minimal.py" `
  --build-root "D:\ps4\ReZeroDeathKiss_tmp\Builds\Release" `
  --restore

python "D:\tools\apply_occulto_minimal.py" `
  --build-root "D:\ps4\ReZeroDeathKiss_tmp\Builds\Release" `
  --metadata-parser "D:\tools\Occulto-bin\MetadataParser\MetadataParser.exe" `
  --middleware-dll "D:\tools\Occulto-bin\Middleware\GameAssembly.dll" `
  --config "D:\tools\Occulto-bin\Config\ReZeroDeathKiss_occulto_T1_include_unity_components.txt"
```

7. Re-run:

```powershell
python "D:\tools\check_occulto_gameassembly.py" `
  --build-root "D:\ps4\ReZeroDeathKiss_tmp\Builds\Release" `
  --middleware "D:\tools\Occulto-bin\Middleware\GameAssembly.dll"
```

Expected:

- `missing from current : 0`
