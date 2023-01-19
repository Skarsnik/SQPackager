#include <desktoprc.h>
#include <basestuff.h>
#include <runner.h>

bool    checkDesktopRC(ProjectDefinition& proj)
{
    println("Checking if the project description file fill the requierment to generate a valid .desktop file");
    bool toret = true;
    if (proj.shortDescription.isEmpty())
    {
        println("Your project need a short description <short-description>. It will appear as a description of your soltware on the DE menu");
        toret = false;
    }
    if (proj.categories.isEmpty())
    {
        println("Your project need one or more categories in your project description <desktop-categories>");
        toret = false;
    }
    if (proj.desktopIcon.isEmpty())
    {
        if (proj.icon.isEmpty())
        {
            println("Your project need to specify a icon for the .desktop file <desktop-icon> or <icon>");
            toret = false;
        } else {
            println("Using the <icon> entry as desktop icon, you can specify a <desktop-icon> if you want a specific other icon");
            proj.desktopIcon = proj.icon;
        }
    }
    return toret;
}

bool    generateLinuxDesktopRC(ProjectDefinition& proj)
{
    QString desktopFilePath = proj.basePath + "/" + proj.name + ".desktop";
    QMap<QString, QString> mapping;
    mapping["NAME"] = proj.name;
    mapping["COMMENT"] = proj.shortDescription;
    mapping["EXEC"] = proj.name;
    mapping["ICON"] = proj.desktopIcon;
    mapping["CATEGORIES"] = proj.categories.join(";");
    QString desktopString = useTemplateFile(":/desktop_template.tt", mapping);
    println("Creating .desktop file : " + desktopFilePath);
    QFile desktopFile(desktopFilePath);
    if (!desktopFile.open(QIODevice::WriteOnly))
    {
        error_and_exit("Can't write the .desktop file : " + desktopFile.errorString());
    }
    desktopFile.write(desktopString.toLocal8Bit());
    desktopFile.close();
    return false;
}

void    setIconSize(ProjectDefinition& proj)
{
    Runner run;
    bool ok = run.run("identify", QStringList() << proj.basePath + "/" + proj.desktopIcon);
    if (!ok)
        error_and_exit("Could not run identify on the desktop icon file. Do you have imagemagicks installed?");
    QStringList plop = run.getStdout().split(" ");
    proj.iconSize = QSize(plop[2].split("x")[0], plop[2].split("x")[1]);
}
