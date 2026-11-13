#define NOMINMAX
#include <windows.h>
#include <conio.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

struct ImuSample
{
    float ax, ay, az;
    float gx, gy, gz;
};

std::string drawBoard(const ImuSample &sample)
{
    constexpr float radiansToDegrees = 57.2957795f;
    constexpr float minimumProjection = 0.30f; // m/s^2, about 1.8° near level
    constexpr int xRadius = 16;
    constexpr int yRadius = 8;

    const float x = sample.ax;
    const float y = sample.ay;
    const float z = sample.az;

    const float projection = std::hypot(x, y);
    const float magnitude = std::hypot(projection, z);
    const bool hasDirection = projection >= minimumProjection;

    // Direction in the board plane, measured counterclockwise from +X.
    const float directionRadians = std::atan2(y, x);
    float directionDegrees = directionRadians * radiansToDegrees;
    if (directionDegrees < 0.0f)
    {
        directionDegrees += 360.0f;
    }

    // Tilt is 0° with +Z up, 90° on edge, and 180° upside down.
    const float tiltDegrees = std::atan2(projection, z) * radiansToDegrees;
    const float rollDegrees = std::atan2(y, z) * radiansToDegrees;
    const float pitchDegrees =
        std::atan2(-x, std::hypot(y, z)) * radiansToDegrees;

    // A terminal cell is typically taller than it is wide.
    // Twice as many columns as rows makes the board look round.
    constexpr int width = 2 * xRadius + 1;
    constexpr int height = 2 * yRadius + 1;
    std::array<std::array<const char *, width>, height> board{};

    for (int row = 0; row < height; ++row)
    {
        for (int col = 0; col < width; ++col)
        {
            const float nx = static_cast<float>(col - xRadius) / xRadius;
            const float ny = static_cast<float>(yRadius - row) / yRadius;
            const float distance = std::hypot(nx, ny);

            const char *cell = " ";
            if (std::abs(distance - 1.0f) < 0.075f)
            {
                cell = "·";
            }
            else if (distance < 0.92f && row == yRadius)
            {
                cell = "─";
            }
            else if (distance < 0.92f && col == xRadius)
            {
                cell = "│";
            }
            board[row][col] = cell;
        }
    }

    board[yRadius][xRadius] = "●";

    if (hasDirection)
    {
        constexpr const char *arrows[] = {
            "→", "↗", "↑", "↖", "←", "↙", "↓", "↘"};
        constexpr float sectorAngle = 0.7853981634f;
        const int sector =
            static_cast<int>(std::lround(directionRadians / sectorAngle));

        // Scale the shaft by how much acceleration projects onto X/Y.
        const float length = std::clamp(
            0.80f * projection / magnitude, 0.18f, 0.80f);
        const float dx = std::cos(directionRadians) * length * xRadius;
        const float dy = std::sin(directionRadians) * length * yRadius;

        // Sample along the shaft so the arrow can point between the
        // eight directions available as individual arrow characters.
        constexpr int steps = 32;
        for (int step = 1; step < steps; ++step)
        {
            const float fraction = static_cast<float>(step) / steps;
            const int col = xRadius + static_cast<int>(std::lround(dx * fraction));
            const int row = yRadius - static_cast<int>(std::lround(dy * fraction));

            if (row != yRadius || col != xRadius)
            {
                board[row][col] = "•";
            }
        }

        const int tipCol = xRadius + static_cast<int>(std::lround(dx));
        const int tipRow = yRadius - static_cast<int>(std::lround(dy));
        board[tipRow][tipCol] = arrows[(sector + 8) % 8];
    }

    std::ostringstream out;
    out << std::string(xRadius + 3, ' ') << "+Y\x1b[K\n";
    for (int row = 0; row < height; ++row)
    {
        out << (row == yRadius ? "-X  " : "    ");
        for (const char *cell : board[row])
        {
            out << cell;
        }
        if (row == yRadius)
        {
            out << "  +X";
        }
        out << "\x1b[K\n";
    }
    out << std::string(xRadius + 3, ' ') << "-Y\x1b[K\n\n";

    out << std::fixed << std::setprecision(1);
    if (magnitude < 0.30f)
    {
        out << "Orientation unavailable: acceleration magnitude too low\x1b[K\n";
    }
    else
    {
        out << "Tilt from +Z: " << tiltDegrees << " deg";
        if (hasDirection)
        {
            out << "   Direction from +X: " << directionDegrees << " deg";
        }
        else
        {
            out << "   Direction: near vertical";
        }
        out << "\x1b[K\n";

        out << std::showpos << "Roll: " << rollDegrees << " deg"
            << "   Pitch: " << pitchDegrees << " deg"
            << std::noshowpos << "\x1b[K\n";
    }

    out << std::setprecision(2)
        << "Acceleration magnitude: " << magnitude << " m/s^2\x1b[K\n";
    return out.str();
}

