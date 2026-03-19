param(
    [string]$ExePath = (Join-Path $PSScriptRoot "..\cmake-build-debug\Tinalux.exe"),
    [string]$OutputPath = (Join-Path $PSScriptRoot "..\artifacts\windows-automation.png"),
    [string[]]$MoveRatios = @(),
    [string[]]$ClickRatios = @(),
    [string]$ResetCursorRatio = "0.92,0.92",
    [string]$PreferredWindowTitle = "Tinalux",
    [switch]$CaptureClientArea,
    [int]$WindowX = 80,
    [int]$WindowY = 80,
    [int]$WindowWidth = 1280,
    [int]$WindowHeight = 900,
    [int]$StartupTimeoutMs = 20000,
    [int]$SettleMs = 800,
    [int]$StepDelayMs = 350,
    [switch]$KeepOpen
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;

public static class TinaluxWindowAutomation {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct POINT {
        public int X;
        public int Y;
    }

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);

    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr hWnd, out RECT rect);

    [DllImport("user32.dll")]
    public static extern bool ClientToScreen(IntPtr hWnd, ref POINT point);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc callback, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowText(IntPtr hWnd, System.Text.StringBuilder text, int maxCount);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowTextLength(IntPtr hWnd);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetClassName(IntPtr hWnd, System.Text.StringBuilder className, int maxCount);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool SetCursorPos(int x, int y);

    [DllImport("user32.dll")]
    public static extern bool MoveWindow(IntPtr hWnd, int x, int y, int width, int height, bool repaint);

    [DllImport("user32.dll")]
    public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint dwData, UIntPtr dwExtraInfo);

    public const uint MOUSEEVENTF_LEFTDOWN = 0x0002;
    public const uint MOUSEEVENTF_LEFTUP = 0x0004;
}
"@

function Resolve-WindowPoint {
    param(
        [TinaluxWindowAutomation+RECT]$Rect,
        [string]$RatioSpec
    )

    $parts = $RatioSpec.Split(",", [System.StringSplitOptions]::RemoveEmptyEntries)
    if ($parts.Length -ne 2) {
        throw "Invalid ratio spec '$RatioSpec'. Use 'x,y' with values from 0.0 to 1.0."
    }

    $xRatio = [double]::Parse($parts[0].Trim(), [System.Globalization.CultureInfo]::InvariantCulture)
    $yRatio = [double]::Parse($parts[1].Trim(), [System.Globalization.CultureInfo]::InvariantCulture)
    if ($xRatio -lt 0.0 -or $xRatio -gt 1.0 -or $yRatio -lt 0.0 -or $yRatio -gt 1.0) {
        throw "Ratio spec '$RatioSpec' is out of range. Expected values from 0.0 to 1.0."
    }

    $width = [Math]::Max(1, $Rect.Right - $Rect.Left)
    $height = [Math]::Max(1, $Rect.Bottom - $Rect.Top)
    return @{
        X = $Rect.Left + [int][Math]::Round($width * $xRatio)
        Y = $Rect.Top + [int][Math]::Round($height * $yRatio)
    }
}

function Get-CaptureRect {
    param(
        [IntPtr]$Handle,
        [switch]$ClientArea
    )

    if (-not $ClientArea) {
        $windowRect = New-Object TinaluxWindowAutomation+RECT
        if (-not [TinaluxWindowAutomation]::GetWindowRect($Handle, [ref]$windowRect)) {
            throw "Failed to read the GUI window bounds."
        }
        return $windowRect
    }

    $clientRect = New-Object TinaluxWindowAutomation+RECT
    if (-not [TinaluxWindowAutomation]::GetClientRect($Handle, [ref]$clientRect)) {
        throw "Failed to read the GUI client bounds."
    }

    $topLeft = New-Object TinaluxWindowAutomation+POINT
    $topLeft.X = $clientRect.Left
    $topLeft.Y = $clientRect.Top
    if (-not [TinaluxWindowAutomation]::ClientToScreen($Handle, [ref]$topLeft)) {
        throw "Failed to translate GUI client origin to screen coordinates."
    }

    $bottomRight = New-Object TinaluxWindowAutomation+POINT
    $bottomRight.X = $clientRect.Right
    $bottomRight.Y = $clientRect.Bottom
    if (-not [TinaluxWindowAutomation]::ClientToScreen($Handle, [ref]$bottomRight)) {
        throw "Failed to translate GUI client extent to screen coordinates."
    }

    $captureRect = New-Object TinaluxWindowAutomation+RECT
    $captureRect.Left = $topLeft.X
    $captureRect.Top = $topLeft.Y
    $captureRect.Right = $bottomRight.X
    $captureRect.Bottom = $bottomRight.Y
    return $captureRect
}

