#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    // https://woboq.com/blog/nice-debug-output-with-qt.html
    QString QT_MESSAGE_PATTERN=
            "[%{if-debug}D%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}]"
            ": %{file}:%{line} - %{message}";
    qSetMessagePattern(QT_MESSAGE_PATTERN);

    return a.exec();
}
