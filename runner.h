#ifndef RUNNER_H
#define RUNNER_H

#include <QProcess>
#include <QString>
#include <QByteArray>

class Runner
{
public:
    Runner();
    Runner(bool verbose);
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
};

#endif // RUNNER_H
