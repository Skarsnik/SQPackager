#pragma once

#include <projectdefinition.h>

bool    checkDesktopRC(const ProjectDefinition& proj, bool bypass = false);
bool    generateLinuxDesktopRC(ProjectDefinition& proj);
void    setDesktopRC(ProjectDefinition& proj);
void    setIconSize(ProjectDefinition& proj);
