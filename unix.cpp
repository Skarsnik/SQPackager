#include <QFile>
#include <QFileInfo>
#include "projectdefinition.h"
#include "basestuff.h"
#include "print.h"
#include "runner.h"

void    generateUnixInstallFile(const ProjectDefinition& project)
{
    println("Creating Unix Install file");
    QMap<QString, QString> mapping;

    mapping["SQPACKAGER_VERSION"] = "0.1";
    mapping["PROJECT_TARGET"] = project.targetName;
    mapping["APPLICATION_NAME"] = project.unixNormalizedName;
    mapping["DESKTOP_FILE"] = project.desktopFile;
    mapping["NORMALIZED_DESKTOP_FILE_NAME"] = project.desktopFileNormalizedName;
    mapping["DEBIAN_PACKAGE_NAME"] = project.debianPackageName;
    mapping["NORMALIZED_PROJECT_ICON_PATH"] = project.desktopIconNormalizedName;
    mapping["PROJECT_ICON_FILE"] = project.icon;
    mapping["ICON_SIZE"] = QString("%1x%2").arg(project.iconSize.width()).arg(project.iconSize.height());
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
                    mapping["RELEASE_FILES_STRING"] += "install_file \"" + releaseInfo.source + "\" \"" + releaseInfo.destination + "\"\n";
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
    mapping["VERSION"] = project.version;
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

void    createArchive(const ProjectDefinition& project)
{

}
