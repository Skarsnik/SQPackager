#include <QMap>
#include <QFile>
#include <desktoprc.h>
#include <basestuff.h>
#include <print.h>
#include <runner.h>
#include <QFileInfo>

const QStringList defaultCategories = {
    "AudioVideo",
    "Audio",
    "Video",
    "Development",
    "Education",
    "Game",
    "Graphics",
    "Network",
    "Office",
    "Science",
    "Settings",
    "System",
    "Utility"
};

bool    checkDesktopRC(const ProjectDefinition& proj, bool bypass)
{
    if (!bypass &&
        (!proj.desktopFile.isEmpty()
        || !checkForFile(proj.basePath, QRegularExpression(".+\\.desktop$")).isEmpty())
       )
    {
        println("Project already has a .desktop file, you can still use --gen-desktop if you want to have sqpackager to regenerate a new one");
        if (proj.desktopIcon.isEmpty())
        {
            if (!proj.icon.isEmpty())
            {
                println("The project description does not specify the icon file name for the desktop file <desktop-icon> field. Assusming <icon> is the desktop icon");
                return true;
            } else {
                println("The project description does not specify any <icon> or <desktop-icon>. One of this field is needed even whith a provided .desktop file since it's need to be able to install the write file");
                return false;
            }
        }
        return true;
    }
    println("Checking if the project description file fill the requierment to generate a valid .desktop file");
    bool toret = true;
    if (proj.shortDescription.isEmpty())
    {
        println("Your project need a short description <short-description>. It will appear as a description of your software on the DE menu");
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
        }
    }
    return toret;
}

void    setDesktopRC(ProjectDefinition& proj)
{
    if (!proj.desktopFile.isEmpty())
    {
        if (proj.desktopIcon.isEmpty())
            proj.desktopIcon = proj.icon;
        QFileInfo fi(proj.basePath + "/" + proj.desktopIcon);
        proj.desktopIconNormalizedName = proj.org + "." + proj.unixNormalizedName + "." + fi.suffix();
        setIconSize(proj);
        return ;
    }
    if (proj.desktopIcon.isEmpty())
        proj.desktopIcon = proj.icon;
    QFileInfo fi(proj.basePath + "/" + proj.desktopIcon);
    proj.desktopIconNormalizedName = proj.org + "." + proj.unixNormalizedName + "." + fi.suffix();
    setIconSize(proj);
    QString file = checkForFile(proj.basePath, QRegularExpression(".+\\.desktop$"));
    //println("Desktop file is : " + file);
    if (!file.isEmpty())
    {
        proj.desktopFile = file;
    } else {
        generateLinuxDesktopRC(proj);
    }
}

bool    generateLinuxDesktopRC(ProjectDefinition& proj)
{
    for (QString projCategory : proj.categories)
    {
        if (defaultCategories.contains(projCategory) == false)
            error_and_exit("The category you specified for the .desktop file is not valid : " + projCategory);
    }
    QString desktopFilePath = proj.basePath + "/" + proj.unixNormalizedName + ".desktop";
    if (proj.desktopIcon.isEmpty())
        proj.desktopIcon = proj.icon;
    QMap<QString, QString> mapping;
    mapping["NAME"] = proj.name;
    mapping["COMMENT"] = proj.shortDescription;
    mapping["EXEC"] = proj.targetName;
    QFileInfo fi(proj.basePath + "/" + proj.desktopFile);
    QFileInfo iconFi(proj.desktopIconNormalizedName);
    mapping["ICON"] = iconFi.completeBaseName();
    println(mapping["ICON"]);
    mapping["CATEGORIES"] = proj.categories.join(";");
    QString desktopString = useTemplateFile(":/desktop_template.tt", mapping);
    println("Creating .desktop file : " + desktopFilePath);
    proj.desktopFile = proj.unixNormalizedName + ".desktop";
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
    QStringList plop;
    for (QByteArray p : run.getStdout().split(' '))
    {
        plop.append(QString::fromLocal8Bit(p));
    }
    proj.iconSize = QSize(plop[2].split("x")[0].toInt(), plop[2].split("x")[1].toInt());
}
