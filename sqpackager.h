#ifndef SQPACKAGER_H
#define SQPACKAGER_H
#include <projectdefinition.h>

struct  PackagerOptions
{
    QString qmakePath;
    QString windowsBuildPath;
    QString windowsDeployPath;
};

bool    checkFlatPak(const ProjectDefinition project, bool bypass = false);
void    generateFlatPakFile(ProjectDefinition& project);
void    buildFlatPak(const ProjectDefinition& project);

void    genWindows(ProjectDefinition& project);
void    buildWindows(ProjectDefinition &project);

#endif // SQPACKAGER_H
