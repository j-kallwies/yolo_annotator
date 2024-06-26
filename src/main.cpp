#include <QApplication>
#include <QCommandLineParser>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  QCommandLineParser parser;

  parser.addPositionalArgument("root_path", "The root folder");
  parser.addPositionalArgument("label_names", "The label names...");

  parser.process(app);

  QStringList label_names = parser.positionalArguments();
  label_names.pop_front(); // Remove the root_path

  MainWindow mainWindow(parser.positionalArguments().at(0), label_names);
  mainWindow.show();
  return app.exec();
}
