#include "dialog.h"
#include <QApplication>

#include "hardware/audiosys.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Dialog w;

    setvbuf(stdout, NULL, _IONBF, 0);

    // init the hardware bits we need:
    if(initAudioHardware()){
        printf("Failed to open the audio systems\n");
    } else
        printf("Audio opened ok\n");

    w.show();


    return a.exec();
}
