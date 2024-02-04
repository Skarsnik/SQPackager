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
    if (!path.isEmpty())
    {
        jsonPath = path;
        basePath = QFileInfo(path).absolutePath();
    }
    QFile   desc(jsonPath);
    if (!desc.open(QIODevice::ReadOnly))
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
    def.targetName = def.name;
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
    def.qtMajorVersion = "auto";
    if (obj.contains("qt-major-version"))
        def.qtMajorVersion = obj["qt-major-version"].toString();
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
    def.desktopFileNormalizedName = def.org + "." + def.name + ".desktop";
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
    return def;
}

void    handleFiles(ProjectDefinition& def, QJsonObject& obj)
{
    QJsonObject fileObj = obj["files"].toObject();
    for (const QString& key : fileObj.keys())
    {
        QString value = fileObj[key].toString();
        ReleaseFile relFile;
        relFile.name = QFileInfo(key).baseName();
        relFile.destination = key;
        relFile.type = Local;
        relFile.source = value;
        if (value.startsWith("http://") || value.startsWith("https://"))
        {
            relFile.type = Remote;
        }
        def.releaseFiles.append(relFile);
    }
}

void    findQtModules(ProjectDefinition& def)
{
    if (QFileInfo::exists(def.name + ".pro"))
    {
        QFile proFile(def.name + ".pro");
        while (!proFile.atEnd())
        {
            QString line = proFile.readLine();
#if QT_VERSION_MAJOR == 6
#include <QRegularExpression>
            const QRegularExpression qtDef("QT\\s+\\+=");
            const QRegularExpression blankExp("\\s+");
            if (qtDef.match(line).hasMatch())
            {
#else
            QRegExp qtDef("QT\\s+\\+=");
            const QRegExp blankExp("\\s+");
            if (qtDef.indexIn(line) != -1)
            {
#endif
                QString stringDef = line.split("+=").at(1);
                QStringList qtmodule = stringDef.split(blankExp);
                def.qtModules = qtmodule;
                break;
            }
        }
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
    std::function<QString()> gitFind([basePath=proj.basePath] {
        Runner run(true);
        bool ok = run.run("git", basePath, QStringList() << "status");
        if (!ok)
            return QString();
        run.run("git", basePath,  QStringList() << "rev-parse" << "--abbrev-ref" << "HEAD");
        QString branchName = run.getStdout().trimmed();
        // Checking if we are in a tag
        ok = run.run("git", basePath, QStringList() << "describe" << "--tags" << "--exact-match");
        if (ok)
        {
            QString out = run.getStdout();
            return out.trimmed();
        }
        // If not, get the nice tag-numberofcommit-commit format git gave us
        ok = run.run("git", basePath, QStringList() << "describe" << "--tags");
        if (ok)
        {
            QString out = run.getStdout();
            return out.trimmed();
        }
        // If no tag, use the last commit hash first 8 characters
        ok = run.run("git", basePath, QStringList() << "rev-parse" << "--verify" << branchName);
        QString out = run.getStdout();
        return out.left(8);
    });
    if (!proj.version.isEmpty())
    {
        println("Project version is user specified : " + proj.version);
        return ;
    }
    if (proj.version == "git")
    {
        proj.version = gitFind();
        println("Project version specified to use git");
        if (proj.version.isEmpty())
        {
            error_and_exit("Did not managed to determine a version using git");
        }
        proj.versionType = "git";
        println("Project version is " + proj.version);
        return ;
    }
    if (proj.version == "date")
    {
        println("Project version specified to use current date");
        proj.version = QDateTime::currentDateTime().toString("yyyy-MM-dd");
        proj.versionType = "date";
        println("Project version is " + proj.version);
        return ;
    }
    println("Trying to find project version");
    proj.version = gitFind();
    if (!proj.version.isEmpty())
    {
        println("Project version is " + proj.version);
        proj.versionType = "git";
        return ;
    }
    println("Git failed, falling back to using current date");
    proj.version = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    println("Project version is " + proj.version);
}

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
    for (QString debLicence : debianLicenses)
    {
        if (project.licenseFile.contains(debLicence))
            project.licenseName = debLicence;
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

