#ifndef COMPILE_DEFINES_H
#define COMPILE_DEFINES_H
#include <QString>

namespace CompileDefines
{
    const QString installed = "SQPROJECT_INSTALLED";
    const QString standalone = "SQPROJECT_STANDALONE";
    const QString unix_install_prefix = "SQPROJECT_UNIX_INSTALL_PREFIX";
    const QString windows_install = "SQPROJECT_WINDOWS_INSTALL";
    const QString debian_install = "SQPROJECT_DEBIAN_INSTALL";
    const QString flatpak_install = "SQPROJECT_FLATPAK_INSTALL";
    const QString unix_install_share_path = "SQPROJECT_UNIX_APP_SHARE";
}

#endif // COMPILE_DEFINES_H
