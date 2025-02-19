#include <QDir>
#include <QJsonDocument>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>

#include <QDateTime>
#include <basestuff.h>
#include <runner.h>
#include <print.h>

static void    handleFiles(ProjectDefinition& def, QJsonObject& obj);

void    error_and_exit(QString error)
{
   fprintf(stderr, "%s\n", error.toLocal8Bit().constData());
   exit(1);
}

ProjectDefinition    getProjectDescription(QString path)
{
    QString jsonPath = "sqproject.json";
    QString basePath = QDir::currentPath();
    if (path.isEmpty() == false)
    {
        jsonPath = path;
        basePath = QFileInfo(path).absolutePath();
    }
    QFile   desc(jsonPath);
    if (desc.open(QIODevice::ReadOnly) == false)
        error_and_exit("Can't open the project description file : " + desc.errorString());
    QByteArray json = desc.readAll();
    QJsonParseError* err = nullptr;
    QJsonDocument jsondoc = QJsonDocument::fromJson(json, err);
    if (err != nullptr)
        error_and_exit("Error parsing the description file : " + err->errorString());
    if (jsondoc.isNull())
    {
        error_and_exit("Invalid Json file somehow");
    }
    QJsonObject obj = jsondoc.object();
    println("Project name is : " + obj["name"].toString() + "\n");
    ProjectDefinition def;
    def.name = obj["name"].toString();
    //def.targetName = def.name;
    QString tmpString = def.name;
    def.unixNormalizedName = tmpString.replace(" ", "-");
    def.author = obj["author"].toString();
    def.authorMail = obj["author-mail"].toString();
    def.shortDescription = obj["short-description"].toString();
    def.description = obj["description"].toString();
    def.icon = obj["icon"].toString();
    def.org = obj["org"].toString();
    def.proFile = basePath + "/" + def.name + ".pro";
    if (obj.contains("pro-file"))
        def.proFile = basePath + "/" + obj["pro-file"].toString();
    def.basePath = basePath;
    def.projectBasePath = basePath;
    if (obj.contains("project-base-path"))
    {
        QFileInfo fi(basePath + "/" + obj.value("project-base-path").toString());
        def.projectBasePath = fi.absolutePath();
    }

    if (obj.contains("license-file"))
        def.licenseFile = obj["license-file"].toString();
    if (obj.contains("license-name"))
        def.licenseFile = obj["license-name"].toString();

    def.version.type = VersionType::Auto;
    if (obj.contains("version"))
    {
        QString versionString = obj["version"].toString();
        if (versionString == "date")
            def.version.type = VersionType::Date;
        if (versionString == "git")
            def.version.type = VersionType::Git;
        if (def.version.type == VersionType::Auto)
        {
            def.version.type = VersionType::Forced;
            def.version.forcedVersion = versionString;
        }
    }
    def.qtMajorVersion = QtMajorVersion::Auto;
    if (obj.contains("qt-major-version"))
    {
        QString qtVersion = obj["qt-major-version"].toString();
        if (qtVersion == "qt5" || qtVersion == "5")
            def.qtMajorVersion = QtMajorVersion::Qt5;
        if (qtVersion == "qt6" || qtVersion == "6")
            def.qtMajorVersion = QtMajorVersion::Qt6;
        if (def.qtMajorVersion == QtMajorVersion::Auto)
            error_and_exit("Can't make sense of the <qt-major-version> field, accepted value are : qt5, 5, qt6, 6");
    }

    if (obj.contains("target-name"))
        def.targetName = obj["target-name"].toString();
    if (obj.contains("translations-dir"))
        def.translationDir = obj["translations-dir"].toString();
    if (obj.contains("files"))
        handleFiles(def, obj);

    // Unix DESKTOP stuff
    if (obj.contains("desktop-file"))
    {
        def.desktopFile = obj["desktop-file"].toString();
    }
    def.desktopFileNormalizedName = def.org + "." + def.unixNormalizedName + ".desktop";
    def.desktopIconNormalizedName = def.org + "." + def.unixNormalizedName;
    def.desktopIcon = "";
    if (obj.contains("desktop-icon"))
        def.desktopIcon = obj.value("desktop-icon").toString();
    if (obj.contains("desktop-categories"))
    {
        for (auto entry : obj.value("desktop-categories").toArray().toVariantList())
        {
            def.categories.append(entry.toString());
        }
    }
    def.debianPackageName = def.name.toLower();
    if (obj.contains("debian-maintainer"))
    {
        def.debianMaintainer = obj.value("debian-maintainer").toString();
    }
    if (obj.contains("debian-maintainer-mail"))
    {
        def.debianMaintainerMail = obj.value("debian-maintainer-mail").toString();
    }

    return def;
}

