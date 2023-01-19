#include <QFile>
#include <QFileInfo>
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
    mapping["BINARY_NAME"] = project.name; // TODO check this
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

void    genBuildAndInstall(const ProjectDefinition& project)
{
    QMap<QString, QString> mapping;
    QString fullName = project.org + "." + project.name;
    mapping["PROJECT_FILE"] = project.proFile;
    mapping["PROJECT_TARGET"] = project.name;
    mapping["FLATPAK_TARGET"] = project.name;
    mapping["PROJECT_ICON_FILE"] = project.desktopIcon;
    QFileInfo fi(project.basePath + "/" + project.desktopIcon);
    mapping["FLATPAK_ICON_FILE"] = project.org + "." + project.name + "." + fi.suffix();
    mapping["DESKTOP_FILE"] = project.deskopFile;
    mapping["FLATPAK_DESKTOP_FILE"] = fullName + "." + "desktop";
    mapping["FLATPAK_ICON_BASENAME"] = fullName;
}

void    buildFlatPak(const ProjectDefinition& project)
{

}
