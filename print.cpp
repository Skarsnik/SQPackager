#include <QTextStream>

#include <print.h>


extern QTextStream cout;

QString green(QString text)
{
    return QString("\033[0m \033[1;32m%1\033[0m").arg(text);
}
QString red(QString text)
{
    return QString("\033[0m \033[1;31m%1\033[0m").arg(text);
}

void print(QString text)
{
    cout << text;
}

void println(QString text)
{
    cout << text << "\n";
    cout.flush();
}

void printlnOk(QString text, bool ok)
{
    cout << text << ": " << (ok ? green("Ok") : red("Ko")) << "\n";
    cout.flush();
}

void printlnYes(QString text, bool ok)
{
    cout << text << ": " << (ok ? green("Yes") : red("No")) << "\n";
    cout.flush();
}
