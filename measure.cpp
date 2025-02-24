#ifdef _WIN32
#include <windows.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: measure.exe <target_program> [arguments...]" << std::endl;
        return 1;
    }

    std::ostringstream cmdLineStream;
    cmdLineStream << "\"" << argv[1] << "\"";
    for (int i = 2; i < argc; ++i) {
        cmdLineStream << " " << argv[i];
    }
    std::string cmdLineStr = cmdLineStream.str();
    std::vector<char> cmdLineBuffer(cmdLineStr.begin(), cmdLineStr.end());
    cmdLineBuffer.push_back('\0');

    // Создаём процесс
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (!CreateProcessA(
        NULL,
        cmdLineBuffer.data(),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi))
    {
        std::cerr << "CreateProcess failed (" << GetLastError() << ")." << std::endl;
        return 1;
    }

    // Ждём завершения процесса
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Получаем времена процесса
    FILETIME ftCreation, ftExit, ftKernel, ftUser;
    if (!GetProcessTimes(pi.hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
        std::cerr << "GetProcessTimes failed (" << GetLastError() << ")." << std::endl;
        return 1;
    }

    // Преобразуем FILETIME в секунды
    auto FileTimeToSeconds = [](const FILETIME &ft) {
        ULARGE_INTEGER li;
        li.LowPart = ft.dwLowDateTime;
        li.HighPart = ft.dwHighDateTime;
        // FILETIME имеет единицу равную 100 наносекундам, делим на 1e7, получаем секунды
        return static_cast<double>(li.QuadPart) / 1e7;
    };

    double creationTime = FileTimeToSeconds(ftCreation);
    double exitTime     = FileTimeToSeconds(ftExit);
    double kernelTime   = FileTimeToSeconds(ftKernel);
    double userTime     = FileTimeToSeconds(ftUser);

    double realTimeSec   = exitTime - creationTime;
    double userTimeSec   = userTime;
    double systemTimeSec = kernelTime;
    double waitTimeSec   = realTimeSec - (userTimeSec + systemTimeSec);


    std::cout << std::fixed << std::setprecision(2);
    std::cout << "- Real time: "   << realTimeSec   << " sec\n"
              << "- User time: "   << userTimeSec   << " sec\n"
              << "- System time: " << systemTimeSec << " sec\n"
              << "- Wait time: "   << waitTimeSec   << " sec\n";

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}
#endif