function Get-ProcessWindows {
    param(
        [int]$ProcessId
    )

    $windows = New-Object System.Collections.Generic.List[object]
    $callback = [TinaluxWindowAutomation+EnumWindowsProc]{
        param([IntPtr]$Handle, [IntPtr]$LParam)

        $windowProcessId = [uint32]0
        [void][TinaluxWindowAutomation]::GetWindowThreadProcessId($Handle, [ref]$windowProcessId)
        if ($windowProcessId -ne $ProcessId) {
            return $true
        }

        if (-not [TinaluxWindowAutomation]::IsWindowVisible($Handle)) {
            return $true
        }

        $rect = New-Object TinaluxWindowAutomation+RECT
        if (-not [TinaluxWindowAutomation]::GetWindowRect($Handle, [ref]$rect)) {
            return $true
        }

        $width = $rect.Right - $rect.Left
        $height = $rect.Bottom - $rect.Top
        if ($width -le 0 -or $height -le 0) {
            return $true
        }

        $titleCapacity = [Math]::Max([TinaluxWindowAutomation]::GetWindowTextLength($Handle) + 1, 256)
        $titleBuilder = New-Object System.Text.StringBuilder $titleCapacity
        [void][TinaluxWindowAutomation]::GetWindowText($Handle, $titleBuilder, $titleCapacity)

        $classBuilder = New-Object System.Text.StringBuilder 256
        [void][TinaluxWindowAutomation]::GetClassName($Handle, $classBuilder, $classBuilder.Capacity)

        $windows.Add([pscustomobject]@{
                Handle = $Handle
                Title = $titleBuilder.ToString()
                ClassName = $classBuilder.ToString()
                Rect = $rect
                Width = $width
                Height = $height
                Area = $width * $height
            })

        return $true
    }

    [void][TinaluxWindowAutomation]::EnumWindows($callback, [IntPtr]::Zero)
    return $windows
}

function Select-TargetWindow {
    param(
        [object[]]$Windows,
        [string]$PreferredTitle
    )

    $preferred = $Windows | Where-Object {
        $_.ClassName -ne "ConsoleWindowClass" -and $_.Title -eq $PreferredTitle
    } | Sort-Object Area -Descending | Select-Object -First 1
    if ($null -ne $preferred) {
        return $preferred
    }

    $glfwWindow = $Windows | Where-Object {
        $_.ClassName -ne "ConsoleWindowClass" -and $_.ClassName -like "GLFW*"
    } | Sort-Object Area -Descending | Select-Object -First 1
    if ($null -ne $glfwWindow) {
        return $glfwWindow
    }

    $nonConsole = $Windows | Where-Object { $_.ClassName -ne "ConsoleWindowClass" } |
        Sort-Object Area -Descending |
        Select-Object -First 1
    if ($null -ne $nonConsole) {
        return $nonConsole
    }

    return $Windows | Sort-Object Area -Descending | Select-Object -First 1
}

