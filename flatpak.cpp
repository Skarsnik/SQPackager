#include <QFile>
#include <QMap>
#include <projectdefinition.h>
#include <basestuff.h>
#include "print.h"


static QString kdeSdkVersion = "org.kde.Sdk/x86_64/5.15-22.08";
static QString plateformSdk = "org.kde.Platform/x86_64/5.15-22.08";

void    genFlatPakFile(const ProjectDefinition& project)
{
    QString fullName = project.org + "." + project.name;
    QMap<QString, QString> mapping;
    mapping["PROJECT_NAME"] = project.name;
    mapping["PROJECT_ORG"] = project.org;
    //TODO maybe run flatpak search? for Qt versions
    if (project.qtMajorVersion == "5")
    {
        mapping["KDE_SDK_VERSION"] = "5.15-22.08";
    }
    if (project.qtMajorVersion == "6" || project.qtMajorVersion == "auto")
    {
        kdeSdkVersion = "org.kde.Sdk/x86_64/6.4";
        plateformSdk = "org.kde.Plateform/x86_64/6.4";
        mapping["KDE_SDK_VERSION"] = "6.4";
    }
    mapping["PROJECT_PATH"] = project.basePath;
    mapping["FLATPACK_FILE_PERM"] = project.flatpakFilesystemPermission.isEmpty() ? "xdg-config" : project.flatpakFilesystemPermission;
    QFile manifest(project.basePath + "/" + fullName + ".yml");
    if (!manifest.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        error_and_exit("Could not open file <" + manifest.fileName() + "> to write flatpak manifest : " + manifest.errorString());
    }
    QString content = useTemplateFile(":/flatpak/flatpak_template.tt", mapping);
    manifest.write(content.toLocal8Bit());
    println("Flatpak file generated : " + manifest.fileName());
}

void    buildFlatPak(const ProjectDefinition& project)
{

}
