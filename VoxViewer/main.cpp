#include "MainWindow.h"

#include <QApplication>
#include "Vox3DForm.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    /// 引数対応
    QString     vox_file, fdtd_file;
    const auto& arguments = QCoreApplication::arguments();
    if (arguments.count() > 1) {
        for (int ic = 1; ic < arguments.count(); ++ic) {
            const auto& tmparg = arguments[ic];
            if (tmparg.startsWith("voxpath=")) {
                vox_file = tmparg.mid(QString("voxpath=").length());
                vox_file.replace("\"", "");
            }
            else if (tmparg.startsWith("fdtdpath=")) {
                fdtd_file = tmparg.mid(QString("fdtdpath=").length());
                fdtd_file.replace("\"", "");
            }
        }
    }

    Vox3DForm w;
    if (!fdtd_file.isEmpty()) {
        w.setFdtdFile(fdtd_file);
    }
    if (!vox_file.isEmpty()) {
        w.setVoxFile(vox_file);
    }
    w.show();
    return a.exec();
}
