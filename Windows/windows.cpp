#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QVersionNumber>
#include <QTextStream>
#include <QCoreApplication>
#include <print.h>
#include <projectdefinition.h>
#include <sqpackager.h>
#include <basestuff.h>
#include <runner.h>
#include <QTemporaryDir>

enum WindowsArch {
    X86,
    X64,
    ARM64
};

const QMap<WindowsArch, QString> vsArchName =
{
    {X86, "amd64_x86"},
    {X64, "amd64"},
    {ARM64, "amd64_arm"}
};

const QMap<WindowsArch, QString> archToString =
{
    {X86, "x86"},
    {X64, "x64"},
    {ARM64, "arm64"}
};

struct QtVersion
{
    QVersionNumber  version;
    QString         path;
    quint32         vsVersion;
    WindowsArch     arch;
    QString         toString() const {
        QString archString = archToString[arch];
        return "Version: " + version.toString() + " - Located at : " + path + " MSVC " + QString::number(vsVersion) + " " + archString;
    }
};

struct MSVCVersion
{
    quint32         visualStudioVersion;
    quint32         visualStudioInstalled;
    QVersionNumber  msvcVersion;
    QString         vsPath;
    QString         msvcPath;
    QList<WindowsArch> archs;
};

struct WindowsStuff
{
    QString             sevenZipPath;
    QString             deployBasePath;
    QString             buildBasePath;
    QString             deployPath;
    QList<QtVersion>    qtVersions;
    QList<MSVCVersion>  msvcVersions;
    QString             innosetupPath;
    QtVersion           x64Qt;
    QtVersion           x86Qt;
    MSVCVersion         x64MSVC;
    MSVCVersion         x86MSVC;
    bool                findJom;
    QString             jomExe;
};

struct ReleaseFiles
{
    QString standaloneZip;
    QString standalone7Zip;
    QString innoSetup;
};

static QMap<WindowsArch, ReleaseFiles> releaseFiles;
static WindowsStuff stuff;

struct WindowsBuild
{
    WindowsBuild(const ProjectDefinition& proj, WindowsArch _arch, bool _standalone)
    {
        arch = _arch;
        archString = archToString[arch];
        standalone = _standalone;
        m_projectBasePath = proj.basePath;
        setPath();
        releaseNameFull = proj.name + "-" + proj.version + "-win32-" + archString;
        releaseBaseName = proj.name + "-" + proj.version;
    }
    WindowsArch arch;
    QString     archString;
    MSVCVersion msvc;
    QtVersion   qt;
    private:
    void    setPath()
    {
        deployDirName = QString() + archString + "_" + (standalone ? "standalone" : "installer");
        deployPath = "windows_deploy/" + deployDirName;
        buildPath = "windows_build/" + archString + "_" + (standalone ? "standalone" : "installer");
        deployBasePath = (stuff.deployBasePath.isEmpty() ? m_projectBasePath : stuff.deployBasePath);
        buildBasePath = (stuff.buildBasePath.isEmpty() ? m_projectBasePath : stuff.buildBasePath);
        deployFullPath = deployBasePath + "/" + deployPath;
        buildFullPath = buildBasePath + "/" + buildPath;
    }
    QString m_projectBasePath;
    public:
    void    setStandalone(bool _standalone)
    {
        standalone = _standalone;
        setPath();
    }
    bool        standalone;
    QString     deployBasePath;
    QString     buildBasePath;
    QString     deployPath;
    QString     buildPath;
    QString     deployFullPath;
    QString     buildFullPath;
    QString     deployDirName;
    QString     releaseBaseName;
    QString     releaseNameFull;
    QStringList executables;
    QString        toString() const
    {
        return QString("Qt: %1 - MSVC: %5 (%2) For %3 %4").arg(qt.version.toString(), msvc.msvcVersion.toString(), archString, standalone ? "Standalone" : "Install").arg(msvc.visualStudioVersion);
    }
};

struct s_ValidWindowsBuilds {
    WindowsBuild*    x86Build = nullptr;
    WindowsBuild*    x64Build = nullptr;
    WindowsBuild*    arm64Build = nullptr;
};

struct s_ValidWindowsBuilds validWindowsBuilds;

static QString findFile(QString path, QRegularExpression exp)
{
    QDir dir(path);
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    for (auto fileInfo : dir.entryInfoList())
    {
        if (exp.match(fileInfo.fileName()).hasMatch())
            return fileInfo.fileName();
    }
    return QString();
}

static bool lineStart(QString data, QString keyword)
{
    QRegularExpression e("\n\\s+" + keyword);
    return e.match(data).hasMatch();
}

static QString rcLine = "IDI_ICON1               ICON    DISCARDABLE     \"%1.ico\"\n";

