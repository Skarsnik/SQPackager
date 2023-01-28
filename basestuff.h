#ifndef BASESTUFF_H
#define BASESTUFF_H

#include <QString>
#include <sqpackager.h>
#include <QRegularExpression>

void                error_and_exit(QString error);
ProjectDefinition   getProjectDescription(QString path = "");
void                findQtModules(ProjectDefinition& def);
void                findVersion(ProjectDefinition& proj);
QString             useTemplateFile(QString rcPath, QMap<QString, QString> mapping);
bool                generateLinuxDesktopRC(ProjectDefinition& proj);
QString             checkForFile(const QString path, const QRegularExpression searchPattern);

#endif // BASESTUFF_H
