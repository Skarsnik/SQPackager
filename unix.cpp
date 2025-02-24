#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include "projectdefinition.h"
#include "basestuff.h"
#include "print.h"
#include "runner.h"
#include <compile_defines.h>

void    generateUnixInstallFile(const ProjectDefinition& project)
{
    println("Creating Unix Install file");
    QMap<QString, QString> mapping;

    mapping["SQPACKAGER_VERSION"] = "0.1";
    mapping["PRO_FILE"] = project.proFile;
    mapping["PROJECT_TARGET"] = project.targetName;
    mapping["APPLICATION_NAME"] = project.unixNormalizedName;
    mapping["DESKTOP_FILE"] = project.desktopFile;
    mapping["NORMALIZED_DESKTOP_FILE_NAME"] = project.desktopFileNormalizedName;
    mapping["DEBIAN_PACKAGE_NAME"] = project.debianPackageName;
    mapping["NORMALIZED_PROJECT_ICON_PATH"] = project.desktopIconNormalizedName;
    mapping["PROJECT_ICON_FILE"] = project.icon;
    mapping["ICON_SIZE"] = QString("%1x%2").arg(project.iconSize.width()).arg(project.iconSize.height());
    mapping["DEFINE_INSTALLED"] = CompileDefines::installed;
    mapping["DEFINE_INSTALL_PREFIX"] = CompileDefines::unix_install_prefix;
    mapping["DEFINE_APP_SHARE"] = CompileDefines::unix_install_share_path;
    mapping["DEFAULT_QMAKE_EXEC"] = "qmake6";
    if (project.qtMajorVersion == QtMajorVersion::Qt5)
        mapping["DEFAULT_QMAKE_EXEC"] = "qmake";
    if (project.readmeFile.isEmpty() == false)
    {
        mapping["HAS_README"] = "";
        mapping["README"] = project.readmeFile;
    }
    if (project.translationDir.isEmpty() == false)
    {
        mapping["HAS_TRANSLATIONS"] = "";
        mapping["TRANSLATION_DIR"] = project.translationDir;
    }
    QFile unixInstallFile(project.basePath + "/sqpackager_unix_installer.sh");
    if (!unixInstallFile.open(QIODevice::Text | QIODevice::WriteOnly))
    {
        error_and_exit("Could not create the sqpackager_unix_installer.sh file : " + unixInstallFile.errorString());
    }
    // Handling release files
    if (!project.releaseFiles.isEmpty())
    {
        mapping["HAS_RELEASE_FILES"] = "";
        for (auto releaseInfo : project.releaseFiles)
        {
            if (releaseInfo.type == ReleaseFileType::Local)
            {
                QFileInfo fi(project.basePath + "/" + releaseInfo.source);
                if (fi.isDir())
                {
                    mapping["RELEASE_FILES_STRING"] += "install_directory \"" + releaseInfo.source + "\"  \"" + releaseInfo.destination + "\"\n";
                } else {
                    QString perm = "644";
                    if (fi.isExecutable())
                        perm = "755";
                    mapping["RELEASE_FILES_STRING"] += "install_file \"" + releaseInfo.source + "\" \"" + releaseInfo.destination + "\" \"" + perm + "\"\n";
                }
            }
        }
    }

    QString fileString = useTemplateFile(":/unix_install.tt", mapping);
    unixInstallFile.write(fileString.toLocal8Bit());
    unixInstallFile.close();
    Runner run;
    QFileInfo fiSh(unixInstallFile);
    run.run("cmd", QStringList() << "+x" << fiSh.absoluteFilePath());
    println("\tFile sqpackager_unix_installer.sh created");
}

void    generateManPage(const ProjectDefinition& project)
{
    println("Creating manpage");

    QMap<QString, QString> mapping;
    mapping["TARGET_NAME"] = project.targetName;
    mapping["SHORT_DESCRIPTION"] = project.shortDescription;
    mapping["LONG_DESCRIPTION"] = project.description;
    mapping["AUTHOR"] = project.author;
    mapping["AUTHOR_MAIL"] = project.authorMail;
    mapping["VERSION"] = project.version.simpleVersion;
    mapping["DATE"] = QDateTime::currentDateTime().toString("dd MMM yyyy");

    QFile manPageFile(project.basePath + "/" + project.targetName + ".manpage.1");
    if (!manPageFile.open(QIODevice::Text | QIODevice::WriteOnly))
    {
        error_and_exit("Could not create the manpage.1 file" + manPageFile.errorString());
    }
    QString manString = useTemplateFile(":/manpage.tt", mapping);
    manPageFile.write(manString.toLocal8Bit());
    manPageFile.close();
    println("\tManpage " + project.targetName + ".manpage.1 file created");
}

QString    createArchive(const ProjectDefinition& project, QString version)
{
    Runner run(true);

    QStringList excludeList;
    QString versionString = project.version.simpleVersion;

    if (version.isEmpty() == false)
    {
        versionString = version;
    }

    QDir projectDir(project.basePath);
    if (projectDir.exists(".git") == true)
    {
        excludeList << "--exclude" << ".git*";
    }

    QFileInfo fi(project.basePath);

    QString archiveFile = fi.absoluteFilePath() + "/" + fi.baseName().toLower() + "-" + versionString + ".tar.gz";
    QFile newPri(fi.absoluteFilePath() + "/sq_project_forced_version.pri");
    if (newPri.open(QIODevice::WriteOnly) == false)
    {
        error_and_exit("Could not create the forced version pri file");
    }
    newPri.write(QByteArray("SQ_PROJECT_FORCED_VERSION = " + project.version.simpleVersion.toLocal8Bit() + "\n"));
    newPri.close();
    run.runWithOut("tar", QStringList() << "--transform" << "s,^," + fi.baseName().toLower() + "-" + versionString + "/," << excludeList << "--exclude-vcs" << "-zcf" << archiveFile << ".", fi.absoluteFilePath());
    run.run("rm", QStringList() << fi.absoluteFilePath() + "/sq_project_forced_version.pri");
    return archiveFile;
}
