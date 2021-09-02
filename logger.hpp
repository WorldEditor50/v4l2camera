#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>

template <int N>
class LogWriter
{
public:
    enum Level {
        ALL = 0,
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        OFF
    };
    static QString logFileName;
    static bool on;
private:
    static QMutex mutex;
    static const QStringList levelString;
public:
    LogWriter()=default;
    static void write(int level, int lineNo, const QString &fileName, const QString &function, const QString &info)
    {
        if (on == false) {
            return;
        }
        QMutexLocker locker(&mutex);
        QFile file(logFileName);
        if (file.open(QIODevice::Append | QIODevice::WriteOnly) == false) {
            return;
        }

        QString content = QString("[%1][%2][FILE:%3][LINE:%4][FUNC:%5][MESSAGE:%6]")
                .arg(QDateTime::currentDateTime().toString())
                .arg(levelString[level])
                .arg(fileName)
                .arg(lineNo)
                .arg(function)
                .arg(info);
        file.write(content.toUtf8());
        file.close();
        return;
    }

    static void clear()
    {
        QMutexLocker locker(&mutex);
        QFile file(logFileName);
        if (file.open(QIODevice::WriteOnly) == false) {
            return;
        }
        file.resize(0);
        file.close();
        return;
    }
};
template<int N>
QString LogWriter<N>::logFileName = QString("%1.log").arg(QDateTime::currentDateTime().toString("yyyy-mm-dd hh:mm:ss"));
template<int N>
bool LogWriter<N>::on(true);
template<int N>
QMutex LogWriter<N>::mutex;
template<int N>
const QStringList LogWriter<N>::levelString = {"ALL", "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
using Log = LogWriter<0>;

#if 1
    #define LOG(level, message) Log::write(level, __LINE__, __FILE__, __FUNCTION__, message)
#else
    #define LOG
#endif
#endif // LOGGER_HPP
