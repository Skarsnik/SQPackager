#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QFile>
#include <QVersionNumber>
#include <print.h>
#include <runner.h>
#include <basestuff.h>
#include <sqpackager.h>
#include <compile_defines.h>
#include <github.h>
#include <QThread>


const QMap<QString, QString> debianQt5ModulesName = {
    {"websockets", "libqt5websockets5-dev"},
    {"gamepad", "libqt5gamepad5-dev"},
    {"serialport", "libqt5serialport5-dev"},
};

const QMap<QString, QString> debianQt6ModulesName = {
    {"websockets", "libqt6websockets6-dev"},
    {"serialport", "libqt6serialport6-dev"}
};

const QStringList qt6ModulesInBase = {
    "core",
    "gui",
    "widgets",
    "network",
    "xml",
    "dbus",
    "sql",
    "concurrent"
};

const QStringList qt5ModulesInBase = {
    "core",
    "gui",
    "widgets",
    "network",
    "xml",
    "dbus",
    "sql",
    "concurrent"
};

static QString getDebianVersion(const ProjectDefinition& proj);
static QStringList getModulesList(const ProjectDefinition& project);

QString qmakeExecutable = "qmake";
static void setQMakeVersion(const ProjectDefinition& project);


void    prepareDebian(const ProjectDefinition& project)
{
    if (project.qtMajorVersion == QtMajorVersion::Auto|| project.qtMajorVersion == QtMajorVersion::Qt6)
        qmakeExecutable = "qmake6";
    QStringList projectDeps = getModulesList(project);
    Runner run(true);
    QStringList aptGetInstallOptions;
    aptGetInstallOptions << "--yes" << "install";

    println("Installing debian package creation tools");
    run.run("apt-get", QStringList() << aptGetInstallOptions << "build-essential" << "fakeroot" << "devscripts" << "debhelper");
    println("Installing qt base dev package");
    // lrelease is in a separate package, weird
    if (qmakeExecutable == "qmake6")
        run.run("apt-get", QStringList() << aptGetInstallOptions << "qt6-base-dev" << "qt6-l10n-tools" << "qt6-tools-dev-tools");
    else
        run.run("apt-get", QStringList() << aptGetInstallOptions << "qtbase5-dev" << "qttools5-dev-tools");
    if (projectDeps.isEmpty() == false)
    {
        println("Installing additionnal(s) Qt module(s)");
        run.run("apt-get", QStringList() << aptGetInstallOptions << projectDeps);
    }
}

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
    proj.debianPackageName = proj.name.toLower().replace(' ', '-');
    QString lreleaseExecutable = "/usr/lib/qt5/bin/lrelease";
    setQMakeVersion(proj);
    if (qmakeExecutable == "qmake6")
    {
        lreleaseExecutable = "/usr/lib/qt6/bin/lrelease";
    }

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
 // Changelog
    println("Creating Changelog");
    Runner run(true);
    run.addEnv("DEBEMAIL", proj.debianMaintainerMail);
    run.addEnv("DEBFULLNAME", proj.debianMaintainer);
    run.runWithOut("dch", QStringList() << "--create" << "-v" << debianVersion << "--package" << proj.debianPackageName << "Initial release generated with SQPackager", proj.basePath);
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

 // rules file
    println("Creating rules file");
    QFile ruleFile(proj.basePath + "/debian/rules");
    if (!ruleFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        error_and_exit("Could not open debian/rules " + ruleFile.errorString());
    }
    QMap<QString, QString> map;
    map["QMAKE"] = qmakeExecutable;
    map["LRELEASE"] = lreleaseExecutable;
    map["PACKAGE_NAME"] = proj.debianPackageName;
    QFileInfo fiPro(proj.proFile);
    map["PRO_FILE"] = fiPro.fileName();
    const QStringList qmake_defines = {CompileDefines::debian_install};
    QString defines_option;
    for (auto define : qmake_defines)
    {
        defines_option += "DEFINES+=\"" + define + "\" ";
    }
    //defines_option += "DEFINES+='" + CompileDefines::unix_install_prefix + "=\\\\\\\"/usr/\\\\\\\"' ";
    //defines_option += "DEFINES+='" + CompileDefines::unix_install_share_path + "=\\\\\\\"/usr/share/" + proj.unixNormalizedName + "\\\\\\\"' ";
    map["QMAKE_OPTIONS"] = defines_option +  " CONFIG+=\\'release\\'";
    if (proj.translationDir.isEmpty() == false)
    {
        map["HAS_TRANSLATIONS"] = "yes";
    }
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

 // control File
    map["SOURCE_NAME"] = proj.debianPackageName;
    map["PACKAGE_NAME"] = proj.debianPackageName;
    map["MAINTAINER_NAME"] = proj.debianMaintainer;
    map["MAINTAINER_MAIL"] = proj.debianMaintainerMail;
    map["SHORT_DESCRIPTION"] = proj.shortDescription;
    map["QT_BASE_DEV"] = "qtbase5-dev";
    if (qmakeExecutable == "qmake6")
    {
        map["QT_BASE_DEV"] = "qt6-base-dev";
    }
    QStringList modulesDepend = getModulesList(proj);
    //println(proj.qtModules.join(", "));
    if (modulesDepend.isEmpty() == false)
    {
        map["QT_MODULES"] = modulesDepend.join(", ");
    }
    QString longDescription;
    for (QString line : proj.description.split('\n'))
    {
        QString newLine = "";
        if (line.startsWith(' '))
            newLine = line;
        else
            newLine = line.prepend(' ');
        if (newLine.size() > 80)
        {
            QStringList listLines;
            unsigned int i = 0;
            while (newLine.size() > i * 79)
            {
                listLines.append(newLine.mid(i * 79, 79));
                i++;
            }
            newLine = listLines.join("\n ");
        }
        newLine.append("\n");
        longDescription.append(newLine);
    }
    println(longDescription);
    map["LONG_DESCRIPTION"] = longDescription;
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
    map["CURRENT_YEAR"] = QString::number(QDateTime::currentDateTime().date().year());
    map["TARGET_NAME"] = proj.targetName;
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
    QString projectBasePath = project.basePath;
    QString subDir = "";
    if (project.projectBasePath != project.basePath)
    {
        projectBasePath = project.projectBasePath;
        QString cpy = project.basePath;
        subDir = cpy.replace(project.projectBasePath, "");
    }
    QString debianVersion = getDebianVersion(project);
    QString debianNormalizedName = project.debianPackageName + "_" + debianVersion;
    Runner  run(true);
    println("Copying project files into another directory");
    QString tmpPath = "/tmp/" + debianNormalizedName;
    run.runWithOut("rm", QStringList() << "-rf" << tmpPath);
    run.runWithOut("cp", QStringList() << "-r" << projectBasePath << tmpPath);
    //debuild --no-tgz-check -us -uc -b
    //run.runWithOut("ls", QStringList() << "-l" << tmpPath);
    println("Building the .deb package");
    bool ok = run.runWithOut("debuild", QStringList() << "-us" << "-uc" << "-b", tmpPath + "/" + subDir);
    if (!ok)
    {
        error_and_exit("Failed to build the debian package");
    }
    if (isGithubAction())
    {
        QThread::sleep(1);
        println("Inside GitHub Action environement, adding the debian package as output");
        run.run("uname", QStringList() << "-m");
        QString arch = run.getStdout().trimmed();
        QString buildArch = "";
        if (arch == "x86_64")
            buildArch = "amd64";
        println("Finding the package version");
        //dpkg-parsechangelog -S version
        run.run("dpkg-parsechangelog", QStringList() << "-l" << tmpPath + "/debian/changelog" << "-S" << "version");
        QString packageVersion = run.getStdout().trimmed();
        addGithubOutput("spackager_" + buildArch + "_deb", QString("/tmp/%1_%2_%3.deb").arg(project.debianPackageName, packageVersion, buildArch));
    }
}