std::string accelerationBar(float value)
{
    constexpr int width = 16;
    constexpr float fullScale = 2.0f * 9.80665f;
    const float fraction = std::min(std::abs(value) / fullScale, 1.0f);
    const int filled = static_cast<int>(std::lround(fraction * width));

    std::string left(width, ' ');
    std::string right(width, ' ');
    if (value < 0)
    {
        left.replace(width - filled, filled, filled, '#');
    }
    else
    {
        right.replace(0, filled, filled, '#');
    }

    std::ostringstream out;
    out << '[' << left << '|' << right << "] "
        << std::fixed << std::showpos << std::setprecision(2)
        << std::setw(7) << value << " m/s^2";
    return out.str();
}

void parseLine(const std::string &line, int selectedImu,
               std::optional<ImuSample> &imu,
               std::array<std::optional<float>, 3> &pressure,
               std::string &sdStatus)
{
    int id = 0;
    float temperature = 0;
    ImuSample sample{};

    if (std::sscanf(line.c_str(),
                    "[IMU%d] Temp: %f C | Accel m/s^2: [ %f , %f , %f ] "
                    "| Gyro rad/s: [ %f , %f , %f ]",
                    &id, &temperature, &sample.ax, &sample.ay, &sample.az,
                    &sample.gx, &sample.gy, &sample.gz) == 8)
    {
        if (id == selectedImu)
        {
            imu = sample;
        }
        return;
    }

    float hPa = 0;
    if (std::sscanf(line.c_str(), "[BMP %d] P: %f", &id, &hPa) == 2 &&
        id >= 1 && id <= 3)
    {
        pressure[id - 1] = hPa;
        return;
    }

    if (line.rfind("SD ", 0) == 0 || line.rfind("No SD ", 0) == 0)
    {
        sdStatus = line;
    }
}

void render(int selectedImu, const std::optional<ImuSample> &imu,
            const std::array<std::optional<float>, 3> &pressure,
            const std::string &sdStatus)
{
    std::ostringstream screen;
    screen << "\x1b[H"
           << "Teensy live sensors | IMU " << selectedImu << "\x1b[K\n"
           << "1-3: select IMU   q: close SD log   Esc: exit\x1b[K\n\n";

    if (imu)
    {
        screen << drawBoard(*imu)
               << "X " << accelerationBar(imu->ax) << "\x1b[K\n"
               << "Y " << accelerationBar(imu->ay) << "\x1b[K\n"
               << "Z " << accelerationBar(imu->az) << "\x1b[K\n"
               << std::fixed << std::setprecision(3) << std::showpos
               << "Gyro rad/s  X " << imu->gx << "  Y " << imu->gy
               << "  Z " << imu->gz << "\x1b[K\n"
               << std::noshowpos;
    }
    else
    {
        screen << "Waiting for IMU " << selectedImu << "...\x1b[K\n";
    }

    screen << '\n'
           << std::fixed << std::setprecision(2);
    for (std::size_t i = 0; i < pressure.size(); ++i)
    {
        screen << "BMP " << i + 1 << "  ";
        if (pressure[i])
        {
            screen << *pressure[i] << " hPa";
        }
        else
        {
            screen << "waiting";
        }
        screen << "\x1b[K\n";
    }

    screen << "\nSD: " << sdStatus << "\x1b[K\n\x1b[J";
    std::cout << screen.str() << std::flush;
}