void    genWindows(ProjectDefinition& project)
{
    QFile f(project.proFile);
    print("Checking for RC_FILE entry in .pro ");
    if (!f.open(QIODevice::Text | QIODevice::ReadOnly))
    {
        printlnYes("", false);
        error_and_exit("Can't open .pro file " + f.errorString());
    }
    QString proData = f.readAll();
    bool hasRC = lineStart(proData, "RC_FILE");
    f.close();
    printlnYes("", hasRC);
    if (!hasRC)
    {
        QString rcFileName = findFile(project.basePath, QRegularExpression("\\.rc$"));
        println("A .rc file is needed by Windows to give an icon to your executable");
        if (!rcFileName.isEmpty())
        {
            println("Found a rc file <" + rcFileName + ">");
        } else {
            rcFileName = project.name + ".rc";
            print("\tCreating a a rc file " + rcFileName);
            QFile rcFile(project.basePath + "/" + rcFileName);
            if (!rcFile.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                printlnOk("", false);
                error_and_exit("Can't create rc file " + rcFile.errorString());
            }
            rcFile.write(rcLine.arg(project.name).toLocal8Bit());
            rcFile.close();
            printlnOk("", true);
            println("RC file <" + rcFileName + "> created. expected icon name is " + project.name + ".ico.");
        }
        println("Don't forget to add :\n\t'RC_FILE = " + rcFileName + "' in your .pro file");
    }
    print("Checking for VERSION in the .pro file");
    if (!lineStart(proData, "VERSION"))
    {
        printlnOk("", false);
        println("You should add a VERSION entry in the .pro file, this is used to add a version to the executable that will appear on its property");
    } else {
        printlnOk("", true);
    }
}

static bool copy_file(QString src, QString dest)
{

}

static void checkJom();
static void findQtVersion();
static QMap<WindowsArch, WindowsBuild *> pickQtVersion(const ProjectDefinition& project);
static void checkMSVCVersion();
static void buildProject(Runner& runner, const ProjectDefinition& project, WindowsBuild buildInfo);
static void setMSVCEnv(Runner& runner, MSVCVersion& vers, WindowsArch arch);
static void find7zip();
static void findInnoSetup();
static void deployExtras(ProjectDefinition& project, const WindowsBuild& build);
static void deployQt(Runner& runner, const ProjectDefinition& project, WindowsBuild& build);
static void createRelease(const ProjectDefinition& proj, const WindowsBuild& build);
static void generateInstaller(const ProjectDefinition& project, const WindowsBuild& build);
static void buildLocalization(Runner& runner, const ProjectDefinition& project, WindowsBuild& buildInfo);


extern PackagerOptions gOptions;

void    buildWindows(ProjectDefinition& project)
{
    Runner      runner(true);

    stuff.deployBasePath = gOptions.windowsDeployPath;
    stuff.buildBasePath = gOptions.windowsBuildPath;

    //runner.setEnv(QProcessEnvironment::systemEnvironment());
    println("===== Building for Windows =====");
    findQtVersion();
    checkMSVCVersion();
    auto pickedBuild = pickQtVersion(project);

    find7zip();
    findInnoSetup();

    QList<WindowsBuild> builds;

    QMapIterator<WindowsArch, WindowsBuild*> it(pickedBuild);
    println(QString("Found %1 usable build").arg(pickedBuild.size()));
    while (it.hasNext())
    {
        it.next();
        WindowsBuild build = *it.value();
        println("The following build with be used : " + build.toString());
        build.setStandalone(false);
        builds << build;
        build.setStandalone(true);
        builds << build;
    }
    for (WindowsBuild& build : builds)
    {
        if (build.arch == ARM64)
            break;
        //println(build.toString());
        Runner runnerBuild(true);
        buildProject(runnerBuild, project, build);
        deployExtras(project, build);
        deployQt(runnerBuild, project, build);
        if (project.translationDir.isEmpty() == false)
            buildLocalization(runnerBuild, project, build);
        if (build.standalone == false)
        {
            generateInstaller(project, build);
        } else {
            createRelease(project, build);
        }
    }
    println("Release files are the following");
    for (auto plop : releaseFiles)
    {
        println("Standalone zip  :" + QDir::toNativeSeparators(plop.standaloneZip));
        println("Standalone 7zip :" + QDir::toNativeSeparators(plop.standalone7Zip));
        println("Inno Setup file :" + QDir::toNativeSeparators(plop.innoSetup));
    }
    if (QProcessEnvironment::systemEnvironment().contains("GITHUB_OUTPUT"))
    {
        QFile githubOutput(QProcessEnvironment::systemEnvironment().value("GITHUB_OUTPUT"));
        if (githubOutput.open(QIODevice::Text | QIODevice::Append))
        {
            for (WindowsArch arch : releaseFiles.keys())
            {
                ReleaseFiles& plop = releaseFiles[arch];
                QString nativeFilePath = QDir::toNativeSeparators(plop.standaloneZip);
                githubOutput.write(QString("sqpackager_win32_" + archToString[arch] + "_standalone_zip=" + nativeFilePath + "\n").toLatin1());
                if (plop.standalone7Zip.isEmpty() == false)
                {
                    nativeFilePath = QDir::toNativeSeparators(plop.standalone7Zip);
                    githubOutput.write(QString("sqpackager_win32_" + archToString[arch] + "_standalone_7zip=" + nativeFilePath + "\n").toLatin1());
                }
                if (plop.innoSetup.isEmpty() == false)
                {
                    nativeFilePath = QDir::toNativeSeparators(plop.innoSetup);
                    githubOutput.write(QString("sqpackager_win32_" + archToString[arch] + "_innosetup=" + nativeFilePath + "\n").toLatin1());
                }
            }
        }
        githubOutput.close();
    }
}

