#ifndef CORE_COREDEBUG_H
#define CORE_COREDEBUG_H

#ifdef USE_QT
    #include <QDebug>
    #define DEBUG() qDebug()
    #define INFO() qInfo()
    #define WARNING() qWarning()
    #define CRITICAL() qCritical()
#else
    #include <iostream>
    #include <sstream>
    #define DEBUG()                                                                                                    \
        if (false) std::cout
    #define INFO()                                                                                                     \
        if (false) std::cout << "[INFO] "
    #define WARNING()                                                                                                  \
        if (false) std::cout << "[WARNING] "
    #define CRITICAL()                                                                                                 \
        if (false) std::cout << "[CRITICAL] "
#endif

#endif    // CORE_COREDEBUG_H
