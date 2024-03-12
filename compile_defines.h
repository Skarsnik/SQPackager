#ifndef COMPILE_DEFINES_H
#define COMPILE_DEFINES_H
#include <QString>

namespace CompileDefines
{
    QString installed = "SQPROJECT_INSTALLED";
    QString standalone = "SQPROJECT_STANDALONE";
    QString unix_install_prefix = "SQPROJECT_UNIX_INSTALL_PREFIX";
    QString windows_install = "SQPROJECT_WINDOWS_INSTALL";
    QString debian_install = "SQPROJECT_DEBIAN_INSTALL";
    QString flatpak_install = "SQPROJECT_FLATPAK_INSTALL";
    QString unix_install_share_path = "SQPROJECT_UNIX_APP_SHARE";
}

#endif // COMPILE_DEFINES_H
