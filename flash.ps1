# flash.ps1 - Undertow Auto Flash Script
# ホストPCから自動でブートローダーに切り替え、UF2を書き込むスクリプト

param (
    [string]$Uf2Path = "$PSScriptRoot\undertow_default.uf2"
)

$ErrorActionPreference = "Stop"

Write-Host "=== Undertow Auto Flash ===" -ForegroundColor Cyan

if (-not (Test-Path $Uf2Path)) {
    Write-Error "UF2 file not found: $Uf2Path"
    exit 1
}

# 1. 既に RPI-RP2 ドライブが存在するかチェック
$rp2Drive = Get-Volume | Where-Object { $_.FriendlyName -eq "RPI-RP2" -or $_.FileSystemLabel -eq "RPI-RP2" } | Select-Object -First 1

if (-not $rp2Drive) {
    Write-Host "[1/4] Sending bootloader jump command via Raw HID..." -ForegroundColor Yellow

    # C# を用いた Raw HID 通信ヘルパー
    $csharp = @"
using System;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

public class UndertowBootloader {
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    public static extern SafeFileHandle CreateFile(
        string lpFileName,
        uint dwDesiredAccess,
        uint dwShareMode,
        IntPtr lpSecurityAttributes,
        uint dwCreationDisposition,
        uint dwFlagsAndAttributes,
        IntPtr hTemplateFile
    );

    const uint GENERIC_READ = 0x80000000;
    const uint GENERIC_WRITE = 0x40000000;
    const uint FILE_SHARE_READ = 0x00000001;
    const uint FILE_SHARE_WRITE = 0x00000002;
    const uint OPEN_EXISTING = 3;

    public static bool Jump() {
        string path = @"\\?\HID#VID_8884&PID_0900&MI_01#B&2E0B88E&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}";
        SafeFileHandle handle = CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, IntPtr.Zero, OPEN_EXISTING, 0, IntPtr.Zero);
        if (handle.IsInvalid) {
            // パスが見つからない場合は PNP から動的に検索
            return false;
        }

        try {
            using (FileStream stream = new FileStream(handle, FileAccess.ReadWrite, 33, false)) {
                byte[] send = new byte[33];
                send[0] = 0;    // Report ID
                send[1] = 0x0B; // id_bootloader_jump (QMK VIA)
                stream.Write(send, 0, 33);
                stream.Flush();
            }
            return true;
        } catch {
            return false;
        }
    }
}
"@

    try {
        if (-not ([System.Management.Automation.PSTypeName]'UndertowBootloader').Type) {
            Add-Type -TypeDefinition $csharp -Language CSharp
        }
        $sent = [UndertowBootloader]::Jump()
        if ($sent) {
            Write-Host "      Jump command sent successfully!" -ForegroundColor Green
        } else {
            Write-Host "      Could not connect to Undertow Raw HID (Device may already be in bootloader, or older FW)" -ForegroundColor DarkGray
        }
    } catch {
        Write-Warning "Failed to send jump command: $_"
    }

    # 2. RPI-RP2 ドライブのマウントを待機 (最大10秒)
    Write-Host "[2/4] Waiting for RPI-RP2 drive..." -ForegroundColor Yellow
    $timeout = 10
    $elapsed = 0
    while ($elapsed -lt $timeout) {
        Start-Sleep -Milliseconds 500
        $elapsed += 0.5
        $rp2Drive = Get-Volume | Where-Object { $_.FriendlyName -eq "RPI-RP2" -or $_.FileSystemLabel -eq "RPI-RP2" } | Select-Object -First 1
        if ($rp2Drive) {
            break
        }
    }
}

if (-not $rp2Drive) {
    Write-Error "RPI-RP2 drive did not appear. Please put the keyboard into bootloader mode manually."
    exit 1
}

$destDrive = "$($rp2Drive.DriveLetter):\"
Write-Host "[3/4] Flashing $Uf2Path to $destDrive ..." -ForegroundColor Yellow
Copy-Item $Uf2Path -Destination $destDrive

Write-Host "[4/4] Flash complete! Waiting for reboot..." -ForegroundColor Green
Start-Sleep -Seconds 3

Write-Host "=== Done! Undertow is ready. ===" -ForegroundColor Cyan
