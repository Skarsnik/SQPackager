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

QTextStream cout(stdout);

PackagerOptions gOptions;


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCommandLineParser parser;

    parser.addPositionalArgument("sqproject.json", "A path to a sqproject.json file");
    parser.addOptions({
                    {"version", "version", "Force the given version for the project"},
                    {"gen-flatpak", "Generate a flatpak manifest"},
                    {"gen-windows", "Check and generate Windows related stuff"},
                    {"build", "type", "Build the selected type"},
                    {"windows-build-path", "path", "Set the base directory where compilation takes place"},
                    {"windows-deploy-path", "path", "Set the base directory where deployement takes place"},
                    {"gen-desktoprc", "Generate a .desktop file"}
                      });
    //return a.exec();
    parser.process(a);
    ProjectDefinition project;
    if (parser.positionalArguments().isEmpty())
    {
        project = getProjectDescription();
    } else {
        project = getProjectDescription(parser.positionalArguments().at(0));
    }
    findQtModules(project);
    if (parser.isSet("version"))
        project.version = parser.value("version");
    else
    {
        findVersion(project);
    }
    if (parser.isSet("windows-build-path"))
        gOptions.windowsBuildPath = parser.value("windows-build-path");
    if (parser.isSet("windows-deploy-path"))
        gOptions.windowsDeployPath = parser.value("windows-deploy-path");
    if (parser.isSet("gen-flatpak"))
        genFlatPakFile(project);
    if (parser.isSet("gen-windows"))
        genWindows(project);
    if (parser.isSet("build") && parser.value("build") == "windows")
        buildWindows(project);
    if (parser.isSet("build") && parser.value("build") == "flatpak")
        buildFlatPak(project);
    if (parser.isSet("gen-desktoprc"))
    {
        if (checkDesktopRC(project))
            generateLinuxDesktopRC(project);
        else
            error_and_exit("The project description is not suited to generate a .desktop file. Please follow the previously error");
    }
}