void    handleFiles(ProjectDefinition& def, QJsonObject& obj)
{
    QJsonObject fileObj = obj["files"].toObject();
    for (const QString& key : fileObj.keys())
    {
        QString dest = key;
        if (key.left(6) == "WIN32:")
        {
            if (QSysInfo::productType() != "windows")
            {
                continue;
            }
            dest = key.mid(6);
        }
        QString value = fileObj[key].toString();
        ReleaseFile relFile;
        relFile.name = QFileInfo(dest).baseName();
        relFile.destination = dest;
        relFile.type = Local;
        relFile.source = value;
        if (value.startsWith("http://") || value.startsWith("https://"))
        {
            relFile.type = Remote;
        }
        def.releaseFiles.append(relFile);
    }
}

#include <QRegularExpression>

void    extractInfosFromProFile(ProjectDefinition& def)
{
    const QRegularExpression qtDef("^QT\\s*\\+=");
    const QRegularExpression blankExp("\\s+");
    const QRegularExpression targetDef("TARGET\\s*=\\s*([a-zA-Z-]+)");
    println(def.proFile);
    if (QFileInfo::exists(def.proFile) == false)
    {
        QString tmpString = def.name;
        tmpString.replace(" ", "");
        tmpString = def.basePath + "/" + tmpString + ".pro";
        if (QFileInfo::exists(tmpString))
        {
            def.proFile = tmpString;
        } else {
            error_and_exit("Could not find the .pro file for the project, you can specify it using the <pro-file> field");
        }
    }
    QFile proFile(def.proFile);
    if (proFile.open(QIODevice::Text | QIODevice::ReadOnly) == false)
        error_and_exit("Could not open the .pro file");
    while (!proFile.atEnd())
    {
        QString line = proFile.readLine();
        if (qtDef.match(line).hasMatch())
        {
            QString stringDef = line.split("+=").at(1);
            println(stringDef);
            QStringList qtmodule = stringDef.split(blankExp, Qt::SkipEmptyParts);
            def.qtModules = qtmodule;
        }
        if (targetDef.match(line).hasMatch() && def.targetName.isEmpty())
        {
            auto targetMatch = targetDef.match(line);
            def.targetName = targetMatch.captured(1);
            println("Target found in .pro file is : " + def.targetName);
        }
    }
    if (def.targetName.isEmpty())
    {
        println("No target name specified and found in the .pro file, using the pro file base name as target (default qmake behavior)");
        QFileInfo fi(def.proFile);
        def.targetName = fi.baseName();
    }
}

/*
 * The version field behave like this
 * If the user set the version already, don't bother
 * the value "git" this will try to find a current tag, otherwise use a shortened commit number
 * the value "date" use the current date as date
*/