void    createRelease(const ProjectDefinition& proj, const WindowsBuild& build)
{
    //tar.exe -a -cf Test.zip Test
    Runner runner(true);

    println("Creating standalone release archives");
    QDir deployDir(build.deployBasePath);
    deployDir.cd("windows_deploy");
    QString zipFileName = build.releaseNameFull + ".zip";
    QString zip7FileName = build.releaseNameFull + ".7z";
    QString dirToCompress = build.releaseBaseName;
    QFile::remove(build.deployBasePath + "/windows_deploy/" + zipFileName);
    QFile::remove(build.deployBasePath + "/windows_deploy/" + zip7FileName);
    printlnOk("Renaming deploy directory to " + dirToCompress, QFile::rename(build.deployFullPath, deployDir.absolutePath() + "/" + dirToCompress));
    println("Creating .zip file");
    bool ok = runner.run("tar.exe", deployDir.absolutePath(), QStringList() << "-a" << "-cf" << build.deployFullPath + "/../" +  zipFileName << dirToCompress);
    if (!ok)
    {
        QFile::rename(deployDir.absolutePath() + "/" + dirToCompress, build.deployFullPath);
        error_and_exit("Error trying to create the zip file");
    }
    releaseFiles[build.arch].standaloneZip = build.deployBasePath + "//windows_deploy/" + zipFileName;
    if (stuff.sevenZipPath.isEmpty() == false)
    {
        println("Creating 7zip file");
        ok = runner.run(stuff.sevenZipPath + "/7z.exe", deployDir.absolutePath(), QStringList() << "a" << "-t7z" << build.deployFullPath + "/../" + zip7FileName << dirToCompress);
        println(runner.getStdout());
        QFile::rename(deployDir.absolutePath() + "/" + dirToCompress, build.deployFullPath);
        if (!ok)
        {
            println(runner.getStderr());
            error_and_exit("Error trying to create the 7zip file");
        }
        releaseFiles[build.arch].standalone7Zip = build.deployBasePath + "/windows_deploy/" + zip7FileName;
    } else {
        QFile::rename(deployDir.absolutePath() + "/" + dirToCompress, build.deployFullPath);
    }
}

void    deployQt(Runner& runner, const ProjectDefinition& project, WindowsBuild& build)
{
    QStringList args;
    args << "--no-translations" << "--no-system-d3d-compiler" << "--no-opengl" << "--release";
    if (build.qt.version.majorVersion() == 5)
        args << "--no-webkit" << "--no-webkit2";
    QDir deployDir(build.deployFullPath);
    QString firstExe;
    for (const QFileInfo& fi : deployDir.entryInfoList())
    {
        if (fi.suffix() == "exe")
        {
            if (firstExe.isEmpty())
                firstExe = fi.fileName();
            build.executables << fi.fileName();
        }
    }
    runner.runWithOut(build.qt.path + "/bin/windeployqt.exe", args << firstExe, build.deployFullPath);

    // Qt deploy is cute but this need to go
    QFile::remove(build.deployFullPath + "/" + "opengl32sw.dll");
    QFile::remove(build.deployFullPath + "/" + "libEGL.dll");
    QFile::remove(build.deployFullPath + "/" + "libGLESV2.dll");
    QFile::remove(build.deployFullPath + "/" + "vc_redist.x64.exe");
    QFile::remove(build.deployFullPath + "/" + "vc_redist.x86.exe");
}

