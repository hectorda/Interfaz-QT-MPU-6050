#include "mainwindow.h"

#include "ui_mainwindow.h"
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QMessageBox>
#include <QList>
#include <QDebug>
#include <QScrollBar>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    inicializar();
    conexiones();
    //actions();
}

MainWindow::~MainWindow() //Destroy
{
    //this->closeSerialPort();
    delete ui;
}

void MainWindow::inicializar()
{
    serial=new QSerialPort(this);
    //file.setFileName("out.txt");
    ui->setupUi(this);
    ui->menuVer->addAction(ui->dockWidget->toggleViewAction());
    ui->stopButton->setDisabled(true);
    graficos=new Graficos;
    //ui->tiempo->setValidator(new QIntValidator(0,200,this));
    foreach (QSerialPortInfo info, QSerialPortInfo::availablePorts()) {
        ui->portNameCB->addItem(info.portName());
    }
    ui->baudRate->setText("Baudios: 115200");
    ui->baudRateCB->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);
    ui->baudRateCB->addItem(QStringLiteral("57600"), QSerialPort::Baud57600);
    ui->baudRateCB->addItem(QStringLiteral("38400"), QSerialPort::Baud38400);
    ui->baudRateCB->addItem(QStringLiteral("19200"), QSerialPort::Baud19200);
    ui->baudRateCB->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);    
    status = new QLabel;
    ui->statusBar->addWidget(status);
}