void    findVersion(ProjectDefinition& proj)
{
    println("Trying to find project version");
    if (proj.version.type == VersionType::Forced)
    {
        println("Project version is user specified : " + proj.version.forcedVersion);
        proj.version.simpleVersion = proj.version.forcedVersion;
        return ;
    }
    if (proj.version.type == VersionType::Git || proj.version.type == VersionType::Auto)
    {
        println("Project version is determined by git or was not set");
        Runner run(true);
        bool ok = run.run("git", proj.basePath, QStringList() << "status");
        if (!ok && proj.version.type == VersionType::Auto)
        {
            println("Git failed, falling back to using current date");
            proj.version.type = VersionType::Date;
            proj.version.dateVersion = QDateTime::currentDateTime().toString("yyyy-MM-dd");
            proj.version.simpleVersion = proj.version.dateVersion;
            return ;
        }
        proj.version.type = VersionType::Git;
        if (!ok && proj.version.type == VersionType::Git)
        {
            error_and_exit("\tGetting info from git failed");
        }
        run.run("git", proj.basePath,  QStringList() << "rev-parse" << "--abbrev-ref" << "HEAD");
        QString branchName = run.getStdout().trimmed();
        // Trying to find if we are in tagged version
        ok = run.run("git", proj.basePath, QStringList() << "describe" << "--tags" << "--exact-match");
        if (ok)
        {
            println("We are on a tagged version");
            QString plop = run.getStdout();
            //println("Plop : " + plop);
            proj.version.gitTag = plop.trimmed();
        } else {
            // If not, get the nice tag-numberofcommit-commit format git gave us
            ok = run.run("git", proj.basePath, QStringList() << "describe" << "--tags");
            if (ok)
            {
                proj.version.gitVersionString = run.getStdout().trimmed();
                proj.version.simpleVersion = proj.version.gitVersionString;
                run.run("git", proj.basePath, QStringList() << "describe" << "--tags" << "--abbrev=0");
                proj.version.gitLastTag = run.getStdout().trimmed();
            }
        }
        ok = run.run("git", proj.basePath, QStringList() << "rev-parse" << "--verify" << branchName);
        proj.version.gitCommitId = run.getStdout().trimmed();
/*        println("Git version string" + proj.version.gitVersionString);
        println("Is empty : " + QString::number(proj.version.gitVersionString.isEmpty()));*/
        if (proj.version.gitVersionString.isEmpty())
        {
            //println("Git tag " + proj.version.gitTag);
            if (proj.version.gitTag.isEmpty() == false)
            {
                //println("Setting version");
                proj.version.gitVersionString = proj.version.gitTag;
                proj.version.simpleVersion = proj.version.gitTag;
            } else {
                proj.version.gitVersionString = proj.version.gitCommitId.left(8);
                proj.version.simpleVersion = proj.version.gitVersionString;
            }
        }
        println("Project version is " + proj.version.simpleVersion);
        return ;
    }
    if (proj.version.type == VersionType::Date)
    {
        println("Project version specified to use current date");
        proj.version.dateVersion = QDateTime::currentDateTime().toString("yyyy-MM-dd");
        println("Project version is " + proj.version.dateVersion);
        return ;
    }
}

const QMap<QString, QString> correctLicense = {
    {"GPL3", "GPL-3"}
};

const QStringList debianLicenses = {"Apache-2.0", "CC0-1.0", "GFDL-1.3", "GPL-2", "LGPL-2", "MPL-1.1", "Artistic", "GFDL", "GPL", "GPL-3", "LGPL-2.1", "MPL-2.0", "BSD", "GFDL-1.2", "GPL-1", "LGPL", "LGPL-3"};
const QString GPLV3 = "GNU GENERAL PUBLIC LICENSE\n" \
"                       Version 3, 29 June 2007";

