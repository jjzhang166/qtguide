﻿#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QFileDialog>  //打开文件对话框
#include <QScrollArea>  //为标签添加滚动区域
#include <QMessageBox>  //消息框
#include <QResizeEvent> //调整窗口大小的事件类

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    //初始化成员变量
    m_pPixMap = NULL;
    m_pMovie = NULL;
    m_bIsMovie = false;
    m_bIsPlaying = false;

    //获取标签矩形
    QRect rcLabel = ui->labelShow->geometry();
    //为标签添加滚动区域，方便浏览大图
    QScrollArea *pSA = new QScrollArea(this);   //该对象交给主窗体自动管理，不用手动删除
    //把标签填充到滚动区域里
    pSA->setWidget(ui->labelShow);
    //设置滚动区域占据矩形
    pSA->setGeometry(rcLabel);

    //打印支持的图片格式
    qDebug()<<QImageReader::supportedImageFormats();
    //打印支持的动态图格式
    qDebug()<<QMovie::supportedFormats();

    //设置主界面窗体最小尺寸
    this->setMinimumSize(350, 350);
}

Widget::~Widget()
{
    //手动清空
    ClearOldShow();
    //原有的代码
    delete ui;
}

void Widget::ClearOldShow()
{
    //清空标签内容
    ui->labelShow->clear();
    //像素图不空就删除
    if( m_pPixMap != NULL )
    {
        //删除像素图
        delete m_pPixMap;   m_pPixMap = NULL;
    }
    //如果短片不为空，就删除
    if( m_pMovie != NULL )
    {
        //如果正在播放则停止
        if(m_bIsPlaying)
        {
            m_pMovie->stop();
        }
        //删除动态图
        delete m_pMovie;    m_pMovie = NULL;
    }
    //标志位重置
    m_bIsMovie = false;
    m_bIsPlaying = false;
}

void Widget::on_pushButtonOpenPic_clicked()
{
    QString strFileName;    //文件名
    strFileName = QFileDialog::getOpenFileName(this, tr("打开静态图片"), "",
                               "Pictures (*.bmp *.jpg *.jpeg *.png *.xpm);;All files(*)");
    if(strFileName.isEmpty())
    {
        //文件名为空，返回
        return;
    }
    //清空旧的图片或短片
    ClearOldShow();
    //打印文件名
    qDebug()<<strFileName;

    //新建像素图
    m_pPixMap = new QPixmap();
    //加载
    if( m_pPixMap->load(strFileName) )
    {
        //加载成功
        //设置给标签
        ui->labelShow->setPixmap(*m_pPixMap);
        //设置标签的新大小，与像素图一样大
        ui->labelShow->setGeometry( m_pPixMap->rect() );
        //设置 bool 状态
        m_bIsMovie = false;     //不是动态图
        m_bIsPlaying = false;   //不是动态图播放
    }
    else
    {
        //加载失败，删除图片对象，返回
        delete m_pPixMap;   m_pPixMap = NULL;
        //提示失败
        QMessageBox::critical(this, tr("打开失败"),
                              tr("打开图片失败，文件名为：\r\n%1").arg(strFileName));
    }
}

