#include "runner.h"
#include "print.h"

Runner::Runner(bool verbose, bool dummy)
{
    m_verbose = verbose;
    m_dummy = dummy;
}

bool Runner::run(QString command)
{
    return run(command, QStringList());
}


bool Runner::run(QString command, QStringList args)
{
    if (m_verbose)
        println(command + " " + args.join(" "));
    if (m_dummy)
        return true;
    m_process.start(command, args);
    bool finished = m_process.waitForFinished();
    if (finished)
        return m_process.exitCode() == 0;
    return false;
}

bool Runner::run(QString command, QString workingDir, QStringList args)
{
    QString oldWD = m_process.workingDirectory();
    //println("CWD :" + workingDir);
    if (m_verbose)
        println(command + " " + args.join(" "));
    if (m_dummy)
        return true;
    m_process.setWorkingDirectory(workingDir);
    m_process.start(command, args);
    //printlnYes("Started : ", m_process.waitForStarted());
    //println(m_process.errorString());
    bool finished = m_process.waitForFinished();
    //printlnYes("Finished : ", finished);
    //println(QString::number(m_process.exitCode()));
    m_process.setWorkingDirectory(oldWD);
    if (finished)
        return m_process.exitCode() == 0;
    return false;
}

bool Runner::runWithOut(QString command, QStringList args, QString workingDir)
{
    if (m_verbose)
        println(command + " " + args.join(" "));
    if (m_dummy)
        return true;
    QString oldWD = m_process.workingDirectory();
    m_process.setWorkingDirectory(workingDir);
    QProcess::ProcessChannelMode oldMode = m_process.processChannelMode();
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    m_process.setReadChannel(QProcess::StandardOutput);
    m_process.start(command, args);
    bool started = m_process.waitForStarted();
    if (!started)
        return false;
    m_process.waitForReadyRead();
    while (!m_process.atEnd())
    {
        println(m_process.readAll());
        m_process.waitForReadyRead();
    }
    m_process.setWorkingDirectory(oldWD);
    m_process.setProcessChannelMode(oldMode);
    return m_process.exitCode() == 0;
}

bool Runner::pathContains(QString toSearch)
{
    QString path = m_process.processEnvironment().value("PATH");
    return path.split(":").contains(toSearch);
}

void Runner::addPath(QString toAdd)
{
    QString path = m_process.processEnvironment().value("PATH");
    path.prepend(toAdd + ":");
    auto env = m_process.processEnvironment();
    env.insert("PATH", path);
    m_process.setProcessEnvironment(env);
}

QByteArray Runner::getStdout()
{
    return m_process.readAllStandardOutput();
}

QByteArray Runner::getStderr()
{
    return m_process.readAllStandardError();
}

QProcessEnvironment Runner::env() const
{
    return m_process.processEnvironment();
}

void Runner::addEnv(const QString &name, const QString &value)
{
    auto env = m_process.processEnvironment();
    env.insert(name, value);
    m_process.setProcessEnvironment(env);
}

void Runner::setEnv(QProcessEnvironment env)
{
    m_process.setProcessEnvironment(env);
}

Runner::Runner()
{
    m_verbose = false;
    m_dummy = false;
}
