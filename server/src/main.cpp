#include <QCoreApplication>
#include <QTextStream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QTextStream(stdout) << "Server placeholder (Qt 5.12 compatible). Implementations will follow in subsequent commits." << Qt::endl;

    return 0;
}