function Invoke-LeftClick {
    param(
        [int]$X,
        [int]$Y
    )

    [TinaluxWindowAutomation]::SetCursorPos($X, $Y) | Out-Null
    Start-Sleep -Milliseconds 80
    [TinaluxWindowAutomation]::mouse_event([TinaluxWindowAutomation]::MOUSEEVENTF_LEFTDOWN, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 40
    [TinaluxWindowAutomation]::mouse_event([TinaluxWindowAutomation]::MOUSEEVENTF_LEFTUP, 0, 0, 0, [UIntPtr]::Zero)
}

function Save-WindowScreenshot {
    param(
        [TinaluxWindowAutomation+RECT]$Rect,
        [string]$Path
    )

    $directory = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Force -Path $directory | Out-Null
    }

    $width = [Math]::Max(1, $Rect.Right - $Rect.Left)
    $height = [Math]::Max(1, $Rect.Bottom - $Rect.Top)
    $bitmap = New-Object System.Drawing.Bitmap($width, $height)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($Rect.Left, $Rect.Top, 0, 0, $bitmap.Size)
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

$resolvedExePath = [System.IO.Path]::GetFullPath($ExePath)
$processName = [System.IO.Path]::GetFileNameWithoutExtension($resolvedExePath)
$process = Get-Process -Name $processName -ErrorAction SilentlyContinue |
    Where-Object { $_.MainWindowHandle -ne 0 } |
    Select-Object -First 1
$startedProcess = $null

if ($null -eq $process) {
    if (-not (Test-Path $resolvedExePath)) {
        throw "Executable not found: $resolvedExePath"
    }

    $startedProcess = Start-Process -FilePath $resolvedExePath -PassThru
    $process = $startedProcess
}

try {
    $deadline = (Get-Date).AddMilliseconds($StartupTimeoutMs)
    $targetWindow = $null
    while ((Get-Date) -lt $deadline) {
        $process.Refresh()
        $windows = Get-ProcessWindows -ProcessId $process.Id
        $targetWindow = Select-TargetWindow -Windows $windows -PreferredTitle $PreferredWindowTitle
        if ($null -ne $targetWindow) {
            break
        }
        Start-Sleep -Milliseconds 200
    }

    if ($null -eq $targetWindow) {
        throw "Could not acquire the Tinalux GUI window."
    }

    if ($WindowWidth -gt 0 -and $WindowHeight -gt 0) {
        [void][TinaluxWindowAutomation]::MoveWindow(
            $targetWindow.Handle,
            $WindowX,
            $WindowY,
            $WindowWidth,
            $WindowHeight,
            $true)
        Start-Sleep -Milliseconds 200
        $windows = Get-ProcessWindows -ProcessId $process.Id
        $updatedWindow = Select-TargetWindow -Windows $windows -PreferredTitle $PreferredWindowTitle
        if ($null -ne $updatedWindow) {
            $targetWindow = $updatedWindow
        }
    }

    [TinaluxWindowAutomation]::SetForegroundWindow($targetWindow.Handle) | Out-Null
    Start-Sleep -Milliseconds $SettleMs

    $rect = Get-CaptureRect -Handle $targetWindow.Handle -ClientArea:$CaptureClientArea

    if (-not [string]::IsNullOrWhiteSpace($ResetCursorRatio)) {
        $resetPoint = Resolve-WindowPoint -Rect $rect -RatioSpec $ResetCursorRatio
        [TinaluxWindowAutomation]::SetCursorPos($resetPoint.X, $resetPoint.Y) | Out-Null
        Start-Sleep -Milliseconds $StepDelayMs
    }

    foreach ($ratio in $MoveRatios) {
        $point = Resolve-WindowPoint -Rect $rect -RatioSpec $ratio
        [TinaluxWindowAutomation]::SetCursorPos($point.X, $point.Y) | Out-Null
        Start-Sleep -Milliseconds $StepDelayMs
    }

    foreach ($ratio in $ClickRatios) {
        $point = Resolve-WindowPoint -Rect $rect -RatioSpec $ratio
        Invoke-LeftClick -X $point.X -Y $point.Y
        Start-Sleep -Milliseconds $StepDelayMs
    }

    $resolvedOutputPath = [System.IO.Path]::GetFullPath($OutputPath)
    Save-WindowScreenshot -Rect $rect -Path $resolvedOutputPath
    Write-Output $resolvedOutputPath
} finally {
    if (-not $KeepOpen -and $null -ne $startedProcess -and -not $startedProcess.HasExited) {
        $startedProcess.CloseMainWindow() | Out-Null
        Start-Sleep -Milliseconds 500
        if (-not $startedProcess.HasExited) {
            Stop-Process -Id $startedProcess.Id -Force
        }
    }
}
