#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <print.h>
#include <runner.h>
#include <basestuff.h>
#include <sqpackager.h>
#include <QVersionNumber>

static QString getDebianVersion(const ProjectDefinition& proj);

void    generateDebianFiles(ProjectDefinition& proj)
{

    if (proj.debianMaintainer.isEmpty())
    {
        proj.debianMaintainer = proj.author;
    }
    if (proj.debianMaintainerMail.isEmpty())
    {
        proj.debianMaintainerMail = proj.authorMail;
    }
    QString packageName = proj.name.toLower();
    QString debianVersion = getDebianVersion(proj);
    debianVersion += "-1";
    QDir debDir(proj.basePath + "/debian");
    if (debDir.exists() == false)
    {
        println("Creating Debian directory in " + proj.basePath);
        debDir.cdUp();
        debDir.mkpath("debian");
        debDir.cd("debian");
    }
    println("Creating Changelog");
    Runner run(true);
    run.addEnv("DEBEMAIL", proj.debianMaintainerMail);
    run.addEnv("DEBFULNAME", proj.debianMaintainer);
    run.runWithOut("dch", QStringList() << "--create" << "-v" << debianVersion << "--package" << packageName << "Initial release generated with SQPackager", proj.basePath);
    // Changelog dch --create -v 1.0-1 --package hithere
    println("Creating compat file");
    QFile compatFile(proj.basePath + "/debian/compat");
    if (!compatFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        error_and_exit("Could not open debian/compat " + compatFile.errorString());
    }
    compatFile.write("10\n");
    compatFile.close();
    println("Creating source/format file");
    debDir.mkpath("source");
    QFile formatFile(proj.basePath + "/debian/source/format");
    if (!formatFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        error_and_exit("Could not open debian/source/format " + formatFile.errorString());
    }
    formatFile.write("3.0 (quilt)\n");
    formatFile.close();
    println("Creating rules file");
    QFile ruleFile(proj.basePath + "/debian/rules");
    if (!ruleFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        error_and_exit("Could not open debian/rules " + ruleFile.errorString());
    }
    QMap<QString, QString> map;
    map["QMAKE"] = "qmake";
    if (proj.qtMajorVersion == "6")
    {
        map["QMAKE"] = "qmake6";
    }
    if (proj.qtMajorVersion == "auto")
    {
        println("No Qt major version provided, detecting qmake executable");
        Runner testqmake;
        bool ok = testqmake.run("qmake6", QStringList() << "--version");
        if (ok)
        {
            map["QMAKE"] = "qmake6";
        } else {
            println("\tqmake6 executable not found, falling back to qmake");
        }
    }
    map["PROJECT_TARGET"] = proj.targetName;
    map["PACKAGE_NAME"] = packageName;
    map["ICON_SIZE"] = QString("%1x%2").arg(proj.iconSize.width()).arg(proj.iconSize.height());
    map["QMAKE_OPTIONS"] = "DEFINES+=\"SQPROJECT_LINUX_INSTALL\" DEFINES+=\"SQPROJECT_DEBIAN_BUILD\" DEFINES+=\"SQPROJECT_INSTALL_PREFIX=/usr/\" CONFIG+=\"release no_batch\"";
    map["DESKTOP_FILE"] = proj.desktopFile;
    map["PROJECT_ICON_FILE"] = proj.desktopIcon;
    map["PROJECT_ICON_NORMALIZED_NAME"] = proj.desktopIconNormalizedName;
    map["PROJECT_ICON_NORMALIZED_BASE_NAME"] = proj.org + "." + proj.name;
    QString rules = useTemplateFile(":/debian/rules_template.tt", map);
    ruleFile.write(rules.toLocal8Bit());
    ruleFile.close();
    println("Creating control file");
    QFile controlFile(proj.basePath + "/debian/control");
    if (!controlFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        error_and_exit("Could not open debian/rules" + controlFile.errorString());
    }
    map.clear();
    map["SOURCE_NAME"] = proj.name.toLower();
    map["PACKAGE_NAME"] = proj.name.toLower();
    map["MAINTAINER_NAME"] = proj.debianMaintainer;
    map["MAINTAINER_MAIL"] = proj.debianMaintainerMail;
    map["LONG_DESCRIPTION"] = proj.description;
    map["SHORT_DESCRIPTION"] = proj.shortDescription;
    QString control = useTemplateFile(":/debian/control_template.tt", map);
    controlFile.write(control.toLocal8Bit());
    controlFile.close();
    QFile copyrightFile(proj.basePath + "/debian/copyright");
    if (!copyrightFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        error_and_exit("Could not open debian/copyright" + copyrightFile.errorString());
    }
    map.clear();
    map["SOURCE_URL"] = "";
    map["PROJECT_NAME"] = proj.name;
    map["AUTHOR"] = proj.author;
    map["AUTHOR_MAIL"] = proj.authorMail;
    map["LICENSE_NAME"] = proj.licenseName;
    QString copyrightString = useTemplateFile(":/debian/copyright_template.tt", map);
    copyrightFile.write(copyrightString.toLocal8Bit());
    copyrightFile.close();
}

// TODO Check if the tools are here
bool    checkDebian(const ProjectDefinition& proj)
{
    Runner run;
    if (!run.run("dch", QStringList() << "--version"))
    {
        println("devscripts package not installed");
        return false;
    }
    if (proj.debianMaintainer.isEmpty())
    {
        if (proj.author.isEmpty())
        {
            println("You need to specify a debian maintainer name, etheir specify the <author> field or the <debian-maintainer> field");
            return false;
        }
    }
    if (proj.debianMaintainerMail.isEmpty())
    {
        if (proj.authorMail.isEmpty())
        {
            println("You need to specify a debian maintainer mail, etheir specify the <author-mail> field or the <debian-maintainer-mail> field");
            return false;
        }
    }
    if (proj.shortDescription.isEmpty())
    {
        println("Your project need a short description, please fill the <short-description> field");
        return false;
    }
    if (proj.description.isEmpty())
    {
        println("Your project need a long description, please fill the <description> field");
        return false;
    }
    if (proj.licenseName.isEmpty())
    {
        println("SQPackage could not find your License Name, debian package need it, please specify the <license-name> filed");
        return false;
    }
    return true;
}

void    buildDebian(const ProjectDefinition& project)
{
    // First we create the .orig.tar.gz
    QString projectBasePath = project.basePath;
    QString subDir = "";
    if (project.projectBasePath != project.basePath)
    {
        projectBasePath = project.projectBasePath;
        QString cpy = project.basePath;
        subDir = cpy.replace(project.projectBasePath, "");
    }
    QString debianVersion = getDebianVersion(project);
    QString debianNormalizedName = project.name.toLower() + "_" + debianVersion;
    Runner  run(true);
    println("Copying project files into another directory");
    QString tmpPath = "/tmp/" + debianNormalizedName;
    run.runWithOut("rm", QStringList() << "-rf" << tmpPath);
    run.runWithOut("cp", QStringList() << "-r" << projectBasePath << tmpPath);
    //debuild --no-tgz-check -us -uc -b
    println("Building the .deb package");
    run.runWithOut("debuild", QStringList() << "-us" << "-uc" << "-b", tmpPath + "/" + subDir);
}

QString getDebianVersion(const ProjectDefinition& proj)
{
    QString debVersion;
    QVersionNumber tmp = QVersionNumber::fromString(proj.version);
    if (tmp.isNull())
    {
        debVersion = "1-" + proj.version;
    } else {
        debVersion = proj.version;
    }
    return debVersion;
}
