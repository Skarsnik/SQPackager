#include <QCoreApplication>
#include <QString>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QCommandLineParser>
#include <QDateTime>

#include <runner.h>
#include <sqpackager.h>
#include <basestuff.h>
#include <desktoprc.h>

QTextStream cout(stdout);

PackagerOptions gOptions;


void    testTemplate();

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCommandLineParser parser;

    parser.addPositionalArgument("sqproject.json", "A path to a sqproject.json file");
    parser.addOptions({
                    {"version", "version", "Force the given version for the project"},
                    {"gen-flatpak", "Generate a flatpak manifest"},
                    {"gen-windows", "Check and generate Windows related stuff"},
                    {"gen-debian", "Check and generate Debian files"},
                    {"build", "type", "Build the selected type"},
                    {"prepare", "type", "Prepare the system to be able to build the type"},
                    {"windows-build-path", "path", "Set the base directory where compilation takes place"},
                    {"windows-deploy-path", "path", "Set the base directory where deployement takes place"},
                    {"gen-desktop", "Generate a .desktop file"},
                    {"gen-unix", "Generate a .desktop file and an unix installer"}
                      });
    //return a.exec();
    parser.process(a);
    //testTemplate();
    ProjectDefinition project;
    if (parser.positionalArguments().isEmpty())
    {
        project = getProjectDescription();
    } else {
        project = getProjectDescription(parser.positionalArguments().at(0));
    }
    extractInfosFromProFile(project);
    findLicense(project);
    findReadme(project);
    if (parser.isSet("version"))
    {
        project.version.type = VersionType::Forced;
        project.version.forcedVersion = parser.value("version");
    }
    else
    {
        findVersion(project);
    }

    // Windows Stuff
    if (parser.isSet("windows-build-path"))
        gOptions.windowsBuildPath = parser.value("windows-build-path");
    if (parser.isSet("windows-deploy-path"))
        gOptions.windowsDeployPath = parser.value("windows-deploy-path");
    if (parser.isSet("gen-windows"))
        genWindows(project);
    if (parser.isSet("build") && parser.value("build") == "windows")
        buildWindows(project);

    if (parser.isSet("gen-desktop"))
    {
        if (checkDesktopRC(project, true))
            generateLinuxDesktopRC(project);
        else
            error_and_exit("The project description is not suited to generate a .desktop file. Please follow the previously error");
    }
    if (parser.isSet("gen-unix"))
    {
        if (checkDesktopRC(project, true))
            generateLinuxDesktopRC(project);
        else
            error_and_exit("The project description is not suited to generate a .desktop file. Please follow the previously error");
        setDesktopRC(project);
        generateUnixInstallFile(project);
        //exit(0);
    }

    // FlatPak stuff
    if (parser.isSet("gen-flatpak"))
    {
        if (!checkFlatPak(project, true))
        {
            error_and_exit("The project definition is not suited to generate a flatpak file");
        }
        setDesktopRC(project);
        generateFlatPakFile(project);
    }
    if (parser.isSet("build") && parser.value("build") == "flatpak")
    {
        if (!checkFlatPak(project))
        {
            error_and_exit("The project definition is not suited to generate a flatpak file");
        }
        setDesktopRC(project);
        if (project.flatpakFile.isEmpty())
            generateFlatPakFile(project);
        buildFlatPak(project);
    }
    // Debian
    if (parser.isSet("prepare") && parser.value("prepare") == "debian")
    {
        prepareDebian(project);
        exit(0);
    }
    if (parser.isSet("gen-debian"))
    {
        if (!checkDebian(project))
        {
            error_and_exit("The project definition is not suited to generate debian packaging files");
        }
        setDesktopRC(project);
        generateManPage(project);
        generateUnixInstallFile(project);
        generateDebianFiles(project);
    }
    if (parser.isSet("build") && parser.value("build") == "debian")
    {
        if (!checkDebian(project))
        {
            error_and_exit("The project definition is not suited to generate debian packaging files");
        }
        buildDebian(project);
    }
}

#include "print.h"

void    testTemplate()
{
    QMap<QString, QString> map;
    println("Testing without if");
    map["test1"] = "Fifou";
    map["test2"] = "Piko";
    map["test3"] = "Nyo";
    QString withoutIf = useTemplateFile(":/testtemplate.tt", map);
    println(withoutIf);
    println("==== WITH IF ====");
    map["testif"];
    QString withIf = useTemplateFile(":/testtemplate.tt", map);
    println(withIf);
    exit(0);
}
