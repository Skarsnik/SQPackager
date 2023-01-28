#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <projectdefinition.h>
#include <basestuff.h>
#include "print.h"
#include "runner.h"
#include <QDir>
#include <desktoprc.h>


static QString kdeSdkVersion = "org.kde.Sdk/x86_64/5.15-22.08";
static QString plateformSdk = "org.kde.Platform/x86_64/5.15-22.08";

static void    generateFlatPakBuildAndInstall(const ProjectDefinition& project);

bool    checkFlatPak(const ProjectDefinition project, bool bypass)
{
    const QString flatpakExpectFileName = project.basePath + "/" + project.org + "." + project.name;
    if (!bypass &&
         (!project.flatpakFile.isEmpty()
         || QFileInfo::exists(flatpakExpectFileName + ".yml")
         || QFileInfo::exists(flatpakExpectFileName + ".json")
         )
       )
    {
        println("flatpak description file found, use --gen-flatpak to force sqpackager to write one");
        return true;
    }
    println("Checking if the project can be build to flatpak");
    bool toret = true;
    if (project.org.isEmpty())
    {
        println("Your project need a org (eg: com.microsoft) name");
        toret = false;
    }
    if (!checkDesktopRC(project))
    {
        toret = false;
    }
    return toret;
}

void    generateFlatPakFile(ProjectDefinition& project)
{
    QString fullName = project.org + "." + project.name;
    QString projectBasePath = project.basePath;
    project.flatpakName = fullName;
    QMap<QString, QString> mapping;
    mapping["PROJECT_NAME"] = project.name;
    mapping["PROJECT_ORG"] = project.org;
    bool isSubDir = false;
    println(project.projectBasePath + " === " + project.basePath);
    if (project.projectBasePath != project.basePath)
    {
        isSubDir = true;
        projectBasePath = project.projectBasePath;
        mapping["SUBDIR"] = "subdir: " + project.basePath.mid(project.projectBasePath.size() + 1);
    }
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
    if (isSubDir)
        mapping["PROJECT_PATH"] = project.projectBasePath;
    else
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
    project.flatpakFile = manifest.fileName();
    generateFlatPakBuildAndInstall(project);
    manifest.close();
}

void    generateFlatPakBuildAndInstall(const ProjectDefinition& project)
{
    println("Generating Build and Install script for flatpak");
    QMap<QString, QString> mapping;
    QString fullName = project.flatpakName;
    mapping["PROJECT_FILE"] = QFileInfo(project.proFile).fileName();
    mapping["PROJECT_TARGET"] = project.targetName;
    mapping["FLATPAK_TARGET"] = project.targetName;
    mapping["PROJECT_ICON_FILE"] = project.desktopIcon;
    QFileInfo fi(project.basePath + "/" + project.desktopIcon);
    mapping["FLATPAK_ICON_FILENAME"] = project.org + "." + project.name + "." + fi.suffix();
    mapping["DESKTOP_FILE"] = project.desktopFile;
    mapping["FLATPAK_DESKTOP_FILE"] = fullName + "." + "desktop";
    mapping["FLATPAK_ICON_BASENAME"] = fullName;
    mapping["ICON_SIZE"] = QString("%1x%2").arg(project.iconSize.width()).arg(project.iconSize.height());
    QFile buildFile(project.basePath + "/flatpak_sqpackager_build.sh");
    if (!buildFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        error_and_exit("Could not open file <" + buildFile.fileName() + "> to write flatpak manifest : " + buildFile.errorString());
    }
    QString content = useTemplateFile(":/flatpak/sqpackager_build.tt", mapping);
    buildFile.write(content.toLocal8Bit());
    buildFile.close();
    println("flatpak_sqpackager_build.sh file genarated");
    content = useTemplateFile(":/flatpak/sqpackager_install.tt", mapping);
    QFile installFile(project.basePath + "/flatpak_sqpackager_install.sh");
    if (!installFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        error_and_exit("Could not open file <" + installFile.fileName() + "> to write flatpak manifest : " + installFile.errorString());
    }
    installFile.write(content.toLocal8Bit());
    installFile.close();
    println("flatpak_sqpackager_install.sh file genarated");
}

void    buildFlatPak(const ProjectDefinition& project)
{
    Runner run(true);
    QDir dir(project.basePath + "/flatpak-build-dir");
    if (!dir.exists()) {
        dir.mkdir(".");
    }
    bool result;
    result = run.runWithOut("flatpak-builder", QStringList() << "--force-clean" << "flat-build-dir/" << project.flatpakFile, project.basePath);
    if (!result)
        error_and_exit("Building flatpak file failed");
    result = run.runWithOut("flatpak", QStringList() << "build-export" << "flatpak-export" << "flat-build-dir/", project.basePath);
    if (!result)
        error_and_exit("Error with flatpak build-export");
    run.runWithOut("flatpak", QStringList() << "build-bundle" << "flatpak-export" << project.name + ".flatpak" << project.flatpakName, project.basePath);
}
