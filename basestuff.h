#ifndef BASESTUFF_H
#define BASESTUFF_H

#include <QString>
#include <sqpackager.h>

void                error_and_exit(QString error);
ProjectDefinition   getProjectDescription(QString path = "");
void                findQtModules(ProjectDefinition& def);
void                findVersion(ProjectDefinition& proj);
QString             useTemplateFile(QString rcPath, QMap<QString, QString> mapping);
bool                generateLinuxDesktopRC(ProjectDefinition& proj);
bool                checkDesktopRC(ProjectDefinition& proj);

#endif // BASESTUFF_H
