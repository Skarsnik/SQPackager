#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <print.h>
#include <runner.h>
#include <sqpackager.h>

void    genDebian(ProjectDefinition& proj)
{
    QDir debDir(proj.basePath + "/debian");
    if (debDir.exists() == false)
        debDir.mkdir(".");
    // Changelog dch --create -v 1.0-1 --package hithere
    QFile compatFile(proj.basePath + "/debian/compat");
    if (!compatFile.open(QIODevice::WriteOnly))
    {
        error_and_exit("Could not open debian/compat" + compatFile.errorString());
    }
    compatFile.write("10\n");
    compatFile.close();
    QDir::mkpath(projet.basePath + "/debian/source");
    QFile formatFile(proj.basePath + "/debian/source/format");
    if (!formatFile.open(QIODevice::WriteOnly))
    {
        error_and_exit("Could not open debian/source/format" + formatFile.errorString());
    }
    formatFile.write("3.0 (quilt)\n");
    formatFile.close();
    QFile ruleFile(proj.basePath + "/debian/rules");
    if (!ruleFile.open(QIODevice::WriteOnly))
    {
        error_and_exit("Could not open debian/rules" + formatFile.errorString());
    }
    QMap<QString, QString> map;
    QString rules = useTemplateFile(":/debian/rules_template.tt", map);

    ruleFile.close();


}

// Check if the tools are here
bool    checkDebian()
{
    return true;
}

void    buildDebian(ProjectDefinition& proj)
{
    // First we create the .orig.tar.gz

    Runner  run;


}