void    generateInstaller(const ProjectDefinition& project, const WindowsBuild& build)
{

    if (stuff.innosetupPath.isEmpty())
    {
        println("Inno Setup was not found, skipping generating Inno Setup installer");
        return ;
    }
    println("Generating Inno Setup Installer");
    QMap<QString, QString>   map;

    map["APP_NAME"] = project.name;
    map["APP_VERSION"] = project.version;
    map["APP_EXE"] = build.executables.at(0);
    map["LICENCE_FILE"] = QFileInfo(project.licenseFile).fileName();
    if (build.arch == X64)
    {
        map["ARCH_SPECIFIC"] = "ArchitecturesAllowed=x64\nArchitecturesInstallIn64BitMode=x64";
    } else {
        map["ARCH_SPECIFIC"] = "";
    }
    QDir deployDir(build.deployFullPath);
    println(deployDir.absolutePath());
    QFile fileList(build.deployFullPath + "/listforinnosetupwindows.txt");
    if (!fileList.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        error_and_exit("Can't open " + fileList.fileName() + " - " + fileList.errorString());
    }
    // FIXME, This need to be changed to a recurce function, fuck this
    for (auto file : deployDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot))
    {
        if (file.isFile())
        {
            fileList.write(QString("Source: \"%1\"; DestDir: \"{app}\"; flags:recursesubdirs\n").arg(file.fileName()).toLocal8Bit());
        } else {
            for (auto dumb : QDir(file.absoluteFilePath()).entryInfoList(QDir::Files))
            {
                fileList.write(QString("Source: \"%1\\%2\"; DestDir: \"{app}\\%1\"; flags:recursesubdirs\n").arg(file.fileName(), dumb.fileName()).toLocal8Bit());
            }
        }
    }
    /*fileList.write("[Dirs]\n");
    for (auto file : deployDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        println(file.fileName());
        fileList.write(QString("Source: \"%1\"; DestDir: \"{app}\"\n").arg(file.fileName()).toLocal8Bit());
    }*/
    fileList.close();
    QString innoFileStr = useTemplateFile(":/Windows/windows_innosetup.tt", map);
    QString issPath = build.deployFullPath + "/innosetup.iss";
    QFile innoFile(issPath);
    if (!innoFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        error_and_exit("Can't open " + innoFile.fileName() + " - " + innoFile.errorString());
    }
    innoFile.write(innoFileStr.toLocal8Bit());
    innoFile.close();
    Runner runner;
    runner.runWithOut(stuff.innosetupPath + "/ISCC.exe", QStringList() << "/O" + build.deployBasePath + "/windows_deploy/" << "/F" + build.releaseNameFull + "-setup" << issPath, build.deployFullPath);
    releaseFiles[build.arch].innoSetup = build.deployBasePath + "/windows_deploy/" + build.releaseNameFull + "-setup.exe";
}

/*
 * This automaticly deploy extras file like Readme or licence
 * it tries to be smart and copy like Readme.md to Readme.txt
 */

void    deployExtras(ProjectDefinition& project, const WindowsBuild& build)
{
    println("Checking and deploying extra files (Readme, License)");
    QDir projectDir(project.basePath);
    QDir deployDir(build.deployFullPath);
    bool hasReadme = !project.licenseFile.isEmpty();
    bool hasLicence = !project.readmeFile.isEmpty();
    //NOTE LICENCE AND LICENSE ARE VALID
    for (const QFileInfo& s : deployDir.entryInfoList())
    {
        if (!hasReadme && s.fileName().toLower() == "readme.txt")
            hasReadme = true;
        if (!hasLicence && (s.baseName().contains("license") || s.baseName().contains("licence")))
            hasLicence = true;
    }
    for (const QFileInfo& fi : projectDir.entryInfoList())
    {
        if (!hasReadme &&
                      (fi.fileName().toLower() == "readme.md"
                       || fi.fileName().toLower() == "readme.txt"
                      ))
        {
            println("Did not find a Readme file, copying <" + fi.fileName() + "> has Readme.txt");
            QFile::copy(fi.absoluteFilePath(), build.deployFullPath + "/Readme.txt");
            project.readmeFile = build.deployFullPath + "/Readme.txt";
        }
        if (!hasLicence && (fi.fileName().toLower().contains("license") || fi.fileName().toLower().contains("licence")))
        {
            QString optExt = fi.suffix().isEmpty() ? ".txt" : "";
            println("Did not find a License file, copying <" + fi.fileName() + "> to the deploy path");
            QFile::copy(fi.absoluteFilePath(), build.deployFullPath + "/" + fi.fileName() + optExt);
            project.licenseFile = build.deployFullPath + "/" + fi.fileName() + optExt;
        }
    }
    if (!project.releaseFiles.isEmpty())
        println("Deploying release files defined in the project");
    for (ReleaseFile file : project.releaseFiles)
    {
        bool ok;
        println("Deploying " + file.name + "- From : " + file.source + " - To : " + file.destination);
        QString fullDest = deployDir.absolutePath() + "/" + file.destination;
        if (file.type == Local)
        {
            ok = QFile::copy(projectDir.absolutePath() + "/" + file.source, fullDest);
            if (!ok)
            {
                error_and_exit("Could not copy one of the project file : " + file.name + " - " + file.source);
            }
        }
        if (file.type == Remote)
        {
            Runner run(true);
            run.runWithOut("curl.exe", QStringList() << "-LJ" << "-o" << fullDest << "--url" << file.source);
            if (QFileInfo::exists(fullDest) == false)
            {
                error_and_exit("Error getting the remote file " + file.name + "from " + file.source);
            }
        }
    }
}

