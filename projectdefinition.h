#ifndef PROJECTDEFINITION_H
#define PROJECTDEFINITION_H

#include <QSize>
#include <QString>
#include <QStringList>

struct ProjectDefinition
{
    QString     name;
    QString     shortDescription;
    QString     description;
    QStringList qtModules;
    QString     icon;
    QSize       iconSize;
    QString     org;
    QString     basePath;
    QString     projectBasePath;
    QString     flatpakFilesystemPermission;
    QString     flatpakFile;
    QString     flatpakName;
    QString     proFile;
    QString     version;
    QString     readmeFile;
    QString     licenseFile;
    QString     qtMajorVersion;
    QString     desktopIcon;
    QString     desktopFile;
    QStringList categories;
    QString     targetName;
    QString     translationDir;
};

#endif // PROJECTDEFINITION_H