void MainWindow::conexiones()
{
    connect(ui->connectButton,SIGNAL(clicked()),this,SLOT(openSerialPort()));
    connect(ui->actionConnect,SIGNAL(triggered()),this,SLOT(openSerialPort()));
    connect(ui->stopButton,SIGNAL(clicked()),this,SLOT(closeSerialPort()));
    connect(ui->actionStop,SIGNAL(triggered()),this,SLOT(closeSerialPort()));
    connect(ui->baudRateCB,SIGNAL(currentTextChanged(QString)),this,SLOT(cambiarBaudRateCB()));
    connect(ui->writeSerial,SIGNAL(clicked()),this,SLOT(writeData()));
    connect(serial, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(serial,SIGNAL(bytesWritten(qint64)),this,SLOT(changeRanges(qint64)));
    connect(ui->exitButton,SIGNAL(clicked()),this,SLOT(closeWindow()));
    connect(ui->actionExit,SIGNAL(triggered()),this,SLOT(closeWindow()));
    connect(this,SIGNAL(emitlinea(QStringList)),this,SLOT(print(QStringList)));
    //connect(this, SIGNAL(enviardatosgrafico(QStringList,QList<double>)), graficos, SLOT(Graficar(QStringList,QList<double>)));
    //connect(this, SIGNAL(enviardatosgrafico(QStringList,QList<double>)), graficos, SLOT(show()));
    connect(this,SIGNAL(vamosagraficar(QList<boolean>)),graficos,SLOT(inicializargraficos(QList<boolean>)));
    connect(this,SIGNAL(emitdato(QStringList,double)),graficos,SLOT(show()));
    connect(this,SIGNAL(emitdato(QStringList,double)),graficos,SLOT(realtimeDataSlot(QStringList,double)));
    connect(this,SIGNAL(emitstatustographics(QString)),graficos,SLOT(showStatusMessage(QString)));
}

void MainWindow::readData(){

    if ( timer.elapsed()/1000.0 <= (double)ui->tiempo->value()){
        while (serial->canReadLine()){
            const QByteArray serialData = serial->readLine();
            serialReaded=QString(serialData);

            QStringList linea=serialReaded.split(" ");
            if(linea.size()==6){
                samplesNumber+=1;
                const QString status="Tiempo: "+QString::number(timer.elapsed()/1000.0)+"   Muestras: "+QString::number(samplesNumber);
                showStatusMessage(status);
                emit emitstatustographics(status);
                if(samplesNumber==1)//Cuando se agrega el primer dato, se inicia el tiempo.
                    timer.start();

                listaTiempos.append(timer.elapsed()/1000.0);
                datos.append(linea);
                emit emitlinea(linea);
                if(samplesNumber % ui->frecgraph->value()==0)//Cada x datos se grafica
                    emit emitdato(linea,timer.elapsed()/1000.0);
            }
            else
                 QTextStream(stdout)<<serialReaded<<endl;
        }
    }
    else{
        ui->connectButton->setDisabled(false);
        ui->stopButton->setDisabled(true);
        serial->close();
    }
}

void MainWindow::writeData()
{
    serial->write(ui->serialDataLineEdit->text().toLocal8Bit());
}

void MainWindow::changeRanges(qint64 bytes)
{
    QTextStream(stdout)<< "Escribiendo Bytes"<<bytes<<endl;
    serial->waitForBytesWritten(2000);
}

void MainWindow::openSerialPort()
{
    //file.reset();
    timer.start();
    samplesNumber=0;
    datos.clear();        //Limpieza de las listas
    listaTiempos.clear();
    serial->setPortName(ui->portNameCB->currentText());
    serial->setBaudRate(ui->baudRateCB->currentText().toInt());
    QTextStream(stdout)<<"Baudios: "<< serial->baudRate();
    QTextStream(stdout)<<"portName"<< serial->portName();
    serial->setStopBits(QSerialPort::OneStop);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    if (serial->open(QIODevice::ReadWrite)){
         serial->clear();
        //serial->dataTerminalReadyChanged(true);
        //serial->requestToSendChanged(true);
        ui->connectButton->setDisabled(true);
        ui->stopButton->setDisabled(false);
        emit vamosagraficar(this->GetGraphicsCheckboxs());
        //QMessageBox::information(this,"Puerto Abierto","El puerto se ha abierto");

    } else {
        QMessageBox::critical(this, tr("Error"), serial->errorString());
    }
}

void MainWindow::closeSerialPort()
{
    datos.clear();
    samplesNumber=0;
    listaTiempos.clear();
    if (serial->isOpen()){
        serial->close();
        QMessageBox::information(this,"Cerrar Puerto","Puerto Cerrado");
        QTextStream(stdout)<<"Cerrado";
        ui->connectButton->setDisabled(false);
    }
    else {
         QMessageBox::information(this,"Cerrar Puerto","El puerto ya estaba cerrado");
    }
}

void MainWindow::closeWindow(){
    qApp->quit();
}

void MainWindow::on_cleanButton_clicked()
{
    ui->plainTextEdit->clear();
}

void MainWindow::cambiarBaudRateCB()
{
    ui->baudRate->setText("Baudios: "+ui->baudRateCB->currentText());
}

void MainWindow::print(QStringList linea)
{
    QTextStream(stdout)<<"Tiempo:"<<timer.elapsed()/1000.0<<" Muestras:"<< samplesNumber <<" AcX:"<<linea.at(0)<<" AcY:"<<linea.at(1)<<" AcZ:"<<linea.at(2)<<" GyX:"<<linea.at(3)<<" GyY:"<<linea.at(4)<<" GyZ:"<<linea.at(5)<<endl;
    //QTextStream(stdout)<<"Tiempo:"<<timer.elapsed()/1000.0<<" Muestras:"<<datos.size()<<" AcX:"<<linea.at(0)<<" AcY:"<<linea.at(1)<<" AcZ:"<<linea.at(2)<<endl;
    ui->plainTextEdit->insertPlainText(QString::number(timer.elapsed()/1000.0)+" "+serialReaded);
    QScrollBar *scrollbar = ui->plainTextEdit->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
    /*
    if (file.open(QIODevice::Append)) {
        QTextStream stream(&file);
        stream << QString::number(timer.elapsed()/1000.0)+" "+linea.at(0)+" "+linea.at(1)+" "+linea.at(2)+" "+linea.at(3)+" "+linea.at(4)+" "+linea.at(5);
    }
    file.close();
    */
}


void MainWindow::on_portNameCB_currentTextChanged()
{
    foreach (const QSerialPortInfo info, QSerialPortInfo::availablePorts()) {
        if (info.portName()==ui->portNameCB->currentText()){
            ui->portNamelabel->setText("Puerto: "+info.portName());
            ui->description->setText("Descripción: "+info.description());
            ui->serialNumber->setText("Numero de Serie: "+info.serialNumber());
            break;
        }
    }
}

void MainWindow::showStatusMessage(const QString &message)
{
    status->setText(message);
}

QList<boolean> MainWindow::GetGraphicsCheckboxs(){
    QList<boolean> checks;
    checks.append(ui->checkBoxAcX->isChecked());
    checks.append(ui->checkBoxAcY->isChecked());
    checks.append(ui->checkBoxAcZ->isChecked());
    checks.append(ui->checkBoxGyX->isChecked());
    checks.append(ui->checkBoxGyY->isChecked());
    checks.append(ui->checkBoxGyZ->isChecked());
    return checks;
}