void    buildLocalization(Runner& runner, const ProjectDefinition& project, WindowsBuild& buildInfo)
{
    QDir deployDir(buildInfo.deployFullPath);
    println("Building Localization files");
    deployDir.mkdir("i18n");
    runner.run("lrelease", QStringList() << project.proFile);
    println("Deploying Localization files");
    QDir translationDir(project.basePath + "/" + project.translationDir);
    for (auto file : translationDir.entryInfoList())
    {
        if (file.suffix() == "qm")
        {
            println("Copying " + file.fileName());
            QFile::copy(file.absoluteFilePath(), buildInfo.deployFullPath + "/i18n/" + file.fileName());
        }
    }
}

void    buildProject(Runner& runner, const ProjectDefinition& project, WindowsBuild buildInfo)
{
    runner.addPath(buildInfo.qt.path + "/bin");
    checkJom();
    setMSVCEnv(runner, buildInfo.msvc, buildInfo.arch);
    runner.run("cl.exe");
    println(runner.getStderr());

    QDir projDir(buildInfo.buildBasePath);
    QDir buildDir(buildInfo.buildFullPath);
    println("Preparing build and deploy directory");
    println("Targeted build path is " + buildDir.absolutePath());
    println("Targeted deploy path is " + buildInfo.buildFullPath);
    if (projDir.exists(buildInfo.buildPath))
    {
        bool ok = buildDir.removeRecursively();
        printlnOk("Emptying build directory", ok);
    }
    println("Creating build directory");
    projDir.mkpath(buildInfo.buildPath);
    bool ok = projDir.exists(buildInfo.buildPath) && buildDir.isEmpty(QDir::NoDotAndDotDot);
    printlnOk("Build path exists and is empty", ok);
    if (!ok)
    {
        error_and_exit("Failed to prepare build path");
    }
    projDir.setPath(buildInfo.deployBasePath);
    if (projDir.exists(buildInfo.deployPath))
    {
        println("Removing existing deploy dir " + projDir.absolutePath() + "/" + buildInfo.deployPath);
        QDir plop(projDir.absolutePath() + "/" + buildInfo.deployPath);
        plop.removeRecursively();
    }
    ok = !projDir.exists(buildInfo.deployPath);
    printlnOk("Deploy path must not exists", ok);
    if (!ok)
    {
        error_and_exit("Failed to prepare deploy path");
    }
    QString deployPath = buildInfo.deployFullPath;
    stuff.deployPath = deployPath;
    QString buildPath = buildInfo.buildFullPath;

    //set QMAKE_MSC_VER=1910 I think this is needed for Qt 5.9 or 5.11-12?

    println("Building project in " + buildPath);
    QStringList qmakeOptions;
    qmakeOptions << project.proFile << "-spec" << "win32-msvc" << "CONFIG+=release no_batch";
    QStringList sqprojectOptions;
    if (buildInfo.standalone)
    {
        sqprojectOptions << "DEFINES+=SQPROJECT_WIN32_STANDALONE 1";
        sqprojectOptions << "DEFINES+=SQPROJECT_STANDALONE 1";
    } else {
        sqprojectOptions << "DEFINES+=SQPROJECT_WIN32_INSTALL 1";
        sqprojectOptions << "DEFINES+=SQPROJECT_INSTALL 1";
    }
    if (!sqprojectOptions.isEmpty())
        qmakeOptions.append(sqprojectOptions);
    // QMake for arm64 is a .bat that call x64 exe
    QString qmakeExe = "/bin/qmake.exe";
    if (buildInfo.arch == ARM64)
        qmakeExe = "/bin/qmake.bat";
    ok = runner.run(buildInfo.qt.path + qmakeExe, buildPath, qmakeOptions);
    if (!ok)
    {
        println(runner.getStdout());
        println(runner.getStderr());
        error_and_exit("QMake failed to run");
    }
    println(runner.getStdout());
    // Jom allow to use all cpu core/thread count
    QString nmake = stuff.findJom ? stuff.jomExe : "nmake";
    ok = runner.runWithOut(nmake, QStringList(), buildPath);
    if (!ok)
    {
        error_and_exit("NMake run failed");
    }
    println("Deploying files in " + deployPath);
    QProcessEnvironment env = runner.env();
    env.insert("INSTALL_ROOT", deployPath);
    runner.setEnv(env);
    runner.runWithOut("nmake", QStringList() << "install", buildPath);

    /*
     * It's very likely that the .pro file does not set stuff for the target
     * and nmake install does nothing, so copy the exe(s) we found
    */
    if (!QFileInfo::exists(deployPath))
    {
        println("Deploy Path is empty. no install set in the project file. Copying executable manually");
        QDir deployDir(deployPath);
        QDir buildDir(buildPath + "/release/");
        deployDir.mkpath(deployPath);
        for (const QFileInfo& fi : buildDir.entryInfoList())
        {
            //println(fi.fileName() + " : " + QString::number(fi.isExecutable()));
            if (fi.isExecutable() && fi.completeSuffix() == "exe")
            {
                println("Found <" + fi.fileName() + "> to copy in the deploy directory");
                println(deployDir.absolutePath());
                QFile::copy(fi.absoluteFilePath(), deployDir.absolutePath() + "/" + fi.fileName());
            }
        }
    }
}