int main(int argc, char *argv[])
{
    const std::string port = argc > 1 ? argv[1] : "COM3";
    const std::string devicePath = "\\\\.\\" + port;
    const HANDLE device = CreateFileA(devicePath.c_str(), GENERIC_READ | GENERIC_WRITE,
                                      0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (device == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Could not open " << port << " (Windows error " << GetLastError()
                  << "). Close the PlatformIO Serial Monitor and check the port.\n";
        return 1;
    }

    DCB config{};
    config.DCBlength = sizeof(config);
    if (!GetCommState(device, &config))
    {
        std::cerr << "Could not read the serial port settings.\n";
        CloseHandle(device);
        return 1;
    }

    config.BaudRate = CBR_115200;
    config.ByteSize = 8;
    config.Parity = NOPARITY;
    config.StopBits = ONESTOPBIT;
    config.fBinary = TRUE;
    config.fParity = FALSE;
    config.fDtrControl = DTR_CONTROL_ENABLE;
    config.fRtsControl = RTS_CONTROL_ENABLE;
    config.fOutxCtsFlow = FALSE;
    config.fOutxDsrFlow = FALSE;
    config.fDsrSensitivity = FALSE;
    config.fInX = FALSE;
    config.fOutX = FALSE;
    config.fAbortOnError = FALSE;

    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 40;
    timeouts.WriteTotalTimeoutConstant = 250;

    if (!SetCommState(device, &config) || !SetCommTimeouts(device, &timeouts))
    {
        std::cerr << "Could not configure " << port << ".\n";
        CloseHandle(device);
        return 1;
    }

    const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD originalMode = 0;
    if (!GetConsoleMode(console, &originalMode) ||
        !SetConsoleMode(console, originalMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
    {
        std::cerr << "Run the dashboard in a Windows terminal window.\n";
        CloseHandle(device);
        return 1;
    }

    const UINT originalCodePage = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);
    std::cout << "\x1b[2J\x1b[?25l";

    int selectedImu = 1;
    int exitCode = 0;
    std::optional<ImuSample> imu;
    std::array<std::optional<float>, 3> pressure{};
    std::string sdStatus = "Waiting for Teensy status";
    std::string line;
    auto nextDraw = std::chrono::steady_clock::now();

    while (true)
    {
        if (_kbhit())
        {
            const int key = _getch();
            if (key == 27)
            {
                break;
            }
            if (key >= '1' && key <= '3')
            {
                selectedImu = key - '0';
                imu.reset();
            }
            if (key == 'q')
            {
                const char command = 'q';
                DWORD sent = 0;
                sdStatus = WriteFile(device, &command, 1, &sent, nullptr) && sent == 1
                               ? "Sent q. Waiting for Teensy confirmation..."
                               : "Could not send q";
            }
        }

        char buffer[512];
        DWORD received = 0;
        if (!ReadFile(device, buffer, sizeof(buffer), &received, nullptr))
        {
            sdStatus = "Serial connection lost";
            render(selectedImu, imu, pressure, sdStatus);
            exitCode = 1;
            break;
        }

        for (DWORD i = 0; i < received; ++i)
        {
            if (buffer[i] == '\n')
            {
                parseLine(line, selectedImu, imu, pressure, sdStatus);
                line.clear();
            }
            else if (buffer[i] != '\r')
            {
                line += buffer[i];
            }
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= nextDraw)
        {
            render(selectedImu, imu, pressure, sdStatus);
            nextDraw = now + std::chrono::milliseconds(100);
        }
    }

    std::cout << "\x1b[?25h" << std::flush;
    SetConsoleMode(console, originalMode);
    SetConsoleOutputCP(originalCodePage);
    CloseHandle(device);
    return exitCode;
}