void    findLicense(ProjectDefinition& project)
{
    if (project.licenseName.isEmpty() == false && project.licenseFile.isEmpty() == false)
    {
        return ;
    }
    println("Trying to find a License file");
    if (project.licenseFile.isEmpty()) {
        QString result = checkForFile(project.basePath, QRegularExpression("licence", QRegularExpression::CaseInsensitiveOption));
        if (!result.isEmpty())
        {
            project.licenseFile = result;
        } else {
            result = checkForFile(project.basePath, QRegularExpression("license", QRegularExpression::CaseInsensitiveOption));
            project.licenseFile = result;
        }
        if (result.isEmpty())
        {
            error_and_exit("\tCan't find a license file, please set the license-file field if you don't use an obvious license file name");
        }
    }
    if (!project.licenseName.isEmpty())
        return ;
    println("Trying to find the License Name");
    for (QString corrected : correctLicense.keys())
    {
        if (project.licenseFile.contains(corrected))
        {
            project.licenseName = correctLicense[corrected];
            break;
        }
    }
    if (project.licenseName.isEmpty())
    {
        for (QString debLicence : debianLicenses)
        {
            if (project.licenseFile.contains(debLicence))
                project.licenseName = debLicence;
        }
    }
    if (project.licenseName.isEmpty())
    {
        QFile licenseFile(project.basePath + "/" + project.licenseFile);
        if (!licenseFile.open(QIODevice::Text | QIODevice::ReadOnly))
        {
            error_and_exit("Can't open the license file: " + licenseFile.fileName() + " - " + licenseFile.errorString());
        }
        QString data = licenseFile.read(512);
        if (data.startsWith(GPLV3))
            project.licenseName = "GPL-3";
    }
    if (!project.licenseName.isEmpty())
        println("\tLicense name is " + project.licenseName);
}

void        findReadme(ProjectDefinition& project)
{
    if (!project.readmeFile.isEmpty())
        return;
    println("Searching for a Readme file");
    QString readmeSearch = checkForFile(project.basePath, QRegularExpression("readme", QRegularExpression::CaseInsensitiveOption));
    if (readmeSearch.isEmpty() == false)
    {
        project.readmeFile = readmeSearch;
        println("\tFound " + readmeSearch);
    }
}

QString    checkForFile(const QString path, const QRegularExpression searchPattern)
{
    QDir dir(path);
    for (auto entry : dir.entryList())
    {

        QRegularExpressionMatch match = searchPattern.match(entry);
        //println(QString("Checking : ") + entry + " -- " + (match.hasMatch() ? "true " : "false"));
        if (match.hasMatch())
        {
            return entry;
        }
    }
    return QString();
}

const QRegularExpression varEntry("%%([\\w_]+)%%");
const QRegularExpression ifStartEntry("%%\\{IF ([\\w_]+)%%");
const QRegularExpression ifEndEntry("%%}IF%%");

QString    useTemplateFile(QString rcPath, QMap<QString, QString> mapping)
{
    QFile   templateFile(rcPath);
    if (!templateFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        error_and_exit("Can't open template file " + templateFile.fileName() + " : " + templateFile.errorString());
    }
    QString toret;
    bool    skipLine = false;
    while (!templateFile.atEnd())
    {
        QString line = templateFile.readLine();
        auto ifStartMatch = ifStartEntry.match(line);
        auto ifEndMatch = ifEndEntry.match(line);
        if (ifEndMatch.hasMatch())
        {
            skipLine = false;
            continue;
        }
        if (ifStartMatch.hasMatch())
        {
            const QString key = ifStartMatch.captured(1);
            if (!mapping.contains(key))
            {
                skipLine = true;
            } else {
                continue;
            }
        }
        if (skipLine)
            continue;
        auto matchs = varEntry.globalMatch(line);
        if (matchs.hasNext())
        {
            QString generatedLine;
            unsigned int indexStart = 0;
            while (matchs.hasNext())
            {
                auto match = matchs.next();
                // This take the part before the matching %%xx%%
                generatedLine.append(line.mid(indexStart, match.capturedStart(0) - indexStart));
                indexStart = match.capturedEnd(0);
                const QString key = match.captured(1);
                if (mapping.contains(key))
                {
                    generatedLine.append(mapping.value(key));
                } else {
                    println("Template warning: Found key in template file that does not have a value: " + key);
                }
            }
            generatedLine.append(line.mid(indexStart));
            toret.append(generatedLine);
        } else {
            toret.append(line);
        }
    }
    return toret;
}

