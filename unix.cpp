#include <QFile>
#include <QFileInfo>
#include "projectdefinition.h"
#include "basestuff.h"
#include "print.h"

void    generateUnixInstallFile(const ProjectDefinition& project)
{
    println("Creating Unix Install file");
    QMap<QString, QString> mapping;

    mapping["SQPACKAGER_VERSION"] = "0.1";
    mapping["PROJECT_TARGET"] = project.targetName;
    mapping["DESKTOP_FILE"] = project.desktopFile;
    mapping["NORMALIZED_DESKTOP_FILE_NAME"] = project.desktopFileNormalizedName;
    mapping["DEBIAN_PACKAGE_NAME"] = project.debianPackageName;
    QFileInfo fi(project.desktopIconNormalizedName);
    mapping["NORMALIZED_PROJECT_ICON_NAME"] = fi.completeBaseName();
    mapping["NORMALIZED_PROJECT_ICON_PATH"] = project.desktopIconNormalizedName;
    mapping["PROJECT_ICON_FILE"] = project.icon;
    mapping["ICON_SIZE"] = QString("%1x%2").arg(project.iconSize.width()).arg(project.iconSize.height());
    QFile unixInstallFile(project.basePath + "/sqpackager_unix_installer.sh");
    if (!unixInstallFile.open(QIODevice::Text | QIODevice::WriteOnly))
    {
        error_and_exit("Could not create the sqpackager_unix_installer.sh file : " + unixInstallFile.errorString());
    }
    QString fileString = useTemplateFile(":/unix_install.tt", mapping);
    unixInstallFile.write(fileString.toLocal8Bit());
    unixInstallFile.close();
}