/*
 * This try to find a Qt version
 * first calling qmake so it points to a current working installation
 * or looking through the default Qt installation
*/

void findQtVersion()
{
    /*Runner      runner;

    println("Searching for QMake");
    print("Seeing if qmake is in path");
    bool ok = runner.run("qmake", QStringList() << "--version");
    printlnYes("", ok);*/
    if (true)
    {
        print("Trying to find QMake in Qt installation");
        if ((QFileInfo::exists("C:\\Qt")
             || QProcessEnvironment::systemEnvironment().contains("Qt6_DIR")
             || QProcessEnvironment::systemEnvironment().contains("Qt5_DIR")) == false)
        {
            printlnOk("", false);
            error_and_exit("Can't find Qt installation (searching in C:\\Qt or QT_ROOT_DIR). You can use the option -qmake-path to specify the qmake executable to use");
        }
        printlnOk("", true);
        QDir qtDir;
        if (QFileInfo::exists("C:\\Qt"))
            qtDir = QDir("C:\\Qt");
        else
        {
            if (QProcessEnvironment::systemEnvironment().contains("Qt6_DIR"))
            {
                qtDir = QDir(QProcessEnvironment::systemEnvironment().value("Qt6_DIR") + "/../../");
            }
            if (QProcessEnvironment::systemEnvironment().contains("Qt5_DIR"))
            {
                qtDir = QDir(QProcessEnvironment::systemEnvironment().value("Qt5_DIR") + "/../../");
            }
        }
        qtDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
        const QRegularExpression RegVers("\\d\\.\\d");
        auto qtFiles = qtDir.entryInfoList();
        for (auto qtFile : qtFiles)
        {
            if (RegVers.match(qtFile.fileName()).hasMatch())
            {
                qtDir.cd(qtFile.fileName());
                println(qtFile.fileName());
                for (auto plop : qtDir.entryInfoList())
                {
                    println(plop.fileName());
                    QtVersion newVer;
                    newVer.version = QVersionNumber::fromString(qtFile.fileName());
                    newVer.path = plop.absoluteFilePath();
                    newVer.vsVersion = 0;
                    if (plop.fileName().contains("msvc"))
                    {
                        newVer.vsVersion = plop.fileName().mid(4, 4).toUInt();
                    }
                    newVer.arch = X86;
                    if (plop.fileName().endsWith("_64"))
                        newVer.arch = X64;
                    if (plop.fileName().endsWith("_arm64"))
                        newVer.arch = ARM64;
                    stuff.qtVersions << newVer;
                }
                qtDir.cdUp();
            }
        }
    }
    /*else {
        QTextStream qmakeOut(runner.getStdout());
        qmakeOut.readLine();
        QRegularExpression infoQtReg("^Using Qt version ([\\d.]+) in (.+)");
        QString qtInfo = qmakeOut.readLine();
        auto match = infoQtReg.match(qtInfo);
        QtVersion plop;
        if (match.hasMatch())
        {
            plop.version = QVersionNumber::fromString(match.captured(1));
            plop.path = match.captured(2).replace(QRegExp("\\lib$"), "");
            plop.x64 = plop.path.contains("_64");
            if (plop.path.contains("msvc"))
            {
                plop.vsVersion = plop.path.mid(plop.path.indexOf("msvc") + 4, 4).toUInt();
            }
        }
        if (!plop.path.contains("msvc"))
        {
            error_and_exit("This tool only support building with MSVC");
        }
        if (plop.x64)
            stuff.x64Qt = plop;
        else
            stuff.x86Qt = plop;
        stuff.qtVersions.append(plop);
    }*/
}

