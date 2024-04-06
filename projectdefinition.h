#ifndef PROJECTDEFINITION_H
#define PROJECTDEFINITION_H

#include <QSize>
#include <QString>
#include <QStringList>

enum ReleaseFileType {
    Local,
    Remote,
    Generated
};

struct ReleaseFile
{
    ReleaseFileType type;
    QString name;
    QString source;
    QString destination;
};

enum class QtMajorVersion {
    Qt6,
    Qt5,
    Auto
};

enum class VersionType {
    Forced,
    Git,
    Date,
    Auto
};

struct ProjectVersion {
    VersionType type;
    QString forcedVersion;
    QString dateVersion;
    QString gitCommitId;
    QString gitVersionString;
    QString gitTag;
    QString simpleVersion;
};

struct ProjectDefinition
{
    QString     name;
    QString     unixNormalizedName;
    QString     shortDescription;
    QString     description;
    QString     author;
    QString     authorMail;

    QStringList qtModules;
    QString     icon;
    QSize       iconSize;
    QString     org;
    QString     basePath; // This is the path where the .pro file is
    QString     projectBasePath; // This can override basePath for when the .pro file is a sub 'project' like the ExampleApp
    QString     flatpakFilesystemPermission;
    QString     flatpakFile;
    QString     flatpakName;
    QString     debianMaintainer;
    QString     debianMaintainerMail;
    QString     debianPackageName;
    QString     proFile;
    ProjectVersion     version;
    QString     readmeFile;
    QString     licenseFile;
    QString     licenseName;
    QtMajorVersion  qtMajorVersion;
    QString     desktopIcon;
    QString     desktopFile;
    QString     desktopFileNormalizedName;
    QString     desktopIconNormalizedName;
    QStringList categories;
    QString     targetName;
    QString     translationDir;
    QList<ReleaseFile>       releaseFiles;
};

#endif // PROJECTDEFINITION_H
