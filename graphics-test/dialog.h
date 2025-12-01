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

private:
    Ui::Dialog *ui;

    QGraphicsScene *scene;
    QGraphicsPixmapItem *pixmapItem;
    QImage screenImage;
    QTimer *timer;

    void updateScreen();
    void setScreenScale(float factor);

protected:
    void resizeEvent(QResizeEvent *event) override;


};
#endif // DIALOG_H
