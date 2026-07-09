#pragma once
#include <QString>
#include <QStringList>
#include <functional>

namespace logitune {

/// Crash info captured by signal handler or set_terminate
struct CrashInfo {
    QString type;           // "SIGSEGV", "std::bad_alloc", etc.
    int signalNumber = 0;   // 0 if exception, not signal
    QStringList stackTrace; // demangled frames
    QStringList logTail;    // last N log lines before crash
    bool fromPreviousSession = false; // lock file recovery
};

class CrashHandler {
public:
    static CrashHandler &instance();

    /// Install signal handlers and set_terminate. Call once in main().
    void install();

    /// Create the lock file. Call after app starts.
    void createLockFile();

    /// Remove the lock file. Call on clean shutdown.
    void removeLockFile();

    /// Check if a previous session crashed (lock file exists).
    bool previousSessionCrashed() const;

    /// Get crash info for the previous session (from lock file + log).
    CrashInfo previousSessionCrashInfo() const;

    /// Set callback to show crash dialog. Called from signal handler context
    /// (for caught crashes) or from main() (for recovery).
    using CrashCallback = std::function<void(const CrashInfo &)>;
    void setCrashCallback(CrashCallback cb);

    /// Capture a backtrace right now (for use in set_terminate).
    static QStringList captureBacktrace(int maxFrames = 20);

private:
    CrashHandler() = default;
    static void signalHandler(int sig);
    static void terminateHandler();

    CrashCallback m_callback;
    QString lockFilePath() const;
};

} // namespace logitune
