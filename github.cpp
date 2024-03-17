#include <QProcessEnvironment>
#include <QFile>
#include <projectdefinition.h>
#include <basestuff.h>
#include <print.h>
#include <github.h>

bool    isGithubAction()
{
    return QProcessEnvironment::systemEnvironment().contains("GITHUB_OUTPUT");
}

void    addGithubOutput(QString key, QString value)
{
    QFile githubOutput(QProcessEnvironment::systemEnvironment().value("GITHUB_OUTPUT"));
    if (!githubOutput.open(QIODevice::Text | QIODevice::Append))
    {
        error_and_exit("Can't open GITHUB_OUPUT file : " + githubOutput.errorString());
    }
    githubOutput.write(QString("%1=%2").arg(key, value).toLatin1());
    println(QString("Added %1 to GITHUB_OUTPUT as : %2").arg(key, value));
}

