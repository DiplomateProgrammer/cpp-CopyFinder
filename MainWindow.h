#ifndef COPYFINDER_H
#define COPYFINDER_H

#include <QMainWindow>
#include <qfilesystemmodel.h>
#include <unordered_map>
#include <list>
#include <qfile.h>
#include <QList>
#include <QDir>
#include <QtConcurrent/QtConcurrent>
#include <QtConcurrent/QtConcurrentRun>
#include <QtConcurrent/QtConcurrentMap>
#include <atomic>

using SizeGroupList = QList<QFileInfoList>;
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QFileSystemModel *fileSystemModel;
    void findDuplicates(QDir folder, std::unordered_map<qint64, QFileInfoList> *sizeGroups);
    void handleFile(QFileInfo file, std::unordered_map<qint64, QFileInfoList> *sizeGroups);
    void handleSizeGroup(const QFileInfoList sizeGroup);
    std::atomic_bool working;
    std::atomic_bool deleted;
    QFuture<void> future1, future2;

private:
    Ui::MainWindow *ui;

private slots:
    void onStartButtonClicked();
    void onCalculatedSizeGroup(SizeGroupList sizeGroup);
    void onFinishedCalculations();
    void onFinishedClustering();

signals:
    void calculatedSizeGroup(SizeGroupList sizeGroup);
    void finishedCalculations();
    void finishedClustering();
};

#endif // COPYFINDER_H