void Widget::on_pushButtonOpenMov_clicked()
{
    QString strFileName;    //文件名
    strFileName = QFileDialog::getOpenFileName(this, tr("打开动态图片"), "",
                               "Movies (*.gif *.mng);;All files(*)");
    if(strFileName.isEmpty())
    {
        //文件名为空，返回
        return;
    }
    //清除旧的图片或短片
    ClearOldShow();
    //打印文件名
    qDebug()<<strFileName;

    //新建动态图
    m_pMovie = new QMovie(strFileName);
    //判断是否动态图文件可用
    if( ! m_pMovie->isValid() )
    {
        //不可用
        QMessageBox::critical(this, tr("动态图不可用"),
                              tr("动态图格式不支持或读取出错，文件名为：\r\n%1").arg(strFileName));
        //清除
        delete m_pMovie;    m_pMovie = NULL;
        return; //不可用就直接返回
    }

    //动态图的总帧数
    int nCount = m_pMovie->frameCount(); //如果动态图格式不支持计数，那么会返回 0
    //打印帧数
    qDebug()<<tr("总帧数：%1").arg(nCount);
    //如果有统计帧数，那就设置滑动条上限
    if( nCount > 0 )
    {
        ui->horizontalSlider->setMaximum(nCount);
    }
    else
    {
        //获取不到帧数，默认当作 100
        ui->horizontalSlider->setMaximum(100);
    }

    //把动态图设置给标签
    ui->labelShow->setMovie(m_pMovie);
    //修改 bool 状态
    m_bIsMovie = true;
    m_bIsPlaying = false;   //还没点击播放开始的按钮

    //关联播放时的信号
    //播放出错信号
    connect(m_pMovie, SIGNAL(error(QImageReader::ImageReaderError)),
            this, SLOT(RecvPlayError(QImageReader::ImageReaderError)));
    //播放的帧号变化信号
    connect(m_pMovie, SIGNAL(frameChanged(int)),
            this, SLOT(RecvFrameNumber(int)));

    //将动态图片跳转到起始帧
    if(  m_pMovie->jumpToFrame(0) )
    {
        //跳转成功
        //对于打头的帧，设置标签的矩形为帧的矩形
        ui->labelShow->setGeometry( m_pMovie->frameRect() );
    }
    //如果跳转失败，槽函数 RecvPlayError() 会提示出错
}
//播放开始按钮
void Widget::on_pushButtonStart_clicked()
{
    if( ! m_bIsMovie)   //不是动态图
    {
        return;
    }
    if( m_bIsPlaying )  //已经在播放了
    {
        return;
    }
    //播放动态图
    m_bIsPlaying = true;    //开始播放状态
    m_pMovie->start();  //播放
    //打印动态图默认的播放循环轮数，0 代表不循环，-1 代表无限循环
    qDebug()<< tr("循环计数：%1").arg( m_pMovie->loopCount() );
}
//停止播放按钮
void Widget::on_pushButtonStop_clicked()
{
    if( ! m_bIsMovie)   //不是动态图
    {
        return;
    }
    if( ! m_bIsPlaying) //没有处于播放状态
    {
        return;
    }
    //停止播放
    m_bIsPlaying = false;
    m_pMovie->stop();
}

//接收播放错误信号
void Widget::RecvPlayError(QImageReader::ImageReaderError error)
{
    //打印
    qDebug()<<tr("读取动态图错误的代码：%1").arg(error);
    //提示播放出错
    QMessageBox::critical(this, tr("播放出错"),
                          tr("播放动态图出错，文件名为：\r\n%1").arg(m_pMovie->fileName()));
    //回到停止状态
    m_bIsPlaying = false;
}

//接收帧号变化信号
void Widget::RecvFrameNumber(int frameNumber)
{
    ui->horizontalSlider->setValue(frameNumber);
}



void Widget::resizeEvent(QResizeEvent *event)
{
    //获取当前宽度、高度
    int W = event->size().width();
    int H = event->size().height();

    //先计算第二行四个按钮的左上角坐标，按钮尺寸固定为 75*23
    //第一个按钮
    int x1 = 10;            //左边距 10
    int y1 = H - 10 - 21 - 10 - 23;  // 10 都是间隔，21 是水平滑动条高度，23 是按钮高度
    //第四个按钮
    int x4 = W - 10 - 75;   //10 是右边距，75 是按钮宽度
    int y4 = y1;            //与第一个按钮同一水平线
    //计算四个按钮的三个间隙总大小
    int nTriGap = W - 10 - 10 - 75 * 4;
    //计算单个间隙
    int nGap = nTriGap / 3 ;
    //计算第二个按钮坐标
    int x2 = x1 + 75 + nGap;
    int y2 = y1;
    //计算第三个按钮左边
    int x3 = x4 - 75 - nGap;
    int y3 = y1;

    //设置四个按钮的矩形
    ui->pushButtonOpenPic->setGeometry(x1, y1, 75, 23);
    ui->pushButtonOpenMov->setGeometry(x2, y2, 75, 23);
    ui->pushButtonStart->setGeometry(x3, y3, 75, 23);
    ui->pushButtonStop->setGeometry(x4, y4, 75, 23);

    //计算第三行水平滑动条的坐标和尺寸
    int xSlider = x2;
    int ySlider = H - 10 - 21;
    int wSlider = W - x2 - 10;
    int hSlider = 21;
    //设置水平滑动条的矩形
    ui->horizontalSlider->setGeometry(xSlider, ySlider, wSlider, hSlider);

    //计算包裹标签的滚动区域占用的矩形
    int xLabel = 10;
    int yLabel = 10;
    int wLabel = W - 10 - 10;
    int hLabel = H - 10 - 21 - 10 - 23 - 10 - 10;
    //设置包裹标签的滚动区域矩形
    QScrollArea *pSA = this->findChild<QScrollArea *>();    //查找子对象
    if( pSA != NULL)    //如果 pSA 不为 NULL 才能设置矩形
    {
        pSA->setGeometry(xLabel, yLabel, wLabel, hLabel);
    }
}