static QMap<WindowsArch, WindowsBuild*> pickQtVersion(const ProjectDefinition& project)
{
    bool qt5Only = project.qtMajorVersion == "qt5";
    bool qt6Only = project.qtMajorVersion == "qt6";
    QList<QtVersion> msvcQts;
    println("Number of Qt version found : " + QString::number(stuff.qtVersions.size()));
    for (QtVersion qtVer : stuff.qtVersions)
    {
        if (qtVer.vsVersion != 0)
            msvcQts << qtVer;
    }
    if (msvcQts.isEmpty())
    {
        error_and_exit("No version of Qt build for MSVC. This software does not support mingw build");
    }
    MSVCVersion pickedMscv;
    QMap<WindowsArch, WindowsBuild*>    winBuilds;
    QMap<WindowsArch, QtVersion>        maxQtVersions;
    for (QtVersion qtVer : msvcQts)
    {
        if ((qt5Only && qtVer.version.majorVersion() == 5) ||
            (qt6Only && qtVer.version.majorVersion() == 6) ||
             project.qtMajorVersion == "auto")
        {
            //println("Qt version : " + qtVer.toString());
            if (!maxQtVersions.contains(qtVer.arch) ||
                maxQtVersions.value(qtVer.arch).version < qtVer.version)
            {
                for (auto msVer : stuff.msvcVersions)
                {
                    if (qtVer.vsVersion <= msVer.visualStudioVersion
                        && msVer.archs.contains(qtVer.arch))
                    {
                        pickedMscv = msVer;
                        maxQtVersions[qtVer.arch] = qtVer;
                        if (! winBuilds.contains(qtVer.arch))
                        {
                            winBuilds[qtVer.arch] = new WindowsBuild(project, qtVer.arch, true);
                        }
                        winBuilds[qtVer.arch]->qt = qtVer;
                        winBuilds[qtVer.arch]->msvc = msVer;
                    }
                }
            }
        }
    }
    QMapIterator<WindowsArch, WindowsBuild*> it(winBuilds);
    while (it.hasNext())
    {
        it.next();
        println(QString("Picked Qt %1 with MSCV %4(%2) to build %3").arg(it.value()->qt.version.toString(), it.value()->msvc.msvcVersion.toString(), it.value()->archString).arg(it.value()->msvc.visualStudioVersion));
    }
    return winBuilds;
}


void    checkJom()
{
    stuff.findJom = QFileInfo::exists("C:/Qt/Tools/QtCreator/bin/jom/jom.exe");
    if (stuff.findJom)
    stuff.jomExe = "C:/Qt/Tools/QtCreator/bin/jom/jom.exe";
}

/*
Visual Studio 2017
14.1	1910 (versions 15.0 + 15.1 + 15.2)
14.11	1911 (version 15.3)
14.12	1912 (version 15.5)
14.13	1913 (Version 15.6)
14.14	1914 (version 15.7)
14.15	1915 (version 15.8)
14.16	1916 (version 15.9)
Visual Studio 2019
14.20	1920 (version 16.0)
14.21	1921 (version 16.1)
14.22	1922 (version 16.2)
14.23	1923 (version 16.3)
14.24	1924 (version 16.4)
14.25	1925 (version 16.5)
14.26	1926 (version 16.6)
14.27	1927 (version 16.7)
14.28	1928 (versions 16.8 + 16.9)
14.29	1929 (versions 16.10 + 16.11)
Visual Studio 2022
14.30	1930 (version 17.0)
14.31	1931 (version 17.1)
14.32	1932 (version 17.2)
14.33	1933 (version 17.3)
14.34	1934 (version 17.4)
 */


void    checkMSVCVersion()
{
    static const QRegularExpression msvcRegex("^(\\d+\\.\\d+\\.\\d+)");
    println("Searching for Visual Studio Installation");
    QDir recentVS("C:/Program Files/Microsoft Visual Studio");
    QDir oldVS("C:/Program Files (x86)/Microsoft Visual Studio");
    auto entries = recentVS.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    entries.append(oldVS.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot));
    for (auto msDir : entries)
    {
        if (msDir.fileName().contains("20"))
        {
            QString gVersion;
            if (QFile::exists(msDir.absoluteFilePath() + "/Community"))
            {
                gVersion = "Community";
            }
            if (QFile::exists(msDir.absoluteFilePath() + "/Enterprise"))
            {
                gVersion = "Enterprise";
            }

            QDir vcDir(msDir.absoluteFilePath() + "/" + gVersion + "/VC/Redist/MSVC");
            for (auto vcInfo : vcDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
            {
                auto match = msvcRegex.match(vcInfo.fileName());
                if (match.hasMatch())
                {
                    MSVCVersion newVers;
                    QDir msvcDir(vcInfo.absoluteFilePath());
                    newVers.visualStudioInstalled = msDir.fileName().toUInt();
                    newVers.msvcVersion = QVersionNumber::fromString(match.captured(1));
                    newVers.msvcPath = vcInfo.absoluteFilePath();
                    if (newVers.msvcVersion >= QVersionNumber(14,1))
                        newVers.visualStudioVersion = 2017;
                    if (newVers.msvcVersion >= QVersionNumber(14,20))
                        newVers.visualStudioVersion = 2019;
                    if (newVers.msvcVersion >= QVersionNumber(14,30))
                        newVers.visualStudioVersion = 2022;
                    if (msvcDir.exists("x86"))
                        newVers.archs << WindowsArch::X86;
                    if (msvcDir.exists("x64"))
                        newVers.archs << WindowsArch::X64;
                    if (msvcDir.exists("arm64"))
                        newVers.archs << WindowsArch::ARM64;
                    newVers.vsPath = msDir.absoluteFilePath() + "/" + gVersion + "/";
                    stuff.msvcVersions << newVers;
                }
            }
        }
    }
    for (auto vers : stuff.msvcVersions)
    {
        println(QString("Found MSVC Version %1 (VS %4) installed within Microsoft Visual Studio %2 in %3"
                        ).arg(vers.msvcVersion.toString()).arg(vers.visualStudioInstalled).arg(vers.vsPath).arg(vers.visualStudioVersion));
    }
}

