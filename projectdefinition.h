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
    QString     flatpakFilesystemPermission;
    QString     proFile;
    QString     version;
    QString     readmeFile;
    QString     licenseFile;
    QString     qtMajorVersion;
    QString     desktopIcon;
    QStringList categories;
};

#endif // PROJECTDEFINITION_H
