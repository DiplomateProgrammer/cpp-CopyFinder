#include "MainWindow.h"
#include "ui_mainwindow.h"
#include <iostream>
const int ONE_READ_BYTES = 1024*1024*10;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    fileSystemModel = new QFileSystemModel;
    fileSystemModel->setRootPath(QDir::currentPath());
    ui->filesTreeView->setModel(fileSystemModel);
    ui->filesTreeView->setUniformRowHeights(true);
    ui->filesTreeView->setSelectionMode(QTreeView::SingleSelection);
    ui->filesTreeView->setColumnWidth(0, 350);
    ui->outputTreeWidget->setHeaderLabels(QStringList{ "File name", "File path" });
    ui->outputTreeWidget->setColumnWidth(0, 100);
    ui->outputTreeWidget->setColumnWidth(1, 100);
    ui->outputTreeWidget->setUniformRowHeights(true);
    qRegisterMetaType<SizeGroupList>("SizeGroupList");
    connect(ui->startButton, SIGNAL(clicked()), this, SLOT(onStartButtonClicked()), Qt::UniqueConnection);
    connect(this, SIGNAL(calculatedSizeGroup(SizeGroupList)), this, SLOT(onCalculatedSizeGroup(SizeGroupList)), Qt::UniqueConnection);
    connect(this, SIGNAL(finishedCalculations()), this, SLOT(onFinishedCalculations()), Qt::UniqueConnection);
    connect(this, SIGNAL(finishedClustering()), this, SLOT(onFinishedClustering()), Qt::UniqueConnection);
    working = false;
    deleted = false;
}

void MainWindow::onStartButtonClicked()
{
    if(working)
    {
        working = false;
        return;
    }
    working = true;
    ui->outputTreeWidget->clear();
    ui->statusLabel->setText("Status: searching...");
    ui->startButton->setText("Stop searching");
    QModelIndexList selectedList = ui->filesTreeView->selectionModel()->selectedIndexes();
    if (selectedList.size() < 1) { return; }
    QModelIndex selected = selectedList.front();
    QVariant data = fileSystemModel->filePath(selected);
    QDir targetDirectory = QDir((QString)data.value<QString>());
    std::unordered_map<qint64, QFileInfoList> *sizeGroups = new std::unordered_map<qint64, QFileInfoList>();
    QtConcurrent::run(this, &MainWindow::findDuplicates, targetDirectory, sizeGroups);
}

void MainWindow::findDuplicates(QDir folder, std::unordered_map<qint64, QFileInfoList> *sizeGroups)
{
    QDirIterator it(folder, QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        if(!working) { break; }
        it.next();
        QFileInfo elem = it.fileInfo();
        if (elem.isFile())
        {
            handleFile(elem.absoluteFilePath(), sizeGroups);
        }
    }
    QList<QFileInfoList> sizeGroupsList;
    for(auto it: *sizeGroups)
    {
        if (!working) { break; }
        sizeGroupsList.append(it.second);
    }
    delete sizeGroups;
    emit finishedClustering();
    QtConcurrent::blockingMap(sizeGroupsList, [this](QFileInfoList sizeGroup) { handleSizeGroup(sizeGroup); });
    emit finishedCalculations();
}

void MainWindow::handleFile(QFileInfo file, std::unordered_map<qint64, QFileInfoList> *sizeGroups)
{
    if (sizeGroups->find(file.size()) == sizeGroups->end())
    {
        QFileInfoList group;
        group.append(file);
        sizeGroups->insert({ file.size(), group });
        return;
    }
    auto sizeGroup = sizeGroups->find(file.size());
    sizeGroup->second.append(file);
}


void MainWindow::handleSizeGroup(const QFileInfoList sizeGroup)
{
    QList<QFileInfoList> output;
    for(QFileInfo file: sizeGroup)
    {
        bool foundPlace = false;
        for (QList<QFileInfoList>::iterator group = output.begin(); group != output.end() && !foundPlace; group++)
        {
            if(!working) { return; }
            QFileInfo repr = group->front();
            QFile a(file.absoluteFilePath()), b(repr.absoluteFilePath());
            a.open(QIODevice::ReadOnly);
            b.open(QIODevice::ReadOnly);
            bool same = true;
            while (a.bytesAvailable() > 0 && same)
            {
                QByteArray one, two;
                one = a.read(ONE_READ_BYTES);
                two = b.read(ONE_READ_BYTES);
                for (int i = 0; i < one.size() && same; i++)
                {
                    if(!working) { return; }
                    if (one.at(i) != two.at(i))
                    {
                        same = false;
                    }
                }
            }
            a.close();
            b.close();
            if (same)
            {
                group->append(file);
                foundPlace = true;
            }
        }
        if (!foundPlace)
        {
            QFileInfoList newGroup;
            newGroup.append(file);
            output.append(newGroup);
        }
    }
    emit calculatedSizeGroup(output);
}

void MainWindow::onCalculatedSizeGroup(SizeGroupList sizeGroup)
{
    qint64 size = sizeGroup.front().front().size();
    QTreeWidgetItem *sizeGroupItem = new QTreeWidgetItem({ "Size: " + QString::number(size, 10), ""});
    int counter = 0;
    ui->outputTreeWidget->addTopLevelItem(sizeGroupItem);
    for (auto group : sizeGroup)
    {
        counter++;
        QTreeWidgetItem *groupItem = new QTreeWidgetItem({ "Group " + QString::number(counter, 10), ""});
        sizeGroupItem->addChild(groupItem);
        for (auto file : group)
        {
            QTreeWidgetItem *fileItem = new QTreeWidgetItem({ file.fileName(), file.absoluteFilePath() });
            groupItem->addChild(fileItem);
        }
    }
}

void MainWindow::onFinishedCalculations()
{
    if(deleted) return;
    if (working) { ui->statusLabel->setText("Status: search complete!"); }
    else { ui->statusLabel->setText("Status: search cancelled"); }
    ui->startButton->setText("Start searching");
    working = false;
}

void MainWindow::onFinishedClustering()
{
    if(deleted) return;
    if (working) { ui->statusLabel->setText("Status: clustering finished, now searching..."); }
}

MainWindow::~MainWindow()
{
    deleted = true;
    working = false;
    delete ui;
}
