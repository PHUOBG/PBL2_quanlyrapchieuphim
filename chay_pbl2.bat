@echo off
title PBL2 - He Thong Quan Ly Rap Chieu Phim ^& Thong Ke Doanh Thu (ADMIN)
echo Dang khoi chay ung dung PBL2phuong (ROLE: Quan Tri Vien)...
cd /d "%~dp0build"
start "" "app.exe"
powershell -NoProfile -Command "Start-Sleep -Milliseconds 800; $p = Get-Process app -ErrorAction SilentlyContinue | Select-Object -First 1; if ($p -and $p.MainWindowHandle -ne 0) { Add-Type 'using System; using System.Runtime.InteropServices; public class W { [DllImport(\"user32.dll\")] public static extern bool SetForegroundWindow(IntPtr h); [DllImport(\"user32.dll\")] public static extern void SwitchToThisWindow(IntPtr h, bool b); }'; [W]::SetForegroundWindow($p.MainWindowHandle); [W]::SwitchToThisWindow($p.MainWindowHandle, $true); }"
exit