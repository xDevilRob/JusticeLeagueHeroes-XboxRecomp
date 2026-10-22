@echo off
setlocal
cd /d "%~dp0.."
"%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe" /nologo /target:winexe /out:gamelauncher.exe /win32icon:launcher\assets\launcher.ico /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.IO.Compression.dll /reference:System.IO.Compression.FileSystem.dll launcher\src\*.cs
exit /b %errorlevel%