void    find7zip()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString path = env.value("PATH");
    print("Trying to find Optionnal 7zip binaries");
    for (const QString& p : path.split(":"))
    {
        if (p.contains("7-Zip"))
        {
            stuff.sevenZipPath = p;
            break;
        }
    }
    if (stuff.sevenZipPath.isEmpty())
    {
        if (QFileInfo::exists("C:/Program Files/7-Zip"))
            stuff.sevenZipPath = "C:/Program Files/7-Zip";
    }
    if (stuff.sevenZipPath.isEmpty())
        printlnYes("", false);
    else
        printlnYes("", true);
}

void    findInnoSetup()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString path = env.value("PATH");
    print("Trying to find Optionnal Innosetup binaries");
    for (const QString& p : path.split(":"))
    {
        if (p.contains("Inno Setup"))
        {
            stuff.innosetupPath = p;
            break;
        }
    }
    if (stuff.innosetupPath.isEmpty())
    {
        if (QFileInfo::exists("C:/Program Files (x86)/Inno Setup 6"))
            stuff.innosetupPath = "C:/Program Files (x86)/Inno Setup 6";
    }
    if (stuff.innosetupPath.isEmpty())
        printlnYes("", false);
    else
        printlnYes("", true);
}
// 2015 14.0
// 2017 14.1
// 2019 14.2
// 2022 14.3

void setMSVCEnv(Runner& runner, MSVCVersion& vers, WindowsArch arch)
{
    println("----\n"
            "Setting up MSVC environnement");
    runner.setEnv(QProcessEnvironment::systemEnvironment());
    auto menv = runner.env();
    menv.insert("VS160COMNTOOLS", vers.vsPath + "/Common7/Tools/");
    runner.setEnv(menv);
    QString firstPath = runner.env().value("PATH");
    //println(runner.env().value("PATH"));
    //return ;
    QTemporaryDir   tempDir;
    if (!tempDir.isValid())
    {
        error_and_exit("Could not create a temporary directory to store a .bat file need to set MSVC env");
    }
    QString startvcarg = tempDir.filePath("startvcargs.bat");
    QFile::copy(":/Windows/startvcvargs.bat", startvcarg);
    bool ok = runner.run(startvcarg,
                         QStringList() << vers.vsPath + "Common7/Tools/VsDevCmd.bat" << "/clean_env");
    QTextStream out(runner.getStdout());
    while (!out.atEnd())
    {
        QString plop = out.readLine();
        //println(plop);
        if (plop.contains("&& SET"))
            break;
    }
    QProcessEnvironment env = runner.env();
    env.clear();
    while (!out.atEnd())
    {
        QString line = out.readLine();
        //println(line);
        if (line.contains("="))
        {
            QStringList plop = line.split('=');
            env.insert(plop.at(0), plop.mid(1).join(":"));
        }
    }
    //print(env.toStringList().join("\n"));
    runner.setEnv(env);
    //printlnOk("Path are the same", firstPath == env.value("PATH"));
    /*
     * Tool version can be different from msvc version IE :
     * Tool version
     * d-----        27/10/2022     22:50                14.33.31629
     * MSVC version
     * d-----        27/10/2022     22:50                14.32.31326
     *
     */

    QDir toolPath(vers.vsPath + "/VC/Tools/MSVC");
    QString toolVersion;
    for (auto dirName : toolPath.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        QVersionNumber toolVers = QVersionNumber::fromString(dirName);
        quint16 msV = 0;
        if (toolVers >= QVersionNumber(14,1))
            msV = 2017;
        if (toolVers >= QVersionNumber(14,20))
            msV = 2019;
        if (toolVers >= QVersionNumber(14,30))
            msV = 2022;
        if (msV == vers.visualStudioVersion)
            toolVersion = dirName;
    }
    ok = runner.run(startvcarg, QStringList() << vers.vsPath + "/VC/Auxiliary/Build/vcvarsall.bat "
                    << "-vcvars_ver=" + toolVersion << vsArchName.value(arch));
    if (!ok)
    {
        error_and_exit("Could not run the MSCV env script");
    }
    QTextStream out2(runner.getStdout());
    //QTextStream out(runner.getStdout());
    //println(runner.getStderr());
    while (!out2.atEnd())
    {
        QString plop = out2.readLine();
        //println(plop);
        if (plop.contains("&& SET"))
            break;
    }
    env = runner.env();
    //print(env.toStringList().join("\n"));
    while (!out2.atEnd())
    {
        QString line = out2.readLine();
        //println(line);
        if (line.contains("="))
        {
            QStringList plop = line.split('=');
            env.insert(plop.at(0), plop.mid(1).join(":"));
        }
    }
    //println(env.value("PATH"));
    //exit(1);
    runner.setEnv(env);
}
