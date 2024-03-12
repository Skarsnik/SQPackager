#ifndef BASESTUFF_H
#define BASESTUFF_H

#include <QString>
#include <sqpackager.h>
#include <QRegularExpression>

void                error_and_exit(QString error);
ProjectDefinition   getProjectDescription(QString path = "");
void                extractInfosFromProFile(ProjectDefinition& def);
void                findVersion(ProjectDefinition& proj);
void                findLicense(ProjectDefinition& project);
void                findReadme(ProjectDefinition& project);
QString             useTemplateFile(QString rcPath, QMap<QString, QString> mapping);
bool                generateLinuxDesktopRC(ProjectDefinition& proj);
void                generateUnixInstallFile(const ProjectDefinition& project);
void                generateManPage(const ProjectDefinition& project);
QString             checkForFile(const QString path, const QRegularExpression searchPattern);

#endif // BASESTUFF_H
