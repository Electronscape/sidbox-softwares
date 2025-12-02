#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QImage>


QT_BEGIN_NAMESPACE
namespace Ui {
class Dialog;
}
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

    void updateSMSScreen();
    void clearSMSScreen();

private:
    Ui::Dialog *ui;

    QGraphicsScene *scene;
    QGraphicsPixmapItem *pixmapItem;
    QImage screenImageF;
    QImage screenImageB;
    QTimer *timer;

    void updateScreen();

    void setScreenScale(float factor);
    void swapBuffers();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

};

extern class Dialog *g_dialog;

#endif // DIALOG_H
