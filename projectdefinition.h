#ifndef PROJECTDEFINITION_H
#define PROJECTDEFINITION_H

#include <QString>
#include <QStringList>

struct ProjectDefinition
{
    QString     name;
    QString     shortDescription;
    QString     description;
    QStringList qtModules;
    QString     icon;
    QString     org;
    QString     basePath;
    QString     flatpakFilesystemPermission;
    QString     proFile;
    QString     version;
    QString     readmeFile;
    QString     licenseFile;
    QString     qtMajorVersion;
};

#endif // PROJECTDEFINITION_H
