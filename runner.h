#ifndef RUNNER_H
#define RUNNER_H

#include <QProcess>
#include <QString>
#include <QByteArray>

class Runner
{
public:
    Runner();
    Runner(bool verbose, bool dummy = false);
    bool        run(QString command);
    bool        run(QString command, QStringList args);
    bool        run(QString command, QString workingDir, QStringList args);
    bool        runWithOut(QString command, QStringList args, QString workingDir = "");

    bool        pathContains(QString path);
    void        addPath(QString path);
    QByteArray  getStdout();
    QByteArray  getStderr();
    QProcessEnvironment env() const;
    void        addEnv(const QString& name, const QString& value);
    void        setEnv(QProcessEnvironment env);

private:
    QProcess    m_process;
    QByteArray  m_stdout;
    QByteArray  m_stderr;
    bool        m_verbose;
    bool        m_dummy;
};

#endif // RUNNER_H