// [epoch:]upstream_version[-debian_revision]
QString getDebianVersion(const ProjectDefinition& proj)
{
    QString debVersion;
    QString version = proj.version.simpleVersion;
    if (proj.version.type == VersionType::Git && proj.version.gitTag.isEmpty())
    {
        return "1+git" + proj.version.gitVersionString;
    }
    QVersionNumber tmp = QVersionNumber::fromString(version);
    if (tmp.isNull())
    {
        if (version.startsWith('v'))
        {
            version.remove(0, 1);
            return version;
        }
        debVersion = "1-" + version;
    } else {
        debVersion = version;
    }
    return debVersion;
}

static void setQMakeVersion(const ProjectDefinition& project)
{
    println("Trying to detect Qt major version installed via qmake (or qmake6)");
    if (project.qtMajorVersion == QtMajorVersion::Qt6)
    {
        qmakeExecutable = "qmake6";
        return ;
    }
    if (project.qtMajorVersion == QtMajorVersion::Auto)
    {
        println("No Qt major version provided, detecting qmake executable");
        Runner testqmake;
        bool ok = testqmake.run("qmake6", QStringList() << "--version");
        if (ok)
        {
            qmakeExecutable = "qmake6";
        } else {
            println("\tqmake6 executable not found, falling back to qmake");
        }
    }
}

QStringList getModulesList(const ProjectDefinition& project)
{
    QStringList modulesDepend;
    const QMap<QString, QString>* debianModulesName = &debianQt5ModulesName;
    QStringList debianModulesInBase = qt5ModulesInBase;
    if (qmakeExecutable == "qmake6")
    {
        debianModulesName = &debianQt6ModulesName;
        debianModulesInBase = qt6ModulesInBase;
    }
    for (QString moduleName : project.qtModules)
    {
        if (debianModulesName->contains(moduleName))
        {
            modulesDepend.append((*debianModulesName)[moduleName]);
        } else {
            if (debianModulesInBase.contains(moduleName) == false)
                error_and_exit("SQPackager doesn't know the debian package corresponding to the module name : " + moduleName);
        }
    }
    return modulesDepend;
}
