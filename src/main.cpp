#include <QApplication>
#include <QCommandLineParser>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  QCommandLineParser parser;

  parser.addPositionalArgument("root_path", "The root folder");

  parser.process(app);

  MainWindow mainWindow(parser.positionalArguments().at(0));
  mainWindow.show();
  return app.exec();
}
