#include <QFile>
#include <projectdefinition.h>


void    genFlatPakFile(const ProjectDefinition& project)
{
    QString baseFlatPak(R"(
app-id: %1
runtime: org.kde.Platform
runtime-version: '5.15-22.08'
sdk: org.kde.Sdk
command: %2
finish-args:
    - --socket=x11
    - --socket=wayland
    - --filesystem=%5
modules:
    - name: %3
      buildsystem: qmake
      sources:
       - type: dir
         path: %4
)"
    );
    QString fullName = project.org + "." + project.name;
    QFile manifest(project.basePath + "/" + fullName + ".yml");
    manifest.open(QIODevice::WriteOnly | QIODevice::Text);
    QString perms = project.flatpakFilesystemPermission;
    if (project.flatpakFilesystemPermission.isEmpty())
        perms = "xdg-config";
    manifest.write(baseFlatPak.arg(fullName).arg(project.name).arg(project.name).arg(project.basePath).arg(perms).toLocal8Bit());
}

void    buildFlatPak(const ProjectDefinition& project)
{

}
