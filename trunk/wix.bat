"c:\Program Files (x86)\WiX Toolset v3.9\bin\candle" knxcfg-program.wxs 
"c:\Program Files (x86)\WiX Toolset v3.9\bin\light.exe" -ext WixUIExtension  knxcfg-program.wixobj 

"c:\Program Files (x86)\WiX Toolset v3.9\bin\candle" -ext WixNetFxExtension -ext WixBalExtension -ext WixUtilExtension   knxcfg-installer.wix 
"c:\Program Files (x86)\WiX Toolset v3.9\bin\light.exe" -ext WixUIExtension  -ext WixNetFxExtension -ext WixBalExtension -ext WixUtilExtension knxcfg-installer.wixobj 

copy knxcfg-installer.exe c:\temp\knxcfg\knxcfg-installer.exe
